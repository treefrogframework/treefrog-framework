file (GLOB_RECURSE orchestration_configs_DIST_pems RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.pem)
file (GLOB_RECURSE orchestration_configs_DIST_jsons RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.json)

set_dist_list (orchestration_configs_DIST
   CMakeLists.txt
   ${orchestration_configs_DIST_pems}
   ${orchestration_configs_DIST_jsons}
)

