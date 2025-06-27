#ifndef MATH_UTILS
#define MATH_UTILS

#define PI 3.14159265

float linearizeDepth(float depth)
{
    float zNear = 0.1;   // Near plane distance
    float zFar  = 100.0; // Far plane distance
    return zNear * zFar / (zFar + depth * (zNear - zFar));
}


mat4 removeScaleFromMat4(mat4 m)
{
    mat4 result = m;
    result[0]   = vec4(normalize(m[0].xyz), 0.0);
    result[1]   = vec4(normalize(m[1].xyz), 0.0);
    result[2]   = vec4(normalize(m[2].xyz), 0.0);
    // The translation component (m[3]) is already correct.
    return result;
}


mat4 perspectiveMatrix(float fov, float aspect, float near, float far)
{
    float tanHalfFov = tan(fov / 2.0);
    return mat4(
        1.0 / (aspect * tanHalfFov), 0.0, 0.0, 0.0,
        0.0, 1.0 / tanHalfFov, 0.0, 0.0,
        0.0, 0.0, -(far + near) / (far - near), -1.0,
        0.0, 0.0, -(2.0 * far * near) / (far - near), 0.0);
}


#endif 