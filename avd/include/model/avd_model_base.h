#ifndef AVD_MODEL_BASE_H
#define AVD_MODEL_BASE_H

#include "core/avd_core.h"
#include "math/avd_math.h"

typedef struct {
    AVD_Vector4 position;
    AVD_Vector4 normal;
    AVD_Vector4 texCoord;
    // NOTE: Vertex weights may be optionally be present in  (position.w, normal.w, texCoord.zw)
    AVD_Vector4 tangent;
    AVD_Vector4 bitangent;
} AVD_ModelVertex;

typedef struct {
    AVD_List verticesList;
    AVD_List indicesList;
} AVD_ModelResources;


void avdModelVertexInit(AVD_ModelVertex *vertex);
bool avdModelResourcesCreate(AVD_ModelResources *resources);
void avdModelResourcesDestroy(AVD_ModelResources *resources);


#endif // AVD_MODEL_BASE_H