# The name of our project is "HELLO".  CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
project (VITA)

cmake_minimum_required(VERSION 2.6)

include(CPack)

if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -w -fopenmp -std=gnu++11 -gdwarf-3")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC -Wall -Wno-unknown-pragmas -Werror -gdwarf-3")
endif()

#FIND_PATH(VTK_INCLUDE_DIR vtkpoints.h /usr/local/include/vtk-5.2/)

#set (VTK_DIR /usr/lib64/cmake/vtk)

set (PPF_DIR /jet/share/lib)
set (PPF_INCLUDE /jet/share/include)
set (FLUSH_DIR /usr/local/depot/flush-2.1.0/lib-pgi)
set (FLUSH_INCLUDE /usr/local/depot/flush-2.1.0/include)

# Recurse into the "Hello" and "Demo" subdirectories.  This does not actually
# cause another cmake executable to run.  The same process will walk through
# the project's entire directory structure.
add_subdirectory (src)
#add_subdirectory ()


