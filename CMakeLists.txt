cmake_minimum_required(VERSION 3.23)
project(Maths_4RVJV_2024-2025_S2)

set(CMAKE_CXX_STANDARD 20)
file(TO_CMAKE_PATH "$ENV{USERPROFILE}" USER_HOME_DIRECTORY)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

find_package(OpenGL REQUIRED)

set(BEZIER_HEADER_FILES
        include/GLShader.h
        include/BezierApp.h
        include/BezierCurve.h
        include/Point.h
        include/Shader.h
        include/CyriusBeck.h
        include/ImGuiManager.h
)

set(BEZIER_SOURCE_FILES
        src/GLShader.cpp
        src/BezierApp.cpp
        src/BezierCurve.cpp
        src/Point.cpp
        src/Shader.cpp
        src/ImGuiManager.cpp
        include/ImGuiManager.h
        src/ImGuiManager.cpp
        src/CyriusBeck.cpp
        src/SutherlandHodgman.cpp
)

add_executable(${PROJECT_NAME} main.cpp ${BEZIER_HEADER_FILES} ${BEZIER_SOURCE_FILES})

#add_executable(${PROJECT_NAME} main.cpp src/GLShader.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE
        ${OPENGL_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR} # Add to find include files
)

target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

find_package(glfw3 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

find_package(GLEW REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE GLEW::GLEW)

if (EXISTS "${CMAKE_SOURCE_DIR}/LocalConfig.cmake")
    message(STATUS "Including user-specific LocalConfig.cmake")
    include("${CMAKE_SOURCE_DIR}/LocalConfig.cmake")
endif ()

# Copy shader files to build directory
add_custom_target(copy_shader ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shader ${CMAKE_BINARY_DIR}/shader
        COMMENT "Copying shader files to build directory"
)
add_dependencies(${PROJECT_NAME} copy_shader)

# Téléchargement d'ImGui depuis GitHub
include(FetchContent)
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG v1.89.9
)
FetchContent_MakeAvailable(imgui)

# Créer une bibliothèque ImGui
add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)

find_package(glm QUIET)
if(NOT glm_FOUND)
    include(FetchContent)
    FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG 0.9.9.8
    )
    FetchContent_MakeAvailable(glm)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
        ${OPENGL_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${glm_SOURCE_DIR}
)

target_link_libraries(imgui PRIVATE glfw OpenGL::GL)

# Ajoutez imgui à votre exécutable principal
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)