cmake_minimum_required(VERSION 2.4)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

find_package(DLL QUIET)
find_package(PThread)

#----------------------------------------------------------------------------------

SET(CAPE_STC_SOURCES
  stc/cape_str.c
  stc/cape_list.c
  stc/cape_map.c
  stc/cape_udc.c
  stc/cape_stream.c
)

SET(CAPE_STC_HEADERS
  stc/cape_str.h
  stc/cape_list.h
  stc/cape_map.h
  stc/cape_udc.h
  stc/cape_stream.h
)

#----------------------------------------------------------------------------------

SET(CAPE_SYS_SOURCES
  sys/cape_err.c
  sys/cape_log.c
  sys/cape_file.c
  sys/cape_dl.c
  sys/cape_mutex.c
  sys/cape_socket.c
  sys/cape_pipe.c
)

SET(CAPE_SYS_HEADERS
  sys/cape_err.h
  sys/cape_log.h
  sys/cape_file.h
  sys/cape_dl.h
  sys/cape_mutex.h
  sys/cape_socket.h
  sys/cape_pipe.h
)

#----------------------------------------------------------------------------------

SET(CAPE_AIO_SOURCES
  aio/cape_aio_ctx.c
  aio/cape_aio_file.c
  aio/cape_aio_sock.c
  aio/cape_aio_timer.c
)

SET(CAPE_AIO_HEADERS
  aio/cape_aio_ctx.h
  aio/cape_aio_file.h
  aio/cape_aio_sock.h
  aio/cape_aio_timer.h
)

#----------------------------------------------------------------------------------

SET(CAPE_FMT_SOURCES
  fmt/cape_parser_json.c
  fmt/cape_json.c
  fmt/cape_tokenizer.c
  fmt/cape_args.c
)

SET(CAPE_FMT_HEADERS
  fmt/cape_parser_json.h
  fmt/cape_json.h
  fmt/cape_tokenizer.h
  fmt/cape_args.h
)

#----------------------------------------------------------------------------------

INCLUDE_DIRECTORIES(".")

add_library             (cape SHARED ${CAPE_STC_SOURCES} ${CAPE_STC_HEADERS} ${CAPE_SYS_SOURCES} ${CAPE_SYS_HEADERS} ${CAPE_AIO_SOURCES} ${CAPE_AIO_HEADERS} ${CAPE_FMT_SOURCES} ${CAPE_FMT_HEADERS})
target_link_libraries   (cape ${DLL_LIBRARIES} ${PTHREAD_LIBRARIES})
set_target_properties   (cape PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

#----------------------------------------------------------------------------------

SET(CAPE_APP_AIO_SOURCES
  app/cape_app_aio.c
)

add_executable          (cape_app ${CAPE_APP_AIO_SOURCES})
target_link_libraries   (cape_app cape)
set_target_properties   (cape_app PROPERTIES VERSION 1.0.1 SOVERSION 1.0.1)

#----------------------------------------------------------------------------------

SUBDIRS(tests)
