cmake_minimum_required (VERSION 2.8.11)
project (mrAiNoises)

# add custom CMake find scripts
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# set ARNOLD_BASE_DIR to whatever folder contains the Arnold SDK (which must be downloaded separately) include dir.
# Pass this value to the `cmake` command, e.g `cmake .. -DARNOLD_BASE_DIR=`pwd`/../deps/Arnold-5.1.1.1-linux`

# set MTOA_BASE_DIR to whatever folder contains the MtoA include dir
IF (NOT DEFINED(ARNOLD_BASE_DIR))
        MESSAGE(WARNING "ARNOLD_BASE_DIR not set. Using default value")
	IF (WIN32)
		set(ARNOLD_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/Arnold-4.2.14.4-windows)
	ELSE()
		set(ARNOLD_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/Arnold-4.2.15.1-linux)
	ENDIF()
ENDIF()
find_package(Arnold REQUIRED)

# if (WIN32) // theoretically, we can use any GLM in the system, but for safety, we always use the one we already have (newer GLM has API incompatibilities)
	set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/glm-0.9.8.2/glm)
# endif(WIN32)
find_package(GLM REQUIRED) # for Simplex.h
include_directories(${GLM_INCLUDE_DIRS})

include_directories(${ARNOLD_INCLUDE_DIRS})
link_directories(${ARNOLD_LIBRARY_DIRS})
set(LIBS
  ${ARNOLD_LIBS}
)
MESSAGE( STATUS "LIBS:     " ${LIBS} )

include_directories(src)

add_library (mrAiWorleynoise SHARED "src/mrAiWorleynoise.cpp" ${HEADER_FILES})
target_link_libraries(mrAiWorleynoise ${LIBS})

add_library (mrAiCurlnoise SHARED "src/mrAiCurlnoise.cpp" ${HEADER_FILES})
target_link_libraries(mrAiCurlnoise ${LIBS})



IF (WIN32)
ELSE()
  set_target_properties(mrAiWorleynoise PROPERTIES PREFIX "") # remove the "lib" prefix
  set_target_properties(mrAiCurlnoise PROPERTIES PREFIX "")
ENDIF()
