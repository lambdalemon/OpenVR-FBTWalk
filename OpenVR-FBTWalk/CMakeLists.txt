cmake_minimum_required(VERSION 3.5)

project(OpenVR-FBTWalk)

set(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty)

add_library(gl3w STATIC ${THIRDPARTY_DIR}/gl3w/src/gl3w.c)
target_include_directories(gl3w PRIVATE ${THIRDPARTY_DIR}/gl3w/include)

add_library(imgui STATIC
	${THIRDPARTY_DIR}/imgui/imgui.cpp
	${THIRDPARTY_DIR}/imgui/imgui_draw.cpp
	${THIRDPARTY_DIR}/imgui/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PRIVATE ${THIRDPARTY_DIR}/gl3w/include)


add_executable(OpenVR-FBTWalk
	OpenVR-FBTWalk.cpp
	config.h
	config.cpp
	fbtwalk_context.h
	image_helpers.h
	image_helpers.cpp
	ui.h
	ui.cpp
	ui_translator.h
	ui_translator.cpp
	move.h
	move.cpp
)

target_include_directories(OpenVR-FBTWalk PRIVATE
	${THIRDPARTY_DIR}/gl3w/include
	${THIRDPARTY_DIR}/openvr/headers
	${THIRDPARTY_DIR}/stb
	${THIRDPARTY_DIR}
)

target_link_libraries(OpenVR-FBTWalk gl3w imgui)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	find_package(glfw3 REQUIRED NO_MODULE)
	target_link_libraries(OpenVR-FBTWalk glfw libopenvr_api.so)

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_include_directories(OpenVR-FBTWalk PRIVATE ${THIRDPARTY_DIR}/glfw/include)
	target_link_libraries(OpenVR-FBTWalk
		gdi32
		${THIRDPARTY_DIR}/glfw/lib-mingw-w64/libglfw3.a
		${THIRDPARTY_DIR}/openvr/lib/win64/openvr_api.lib
	)
endif()
