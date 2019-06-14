/*
 * Copyright 2015-present MongoDB, Inc.
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

#ifndef JSON_TEST_H
#define JSON_TEST_H

#include "TestSuite.h"

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include "test-conveniences.h"
#include "json-test-monitoring.h"
#include "json-test-operations.h"

#define MAX_NUM_TESTS 100

typedef void (*test_hook) (bson_t *test);

typedef struct _json_test_config_t {
   void *ctx;
   const bson_t *scenario;
   json_test_cb_t before_test_cb, after_test_cb;
   json_test_operation_cb_t run_operation_cb;
   json_test_events_check_cb_t events_check_cb;
   bool command_started_events_only;
   bool command_monitoring_allow_subset;
} json_test_config_t;

#define JSON_TEST_CONFIG_INIT {NULL, NULL, NULL, NULL, false}

bson_t *
get_bson_from_json_file (char *filename);

int
collect_tests_from_dir (char (*paths)[MAX_TEST_NAME_LENGTH] /* OUT */,
                        const char *dir_path,
                        int paths_index,
                        int max_paths);

void
assemble_path (const char *parent_path,
               const char *child_name,
               char *dst /* OUT */);

mongoc_topology_description_type_t
topology_type_from_test (const char *type);

mongoc_server_description_t *
server_description_by_hostname (mongoc_topology_description_t *topology,
                                const char *address);

void
process_sdam_test_ismaster_responses (bson_t *phase,
                                      mongoc_topology_description_t *td);

void
test_server_selection_logic_cb (bson_t *test);

mongoc_server_description_type_t
server_type_from_test (const char *type);

void
activate_fail_point (mongoc_client_t *client,
                     uint32_t server_id,
                     const bson_t *opts);

void
deactivate_fail_points (mongoc_client_t *client, uint32_t server_id);

void
run_json_general_test (const json_test_config_t *config);

void
json_test_config_cleanup (json_test_config_t *config);

void
_install_json_test_suite_with_check (TestSuite *suite,
                                     const char *dir_path,
                                     test_hook callback,
                                     ...);

void
install_json_test_suite (TestSuite *suite,
                         const char *dir_path,
                         test_hook callback);

#define install_json_test_suite_with_check(_suite, _dir_path, ...) \
   _install_json_test_suite_with_check (_suite, _dir_path, __VA_ARGS__, NULL)

#endif
