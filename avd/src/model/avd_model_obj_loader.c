#include "model/avd_3d_scene.h"

#include "tinyobj_loader_c.h"

typedef struct {
    void *allocatedObjData;
    void *allocatedMtlData;
} AVD_MemoryContext;

static void __avdReadFile(
    void *ctx,
    const char *filename,
    const int is_mtl,
    const char *obj_filename,
    char **data,
    size_t *len)
{
    AVD_MemoryContext *memoryContext = (AVD_MemoryContext *)ctx;
    AVD_ASSERT(memoryContext != NULL);
    AVD_ASSERT(data != NULL);
    AVD_ASSERT(len != NULL);
    AVD_ASSERT(filename != NULL);

    if (!avdPathExists(filename)) {
        AVD_LOG("File does not exist: %s\n", filename);
        *data = NULL;
        *len  = 0;
        return;
    }

    if(!avdReadBinaryFile(filename, data, len)) {
        AVD_LOG("Failed to read file: %s\n", filename);
        *data = NULL;
        *len  = 0;
        return;
    }

    if (is_mtl) {
        memoryContext->allocatedMtlData = *data;
    } else {
        memoryContext->allocatedObjData = *data;
    }
}

bool avd3DSceneLoadObj(const char *filename, AVD_3DScene *scene)
{
    AVD_ASSERT(scene != NULL);

    AVD_Model model;
    AVD_CHECK(avdModelCreate(&model, 0));
    AVD_CHECK(avdModelLoadObj(filename, &model, &scene->modelResources));
    avdListPushBack(&scene->modelsList, &model);
    return true;
}

bool avdModelLoadObj(const char *filename, AVD_Model *model, AVD_ModelResources *resources)
{
    AVD_ASSERT(model != NULL);
    AVD_ASSERT(resources != NULL);
    AVD_ASSERT(filename != NULL);

    AVD_CHECK_MSG(avdPathExists(filename), "The specified OBJ file does not exist: %s", filename);

    tinyobj_attrib_t attrib                = {0};
    tinyobj_shape_t *shapes                = NULL;
    size_t shapeCount                      = 0;
    tinyobj_material_t *materials          = NULL;
    size_t materialCount                   = 0;
    static AVD_MemoryContext memoryContext = {0};
    memoryContext.allocatedObjData         = NULL;
    memoryContext.allocatedMtlData         = NULL;

    const char *err = tinyobj_parse_obj(
        &attrib,
        &shapes,
        &shapeCount,
        &materials,
        &materialCount,
        filename,
        __avdReadFile,
        &memoryContext,
        TINYOBJ_FLAG_TRIANGULATE);


    if (err != NULL) {
        AVD_LOG("Error parsing OBJ file: %s\n", err);
        return false;
    }

    
    if (memoryContext.allocatedObjData != NULL) {
        free(memoryContext.allocatedObjData);
    }

    if (memoryContext.allocatedMtlData != NULL) {
        free(memoryContext.allocatedMtlData);
    }

    return true;
}