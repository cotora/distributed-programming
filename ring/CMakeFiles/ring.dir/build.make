# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/mk11/workspace/ring

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/mk11/workspace/ring

# Include any dependencies generated for this target.
include CMakeFiles/ring.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/ring.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/ring.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ring.dir/flags.make

CMakeFiles/ring.dir/ring.c.o: CMakeFiles/ring.dir/flags.make
CMakeFiles/ring.dir/ring.c.o: ring.c
CMakeFiles/ring.dir/ring.c.o: CMakeFiles/ring.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mk11/workspace/ring/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/ring.dir/ring.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/ring.dir/ring.c.o -MF CMakeFiles/ring.dir/ring.c.o.d -o CMakeFiles/ring.dir/ring.c.o -c /home/mk11/workspace/ring/ring.c

CMakeFiles/ring.dir/ring.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/ring.dir/ring.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mk11/workspace/ring/ring.c > CMakeFiles/ring.dir/ring.c.i

CMakeFiles/ring.dir/ring.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/ring.dir/ring.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mk11/workspace/ring/ring.c -o CMakeFiles/ring.dir/ring.c.s

# Object files for target ring
ring_OBJECTS = \
"CMakeFiles/ring.dir/ring.c.o"

# External object files for target ring
ring_EXTERNAL_OBJECTS =

ring: CMakeFiles/ring.dir/ring.c.o
ring: CMakeFiles/ring.dir/build.make
ring: /home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/mochi-margo-0.14.1-g7lea6qedrijuwd4bddfj5wlahuhb74l/lib/libmargo.so
ring: /usr/lib/aarch64-linux-gnu/libpthread.a
ring: /home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/mercury-2.3.0-g3ej7kyvdabjfstoe7zexfm6od634j7k/lib/libmercury.so
ring: /home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/argobots-1.1-bzkxecvowmk4usnv6aj3r76ulsncicoq/lib/libabt.so
ring: /home/mk11/spack/opt/spack/linux-ubuntu22.04-aarch64/gcc-11.4.0/json-c-0.16-sjl5xurhsorwree3l3kliyqfzqdaxdqx/lib/libjson-c.so
ring: CMakeFiles/ring.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/mk11/workspace/ring/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable ring"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ring.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ring.dir/build: ring
.PHONY : CMakeFiles/ring.dir/build

CMakeFiles/ring.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ring.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ring.dir/clean

CMakeFiles/ring.dir/depend:
	cd /home/mk11/workspace/ring && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/mk11/workspace/ring /home/mk11/workspace/ring /home/mk11/workspace/ring /home/mk11/workspace/ring /home/mk11/workspace/ring/CMakeFiles/ring.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ring.dir/depend

