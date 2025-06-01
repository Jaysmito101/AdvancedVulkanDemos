#include "math/avd_math_tests.h"

static bool __avdCheckMatrix4x4()
{
    AVD_LOG("  Testing Matrix4x4 operations...\n");
    
    // Test matrix creation and basic operations
    AVD_Matrix4x4 mat1 = avdMat4x4(
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f
    );
    
    AVD_Matrix4x4 mat2 = avdMat4x4(
        2.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 2.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 2.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    
    // Test matrix access
    if (!avdIsFEqual(avdMat4x4Val(mat1, 0, 0), 1.0f) ||
        !avdIsFEqual(avdMat4x4Val(mat1, 1, 2), 7.0f) ||
        !avdIsFEqual(avdMat4x4Val(mat1, 3, 3), 16.0f)) {
        AVD_LOG("    FAILED: Matrix4x4 element access\n");
        return false;
    }
    
    // Test identity matrix
    AVD_Matrix4x4 identity = avdMat4x4Identity();
    if (!avdIsFEqual(avdMat4x4Val(identity, 0, 0), 1.0f) ||
        !avdIsFEqual(avdMat4x4Val(identity, 1, 1), 1.0f) ||
        !avdIsFEqual(avdMat4x4Val(identity, 2, 2), 1.0f) ||
        !avdIsFEqual(avdMat4x4Val(identity, 3, 3), 1.0f) ||
        !avdIsFEqual(avdMat4x4Val(identity, 0, 1), 0.0f)) {
        AVD_LOG("    FAILED: Matrix4x4 identity matrix\n");
        return false;
    }
    
    // Test zero matrix
    AVD_Matrix4x4 zero = avdMat4x4Zero();
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (!avdIsFEqual(avdMat4x4Val(zero, i, j), 0.0f)) {
                AVD_LOG("    FAILED: Matrix4x4 zero matrix\n");
                return false;
            }
        }
    }
    
    // Test matrix addition
    AVD_Matrix4x4 addResult = avdMat4x4Add(mat1, mat2);
    if (!avdIsFEqual(avdMat4x4Val(addResult, 0, 0), 3.0f) ||
        !avdIsFEqual(avdMat4x4Val(addResult, 1, 1), 8.0f)) {
        AVD_LOG("    FAILED: Matrix4x4 addition\n");
        return false;
    }
    
    // Test matrix subtraction
    AVD_Matrix4x4 subResult = avdMat4x4Subtract(mat1, mat2);
    if (!avdIsFEqual(avdMat4x4Val(subResult, 0, 0), -1.0f) ||
        !avdIsFEqual(avdMat4x4Val(subResult, 1, 1), 4.0f)) {
        AVD_LOG("    FAILED: Matrix4x4 subtraction\n");
        return false;
    }
    
    // Test matrix scaling
    AVD_Matrix4x4 scaleResult = avdMat4x4Scale(mat1, 2.0f);
    if (!avdIsFEqual(avdMat4x4Val(scaleResult, 0, 0), 2.0f) ||
        !avdIsFEqual(avdMat4x4Val(scaleResult, 1, 2), 14.0f)) {
        AVD_LOG("    FAILED: Matrix4x4 scaling\n");
        return false;
    }
    
    // Test matrix multiplication with identity
    AVD_Matrix4x4 mulResult = avdMat4x4Multiply(mat1, identity);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (!avdIsFEqual(avdMat4x4Val(mulResult, i, j), avdMat4x4Val(mat1, i, j))) {
                AVD_LOG("    FAILED: Matrix4x4 multiplication with identity\n");
                return false;
            }
        }
    }
    
    // Test matrix-vector multiplication
    AVD_Vector4 vec = avdVec4(1.0f, 1.0f, 1.0f, 1.0f);
    AVD_Vector4 vecResult = avdMat4x4MultiplyVec4(identity, vec);
    if (!avdIsFEqual(vecResult.x, 1.0f) || !avdIsFEqual(vecResult.y, 1.0f) ||
        !avdIsFEqual(vecResult.z, 1.0f) || !avdIsFEqual(vecResult.w, 1.0f)) {
        AVD_LOG("    FAILED: Matrix4x4 vector multiplication\n");
        return false;
    }
    
    // Test transpose
    AVD_Matrix4x4 transpose = avdMat4x4Transpose(mat1);
    if (!avdIsFEqual(avdMat4x4Val(transpose, 0, 1), avdMat4x4Val(mat1, 1, 0)) ||
        !avdIsFEqual(avdMat4x4Val(transpose, 2, 3), avdMat4x4Val(mat1, 3, 2))) {
        return false;
    }
    
    // Test row and column access
    AVD_Vector4 row0 = avdMat4x4Row(mat1, 0);
    AVD_Vector4 col0 = avdMat4x4Col(mat1, 0);
    if (!avdIsFEqual(row0.x, 1.0f) || !avdIsFEqual(row0.y, 2.0f) ||
        !avdIsFEqual(col0.x, 1.0f) || !avdIsFEqual(col0.y, 5.0f)) {
        AVD_LOG("    FAILED: Matrix4x4 row/column access\n");
        return false;
    }
    
    // Test in-place operations
    AVD_Matrix4x4 inPlaceTest = mat1;
    avdMat4x4AddInplace(&inPlaceTest, &mat2);
    if (!avdIsFEqual(avdMat4x4Val(inPlaceTest, 0, 0), 3.0f)) {
        AVD_LOG("    FAILED: Matrix4x4 in-place addition\n");
        return false;
    }
    
    AVD_LOG("    Matrix4x4 tests PASSED\n");
    return true;
}

