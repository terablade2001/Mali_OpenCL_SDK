#https://streamcomputing.eu/blog/2015-09-25/handling-opencl-with-cmake-3-1-and-higher/
#Minimal OpenCL CMakeLists.txt by StreamComputing
cmake_minimum_required (VERSION 3.1)
project(image_scaling)

if(ARM_LINUX_SYSROOT)
	# --------------------------------------------------
	# switch searching path to arm linux
	# --------------------------------------------------
	message("ARM_LINUX_SYSROOT=${ARM_LINUX_SYSROOT}")
	set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
	set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
	set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
	set(OPENCL_VER "${OPENCL_VER}")
	set(OpenCL_INCLUDE_DIRS "${OPENCL_INC}")
	set(OpenCL_LIBRARY "${OPENCL_LIB}/libOpenCL.so")
	message("OpenCL_INCLUDE_DIRS=${OpenCL_INCLUDE_DIRS}")
	include_directories(${OpenCL_INCLUDE_DIRS})
elseif(ANDROID_NDK)
else()
	# Handle OpenCL
	find_package(OpenCL REQUIRED)
	message("OpenCL_FOUND : ${OpenCL_FOUND}")
	if(OpenCL_FOUND)
	message("OpenCL_INCLUDE_DIRS=${OpenCL_INCLUDE_DIRS}")
	message("OpenCL_LIBRARY=${OpenCL_LIBRARY}")
	include_directories(${OpenCL_INCLUDE_DIRS})
	endif()
endif()

# --------------------------------------------------
# Organize output files.  On Windows this also keeps .dll files next
# to the .exe files that need them, making tests easy to run.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${image_scaling_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${image_scaling_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${image_scaling_BINARY_DIR}/lib)
message("image_scaling_BINARY_DIR ${image_scaling_BINARY_DIR}")
message("CMAKE_LIBRARY_OUTPUT_DIRECTORY =${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

#add_subdirectory (${image_scaling_SOURCE_DIR}/../../common common)
#add_subdirectory (/home/thomas/openCL/Mali_OpenCL_SDK/common common)
add_library (Common ../../common/common.cpp ../../common/image.cpp)
include_directories(../../common)

link_directories(${OpenCL_LIBRARY})
add_executable (image_scaling image_scaling.cpp
	)
#target_include_directories (image_scaling PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries (image_scaling
	${OpenCL_LIBRARY}
	Common)
