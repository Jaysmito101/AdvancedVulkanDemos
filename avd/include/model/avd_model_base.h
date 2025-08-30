#ifndef AVD_MODEL_BASE_H
#define AVD_MODEL_BASE_H

#include "core/avd_core.h"
#include "geom/avd_geom.h"
#include "math/avd_math.h"

#ifndef AVD_MAX_MORPH_TARGETS_PER_MESH
#define AVD_MAX_MORPH_TARGETS_PER_MESH 1024
#endif

#ifndef AVD_MODEL_NODE_MAX_CHILDREN
#define AVD_MODEL_NODE_MAX_CHILDREN 1024
#endif

#ifndef AVD_MODEL_MAX_NODES
#define AVD_MODEL_MAX_NODES 1024 * 16
#endif

typedef struct {
    AVD_Vector3 position;
    AVD_Vector3 normal;
    AVD_Vector2 texCoord;
    AVD_Vector4 tangent;
    AVD_Vector3 bitangent;
} AVD_ModelVertex;

typedef struct {
    uint16_t vx, vy, vz;
    uint16_t tp; // packed tangent: 8-8 octahedral
    uint32_t np; // packed normal: 10-10-10-2 vector + bitangent sign
    uint16_t tu, tv;
} AVD_ModelVertexPacked;

typedef struct {
    AVD_List verticesList;
    AVD_List indicesList;
} AVD_ModelResources;

void avdModelVertexInit(AVD_ModelVertex *vertex);
bool avdModelVertexPack(const AVD_ModelVertex *vertex, AVD_ModelVertexPacked *packed);
bool avdModelVertexUnpack(const AVD_ModelVertexPacked *packed, AVD_ModelVertex *vertex);
bool avdModelResourcesCreate(AVD_ModelResources *resources);
void avdModelResourcesDestroy(AVD_ModelResources *resources);

#endif // AVD_MODEL_BASE_H