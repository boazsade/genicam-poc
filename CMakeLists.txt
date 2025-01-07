cmake_minimum_required(VERSION 3.18.0)
cmake_policy(VERSION 3.18.0)

cmake_policy(SET CMP0091 NEW)
include( GNUInstallDirs )

project(recorder
        LANGUAGES C CXX 
        VERSION 1.4.0
        DESCRIPTION "image recorder"
)

option(VIMBA_SDK "Use VIMBA SDK as our backend" ON)

add_compile_definitions(_UNIX_)
add_compile_definitions(_LINUX_)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Build the libraries with -fPIC
set(OPENCV_VERSION "4.10.0")
find_package(OpenCV ${OPENCV_VERSION} REQUIRED)
if (MSVC)
	message("Building for Windows platform")
else()
	message("Building on Linux operation system")
	if (VIMBA_SDK)
		message("-------------------------- We are building the project with Vimba SDK ------------------------------")
		set (SDK_INSTALL_DIR "/opt/Vimba_6_0/")
		# Verify that the SDK is located where it needs to be
		find_path(VimbaSDK_INCLUDE
			NAMES VimbaCPP.h
			PATHS ${SDK_INSTALL_DIR}/VimbaCPP/Include
			REQUIRED
		)
		add_compile_definitions(BUILD_WITH_VIMBA_SDK)
		set(SDK_INCLUDE_DIR ${SDK_INSTALL_DIR})
		set (SDK_CPP_LIB_PATH "${SDK_INSTALL_DIR}/VimbaCPP/DynamicLib/x86_64bit")
		set (SDK_C_LIB_PATH "${SDK_INSTALL_DIR}/VimbaC/DynamicLib/x86_64bit")
		set (SDK_TRANSFORM_LIB_PATH "${SDK_INSTALL_DIR}/VimbaImageTransform/DynamicLib/x86_64bit")
		set (SDK_BASE_LIBS_LIST VimbaCPP VimbaC)
		set (SDK_TRANSFORM_LIB_LIST VimbaImageTransform)
	endif(VIMBA_SDK)
endif()
set(CMAKE_CXX_STANDARD 20)


find_package(glog REQUIRED)
message("found SDK_INCLUDE_DIR at ${SDK_INCLUDE_DIR} from ${SDK_INSTALL_DIR}")

foreach(gc_lib ${SDK_BASE_LIBS_LIST})
	list (APPEND SDK_BASE_LIBS "-l${gc_lib}")
endforeach()
foreach(gc_lib ${SDK_TRANSFORM_LIB_LIST})
	list (APPEND SDK_TRANSFORM_LIBS "-l${gc_lib}")
endforeach()
foreach(gc_lib ${SDK_CPP_LIB_PATH})
	list (APPEND SDK_BASE "-L${gc_lib}")
endforeach()
foreach(gc_lib ${SDK_TRANSFORM_LIB_PATH})
	list (APPEND SDK_TRANSFORM "-L${gc_lib}")
endforeach()

include_directories(
	${SDK_INCLUDE_DIR}
	${OpenCV_INCLUDE_DIR}
	SYSTEM
)

list(APPEND CMAKE_BUILD_RPATH ${SDK_CPP_LIB_PATH})

message("The vimba include dir is at ${SDK_INCLUDE_DIR}")
add_subdirectory(libs)
add_subdirectory(tests)
add_subdirectory(apps)
