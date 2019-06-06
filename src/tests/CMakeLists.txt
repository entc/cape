cmake_minimum_required(VERSION 2.4)

# abstract operation-system layer
INCLUDE_DIRECTORIES("..")

add_executable          (ut_fmt_json ut_fmt_json.c)
target_link_libraries   (ut_fmt_json cape)

add_executable          (ut_stc_string ut_stc_string.c)
target_link_libraries   (ut_stc_string cape)

add_executable          (ut_stc_udc ut_stc_udc.c)
target_link_libraries   (ut_stc_udc cape)
