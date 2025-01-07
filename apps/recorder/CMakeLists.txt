get_filename_component(AppName ${CMAKE_CURRENT_SOURCE_DIR} NAME)
message("===== TestApp: project: ${AppName}")

file(GLOB src_files *.cpp *.h *.hh *.cc)
add_executable(${AppName} ${src_files})
target_compile_definitions(${AppName} PUBLIC AppName="${AppName}")
set_property(TARGET ${appName} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(${AppName} PRIVATE
	${VIMBA_BASE} ${VIMBA_BASE_LIBS}
	${VIMBA_TRANSFORM} ${VIMBA_TRANSFORM_LIBS}
)

include_directories(
    ${CMAKE_SOURCE_DIR}/.
    ${CMAKE_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/../libs
    ${SDK_INCLUDE_DIR}
)
