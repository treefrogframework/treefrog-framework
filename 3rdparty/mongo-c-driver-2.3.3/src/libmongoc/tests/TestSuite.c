/*
 * Copyright 2009-present MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <mongoc/mongoc-thread-private.h>
#include <mongoc/mongoc-util-private.h>

#include <mongoc/mongoc.h>

#include <bson/bson.h>
#include <bson/bson_t.h>

#include <mlib/str.h>

#include <fcntl.h>


#if !defined(_WIN32)
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#include <inttypes.h>

#else
#include <windows.h>
#endif

#include <common-json-private.h>
#include <common-string-private.h>

#include <mlib/config.h>

#include <TestSuite.h>
#include <test-conveniences.h>
#include <test-libmongoc.h>

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SKIP_LINE_BUFFER_SIZE 1024

static bson_once_t once = BSON_ONCE_INIT;
static bson_mutex_t gTestMutex;
static TestSuite *gTestSuite;


MONGOC_PRINTF_FORMAT(1, 2)
static void
test_msg(const char *format, ...)
{
   va_list ap;

   va_start(ap, format);
   vprintf(format, ap);
   printf("\n");
   va_end(ap);
   fflush(stdout);
}


void
_test_error(const char *format, ...)
{
   va_list ap;

   fflush(stdout);
   va_start(ap, format);
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
   fflush(stderr);
   abort();
}

static void
TestSuite_SeedRand(TestSuite *suite, /* IN */
                   Test *test)       /* IN */
{
   BSON_UNUSED(suite);

#ifndef BSON_OS_WIN32
   int fd = open("/dev/urandom", O_RDONLY);
   int n_read;
   unsigned seed;
   if (fd != -1) {
      n_read = read(fd, &seed, 4);
      BSON_ASSERT(n_read == 4);
      close(fd);
      test->seed = seed;
      return;
   } else {
      test->seed = (unsigned)time(NULL) * (unsigned)getpid();
   }
#else
   test->seed = (unsigned)time(NULL);
#endif
}

static BSON_ONCE_FUN(_test_suite_ensure_mutex_once)
{
   bson_mutex_init(&gTestMutex);

   BSON_ONCE_RETURN;
}

void
TestSuite_Init(TestSuite *suite, const char *name, int argc, char **argv)
{
   const char *filename;
   int i;
   char *mock_server_log;

   memset(suite, 0, sizeof *suite);

   suite->name = bson_strdup(name);
   suite->flags = 0;
   suite->prgname = bson_strdup(argv[0]);
   suite->silent = false;

   for (i = 1; i < argc; i++) {
      if (0 == strcmp("-d", argv[i])) {
         suite->flags |= TEST_DEBUGOUTPUT;
      } else if ((0 == strcmp("-f", argv[i])) || (0 == strcmp("--no-fork", argv[i]))) {
         suite->flags |= TEST_NOFORK;
      } else if ((0 == strcmp("-t", argv[i])) || (0 == strcmp("--trace", argv[i]))) {
         mlib_diagnostic_push();
         mlib_disable_constant_conditional_expression_warnings();
         if (!MONGOC_TRACE_ENABLED) {
            test_error("-t requires mongoc compiled with -DENABLE_TRACING=ON.");
         }
         mlib_diagnostic_pop();
         suite->flags |= TEST_TRACE;
      } else if (0 == strcmp("-F", argv[i])) {
         if (argc - 1 == i) {
            test_error("-F requires a filename argument.");
         }
         filename = argv[++i];
         if (0 != strcmp("-", filename)) {
#ifdef _WIN32
            if (0 != fopen_s(&suite->outfile, filename, "w")) {
               suite->outfile = NULL;
            }
#else
            suite->outfile = fopen(filename, "w");
#endif
            if (!suite->outfile) {
               test_error("Failed to open log file: %s", filename);
            }
         }
      } else if ((0 == strcmp("-h", argv[i])) || (0 == strcmp("--help", argv[i]))) {
         suite->flags |= TEST_HELPTEXT;
      } else if (0 == strcmp("--list-tests", argv[i])) {
         suite->flags |= TEST_LISTTESTS;
      } else if (0 == strcmp("--tests-cmake", argv[i])) {
         suite->flags |= TEST_TESTS_CMAKE;
      } else if ((0 == strcmp("-s", argv[i])) || (0 == strcmp("--silent", argv[i]))) {
         suite->silent = true;
      } else if ((0 == strcmp("--ctest-run", argv[i]))) {
         if (suite->ctest_run.data) {
            test_error("'--ctest-run' can only be specified once");
         }
         if (argc - 1 == i) {
            test_error("'--ctest-run' requires an argument");
         }
         suite->flags |= TEST_NOFORK;
         suite->silent = true;
         suite->ctest_run = mstr_copy_cstring(argv[i + 1]);
         ++i;
      } else if ((0 == strcmp("-l", argv[i])) || (0 == strcmp("--match", argv[i]))) {
         if (argc - 1 == i) {
            test_error("%s requires an argument.", argv[i]);
         }
         ++i;
         *mstr_vec_push(&suite->match_patterns) = mstr_copy_cstring(argv[i]);
      } else if (0 == strcmp("--skip-tests", argv[i])) {
         if (argc - 1 == i) {
            test_error("%s requires an argument.", argv[i]);
         }
         filename = argv[++i];
         _process_skip_file(filename, &suite->failing_flaky_skips);
      } else {
         test_error("Unknown option: %s\n"
                    "Try using the --help option.",
                    argv[i]);
      }
   }

   if (suite->match_patterns.size != 0 && suite->ctest_run.data) {
      test_error("'--ctest-run' cannot be specified with '-l' or '--match'");
   }

   mock_server_log = test_framework_getenv("MONGOC_TEST_SERVER_LOG");
   if (mock_server_log) {
      if (!strcmp(mock_server_log, "stdout")) {
         suite->mock_server_log = stdout;
      } else if (!strcmp(mock_server_log, "stderr")) {
         suite->mock_server_log = stderr;
      } else if (!strcmp(mock_server_log, "json")) {
         suite->mock_server_log_buf = mcommon_string_new_with_capacity("", 0, 4096);
      } else {
         test_error("Unrecognized option: MONGOC_TEST_SERVER_LOG=%s", mock_server_log);
      }

      bson_free(mock_server_log);
   }

   if (suite->silent) {
      if (suite->outfile) {
         test_error("Cannot combine -F with --silent");
      }

      suite->flags &= ~(TEST_DEBUGOUTPUT);
   }

   bson_once(&once, &_test_suite_ensure_mutex_once);
   bson_mutex_lock(&gTestMutex);
   gTestSuite = suite;
   bson_mutex_unlock(&gTestMutex);
}


