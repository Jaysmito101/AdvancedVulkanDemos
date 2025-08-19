#include "model/avd_model.h"

bool avdModelCreate(AVD_Model *model, AVD_Int32 id)
{
    AVD_ASSERT(model != NULL);
    model->id = id;
    avdListCreate(&model->meshes, sizeof(AVD_Mesh));
    return true;
}

void avdModelDestroy(AVD_Model *model)
{
    AVD_ASSERT(model != NULL);

    model->id = -1;
    avdListDestroy(&model->meshes);
}