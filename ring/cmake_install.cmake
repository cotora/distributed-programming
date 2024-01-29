# Install script for directory: /home/mk11/workspace/ring

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/ring" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/ring")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/ring"
         RPATH "/home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/mochi-margo-0.14.1-g7lea6qedrijuwd4bddfj5wlahuhb74l/lib:/home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/mercury-2.3.0-g3ej7kyvdabjfstoe7zexfm6od634j7k/lib:/home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/argobots-1.1-bzkxecvowmk4usnv6aj3r76ulsncicoq/lib:/home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/json-c-0.16-sjl5xurhsorwree3l3kliyqfzqdaxdqx/lib")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/mk11/workspace/ring/ring")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/ring" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/ring")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/ring"
         OLD_RPATH "/home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/mochi-margo-0.14.1-g7lea6qedrijuwd4bddfj5wlahuhb74l/lib:/home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/mercury-2.3.0-g3ej7kyvdabjfstoe7zexfm6od634j7k/lib:/home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/argobots-1.1-bzkxecvowmk4usnv6aj3r76ulsncicoq/lib:/home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/json-c-0.16-sjl5xurhsorwree3l3kliyqfzqdaxdqx/lib:"
         NEW_RPATH "/home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/mochi-margo-0.14.1-g7lea6qedrijuwd4bddfj5wlahuhb74l/lib:/home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/mercury-2.3.0-g3ej7kyvdabjfstoe7zexfm6od634j7k/lib:/home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/argobots-1.1-bzkxecvowmk4usnv6aj3r76ulsncicoq/lib:/home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/json-c-0.16-sjl5xurhsorwree3l3kliyqfzqdaxdqx/lib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/ring")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/mk11/workspace/ring/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