int
TestSuite_CheckLive(void)
{
   return test_framework_getenv_bool("MONGOC_TEST_SKIP_LIVE") ? 0 : 1;
}

static int
TestSuite_CheckDummy(void)
{
   return 1;
}

int
TestSuite_CheckMockServerAllowed(void)
{
   if (test_framework_getenv_bool("MONGOC_TEST_SKIP_MOCK")) {
      return 0;
   }
   return 1;
}

static void
TestSuite_AddHelper(void *ctx)
{
   ((TestFnCtx *)ctx)->test_fn();
}

void
TestSuite_Add(TestSuite *suite, /* IN */
              const char *name, /* IN */
              TestFunc func)    /* IN */
{
   TestSuite_AddFullWithTestFn(suite, name, TestSuite_AddHelper, NULL, func, TestSuite_CheckDummy);
}


void
TestSuite_AddLive(TestSuite *suite, /* IN */
                  const char *name, /* IN */
                  TestFunc func)    /* IN */
{
   // Add the `lock:live-server` tag to the test.
   mstr with_tags = mstr_sprintf("%s [lock:live-server]", name);
   TestSuite_AddFullWithTestFn(suite, with_tags.data, TestSuite_AddHelper, NULL, func, TestSuite_CheckLive);
   mstr_destroy(&with_tags);
}


Test *
_V_TestSuite_AddFull(
   TestSuite *suite, const char *name_and_tags, TestFuncWC func, TestFuncDtor dtor, void *ctx, va_list ap)
{
   // Split the name and tags around the first whitespace:
   mstr_view name, tags;
   mstr_split_around(mstr_cstring(name_and_tags), mstr_cstring(" "), &name, &tags);
   // Trim extras:
   tags = mstr_trim(tags);

   if (suite->ctest_run.data && mstr_cmp(suite->ctest_run, !=, name)) {
      // We are running CTest, and not running this particular test, so just skip registering it
      if (dtor) {
         dtor(ctx);
      }
      return NULL;
   }

   Test *test = TestVec_push(&suite->tests);
   test->name = mstr_copy(name);
   test->func = func;

   mstr_view tag, tail;
   tail = tags;
   while (tail.len) {
      mlib_check(mstr_split_around(tail, mstr_cstring("["), NULL, &tag),
                 because,
                 "Invalid test specifier: Expected an opening bracket (following whitespace or closing bracket) for a "
                 "test tag");
      mlib_check(
         mstr_split_around(tag, mstr_cstring("]"), &tag, &tail), because, "Expected a closing bracket for test tag");
      *mstr_vec_push(&test->tags) = mstr_copy(tag);
   }

   CheckFunc check;
   while ((check = va_arg(ap, CheckFunc))) {
      *CheckFuncVec_push(&test->checks) = check;
   }

   test->dtor = dtor;
   test->ctx = ctx;
   TestSuite_SeedRand(suite, test);
   return test;
}


