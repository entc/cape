cmake_minimum_required(VERSION 2.4)

# abstract operation-system layer
INCLUDE_DIRECTORIES("..")

add_executable          (ut_json_parser ut_json_parser.c)
target_link_libraries   (ut_json_parser cape)

