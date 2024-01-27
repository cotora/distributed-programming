cmake_minimum_required(VERSION 3.10)

# set the project name
project(DFS)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED IMPORTED_TARGET fuse)
pkg_check_modules(MARGO REQUIRED IMPORTED_TARGET margo)
link_libraries(PkgConfig::MARGO)
add_compile_options(-g -Wall)

set(CMAKE_MACOSX_RPATH 1)
SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# add the executable
add_executable(dfs_client dfs_client.c types.h)
add_executable(dfs_server dfs_server.c types.h)
target_link_libraries(dfs_client PRIVATE PkgConfig::FUSE)
target_link_libraries(dfs_server PRIVATE PkgConfig::FUSE)
install(TARGETS dfs_client dfs_server)