#ifndef AVD_3D_SCENE_H
#define AVD_3D_SCENE_H

#include "model/avd_model.h"
#include "model/avd_model_base.h"

typedef struct AVD_3DScene {
    AVD_ModelResources modelResources;
    AVD_List modelsList;
} AVD_3DScene;

bool avd3DSceneCreate(AVD_3DScene *scene);
void avd3DSceneDestroy(AVD_3DScene *scene);
bool avd3DSceneLoadObj(const char *filename, AVD_3DScene *scene, AVD_ObjLoadFlags flags);
bool avd3DSceneAddModel(AVD_3DScene *scene, AVD_Model *model);

void avd3DSceneDebugLog(const AVD_3DScene *scene, const char *message);

#endif // AVD_3D_SCENE_H