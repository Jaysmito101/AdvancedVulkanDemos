#include "model/avd_model_base.h"

void avdModelVertexInit(AVD_ModelVertex *vertex)
{
    AVD_ASSERT(vertex != NULL);
    vertex->position  = avdVec3Zero();
    vertex->normal    = avdVec3Zero();
    vertex->texCoord  = avdVec2Zero();
    vertex->tangent   = avdVec4Zero();
    vertex->bitangent = avdVec3Zero();
}

bool avdModelVertexPack(const AVD_ModelVertex *vertex, AVD_ModelVertexPacked *packed)
{
    AVD_ASSERT(vertex != NULL);
    AVD_ASSERT(packed != NULL);

    packed->vx = avdQuantizeHalf(vertex->position.x);
    packed->vy = avdQuantizeHalf(vertex->position.y);
    packed->vz = avdQuantizeHalf(vertex->position.z);

    float tx   = vertex->tangent.x;
    float ty   = vertex->tangent.y;
    float tz   = vertex->tangent.z;
    float tsum = fabsf(tx) + fabsf(ty) + fabsf(tz);
    float tu   = tz >= 0 ? tx / tsum : (1 - fabsf(ty / tsum)) * (tx >= 0 ? 1 : -1);
    float tv   = tz >= 0 ? ty / tsum : (1 - fabsf(tx / tsum)) * (ty >= 0 ? 1 : -1);

    packed->tp = (avdQuantizeSnorm(tu, 8) + 127) | (avdQuantizeSnorm(tv, 8) + 127) << 8;
    packed->np = (avdQuantizeSnorm(vertex->normal.x, 10) + 511) |
                 (avdQuantizeSnorm(vertex->normal.y, 10) + 511) << 10 |
                 (avdQuantizeSnorm(vertex->normal.z, 10) + 511) << 20;
    packed->np |= (vertex->tangent.w >= 0 ? 0 : 1) << 30;
    packed->tu = avdQuantizeHalf(vertex->texCoord.x);
    packed->tv = avdQuantizeHalf(vertex->texCoord.y);

    return true;
}

bool avdModelVertexUnpack(const AVD_ModelVertexPacked *packed, AVD_ModelVertex *vertex)
{
    AVD_ASSERT(packed != NULL);
    AVD_ASSERT(vertex != NULL);

    vertex->position.x = avdDequantizeHalf(packed->vx);
    vertex->position.y = avdDequantizeHalf(packed->vy);
    vertex->position.z = avdDequantizeHalf(packed->vz);

    vertex->normal.x = ((int)(packed->np & 1023) - 511) / 511.0f;
    vertex->normal.y = ((int)((packed->np >> 10) & 1023) - 511) / 511.0f;
    vertex->normal.z = ((int)((packed->np >> 20) & 1023) - 511) / 511.0f;

    float tu = ((int)(packed->tp & 255) - 127) / 127.0f;
    float tv = ((int)((packed->tp >> 8) & 255) - 127) / 127.0f;

    AVD_Vector3 octVec = avdVec3(tu, tv, 1.0f - fabsf(tu) - fabsf(tv));
    float t            = fmaxf(-octVec.z, 0.0f);
    octVec.x += octVec.x >= 0 ? -t : t;
    octVec.y += octVec.y >= 0 ? -t : t;

    float len         = avdVec3Length(octVec);
    vertex->tangent.x = octVec.x / len;
    vertex->tangent.y = octVec.y / len;
    vertex->tangent.z = octVec.z / len;
    vertex->tangent.w = (packed->np & (1u << 30)) ? -1.0f : 1.0f;

    vertex->texCoord.x = avdDequantizeHalf(packed->tu);
    vertex->texCoord.y = avdDequantizeHalf(packed->tv);

    AVD_Vector3 tangentVec3 = avdVec3(vertex->tangent.x, vertex->tangent.y, vertex->tangent.z);
    AVD_Vector3 bitangent   = avdVec3Cross(vertex->normal, tangentVec3);
    vertex->bitangent       = avdVec3Scale(bitangent, vertex->tangent.w);

    return true;
}

bool avdModelResourcesCreate(AVD_ModelResources *resources)
{
    AVD_ASSERT(resources != NULL);

    avdListCreate(&resources->verticesList, sizeof(AVD_ModelVertexPacked));
    avdListCreate(&resources->indicesList, sizeof(uint32_t));
    avdListCreate(&resources->animationsDataList, sizeof(float));

    return true;
}

void avdModelResourcesDestroy(AVD_ModelResources *resources)
{
    AVD_ASSERT(resources != NULL);

    avdListDestroy(&resources->verticesList);
    avdListDestroy(&resources->indicesList);
    avdListDestroy(&resources->animationsDataList);
}
