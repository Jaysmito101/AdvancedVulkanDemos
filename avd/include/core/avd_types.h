
#ifndef AVD_TYPES_H
#define AVD_TYPES_H

#include <stdbool.h>
#include <stdint.h>

// Redefine common types used in the math module
typedef size_t AVD_Size;

typedef float AVD_Float;
typedef double AVD_Double;

typedef int32_t AVD_Int32;
typedef uint32_t AVD_UInt32;

typedef int8_t AVD_Int8;
typedef uint8_t AVD_UInt8;

typedef int16_t AVD_Int16;
typedef uint16_t AVD_UInt16;

typedef int64_t AVD_Int64;
typedef uint64_t AVD_UInt64;

typedef bool AVD_Bool;

typedef char AVD_Char;
typedef unsigned char AVD_UChar;

typedef void AVD_Void;
typedef void *AVD_DataPtr;

#endif // AVD_TYPES_H
