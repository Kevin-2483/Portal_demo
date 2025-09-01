cmake_minimum_required(VERSION 3.16)
project(PortalCore VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 设置目录
set(CORE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(CORE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# 收集源文件
file(GLOB_RECURSE CORE_HEADERS "${CORE_INCLUDE_DIR}/*.h")
file(GLOB_RECURSE CORE_SOURCES "${CORE_SOURCE_DIR}/*.cpp")

# 创建静态库
add_library(portal_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

# 设置包含目录
target_include_directories(portal_core PUBLIC
    $<BUILD_INTERFACE:${CORE_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# 设置编译特性
target_compile_features(portal_core PUBLIC cxx_std_17)

# 可选：创建示例可执行文件
option(BUILD_PORTAL_EXAMPLES "Build Portal System examples" OFF)

if(BUILD_PORTAL_EXAMPLES)
    add_executable(portal_example)
    target_sources(portal_example PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/main.cpp"
    )
    target_link_libraries(portal_example PRIVATE portal_core)
    
    # 设置示例的包含目录
    target_include_directories(portal_example PRIVATE ${CORE_INCLUDE_DIR})
endif()

# 安装配置
install(TARGETS portal_core
    EXPORT PortalCoreTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# 创建配置文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "PortalCoreConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PortalCoreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/PortalCoreConfig.cmake"
    INSTALL_DESTINATION lib/cmake/PortalCore
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/PortalCoreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/PortalCoreConfigVersion.cmake"
    DESTINATION lib/cmake/PortalCore
)

install(EXPORT PortalCoreTargets
    FILE PortalCoreTargets.cmake
    NAMESPACE PortalCore::
    DESTINATION lib/cmake/PortalCore
)

# 可选：创建 pkg-config 文件
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/portal_core.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/portal_core.pc"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/portal_core.pc"
    DESTINATION lib/pkgconfig
)

# 打印构建信息
message(STATUS "Portal Core Library Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Examples: ${BUILD_PORTAL_EXAMPLES}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
