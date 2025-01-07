get_filename_component(libName ${CMAKE_CURRENT_SOURCE_DIR} NAME)
message("===== Library: '${libName}' ====================")

if(NOT MSVC)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fcoroutines -std=c++20")    
endif(NOT MSVC)


file(GLOB src_files *.cpp *.h *.cc *.hh)
add_library(${libName} STATIC ${src_files}) 

target_compile_definitions(${libName} PUBLIC DAA_VERSION="v${CMAKE_PROJECT_VERSION}")

if (MSVC)
    add_compile_definitions(NOMINMAX)
endif(MSVC)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)
target_include_directories(${libName} PUBLIC .)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. 
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

