cmake_minimum_required(VERSION 3.10)
project(BellaEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(VULKAN_SDK "$ENV{VULKAN_SDK}")

if(VULKAN_SDK)
    set(Enable_Vulkan_Shader_Recompile On)
else()
    set(Enable_Vulkan_Shader_Recompile Off)
endif()


find_package(Vulkan REQUIRED)

file(GLOB SRC_FILES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(Bella ${SRC_FILES})

target_include_directories(Bella PRIVATE
    ${CMAKE_SOURCE_DIR}/headers
    ${CMAKE_SOURCE_DIR}/vendor/include
)

target_link_directories(Bella PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/lib/x64
)

target_link_libraries(Bella PRIVATE
    Vulkan::Vulkan
    glfw3
    sl.interposer
)
