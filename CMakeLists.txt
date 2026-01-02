cmake_minimum_required(VERSION 3.10)
project(BellaEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Vulkan_INCLUDE_DIR "C:/VulkanSDK/1.3.290.0/Include") 
set(Vulkan_LIBRARY "C:/VulkanSDK/1.3.290.0/Lib/vulkan-1.lib")

# 2. 包含头文件路径
include_directories(
    ${Vulkan_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/headers
    "D:/Bella/vendor/include"
)

link_directories("D:/Bella/vendor/lib/x64")

# 3. 源文件
file(GLOB SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(Bella ${SRC_FILES})

# 4. 链接 Vulkan 库
target_link_libraries(Bella
    ${Vulkan_LIBRARY}
    glfw3
    sl.interposer
)