void
_TestSuite_AddMockServerTest(TestSuite *suite, const char *name, TestFunc func, ...)
{
   Test *test;
   va_list ap;
   TestFnCtx *ctx = bson_malloc(sizeof(TestFnCtx));
   *ctx = (TestFnCtx){.test_fn = func, .dtor = NULL};

   va_start(ap, func);
   test = _V_TestSuite_AddFull(suite, name, TestSuite_AddHelper, _TestSuite_TestFnCtxDtor, ctx, ap);
   va_end(ap);

   if (test) {
      *CheckFuncVec_push(&test->checks) = TestSuite_CheckMockServerAllowed;
   }
}


void
TestSuite_AddWC(TestSuite *suite,  /* IN */
                const char *name,  /* IN */
                TestFuncWC func,   /* IN */
                TestFuncDtor dtor, /* IN */
                void *ctx)         /* IN */
{
   TestSuite_AddFull(suite, name, func, dtor, ctx, TestSuite_CheckDummy);
}


void(TestSuite_AddFull)(TestSuite *suite,  /* IN */
                        const char *name,  /* IN */
                        TestFuncWC func,   /* IN */
                        TestFuncDtor dtor, /* IN */
                        void *ctx,
                        ...) /* IN */
{
   va_list ap;

   va_start(ap, ctx);
   _V_TestSuite_AddFull(suite, name, func, dtor, ctx, ap);
   va_end(ap);
}


void
_TestSuite_TestFnCtxDtor(void *ctx)
{
   TestFuncDtor dtor = ((TestFnCtx *)ctx)->dtor;
   if (dtor) {
      dtor(ctx);
   }
   bson_free(ctx);
}


#if defined(_WIN32)
static void
_print_getlasterror_win(const char *msg)
{
   char *err_msg = mongoc_winerr_to_string(GetLastError());

   test_error("%s: %s", msg, err_msg);

   bson_free(err_msg);
}


