# Contributing to mongo-c-driver

Thanks for considering contributing to the mongo-c-driver!

This document intends to be a short guide to helping you contribute to the codebase.
It expects a familiarity with the C programming language and writing portable software.
Whenever in doubt, feel free to ask others that have contributed or look at the existing body of code.


## Guidelines

The mongo-c-driver has a few guidelines that help direct the process.


### Portability

mongo-c-driver is portable software. It needs to run on a multitude of
operating systems and architectures.

 * Linux (RHEL 5 and newer)
 * FreeBSD (10 and newer)
 * Windows (Vista and newer)
 * macOS (10.8 and newer)
 * ARM/SPARC/x86/x86_64


### Licensing

Some of the mongo-c-driver users embed the library statically in their
products.  Therefore, the driver and all contributions must be liberally
licensed.  As a policy, we have chosen Apache 2.0 as the license for the
project.


### Coding Style

We try not to be pedantic with taking contributions that are not properly
formatted, but we will likely perform a followup commit that cleans things up.
The basics are, in vim:

```
 : set ts=3 sw=3 et
```

3 space tabs, insert spaces instead of tabs.

For all the gory details, see [.clang-format](.clang-format)

### Commit Style
Commit messages follow this pattern:

```
CDRIVER-#### lowercase commit message
```

There's no colon after the ticket number, and no period at the end of the
subject line. If there's no related Jira ticket, then the message is simply all
lowercase:

```
typos in NEWS
```