static bool __avdCheckMatrix3x3()
{
    AVD_LOG("  Testing Matrix3x3 operations...\n");
    
    // Test matrix creation
    AVD_Matrix3x3 mat1 = avdMat3x3(
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f
    );
        
    // Test matrix access
    if (!avdIsFEqual(avdMat3x3Val(mat1, 0, 0), 1.0f) ||
        !avdIsFEqual(avdMat3x3Val(mat1, 1, 2), 6.0f) ||
        !avdIsFEqual(avdMat3x3Val(mat1, 2, 1), 8.0f)) {
        AVD_LOG("    FAILED: Matrix3x3 element access\n");
        return false;
    }
    
    // Test identity matrix
    AVD_Matrix3x3 identity = avdMat3x3Identity();
    if (!avdIsFEqual(avdMat3x3Val(identity, 0, 0), 1.0f) ||
        !avdIsFEqual(avdMat3x3Val(identity, 1, 1), 1.0f) ||
        !avdIsFEqual(avdMat3x3Val(identity, 2, 2), 1.0f) ||
        !avdIsFEqual(avdMat3x3Val(identity, 0, 1), 0.0f)) {
        AVD_LOG("    FAILED: Matrix3x3 identity matrix\n");
        return false;
    }
    
    // Test zero matrix
    AVD_Matrix3x3 zero = avdMat3x3Zero();
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (!avdIsFEqual(avdMat3x3Val(zero, i, j), 0.0f)) {
                AVD_LOG("    FAILED: Matrix3x3 zero matrix\n");
                return false;
            }
        }
    }
    
    // Test diagonal matrix
    AVD_Matrix3x3 diagonal = avdMat3x3Diagonal(5.0f);
    if (!avdIsFEqual(avdMat3x3Val(diagonal, 0, 0), 5.0f) ||
        !avdIsFEqual(avdMat3x3Val(diagonal, 1, 1), 5.0f) ||
        !avdIsFEqual(avdMat3x3Val(diagonal, 2, 2), 5.0f) ||
        !avdIsFEqual(avdMat3x3Val(diagonal, 0, 1), 0.0f)) {
        AVD_LOG("    FAILED: Matrix3x3 diagonal matrix\n");
        return false;
    }
    
    // Test transpose
    AVD_Matrix3x3 transpose = avdMat3x3Transpose(mat1);
    if (!avdIsFEqual(avdMat3x3Val(transpose, 0, 1), avdMat3x3Val(mat1, 1, 0)) ||
        !avdIsFEqual(avdMat3x3Val(transpose, 1, 2), avdMat3x3Val(mat1, 2, 1)) ||
        !avdIsFEqual(avdMat3x3Val(transpose, 2, 0), avdMat3x3Val(mat1, 0, 2))) {
        // Log the failure
        avdMat3x3Log(mat1);
        AVD_LOG("    Matrix3x3 transpose:\n");
        avdMat3x3Log(transpose);
        AVD_LOG("    FAILED: Matrix3x3 transpose\n");
        return false;
    }
    
    // Test row and column access
    AVD_Vector3 row0 = avdMat3x3Row(mat1, 0);
    AVD_Vector3 col0 = avdMat3x3Col(mat1, 0);
    if (!avdIsFEqual(row0.x, 1.0f) || !avdIsFEqual(row0.y, 2.0f) || !avdIsFEqual(row0.z, 3.0f) ||
        !avdIsFEqual(col0.x, 1.0f) || !avdIsFEqual(col0.y, 4.0f) || !avdIsFEqual(col0.z, 7.0f)) {
        AVD_LOG("    FAILED: Matrix3x3 row/column access\n");
        return false;
    }
    
    // Test conversion from Matrix4x4
    AVD_Matrix4x4 mat4 = avdMat4x4Identity();
    AVD_Matrix3x3 fromMat4 = avdMat3x3FromMat4x4(mat4);
    if (!avdIsFEqual(avdMat3x3Val(fromMat4, 0, 0), 1.0f) ||
        !avdIsFEqual(avdMat3x3Val(fromMat4, 1, 1), 1.0f) ||
        !avdIsFEqual(avdMat3x3Val(fromMat4, 2, 2), 1.0f)) {
        AVD_LOG("    FAILED: Matrix3x3 from Matrix4x4 conversion\n");
        return false;
    }
    
    // Test from columns
    AVD_Vector3 c0 = avdVec3(1.0f, 2.0f, 3.0f);
    AVD_Vector3 c1 = avdVec3(4.0f, 5.0f, 6.0f);
    AVD_Vector3 c2 = avdVec3(7.0f, 8.0f, 9.0f);
    AVD_Matrix3x3 fromCols = avdMat3x3FromCols(c0, c1, c2);
    if (!avdIsFEqual(avdMat3x3Val(fromCols, 0, 0), 1.0f) ||
        !avdIsFEqual(avdMat3x3Val(fromCols, 1, 1), 5.0f) ||
        !avdIsFEqual(avdMat3x3Val(fromCols, 2, 2), 9.0f)) {
        AVD_LOG("    FAILED: Matrix3x3 from columns\n");
        return false;
    }
    
    // Test from rows
    AVD_Vector3 r0 = avdVec3(1.0f, 2.0f, 3.0f);
    AVD_Vector3 r1 = avdVec3(4.0f, 5.0f, 6.0f);
    AVD_Vector3 r2 = avdVec3(7.0f, 8.0f, 9.0f);
    AVD_Matrix3x3 fromRows = avdMat3x3FromRows(r0, r1, r2);
    if (!avdIsFEqual(avdMat3x3Val(fromRows, 0, 0), 1.0f) ||
        !avdIsFEqual(avdMat3x3Val(fromRows, 1, 1), 5.0f) ||
        !avdIsFEqual(avdMat3x3Val(fromRows, 2, 2), 9.0f)) {
        AVD_LOG("    FAILED: Matrix3x3 from rows\n");
        return false;
    }
    
    AVD_LOG("    Matrix3x3 tests PASSED\n");
    return true;
}

