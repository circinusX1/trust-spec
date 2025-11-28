cmake_minimum_required(VERSION 3.19)
project(sanesystem LANGUAGES CXX)

set(BIN_NAME sanesystem)

add_executable(sanesystem
    main.cpp
    bin/sanesystem.conf
    sane-dev/sane-dev.c
    ini_file.h
    crc32sum_t.h
    paralel_crc_t.h paralel_crc_t.cpp
    map_file_mem_t.h
    sane_utils_t.h sane_utils_t.cpp
    sane_sys_t.h sane_sys_t.cpp
    sane_files_t.h sane_files_t.cpp
    sane_cmd_out_t.h sane_cmd_out_t.cpp
    sane_procs_t.h sane_procs_t.cpp
    udp_to_kernel_t.h udp_to_kernel_t.cpp
)

add_definitions(-DCPLUSPLUS)
include(GNUInstallDirs)

message(STATUS ">> ${PROJECT_SOURCE_DIR} <- ${PROJECT_BINARY_DIR} ")

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_BINARY_DIR}/${BIN_NAME}
    ${PROJECT_SOURCE_DIR}/bin/${BIN_NAME}
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/bin/
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILES_MATCHING PATTERN "${BIN_NAME}"
)
