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
} AVD_Model;


#endif // AVD_MODEL_H