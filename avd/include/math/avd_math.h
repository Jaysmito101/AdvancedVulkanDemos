#ifndef AVD_MATH_H
#define AVD_MATH_H

#include "math/avd_math_base.h"

#ifndef AVD_MATH_USE_SIMD
#include "math/avd_vector_non_simd.h"
#include "math/avd_matrix_non_simd.h"
#else
#error "AVD_MATH_USE_SIMD is not supported in this version."
#endif

#include "math/avd_3d_matrices.h"

#endif // AVD_MATH_H