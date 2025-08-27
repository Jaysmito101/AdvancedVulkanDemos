#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require


#include "MeshUtils"
#include "MathUtils"
#include "PBRUtils"

struct UberPushConstantData {
    mat4 viewModelMatrix;
    mat4 projectionMatrix;

    int vertexOffset;
    int vertexCount;
    int textureIndex;
    int pad1;
};
