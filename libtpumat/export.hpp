#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(LIBTPUMAT_BUILDING_DLL)
#define LIBTPUMAT_API __declspec(dllexport)
#else
#define LIBTPUMAT_API __declspec(dllimport)
#endif
#else
#define LIBTPUMAT_API
#endif
