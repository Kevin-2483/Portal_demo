cmake_minimum_required(VERSION 3.16)
project(PortalSystem VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 添加子目录
add_subdirectory(lib)

# 设置目录
set(CONSOLE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/console")

# 控制台应用程序
add_executable(portal_console
    ${CONSOLE_SOURCE_DIR}/main.cpp
    ${CONSOLE_SOURCE_DIR}/portal_console.cpp
    ${CONSOLE_SOURCE_DIR}/portal_console.h
)

# 链接库
target_link_libraries(portal_console PRIVATE portal_core)

# 设置包含目录
target_include_directories(portal_console PRIVATE
    ${CONSOLE_SOURCE_DIR}
    lib/include
)

# 设置编译特性
target_compile_features(portal_console PRIVATE cxx_std_17)

# 安装
install(TARGETS portal_console
    RUNTIME DESTINATION bin
)

# 打印构建信息
message(STATUS "Portal System Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")


# 添加自定义目标
add_custom_target(run_console
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/portal_console
    DEPENDS portal_console
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Portal Console"
)
