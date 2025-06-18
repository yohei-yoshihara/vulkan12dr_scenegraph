cmake_minimum_required(VERSION 3.28)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(triangle)

add_executable(${PROJECT_NAME}
  src/main.hpp src/main.cpp
  src/common.hpp src/common.cpp
  src/types.hpp src/types.cpp
  src/mesh.hpp src/mesh.cpp
  src/node.hpp src/node.cpp

  src/shapes/mesh_box.hpp src/shapes/mesh_box.cpp
  src/shapes/mesh_cone.hpp src/shapes/mesh_cone.cpp
  src/shapes/mesh_plane.hpp src/shapes/mesh_plane.cpp
  src/shapes/mesh_sphere.hpp src/shapes/mesh_sphere.cpp

  shaders/triangle.vert
  shaders/triangle.frag
)

source_group(shaders FILES
  shaders/triangle.vert
  shaders/triangle.frag
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (MSVC)
    # warning level 4
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    # additional warnings
    #target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wno-missing-braces)
endif()

# Vulkan
find_package(Vulkan REQUIRED COMPONENTS glslc)
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIR})
# target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})

# GLFW
#set(GLFW_INSTALL OFF)
#set(GLFW_LIBRARY_TYPE STATIC)
#add_subdirectory(ext/src/glfw-3.4)
#target_include_directories(${PROJECT_NAME} PRIVATE ext/src/glfw-3.4/include)
#target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# SDL
set(SDL_TESTS OFF)
set(SDL_TEST_LIBRARY OFF)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
add_subdirectory(ext/src/SDL3-3.2.14)
target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ext/src/SDL3-3.2.14/include)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

# spdlog
add_subdirectory(ext/src/spdlog-1.15.3)
target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ext/src/spdlog-1.15.3/include)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

# glm
add_subdirectory(ext/src/glm)
target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ext/src/glm)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

# vk-bootstrap
add_subdirectory(ext/src/vk-bootstrap-1.4.313)
target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ext/src/vk-bootstrap-1.4.313/src)
target_link_libraries(${PROJECT_NAME} PRIVATE vk-bootstrap::vk-bootstrap)

# Vulkan Memory Allocator
add_subdirectory(ext/src/VulkanMemoryAllocator-3.3.0)
target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ext/src/VulkanMemoryAllocator-3.3.0/include)
target_link_libraries(${PROJECT_NAME} PRIVATE VulkanMemoryAllocator)

# volk
add_subdirectory(ext/src/volk-1.4.313)
target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ext/src/volk-1.4.313)
target_link_libraries(${PROJECT_NAME} PRIVATE volk::volk)

# Vulkan Headers
#target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ext/src/Vulkan-Headers-vulkan-sdk-1.4.304/include)

# fastglft
#add_subdirectory(ext/src/fastgltf-0.8.0)
#target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ext/src/fastgltf-0.8.0/include)
#target_link_libraries(${PROJECT_NAME} PRIVATE fastgltf::fastgltf)

# fmt
#add_subdirectory(ext/src/fmt-11.1.4)
#target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ext/src/fmt-11.1.4/include)
#target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

# stb
#target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ext/src/stb)

# tiny obj loader
#target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ext/src/tinyobjloader)

# imgui
#target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ext/src/imgui)

# shaders

set(GLSLC ${Vulkan_GLSLC_EXECUTABLE})
set(GLSLC_OPTIONS -g --target-env=vulkan1.2)

add_custom_command(
    OUTPUT shaders/triangle.vert.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory shaders
    COMMAND ${GLSLC} ${GLSLC_OPTIONS} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle.vert.spv 
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert
    COMMENT "Compiling vertex shader"
    )

add_custom_command(
    OUTPUT shaders/triangle.frag.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory shaders
    COMMAND ${GLSLC} ${GLSLC_OPTIONS} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle.frag.spv 
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag
    COMMENT "Compiling fragment shader"
    )

add_custom_target(compile_shaders DEPENDS shaders/triangle.vert.spv shaders/triangle.frag.spv)
add_dependencies(${CMAKE_PROJECT_NAME} compile_shaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_CURRENT_BINARY_DIR}/shaders
                   $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)