static bool __avdCheckVector2()
{
    AVD_LOG("  Testing Vector2 operations...\n");
    
    // Test vector creation
    AVD_Vector2 v1 = avdVec2(3.0f, 4.0f);
    AVD_Vector2 v2 = avdVec2(1.0f, 2.0f);
    
    // Test vector access
    if (!avdIsFEqual(v1.x, 3.0f) || !avdIsFEqual(v1.y, 4.0f)) {
        AVD_LOG("    FAILED: Vector2 component access\n");
        return false;
    }
    
    // Test array access
    if (!avdIsFEqual(v1.v[0], 3.0f) || !avdIsFEqual(v1.v[1], 4.0f)) {
        AVD_LOG("    FAILED: Vector2 array access\n");
        return false;
    }
    
    // Test zero and one vectors
    AVD_Vector2 zero = avdVec2Zero();
    AVD_Vector2 one = avdVec2One();
    if (!avdIsFEqual(zero.x, 0.0f) || !avdIsFEqual(zero.y, 0.0f) ||
        !avdIsFEqual(one.x, 1.0f) || !avdIsFEqual(one.y, 1.0f)) {
        AVD_LOG("    FAILED: Vector2 zero/one vectors\n");
        return false;
    }
    
    // Test vector addition
    AVD_Vector2 addResult = avdVec2Add(v1, v2);
    if (!avdIsFEqual(addResult.x, 4.0f) || !avdIsFEqual(addResult.y, 6.0f)) {
        AVD_LOG("    FAILED: Vector2 addition\n");
        return false;
    }
    
    // Test vector subtraction
    AVD_Vector2 subResult = avdVec2Subtract(v1, v2);
    if (!avdIsFEqual(subResult.x, 2.0f) || !avdIsFEqual(subResult.y, 2.0f)) {
        AVD_LOG("    FAILED: Vector2 subtraction\n");
        return false;
    }
    
    // Test vector scaling
    AVD_Vector2 scaleResult = avdVec2Scale(v1, 2.0f);
    if (!avdIsFEqual(scaleResult.x, 6.0f) || !avdIsFEqual(scaleResult.y, 8.0f)) {
        AVD_LOG("    FAILED: Vector2 scaling\n");
        return false;
    }
    
    // Test vector length and length squared
    AVD_Float lengthSq = avdVec2LengthSq(v1);
    AVD_Float length = avdVec2Length(v1);
    if (!avdIsFEqual(lengthSq, 25.0f) || !avdIsFEqual(length, 5.0f)) {
        AVD_LOG("    FAILED: Vector2 length calculations\n");
        return false;
    }
    
    // Test dot product
    AVD_Float dot = avdVec2Dot(v1, v2);
    if (!avdIsFEqual(dot, 11.0f)) { // 3*1 + 4*2 = 11
        AVD_LOG("    FAILED: Vector2 dot product\n");
        return false;
    }
    
    // Test cross product (2D cross product returns scalar)
    AVD_Float cross = avdVec2Cross(v1, v2);
    if (!avdIsFEqual(cross, 2.0f)) { // 3*2 - 4*1 = 2
        AVD_LOG("    FAILED: Vector2 cross product\n");
        return false;
    }
    
    // Test normalization
    AVD_Vector2 normalized = avdVec2Normalize(v1);
    AVD_Float normalizedLength = avdVec2Length(normalized);
    if (!avdIsFEqual(normalizedLength, 1.0f)) {
        AVD_LOG("    FAILED: Vector2 normalization\n");
        return false;
    }
    
    // Test zero vector normalization (should return zero)
    AVD_Vector2 zeroNormalized = avdVec2Normalize(zero);
    if (!avdIsFEqual(zeroNormalized.x, 0.0f) || !avdIsFEqual(zeroNormalized.y, 0.0f)) {
        AVD_LOG("    FAILED: Vector2 zero normalization\n");
        return false;
    }
    
    // Test linear interpolation
    AVD_Vector2 lerpResult = avdVec2Lerp(v1, v2, 0.5f);
    if (!avdIsFEqual(lerpResult.x, 2.0f) || !avdIsFEqual(lerpResult.y, 3.0f)) {
        AVD_LOG("    FAILED: Vector2 lerp\n");
        return false;
    }
    
    // Test polar coordinates
    AVD_Vector2 fromPolar = avdVec2FromPolar(0.0f, 5.0f); // angle=0, length=5
    if (!avdIsFEqual(fromPolar.x, 5.0f) || !avdIsFEqual(fromPolar.y, 0.0f)) {
        AVD_LOG("    FAILED: Vector2 from polar coordinates\n");
        return false;
    }
    
    // Test swizzling
    AVD_Vector2 swizzled = avdVecSwizzle2(v1, y, x);
    if (!avdIsFEqual(swizzled.x, 4.0f) || !avdIsFEqual(swizzled.y, 3.0f)) {
        AVD_LOG("    FAILED: Vector2 swizzling\n");
        return false;
    }
    
    AVD_LOG("    Vector2 tests PASSED\n");
    return true;
}

