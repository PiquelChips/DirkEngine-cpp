cmake_minimum_required(VERSION 3.31)

project(DirkEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

find_package(Vulkan REQUIRED)

add_subdirectory(thirdparty/glfw-3.4)
add_subdirectory(thirdparty/glm)
add_subdirectory(thirdparty/stb_image)
add_subdirectory(thirdparty/tiny_obj_loader)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
file(GLOB_RECURSE SHADER_FILES 
    "${SHADER_SOURCE_DIR}/*.vert"
    "${SHADER_SOURCE_DIR}/*.frag"
    "${SHADER_SOURCE_DIR}/*.comp"
    "${SHADER_SOURCE_DIR}/*.geom"
    "${SHADER_SOURCE_DIR}/*.tesc"
    "${SHADER_SOURCE_DIR}/*.tese"
)

# Shader compilation target
add_custom_target(shaders)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)

foreach(SHADER_FILE ${SHADER_FILES})
    file(RELATIVE_PATH SHADER_RELATIVE_PATH ${SHADER_SOURCE_DIR} ${SHADER_FILE})
    set(SHADER_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_RELATIVE_PATH}.spv)

    get_filename_component(SHADER_OUTPUT_DIR ${SHADER_OUTPUT} DIRECTORY)

    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
        COMMAND Vulkan::glslc -o ${SHADER_OUTPUT} ${SHADER_FILE}
        DEPENDS ${SHADER_FILE}
        VERBATIM
    )

    string(REPLACE "/" "_" SHADER_TARGET_NAME ${SHADER_RELATIVE_PATH})
    string(REPLACE "." "_" SHADER_TARGET_NAME ${SHADER_TARGET_NAME})

    add_custom_target(shader_${SHADER_TARGET_NAME} DEPENDS ${SHADER_OUTPUT})
    add_dependencies(shaders shader_${SHADER_TARGET_NAME})
endforeach()

add_dependencies(${PROJECT_NAME} shaders)

# Preprocessor macros
target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC
    GLFW_INCLUDE_NONE=1
    GLFW_INCLUDE_VULKAN=1

    RESSOURCE_PATH=$<IF:$<CONFIG:Release>,"./resources","${CMAKE_CURRENT_SOURCE_DIR}/resources">
    SHADER_PATH=$<IF:$<CONFIG:Release>,"./resources/shaders","${CMAKE_CURRENT_BINARY_DIR}/shaders">
    LOG_PATH=$<IF:$<CONFIG:Release>,"./logs","${CMAKE_CURRENT_SOURCE_DIR}/logs">

    DEBUG_BUILD=$<CONFIG:Debug>
    RELEASE_BUILD=$<CONFIG:Release>
)

option(DISABLE_VALIDATION_LAYERS "Use the vulkan validation layers" OFF)

if (DISABLE_VALIDATION_LAYERS)
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC DISABLE_VALIDATION_LAYERS)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glm
    stb_image
    tiny_obj_loader
    Vulkan::Vulkan
)

add_custom_target(run
    COMMAND ${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    COMMENT "Running ${PROJECT_NAME}"
)
