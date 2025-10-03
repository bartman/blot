/* blot: internal object that calculates axis tick/text placement */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#ifdef __cplusplus
#define BLOT_EXTERN_C_START extern "C" {
#define BLOT_EXTERN_C_END }
#else
#define BLOT_EXTERN_C_START
#define BLOT_EXTERN_C_END
#endif

#ifdef _WIN32
    // Windows build
    #ifdef BLOT_LIB_BUILD
        // When building the DLL, export symbols with dllexport
        #define BLOT_API __declspec(dllexport)
    #else
        // When using the DLL, import symbols with dllimport
        #define BLOT_API __declspec(dllimport)
    #endif
#else
    // Non-Windows (GCC, Clang) build
    // Uses attribute visibility for symbol export
    #define BLOT_API __attribute__ ((visibility ("default")))
#endif
