#ifndef AVD_MODEL_H
#define AVD_MODEL_H

#include "model/avd_model_base.h"

typedef struct {
    AVD_Int32 id;
    AVD_Int32 indexOffset;
    AVD_Int32 triangleCount;
    char name[64];
} AVD_Mesh;

typedef struct {
    AVD_Int32 id;
    AVD_List meshes;
} AVD_Model;

bool avdModelCreate(AVD_Model *model, AVD_Int32 id);
void avdModelDestroy(AVD_Model *model);
bool avdModelLoadObj(const char *filename, AVD_Model *model, AVD_ModelResources *resources);


#endif // AVD_MODEL_H