static int
TestSuite_RunFuncInChild(TestSuite *suite, /* IN */
                         Test *test)       /* IN */
{
   STARTUPINFOA si;
   PROCESS_INFORMATION pi;
   char *cmdline;
   DWORD exit_code = -1;

   ZeroMemory(&si, sizeof(si));
   si.cb = sizeof(si);
   ZeroMemory(&pi, sizeof(pi));

   cmdline = bson_strdup_printf("%s --silent --no-fork -l %.*s", suite->prgname, MSTR_FMT(test->name));

   if (!CreateProcessA(NULL,
                       cmdline,
                       NULL,  /* Process handle not inheritable  */
                       NULL,  /* Thread handle not inheritable   */
                       FALSE, /* Set handle inheritance to FALSE */
                       0,     /* No creation flags               */
                       NULL,  /* Use parent's environment block  */
                       NULL,  /* Use parent's starting directory */
                       &si,
                       &pi)) {
      test_error("CreateProcess failed (%lu).", GetLastError());
      bson_free(cmdline);

      return -1;
   }

   if (WaitForSingleObject(pi.hProcess, INFINITE) != WAIT_OBJECT_0) {
      _print_getlasterror_win("Couldn't await process");
      goto done;
   }

   if (!GetExitCodeProcess(pi.hProcess, &exit_code)) {
      _print_getlasterror_win("Couldn't get exit code");
      goto done;
   }

done:
   CloseHandle(pi.hProcess);
   CloseHandle(pi.hThread);
   bson_free(cmdline);

   return exit_code;
}
#else /* Unix */
static int
TestSuite_RunFuncInChild(TestSuite *suite, /* IN */
                         Test *test)       /* IN */
{
   pid_t child;
   int exit_code = -1;
   int fd;
   int pipefd[2];
   const char *envp[] = {"MONGOC_TEST_SERVER_LOG=stdout", NULL};
   char buf[4096];
   ssize_t nread;

   if (suite->outfile) {
      fflush(suite->outfile);
   }

   if (suite->mock_server_log_buf) {
      if (pipe(pipefd) == -1) {
         perror("pipe");
         exit(-1);
      }
   }

   if (-1 == (child = fork())) {
      return -1;
   }

   if (!child) {
      if (suite->outfile) {
         fclose(suite->outfile);
         suite->outfile = NULL;
      }

      if (suite->mock_server_log_buf) {
         /* tell mock server to log to stdout, and read its output */
         dup2(pipefd[1], STDOUT_FILENO);
         close(pipefd[0]);
         close(pipefd[1]);
         execle(suite->prgname, suite->prgname, "--no-fork", "--silent", "-l", test->name.data, (char *)0, envp);
      } else {
         /* suppress child output */
         fd = open("/dev/null", O_WRONLY);
         dup2(fd, STDOUT_FILENO);
         close(fd);

         execl(suite->prgname, suite->prgname, "--no-fork", "-l", test->name.data, (char *)0);
      }

      exit(-1);
   }

   if (suite->mock_server_log_buf) {
      close(pipefd[1]);
      while ((nread = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
         mcommon_string_append_t append;
         mcommon_string_set_append(suite->mock_server_log_buf, &append);
         mcommon_string_append_bytes(&append, buf, nread);
      }
   }

   if (-1 == waitpid(child, &exit_code, 0)) {
      perror("waitpid()");
   }

   return exit_code;
}
#endif


/* returns 1 on failure, 0 on success */
static int
TestSuite_RunTest(TestSuite *suite, /* IN */
                  Test *test,       /* IN */
                  int *count)       /* INOUT */
{
   int64_t t1, t2, t3;
   char name[MAX_TEST_NAME_LENGTH];
   mcommon_string_append_t buf;
   mcommon_string_t *mock_server_log_buf;
   int status = 0;

   bson_snprintf(name, sizeof name, "%s%.*s", suite->name, MSTR_FMT(test->name));

   mcommon_string_new_as_append(&buf);

   if (suite->flags & TEST_DEBUGOUTPUT) {
      test_msg("Begin %s, seed %u", name, test->seed);
   }

   mlib_vec_foreach (TestSkip, skip, suite->failing_flaky_skips) {
      if (mstr_cmp(skip->test_name, ==, mstr_cstring(name)) && !skip->subtest_desc.data) {
         if (suite->ctest_run.data) {
            /* Write a marker that tells CTest that we are skipping this test */
            test_msg("@@ctest-skipped@@");
         }
         if (!suite->silent) {
            mcommon_string_append_printf(&buf,
                                         "    { \"status\": \"skip\", \"test_file\": \"%.*s\","
                                         " \"reason\": \"%.*s\" }%s",
                                         MSTR_FMT(test->name),
                                         MSTR_FMT(skip->reason),
                                         ((*count) == 1) ? "" : ",");
            test_msg("%s", mcommon_str_from_append(&buf));
            if (suite->outfile) {
               fprintf(suite->outfile, "%s", mcommon_str_from_append(&buf));
               fflush(suite->outfile);
            }
         }

         goto done;
      }
   }

   mlib_vec_foreach (CheckFunc, check, test->checks) {
      if (!(*check)()) {
         if (suite->ctest_run.data) {
            /* Write a marker that tells CTest that we are skipping this test */
            test_msg("@@ctest-skipped@@");
         }
         if (!suite->silent) {
            mcommon_string_append_printf(&buf,
                                         "    { \"status\": \"skip\", \"test_file\": \"%.*s\" }%s",
                                         MSTR_FMT(test->name),
                                         ((*count) == 1) ? "" : ",");
            test_msg("%s", mcommon_str_from_append(&buf));
            if (suite->outfile) {
               fprintf(suite->outfile, "%s", mcommon_str_from_append(&buf));
               fflush(suite->outfile);
            }
         }

         goto done;
      }
   }

   t1 = bson_get_monotonic_time();


   if (suite->flags & TEST_NOFORK) {
      if (suite->flags & TEST_TRACE) {
         mongoc_log_set_handler(mongoc_log_default_handler, NULL);
         mongoc_log_trace_enable();
      } else {
         mongoc_log_trace_disable();
      }

      srand(test->seed);
      test_conveniences_init();
      test->func(test->ctx);
      test_conveniences_cleanup();
   } else {
      status = TestSuite_RunFuncInChild(suite, test);
   }

   capture_logs(false);

   if (suite->silent) {
      goto done;
   }

   t2 = bson_get_monotonic_time();
   // CDRIVER-2567: check that bson_get_monotonic_time does not wrap.
   ASSERT_CMPINT64(t2, >=, t1);
   t3 = t2 - t1;

   mcommon_string_append_printf(&buf,
                                "    { \"status\": \"%s\", "
                                "\"test_file\": \"%s\", "
                                "\"seed\": \"%u\", "
                                "\"start\": %u.%06u, "
                                "\"end\": %u.%06u, "
                                "\"elapsed\": %u.%06u ",
                                (status == 0) ? "pass" : "fail",
                                name,
                                test->seed,
                                (unsigned)t1 / (1000 * 1000),
                                (unsigned)t1 % (1000 * 1000),
                                (unsigned)t2 / (1000 * 1000),
                                (unsigned)t2 % (1000 * 1000),
                                (unsigned)t3 / (1000 * 1000),
                                (unsigned)t3 % (1000 * 1000));

   mock_server_log_buf = suite->mock_server_log_buf;

   if (mock_server_log_buf && !mcommon_string_is_empty(mock_server_log_buf)) {
      mcommon_string_append(&buf, ", \"log_raw\": \"");
      mcommon_json_append_escaped(&buf, mock_server_log_buf->str, mock_server_log_buf->len, true);
      mcommon_string_append(&buf, "\"");
      mcommon_string_clear(mock_server_log_buf);
   }

   mcommon_string_append(&buf, " }");

   if (*count > 1) {
      mcommon_string_append(&buf, ",");
   }

   test_msg("%s", mcommon_str_from_append(&buf));
   if (suite->outfile) {
      fprintf(suite->outfile, "%s", mcommon_str_from_append(&buf));
      fflush(suite->outfile);
   }

done:
   mcommon_string_from_append_destroy(&buf);

   return status ? 1 : 0;
}


static void
TestSuite_PrintHelp(TestSuite *suite) /* IN */
{
   printf("usage: %s [OPTIONS]\n"
          "\n"
          "Options:\n"
          "    -h, --help            Show this help menu.\n"
          "    --list-tests          Print list of available tests.\n"
          "    --tests-cmake         Print CMake code that defines test information.\n"
          "    -f, --no-fork         Do not spawn a process per test (abort on "
          "first error).\n"
          "    -l, --match PATTERN   Run test by name, e.g. \"/Client/command\" or "
          "\"/Client/*\". May be repeated.\n"
          "    --ctest-run TEST      Run only the named TEST for CTest\n"
          "                          integration.\n"
          "    -s, --silent          Suppress all output.\n"
          "    -F FILENAME           Write test results (JSON) to FILENAME.\n"
          "    -d                    Print debug output (useful if a test hangs).\n"
          "    --skip-tests FILE     Skip known failing or flaky tests.\n"
          "    -t, --trace           Enable mongoc tracing (useful to debug "
          "tests).\n"
          "\n",
          suite->prgname);
}


static void
TestSuite_PrintTests(TestSuite *suite) /* IN */
{
   printf("\nTests:\n");
   mlib_vec_foreach (Test, t, suite->tests) {
      printf("%s%.*s\n", suite->name, MSTR_FMT(t->name));
   }

   printf("\n");
}

static void
TestSuite_PrintCMake(TestSuite *suite)
{
   printf("set(MONGOC_TESTS)\n");
   mlib_vec_foreach (Test, t, suite->tests) {
      printf("list(APPEND MONGOC_TESTS [[%.*s]])\n", MSTR_FMT(t->name));
      printf("set(MONGOC_TEST_%.*s_TAGS)\n", MSTR_FMT(t->name));
      mlib_vec_foreach (mstr, tag, t->tags) {
         printf("list(APPEND MONGOC_TEST_%.*s_TAGS [[%.*s]])\n", MSTR_FMT(t->name), MSTR_FMT(*tag));
      }
   }
}

static void
TestSuite_PrintJsonSystemHeader(FILE *stream)
{
#ifdef _WIN32
#define INFO_BUFFER_SIZE 32767

   SYSTEM_INFO si;

   GetSystemInfo(&si);

#if defined(_MSC_VER)
   // CDRIVER-4263: GetVersionEx is deprecated.
#pragma warning(suppress : 4996)
   const DWORD version = GetVersion();
#else
   const DWORD version = GetVersion();
#endif

   const DWORD major_version = (DWORD)(LOBYTE(LOWORD(version)));
   const DWORD minor_version = (DWORD)(HIBYTE(LOWORD(version)));
   const DWORD build = version < 0x80000000u ? (DWORD)(HIWORD(version)) : 0u;

   fprintf(stream,
           "  \"host\": {\n"
           "    \"sysname\": \"Windows\",\n"
           "    \"release\": \"%lu.%lu (%lu)\",\n"
           "    \"machine\": \"%lu\",\n"
           "    \"memory\": {\n"
           "      \"pagesize\": %lu,\n"
           "      \"npages\": 0\n"
           "    }\n"
           "  },\n",
           major_version,
           minor_version,
           build,
           si.dwProcessorType,
           si.dwPageSize);
#else
   struct utsname u;
   uint64_t pagesize;
   uint64_t npages = 0;

   if (uname(&u) == -1) {
      perror("uname()");
      return;
   }

   pagesize = sysconf(_SC_PAGE_SIZE);

#if defined(_SC_PHYS_PAGES)
   npages = sysconf(_SC_PHYS_PAGES);
#endif
   fprintf(stream,
           "  \"host\": {\n"
           "    \"sysname\": \"%s\",\n"
           "    \"release\": \"%s\",\n"
           "    \"machine\": \"%s\",\n"
           "    \"memory\": {\n"
           "      \"pagesize\": %" PRIu64 ",\n"
           "      \"npages\": %" PRIu64 "\n"
           "    }\n"
           "  },\n",
           u.sysname,
           u.release,
           u.machine,
           pagesize,
           npages);
#endif
}

static const char *
getenv_or(const char *name, const char *dflt)
{
   const char *s = getenv(name);
   return s ? s : dflt;
}

static const char *
egetenv(const char *name)
{
   return getenv_or(name, "");
}

static void
TestSuite_PrintJsonHeader(TestSuite *suite, /* IN */
                          FILE *stream)     /* IN */
{
   bool ssl = test_framework_get_ssl();

   ASSERT(suite);

   fprintf(stream, "{\n");
   TestSuite_PrintJsonSystemHeader(stream);
   fprintf(stream,
           "  \"auth\": { \"user\": \"%s\", \"pass\": \"%s\" }, \n"
           "  \"addr\": { \"uri\": \"%s\" },\n"
           "  \"gssapi\": { \"host\": \"%s\", \"user\": \"%s\" }, \n"
           "  \"compressors\": \"%s\", \n"
           "  \"SSL\": {\n"
           "    \"enabled\": %s,\n"
           "    \"weak_cert_validation\": %s,\n"
           "    \"pem_file\": \"%s\",\n"
           "    \"pem_pwd\": \"%s\",\n"
           "    \"ca_file\": \"%s\",\n"
           "    \"ca_dir\": \"%s\",\n"
           "    \"crl_file\": \"%s\"\n"
           "  },\n"
           "  \"framework\": {\n"
           "    \"monitoringVerbose\": %s,\n"
           "    \"mockServerLog\": \"%s\",\n"
           "    \"futureTimeoutMS\": %" PRId64 ",\n"
           "    \"majorityReadConcern\": %s,\n"
           "    \"skipLiveTests\": %s\n"
           "  },\n"
           "  \"options\": {\n"
           "    \"fork\": %s,\n"
           "    \"tracing\": %s,\n"
           "    \"apiVersion\": %s\n"
           "  },\n"
           "  \"results\": [\n",
           egetenv("MONGOC_TEST_USER") ? "(set)" : "(unset)",
           egetenv("MONGOC_TEST_PASSWORD") ? "(set)" : "(unset)",
           egetenv("MONGOC_TEST_URI") ? "(set)" : "(unset)",
           egetenv("MONGOC_TEST_GSSAPI_HOST") ? "(set)" : "(unset)",
           egetenv("MONGOC_TEST_GSSAPI_USER") ? "(set)" : "(unset)",
           egetenv("MONGOC_TEST_COMPRESSORS"),
           ssl ? "true" : "false",
           test_framework_getenv_bool("MONGOC_TEST_SSL_WEAK_CERT_VALIDATION") ? "true" : "false",
           egetenv("MONGOC_TEST_SSL_PEM_FILE"),
           egetenv("MONGOC_TEST_SSL_PEM_PWD") ? "(set)" : "(unset)",
           egetenv("MONGOC_TEST_SSL_CA_FILE"),
           egetenv("MONGOC_TEST_SSL_CA_DIR"),
           egetenv("MONGOC_TEST_SSL_CRL_FILE"),
           getenv("MONGOC_TEST_MONITORING_VERBOSE") ? "true" : "false",
           egetenv("MONGOC_TEST_SERVER_LOG"),
           get_future_timeout_ms(),
           test_framework_getenv_bool("MONGOC_ENABLE_MAJORITY_READ_CONCERN") ? "true" : "false",
           test_framework_getenv_bool("MONGOC_TEST_SKIP_LIVE") ? "true" : "false",
           (suite->flags & TEST_NOFORK) ? "false" : "true",
           (suite->flags & TEST_TRACE) ? "true" : "false",
           getenv_or("MONGODB_API_VERSION", "null"));


   fflush(stream);
}


static void
TestSuite_PrintJsonFooter(FILE *stream) /* IN */
{
   fprintf(stream, "  ]\n}\n");
   fflush(stream);
}

static bool
TestSuite_TestMatchesName(const TestSuite *suite, const Test *test, const char *testname)
{
   char name[128];
   bool star = strlen(testname) && testname[strlen(testname) - 1] == '*';

   bson_snprintf(name, sizeof name, "%s%.*s", suite->name, MSTR_FMT(test->name));

   if (star) {
      /* e.g. testname is "/Client*" and name is "/Client/authenticate" */
      return (0 == strncmp(name, testname, strlen(testname) - 1));
   } else {
      return (0 == strcmp(name, testname));
   }
}


bool
test_matches(TestSuite *suite, Test *test)
{
   if (suite->ctest_run.data) {
      /* We only want exactly the named test */
      return mstr_cmp(test->name, ==, suite->ctest_run);
   }

   /* If no match patterns were provided, then assume all match. */
   if (suite->match_patterns.size == 0) {
      return true;
   }

   mlib_vec_foreach (mstr, pat, suite->match_patterns) {
      if (TestSuite_TestMatchesName(suite, test, pat->data)) {
         return true;
      }
   }

   return false;
}

void
_process_skip_file(const char *filename, TestSkipVec *skips)
{
   FILE *skip_file;

#ifdef _WIN32
   if (0 != fopen_s(&skip_file, filename, "r")) {
      skip_file = NULL;
   }
#else
   skip_file = fopen(filename, "r");
#endif
   if (!skip_file) {
      test_error("Failed to open skip file: %s: errno: %d", filename, errno);
   }

   while (1) {
      char buffer[SKIP_LINE_BUFFER_SIZE];
      if (!fgets(buffer, sizeof(buffer), skip_file)) {
         break; /* error */
      }

      mstr_view line = mstr_cstring(buffer);
      if (!line.len) {
         // EOF
         break;
      }
      // Remove whitespace
      line = mstr_trim(line);
      if (line.len == 0 || line.data[0] == '#') {
         // Empty line or comment
         continue;
      }

      TestSkip skip = {0};
      // If there is a trailing comment, drop that:
      mstr_view comment = {0};
      mstr_split_around(line, mstr_cstring("#"), &line, &comment);
      line = mstr_trim(line);
      comment = mstr_trim(comment);

      if (comment.len) {
         skip.reason = mstr_copy(comment);
      }

      // If it contains a '/"' substring, the quoted part is the subtest description,
      // and everything before the '/' is the main test name. Split on that:
      mstr_view test_name;
      mstr_view subtest_desc;
      if (mstr_split_around(line, mstr_cstring("/\""), &test_name, &subtest_desc)) {
         // Drop trailing quote:
         mlib_check(mstr_at(subtest_desc, -1), eq, '"', because, "Subtest description should end with a quote");
         subtest_desc = mstr_slice(subtest_desc, 0, -1);
         skip.subtest_desc = mstr_copy(subtest_desc);
      }
      skip.test_name = mstr_copy(test_name);
      *TestSkipVec_push(skips) = skip;
   }
   fclose(skip_file);
}

static int
TestSuite_RunAll(TestSuite *suite /* IN */)
{
   int count = 0;
   int status = 0;

   ASSERT(suite);

   /* initialize "count" so we can omit comma after last test output */
   mlib_vec_foreach (Test, t, suite->tests) {
      if (test_matches(suite, t)) {
         count++;
      }
   }

   if (suite->ctest_run.data) {
      /* We should have matched *at most* one test */
      ASSERT(count <= 1);
      if (count == 0) {
         test_error("No such test '%.*s'", MSTR_FMT(suite->ctest_run));
      }
   }

   mlib_vec_foreach (Test, t, suite->tests) {
      if (test_matches(suite, t)) {
         status += TestSuite_RunTest(suite, t, &count);
         count--;
      }
   }

   if (suite->silent) {
      return status;
   }

   TestSuite_PrintJsonFooter(stdout);
   if (suite->outfile) {
      TestSuite_PrintJsonFooter(suite->outfile);
   }

   return status;
}


int
TestSuite_Run(TestSuite *suite) /* IN */
{
   int failures = 0;
   int64_t start_us;

   if (suite->flags & TEST_HELPTEXT) {
      TestSuite_PrintHelp(suite);
   }

   if (suite->flags & TEST_LISTTESTS) {
      TestSuite_PrintTests(suite);
   }

   if (suite->flags & TEST_TESTS_CMAKE) {
      TestSuite_PrintCMake(suite);
   }

   if ((suite->flags & TEST_HELPTEXT) || (suite->flags & TEST_LISTTESTS) || (suite->flags & TEST_TESTS_CMAKE)) {
      return 0;
   }

   if (!suite->silent) {
      TestSuite_PrintJsonHeader(suite, stdout);
      if (suite->outfile) {
         TestSuite_PrintJsonHeader(suite, suite->outfile);
      }
   }

   start_us = bson_get_monotonic_time();
   failures += TestSuite_RunAll(suite);
   MONGOC_DEBUG("Duration of all tests (s): %" PRId64, (bson_get_monotonic_time() - start_us) / (1000 * 1000));

   return failures;
}

void
TestSuite_Destroy(TestSuite *suite)
{
   bson_mutex_lock(&gTestMutex);
   gTestSuite = NULL;
   bson_mutex_unlock(&gTestMutex);

   TestVec_destroy(&suite->tests);

   if (suite->outfile) {
      fclose(suite->outfile);
   }

   mcommon_string_destroy(suite->mock_server_log_buf);

   bson_free(suite->name);
   bson_free(suite->prgname);
   mstr_destroy(&suite->ctest_run);
   mstr_vec_destroy(&suite->match_patterns);
   TestSkipVec_destroy(&suite->failing_flaky_skips);
}


int
test_suite_debug_output(void)
{
   int ret;

   bson_mutex_lock(&gTestMutex);
   ret = gTestSuite->flags & TEST_DEBUGOUTPUT;
   bson_mutex_unlock(&gTestMutex);

   return ret;
}


MONGOC_PRINTF_FORMAT(1, 2)
void
test_suite_mock_server_log(const char *msg, ...)
{
   bson_mutex_lock(&gTestMutex);

   if (gTestSuite->mock_server_log_buf) {
      mcommon_string_append_t append;
      mcommon_string_set_append(gTestSuite->mock_server_log_buf, &append);

      va_list ap;
      va_start(ap, msg);
      mcommon_string_append_vprintf(&append, msg, ap);
      va_end(ap);
      mcommon_string_append(&append, "\n");

   } else if (gTestSuite->mock_server_log) {
      va_list ap;
      va_start(ap, msg);
      vfprintf(gTestSuite->mock_server_log, msg, ap);
      va_end(ap);
      fputc('\n', gTestSuite->mock_server_log);
      fflush(gTestSuite->mock_server_log);
   }

   bson_mutex_unlock(&gTestMutex);
}

bool
TestSuite_NoFork(TestSuite *suite)
{
   if (suite->flags & TEST_NOFORK) {
      return true;
   }
   return false;
}

char *
bson_value_to_str(const bson_value_t *val)
{
   bson_t *tmp = bson_new();
   BSON_APPEND_VALUE(tmp, "v", val);
   char *str = bson_as_canonical_extended_json(tmp, NULL);
   // str is the string: { "v": ... }
   // remove the prefix and suffix.
   char *ret = bson_strndup(str + 6, strlen(str) - (6 + 1));
   bson_free(str);
   bson_destroy(tmp);
   return ret;
}

bool
bson_value_eq(const bson_value_t *a, const bson_value_t *b)
{
   bson_t *tmp_a = bson_new();
   bson_t *tmp_b = bson_new();
   BSON_APPEND_VALUE(tmp_a, "v", a);
   BSON_APPEND_VALUE(tmp_b, "v", b);
   bool ret = bson_equal(tmp_a, tmp_b);
   bson_destroy(tmp_b);
   bson_destroy(tmp_a);
   return ret;
}

const char *
test_bulkwriteexception_str(const mongoc_bulkwriteexception_t *bwe)
{
   bson_error_t _error;
   const char *_msg = "(none)";
   if (mongoc_bulkwriteexception_error(bwe, &_error)) {
      _msg = _error.message;
   }
   return tmp_str("Bulk Write Exception:\n"
                  "  Error                 : %s\n"
                  "  Write Errors          : %s\n"
                  "  Write Concern Errors  : %s\n"
                  "  Error Reply           : %s",
                  _msg,
                  tmp_json(mongoc_bulkwriteexception_writeerrors(bwe)),
                  tmp_json(mongoc_bulkwriteexception_writeconcernerrors(bwe)),
                  tmp_json(mongoc_bulkwriteexception_errorreply(bwe)));
}
