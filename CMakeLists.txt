cmake_minimum_required(VERSION 3.23)
project(Maths_4RVJV_2024-2025_S2)

set(CMAKE_CXX_STANDARD 20)
file(TO_CMAKE_PATH "$ENV{USERPROFILE}" USER_HOME_DIRECTORY)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENGL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

find_package(glfw3 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

find_package(GLEW REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE GLEW::GLEW)

if (EXISTS "${CMAKE_SOURCE_DIR}/LocalConfig.cmake")
    message(STATUS "Including user-specific LocalConfig.cmake")
    include("${CMAKE_SOURCE_DIR}/LocalConfig.cmake")
endif ()

add_custom_target(copy_shader ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shader ${CMAKE_BINARY_DIR}/shader
        DEPENDS ${MY_TARGET})
add_dependencies(${PROJECT_NAME} copy_shader)