static bool __avdCheckVector3()
{
    AVD_LOG("  Testing Vector3 operations...\n");
    
    // Test vector creation
    AVD_Vector3 v1 = avdVec3(1.0f, 2.0f, 3.0f);
    AVD_Vector3 v2 = avdVec3(4.0f, 5.0f, 6.0f);
    
    // Test vector access (both xyz and rgb)
    if (!avdIsFEqual(v1.x, 1.0f) || !avdIsFEqual(v1.y, 2.0f) || !avdIsFEqual(v1.z, 3.0f) ||
        !avdIsFEqual(v1.r, 1.0f) || !avdIsFEqual(v1.g, 2.0f) || !avdIsFEqual(v1.b, 3.0f)) {
        AVD_LOG("    FAILED: Vector3 component access\n");
        return false;
    }
    
    // Test array access
    if (!avdIsFEqual(v1.v[0], 1.0f) || !avdIsFEqual(v1.v[1], 2.0f) || !avdIsFEqual(v1.v[2], 3.0f)) {
        AVD_LOG("    FAILED: Vector3 array access\n");
        return false;
    }
    
    // Test zero and one vectors
    AVD_Vector3 zero = avdVec3Zero();
    AVD_Vector3 one = avdVec3One();
    if (!avdIsFEqual(zero.x, 0.0f) || !avdIsFEqual(zero.y, 0.0f) || !avdIsFEqual(zero.z, 0.0f) ||
        !avdIsFEqual(one.x, 1.0f) || !avdIsFEqual(one.y, 1.0f) || !avdIsFEqual(one.z, 1.0f)) {
        AVD_LOG("    FAILED: Vector3 zero/one vectors\n");
        return false;
    }
    
    // Test vector addition
    AVD_Vector3 addResult = avdVec3Add(v1, v2);
    if (!avdIsFEqual(addResult.x, 5.0f) || !avdIsFEqual(addResult.y, 7.0f) || !avdIsFEqual(addResult.z, 9.0f)) {
        AVD_LOG("    FAILED: Vector3 addition\n");
        return false;
    }
    
    // Test vector subtraction
    AVD_Vector3 subResult = avdVec3Subtract(v2, v1);
    if (!avdIsFEqual(subResult.x, 3.0f) || !avdIsFEqual(subResult.y, 3.0f) || !avdIsFEqual(subResult.z, 3.0f)) {
        AVD_LOG("    FAILED: Vector3 subtraction\n");
        return false;
    }
    
    // Test vector scaling
    AVD_Vector3 scaleResult = avdVec3Scale(v1, 2.0f);
    if (!avdIsFEqual(scaleResult.x, 2.0f) || !avdIsFEqual(scaleResult.y, 4.0f) || !avdIsFEqual(scaleResult.z, 6.0f)) {
        AVD_LOG("    FAILED: Vector3 scaling\n");
        return false;
    }
    
    // Test vector length and length squared
    AVD_Float lengthSq = avdVec3LengthSq(v1);
    AVD_Float length = avdVec3Length(v1);
    AVD_Float expectedLengthSq = 1.0f + 4.0f + 9.0f; // 14
    AVD_Float expectedLength = avdSqrt(expectedLengthSq);
    if (!avdIsFEqual(lengthSq, expectedLengthSq) || !avdIsFEqual(length, expectedLength)) {
        AVD_LOG("    FAILED: Vector3 length calculations\n");
        return false;
    }
    
    // Test dot product
    AVD_Float dot = avdVec3Dot(v1, v2);
    if (!avdIsFEqual(dot, 32.0f)) { // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
        AVD_LOG("    FAILED: Vector3 dot product\n");
        return false;
    }
    
    // Test cross product
    AVD_Vector3 cross = avdVec3Cross(v1, v2);
    // v1 x v2 = (2*6 - 3*5, 3*4 - 1*6, 1*5 - 2*4) = (12-15, 12-6, 5-8) = (-3, 6, -3)
    if (!avdIsFEqual(cross.x, -3.0f) || !avdIsFEqual(cross.y, 6.0f) || !avdIsFEqual(cross.z, -3.0f)) {
        AVD_LOG("    FAILED: Vector3 cross product\n");
        return false;
    }
    
    // Test normalization
    AVD_Vector3 normalized = avdVec3Normalize(v1);
    AVD_Float normalizedLength = avdVec3Length(normalized);
    if (!avdIsFEqual(normalizedLength, 1.0f)) {
        AVD_LOG("    FAILED: Vector3 normalization\n");
        return false;
    }
    
    // Test zero vector normalization
    AVD_Vector3 zeroNormalized = avdVec3Normalize(zero);
    if (!avdIsFEqual(zeroNormalized.x, 0.0f) || !avdIsFEqual(zeroNormalized.y, 0.0f) || !avdIsFEqual(zeroNormalized.z, 0.0f)) {
        AVD_LOG("    FAILED: Vector3 zero normalization\n");
        return false;
    }
    
    // Test inverse length
    AVD_Float invLength = avdVec3InvLength(v1);
    if (!avdIsFEqual(invLength, 1.0f / expectedLength)) {
        AVD_LOG("    FAILED: Vector3 inverse length\n");
        return false;
    }
    
    // Test linear interpolation
    AVD_Vector3 lerpResult = avdVec3Lerp(v1, v2, 0.5f);
    if (!avdIsFEqual(lerpResult.x, 2.5f) || !avdIsFEqual(lerpResult.y, 3.5f) || !avdIsFEqual(lerpResult.z, 4.5f)) {
        AVD_LOG("    FAILED: Vector3 lerp\n");
        return false;
    }
    
    // Test polar coordinates
    AVD_Vector3 fromPolar = avdVec3FromPolar(0.0f, 0.0f, 5.0f); // azimuth=0, elevation=0, length=5
    if (!avdIsFEqual(fromPolar.x, 5.0f) || !avdIsFEqual(fromPolar.y, 0.0f) || !avdIsFEqual(fromPolar.z, 0.0f)) {
        AVD_LOG("    FAILED: Vector3 from polar coordinates\n");
        return false;
    }
    
    // Test spherical coordinates  
    AVD_Vector3 fromSpherical = avdVec3FromSpherical(0.0f, 0.0f, 5.0f);
    if (!avdIsFEqual(fromSpherical.x, 5.0f) || !avdIsFEqual(fromSpherical.y, 0.0f) || !avdIsFEqual(fromSpherical.z, 0.0f)) {
        AVD_LOG("    FAILED: Vector3 from spherical coordinates\n");
        return false;
    }
    
    // Test swizzling
    AVD_Vector3 swizzled = avdVecSwizzle3(v1, z, y, x);
    if (!avdIsFEqual(swizzled.x, 3.0f) || !avdIsFEqual(swizzled.y, 2.0f) || !avdIsFEqual(swizzled.z, 1.0f)) {
        AVD_LOG("    FAILED: Vector3 swizzling\n");
        return false;
    }
    
    // Test orthogonality of cross product
    AVD_Float dotCrossV1 = avdVec3Dot(cross, v1);
    AVD_Float dotCrossV2 = avdVec3Dot(cross, v2);
    if (!avdIsFEqual(dotCrossV1, 0.0f) || !avdIsFEqual(dotCrossV2, 0.0f)) {
        AVD_LOG("    FAILED: Vector3 cross product orthogonality\n");
        return false;
    }
    
    AVD_LOG("    Vector3 tests PASSED\n");
    return true;
}

