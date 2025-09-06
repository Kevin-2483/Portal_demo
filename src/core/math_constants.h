#pragma once

// Windows MSVC 数学常数定义
// 为了确保跨平台兼容性，我们在这里统一定义数学常数
#ifdef _WIN32
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962
#endif
#else
// 非 Windows 平台通常已经定义了这些常数
#include <cmath>
#endif
