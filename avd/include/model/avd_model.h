#ifndef AVD_MODEL_H
#define AVD_MODEL_H

#include "model/avd_model_base.h"

typedef struct {
    char name[256];
    AVD_Int32 id;
    AVD_Int32 indexOffset;
    AVD_Int32 triangleCount;
} AVD_Mesh;

typedef struct {
    char name[256];
    AVD_Int32 id;
    AVD_List meshes;
} AVD_Model;

typedef enum {
    AVD_OBJ_LOAD_FLAG_NONE           = 0,
    AVD_OBJ_LOAD_FLAG_IGNORE_OBJECTS = 1 << 0,
} AVD_ObjLoadFlags;


bool avdModelCreate(AVD_Model *model, AVD_Int32 id);
void avdModelDestroy(AVD_Model *model);
bool avdModelLoadObj(const char *filename, AVD_Model *model, AVD_ModelResources *resources, AVD_ObjLoadFlags flags);
bool avdModelAddOctaSphere(
    AVD_Model *model,
    AVD_ModelResources *resources,
    const char *name,
    AVD_Int32 id,
    AVD_Float radius,
    AVD_UInt32 subdivisions);
bool avdModelAddUnitCube(AVD_Model *model, AVD_ModelResources *resources, const char *name, AVD_Int32 id);

#endif // AVD_MODEL_H