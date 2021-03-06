#ifndef BLADE_CONFIG_H
#define BLADE_CONFIG_H

#define BLADE_EXTENSION ".b"
#define BLADE_VERSION_STRING "0.0.5"
#define BVM_VERSION "0.0.5"
#define LIBRARY_DIRECTORY "libs"
#define LIBRARY_DIRECTORY_INDEX "index"

// global debug mode flag
#define DEBUG_MODE 0

#define MAX_USING_CASES 256
#define MAX_FUNCTION_PARAMETERS 255
#define FRAMES_MAX 512
#define NUMBER_FORMAT "%.16g"
#define MAX_INTERPOLATION_NESTING 8
#define MAX_EXCEPTION_HANDLERS 16

// Maximum load factor of 12/14
// see: https://engineering.fb.com/2019/04/25/developer-tools/f14/
#define TABLE_MAX_LOAD 0.85714286

#define GC_HEAP_GROWTH_FACTOR 2

#define USE_NAN_BOXING 1
#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8

#endif