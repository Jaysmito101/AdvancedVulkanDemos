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

bool avdMeshInit(AVD_Mesh *mesh)
{
    *mesh = (AVD_Mesh){0};
    return true;
}

bool avdMeshInitWithNameId(AVD_Mesh *mesh, const char *name, AVD_Int32 id)
{
    AVD_ASSERT(mesh != NULL);
    AVD_CHECK(avdMeshInit(mesh));
    snprintf(mesh->name, sizeof(mesh->name), "%s", name);
    mesh->id = id;

    return true;
}