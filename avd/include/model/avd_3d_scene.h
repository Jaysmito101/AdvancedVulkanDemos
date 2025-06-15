#ifndef AVD_3D_SCENE_H
#define AVD_3D_SCENE_H

#include "model/avd_model_base.h"
#include "model/avd_model.h"

typedef struct AVD_3DScene {
    AVD_ModelResources modelResources;
    AVD_List modelsList;
} AVD_3DScene;


bool avd3DSceneCreate(AVD_3DScene *scene);
void avd3DSceneDestroy(AVD_3DScene *scene);
bool avd3DSceneLoadObj(const char *filename, AVD_3DScene *scene);

#endif // AVD_3D_SCENE_H