We follow [the widely adopted "50/72"
rule](https://medium.com/@preslavrachev/what-s-with-the-50-72-rule-8a906f61f09c):
the subject line is ideally only 50 characters, but definitely only 72
characters. This requires some thoughtful writing but it's worthwhile when
you're scrolling through commits. The message body can be as large as you want,
wrapped at 72 columns.

### Adding a new error code or domain                                              
                                                                                   
When adding a new error code or domain, you must do the following. This is most
applicable if you are adding a new symbol with a bson_error_t as a parameter,
and the existing codes or domains are inappropriate.                               
                                                                                   
 - Add the domain to `mongoc_error_domain_t` in `src/mongoc/mongoc-error.h`        
 - Add the code to `mongoc_error_code_t` in `src/mongoc/mongoc-error.h`            
 - Add documentation for the domain or code to the table in `doc/mongoc_errors.rst`
                              
### Adding a new symbol

This should be done rarely but there are several things that you need to do
when adding a new symbol.

 - Add documentation for the new symbol in `doc/mongoc_your_new_symbol_name.rst`

### Documentation

We strive to document all symbols. See doc/ for documentation examples. If you
add a new public function, add a new .rst file describing the function so that
we can generate man pages and HTML for it.

For complex internal functions, comment above the function definition with
a block comment like the following:

```
/*--------------------------------------------------------------------------
 *
 * mongoc_cmd_parts_append_read_write --
 *
 *       Append user-supplied options to @parts->command_extra, taking the
 *       selected server's max wire version into account.
 *
 * Return:
 *       True if the options were successfully applied. If any options are
 *       invalid, returns false and fills out @error. In that case @parts is
 *       invalid and must not be used.
 *
 * Side effects:
 *       May partly apply options before returning an error.
 *
 *--------------------------------------------------------------------------
 */
```

Public functions do not need these comment blocks, since they are documented in
the .rst files.


### Testing

To run the entire test suite, including authentication tests,
start `mongod` with auth enabled:

```
$ mongod --auth
```

In another terminal, use the `mongo` shell to create a user:

```
$ mongo --eval "db.createUser({user: 'admin', pwd: 'pass', roles: ['root']})" admin
```

Authentication in MongoDB 3.0 and later uses SCRAM-SHA-1, which in turn
requires a driver built with SSL.

Set the user and password environment variables, then build and run the tests:

```
$ export MONGOC_TEST_USER=admin
$ export MONGOC_TEST_PASSWORD=pass
$ ./test-libmongoc
```

Additional environment variables:

* `MONGOC_TEST_HOST`: default `localhost`, the host running MongoDB.
* `MONGOC_TEST_PORT`: default 27017, MongoDB's listening port.
* `MONGOC_TEST_URI`: override both host and port with a full connection string,
  like "mongodb://server1,server2".
* `MONGOC_TEST_SERVER_LOG`: set to `stdout` or `stderr` for wire protocol 
  logging from tests that use `mock_server_t`. Set to `json` to include these
  logs in the test framework's JSON output, in a format compatible with
  [Evergreen](https://github.com/evergreen-ci/evergreen).
* `MONGOC_TEST_MONITORING_VERBOSE`: set to `on` for verbose output from
  Application Performance Monitoring tests.
* `MONGOC_TEST_COMPRESSORS=snappy,zlib`: wire protocol compressors to use

If you start `mongod` with SSL, set these variables to configure how
`test-libmongoc` connects to it:

* `MONGOC_TEST_SSL`: set to `on` to connect to the server with SSL.
* `MONGOC_TEST_SSL_PEM_FILE`: path to a client PEM file.
* `MONGOC_TEST_SSL_PEM_PWD`: the PEM file's password.
* `MONGOC_TEST_SSL_CA_FILE`: path to a certificate authority file.
* `MONGOC_TEST_SSL_CA_DIR`: path to a certificate authority directory.
* `MONGOC_TEST_SSL_CRL_FILE`: path to a certificate revocation list.
* `MONGOC_TEST_SSL_WEAK_CERT_VALIDATION`: set to `on` to relax the client's
  validation of the server's certificate.

The SASL / GSSAPI / Kerberos tests are skipped by default. To run them, set up a
separate `mongod` with Kerberos and set its host and Kerberos principal name
as environment variables:

* `MONGOC_TEST_GSSAPI_HOST` 
* `MONGOC_TEST_GSSAPI_USER` 

URI-escape the username, for example write "user@realm" as "user%40realm".
The user must be authorized to query `kerberos.test`.

MongoDB 3.2 adds support for readConcern, but does not enable support for
read concern majority by default. mongod must be launched using
`--enableMajorityReadConcern`.
The test framework does not (and can't) automatically discover if this option was
provided to MongoDB, so an additional variable must be set to enable these tests:

* `MONGOC_ENABLE_MAJORITY_READ_CONCERN`

Set this environment variable to `on` if MongoDB has enabled majority read concern.

Some tests require Internet access, e.g. to check the error message when failing
to open a MongoDB connection to example.com. Skip them with:

* `MONGOC_TEST_OFFLINE=on`

Some tests require a running MongoDB server. Skip them with:

* `MONGOC_TEST_SKIP_LIVE=on`

For quick checks during development, disable long-running tests:

* `MONGOC_TEST_SKIP_SLOW=on`

Some tests run against a local mock server, these can be skipped with:

* `MONGOC_TEST_SKIP_MOCK=on`

If you have started with MongoDB with `--ipv6`, you can test IPv6 with:

* `MONGOC_CHECK_IPV6=on`

The tests for mongodb+srv:// connection strings require some setup, see the
Initial DNS Seedlist Discovery Spec. By default these connection strings are
NOT tested, enable them with:

* `MONGOC_TEST_DNS=on`

The mock server timeout threshold for future functions can be set with:

* `MONGOC_TEST_FUTURE_TIMEOUT_MS=<int>`

This is useful for debugging, so future calls don't timeout when stepping through code.

All tests should pass before submitting a patch.

## Configuring the test runner

The test runner can be configured with command-line options. Run `test-libmongoc
--help` for details.

To run just a specific portion of the test suite use the -l option like so:

```
$ ./test-libmongoc -l "/server_selection/*"
```

The full list of tests is shown in the help.

## Creating and checking a distribution tarball

The `make distcheck` command can be used to confirm that any modifications are
able to be packaged into the distribution tarball and that the resulting
distribution tarball can be used to successfully build the project.

A failure of the `make distcheck` target is an indicator of an oversight in the
modification to the project. For example, if a new source file is added to the
project but it is not added to the proper distribution list, it is possible that
the distribution tarball will be created without that file. An attempt to build
the project without the file is likely to fail.

When `make distcheck` is invoked, several things happen. The `dist` target is
executed to create a distribution tarball. Then the tarball is unpacked,
configured (with an invocation of `cmake`), built (by calling `make`), installed
(by calling `make install`), and tested (by calling `make check`). Three
environment variables can be used to modify these steps.

To adjust the options passed to `make` during the build step, set:

* `DISTCHECK_BUILD_OPTS`

If this variable is not set, then `make` is called with a default of "-j 8".

To adjust the options passed to `make install` during the installation step,
set:

* `DISTCHECK_INSTALL_OPTS`

To adjust the options passed to `make check` during the test step, set:

* `DISTCHECK_CHECK_OPTS`

Remember, if you want to modify the top-level `make` invocation, you will need
to pass options on the command line as normal.

For example, the command `make -j 6 distcheck DISTCHECK_BUILD_OPTS="-j 4"` will
call the standard sequence of targets depended upon by `distcheck` with a
parallelism level of 6, while the build step that is later called by the
`distcheck` target will be executed with a parallelism level of 4.
