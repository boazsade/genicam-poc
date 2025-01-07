get_filename_component(libName ${CMAKE_CURRENT_SOURCE_DIR} NAME)

file(GLOB src_files *.cpp *.h)
if (VIMBA_SDK)
  file(GLOB vimba_src vimba/*.hpp vimba/*.hh vimba/*.cpp vimba/*.h)
endif()
add_library(${libName} STATIC ${src_files} ${vimba_src})
target_include_directories(${libName} PUBLIC . ${glog_INCLUDE_DIRS})
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)
target_link_libraries( ${libName} glog::glog)
target_include_directories(${libName} PUBLIC .)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. 
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
