cmake_minimum_required(VERSION 3.10)
project(play C)

set(CMAKE_C_STANDARD 99)
set(PHP_SOURCE /usr/local/php7_debug/include/php)

set(PHPEXT_SOURCE_FILES
        config.h
        play.c
        php_play.h
        play_interface/play_interface_action.c
        play_interface/play_interface_context.c
        play_interface/play_interface_crontab.c
        play_interface/play_interface_db.c
        play_interface/play_interface_input.c
        play_interface/play_interface_meta_router_abstract.c
        play_interface/play_interface_output.c
        play_interface/play_interface_play.c
        play_interface/play_interface_play_crontab.c
        play_interface/play_interface_play_init.c
        play_interface/play_interface_play_reconst.c
        play_interface/play_interface_processor.c
        play_interface/play_interface_query.c
        play_interface/play_interface_render_abstract.c
        play_interface/play_interface_utils.c
        play_interface/play_interface_net.c)
set(SOURCE_FILES
        main.c
        play_core/play_string.c
        play_core/tool.c
        play_core/play_manager_action.c
        play_core/play_global_config.c
        play_core/play_manager_meta.c
        play_core/play_socket.c)

set (Test main.c)

add_executable(test ${Test})
add_executable(play ${PHPEXT_SOURCE_FILES})
add_executable(run_play ${SOURCE_FILES})
target_link_libraries(run_play libxml2.dylib)
target_link_libraries(run_play libssl.dylib)

include_directories(${PHP_SOURCE}/Zend)
include_directories(${PHP_SOURCE}/sapi)
include_directories(${PHP_SOURCE}/pear)
include_directories(${PHP_SOURCE}/main)
include_directories(${PHP_SOURCE})