static bool __avdCheckVector4()
{
    AVD_LOG("  Testing Vector4 operations...\n");
    
    // Test vector creation
    AVD_Vector4 v1 = avdVec4(1.0f, 2.0f, 3.0f, 4.0f);
    AVD_Vector4 v2 = avdVec4(5.0f, 6.0f, 7.0f, 8.0f);
    
    // Test vector access (both xyzw and rgba)
    if (!avdIsFEqual(v1.x, 1.0f) || !avdIsFEqual(v1.y, 2.0f) || !avdIsFEqual(v1.z, 3.0f) || !avdIsFEqual(v1.w, 4.0f) ||
        !avdIsFEqual(v1.r, 1.0f) || !avdIsFEqual(v1.g, 2.0f) || !avdIsFEqual(v1.b, 3.0f) || !avdIsFEqual(v1.a, 4.0f)) {
        AVD_LOG("    FAILED: Vector4 component access\n");
        return false;
    }
    
    // Test array access
    if (!avdIsFEqual(v1.v[0], 1.0f) || !avdIsFEqual(v1.v[1], 2.0f) || 
        !avdIsFEqual(v1.v[2], 3.0f) || !avdIsFEqual(v1.v[3], 4.0f)) {
        AVD_LOG("    FAILED: Vector4 array access\n");
        return false;
    }
    
    // Test zero and one vectors
    AVD_Vector4 zero = avdVec4Zero();
    AVD_Vector4 one = avdVec4One();
    if (!avdIsFEqual(zero.x, 0.0f) || !avdIsFEqual(zero.y, 0.0f) || !avdIsFEqual(zero.z, 0.0f) || !avdIsFEqual(zero.w, 0.0f) ||
        !avdIsFEqual(one.x, 1.0f) || !avdIsFEqual(one.y, 1.0f) || !avdIsFEqual(one.z, 1.0f) || !avdIsFEqual(one.w, 1.0f)) {
        AVD_LOG("    FAILED: Vector4 zero/one vectors\n");
        return false;
    }
    
    // Test vector creation from Vec3
    AVD_Vector3 v3 = avdVec3(1.0f, 2.0f, 3.0f);
    AVD_Vector4 fromVec3 = avdVec4FromVec3(v3, 4.0f);
    if (!avdIsFEqual(fromVec3.x, 1.0f) || !avdIsFEqual(fromVec3.y, 2.0f) || 
        !avdIsFEqual(fromVec3.z, 3.0f) || !avdIsFEqual(fromVec3.w, 4.0f)) {
        AVD_LOG("    FAILED: Vector4 from Vector3\n");
        return false;
    }
    
    // Test vector addition
    AVD_Vector4 addResult = avdVec4Add(v1, v2);
    if (!avdIsFEqual(addResult.x, 6.0f) || !avdIsFEqual(addResult.y, 8.0f) || 
        !avdIsFEqual(addResult.z, 10.0f) || !avdIsFEqual(addResult.w, 12.0f)) {
        AVD_LOG("    FAILED: Vector4 addition\n");
        return false;
    }
    
    // Test vector subtraction
    AVD_Vector4 subResult = avdVec4Subtract(v2, v1);
    if (!avdIsFEqual(subResult.x, 4.0f) || !avdIsFEqual(subResult.y, 4.0f) || 
        !avdIsFEqual(subResult.z, 4.0f) || !avdIsFEqual(subResult.w, 4.0f)) {
        AVD_LOG("    FAILED: Vector4 subtraction\n");
        return false;
    }
    
    // Test vector scaling
    AVD_Vector4 scaleResult = avdVec4Scale(v1, 2.0f);
    if (!avdIsFEqual(scaleResult.x, 2.0f) || !avdIsFEqual(scaleResult.y, 4.0f) || 
        !avdIsFEqual(scaleResult.z, 6.0f) || !avdIsFEqual(scaleResult.w, 8.0f)) {
        AVD_LOG("    FAILED: Vector4 scaling\n");
        return false;
    }
    
    // Test vector length and length squared
    AVD_Float lengthSq = avdVec4LengthSq(v1);
    AVD_Float length = avdVec4Length(v1);
    AVD_Float expectedLengthSq = 1.0f + 4.0f + 9.0f + 16.0f; // 30
    AVD_Float expectedLength = avdSqrt(expectedLengthSq);
    if (!avdIsFEqual(lengthSq, expectedLengthSq) || !avdIsFEqual(length, expectedLength)) {
        AVD_LOG("    FAILED: Vector4 length calculations\n");
        return false;
    }
    
    // Test normalization
    AVD_Vector4 normalized = avdVec4Normalize(v1);
    AVD_Float normalizedLength = avdVec4Length(normalized);
    if (!avdIsFEqual(normalizedLength, 1.0f)) {
        AVD_LOG("    FAILED: Vector4 normalization\n");
        return false;
    }
    
    // Test zero vector normalization
    AVD_Vector4 zeroNormalized = avdVec4Normalize(zero);
    if (!avdIsFEqual(zeroNormalized.x, 0.0f) || !avdIsFEqual(zeroNormalized.y, 0.0f) || 
        !avdIsFEqual(zeroNormalized.z, 0.0f) || !avdIsFEqual(zeroNormalized.w, 0.0f)) {
        AVD_LOG("    FAILED: Vector4 zero normalization\n");
        return false;
    }
    
    // Test inverse length
    AVD_Float invLength = avdVec4InvLength(v1);
    if (!avdIsFEqual(invLength, 1.0f / expectedLength)) {
        AVD_LOG("    FAILED: Vector4 inverse length\n");
        return false;
    }
    
    // Test cross product (only uses xyz components)
    AVD_Vector4 cross = avdVec4Cross(v1, v2);
    // Cross product of (1,2,3) x (5,6,7) = (2*7-3*6, 3*5-1*7, 1*6-2*5) = (14-18, 15-7, 6-10) = (-4, 8, -4)
    if (!avdIsFEqual(cross.x, -4.0f) || !avdIsFEqual(cross.y, 8.0f) || 
        !avdIsFEqual(cross.z, -4.0f) || !avdIsFEqual(cross.w, 1.0f)) {
        AVD_LOG("    FAILED: Vector4 cross product\n");
        return false;
    }
    
    // Test linear interpolation
    AVD_Vector4 lerpResult = avdVec4Lerp(v1, v2, 0.5f);
    if (!avdIsFEqual(lerpResult.x, 3.0f) || !avdIsFEqual(lerpResult.y, 4.0f) || 
        !avdIsFEqual(lerpResult.z, 5.0f) || !avdIsFEqual(lerpResult.w, 6.0f)) {
        AVD_LOG("    FAILED: Vector4 lerp\n");
        return false;
    }
    
    // Test swizzling
    AVD_Vector4 swizzled = avdVecSwizzle4(v1, w, z, y, x);
    if (!avdIsFEqual(swizzled.x, 4.0f) || !avdIsFEqual(swizzled.y, 3.0f) || 
        !avdIsFEqual(swizzled.z, 2.0f) || !avdIsFEqual(swizzled.w, 1.0f)) {
        AVD_LOG("    FAILED: Vector4 swizzling\n");
        return false;
    }
    
    // Test 2D and 3D swizzling from Vector4
    AVD_Vector2 swizzled2D = avdVecSwizzle2(v1, x, w);
    AVD_Vector3 swizzled3D = avdVecSwizzle3(v1, z, y, x);
    if (!avdIsFEqual(swizzled2D.x, 1.0f) || !avdIsFEqual(swizzled2D.y, 4.0f) ||
        !avdIsFEqual(swizzled3D.x, 3.0f) || !avdIsFEqual(swizzled3D.y, 2.0f) || !avdIsFEqual(swizzled3D.z, 1.0f)) {
        AVD_LOG("    FAILED: Vector4 mixed swizzling\n");
        return false;
    }
    
    AVD_LOG("    Vector4 tests PASSED\n");
    return true;
}

bool avdMathTestsRun()
{
    AVD_LOG("Running AVD Math Tests...\n");

    AVD_CHECK(__avdCheckMatrix4x4());
    AVD_CHECK(__avdCheckMatrix3x3());
    AVD_CHECK(__avdCheckVector2());
    AVD_CHECK(__avdCheckVector3());
    AVD_CHECK(__avdCheckVector4());

    AVD_LOG("All AVD Math Tests passed successfully!\n");
    return true;
}