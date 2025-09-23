#ifndef PORTAL_DEBUG_LOGGING_H
#define PORTAL_DEBUG_LOGGING_H

// 包含了项目的主编译配置文件，以获取 PORTAL_DEBUG_ENABLED 的定义
#include "portal_build_config.h"

// 只有在调试模式开启时，才包含 iostream 并定义有效的日志宏
#if defined(PORTAL_DEBUG_ENABLED) && PORTAL_DEBUG_ENABLED

  #include <iostream>

  // 带换行符的日志
  #define PORTAL_DEBUG_LOG(message)       do { std::cout << message << std::endl; } while(0)
  #define PORTAL_DEBUG_ERROR(message)     do { std::cerr << message << std::endl; } while(0)
  
  // 不带换行符的日志
  #define PORTAL_DEBUG_LOG_SIMPLE(message)    do { std::cout << message; } while(0)
  #define PORTAL_DEBUG_ERROR_SIMPLE(message)  do { std::cerr << message; } while(0)

#else

  // 在发布模式下，所有日志宏都为空，编译器会将其完全优化掉
  #define PORTAL_DEBUG_LOG(message)       do {} while(0)
  #define PORTAL_DEBUG_ERROR(message)     do {} while(0)
  #define PORTAL_DEBUG_LOG_SIMPLE(message)    do {} while(0)
  #define PORTAL_DEBUG_ERROR_SIMPLE(message)  do {} while(0)

#endif // PORTAL_DEBUG_ENABLED

#endif // PORTAL_DEBUG_LOGGING_H