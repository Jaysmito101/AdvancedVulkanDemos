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

    if (!avdReadBinaryFile(filename, data, len)) {
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

static bool __avdMeshLoadFaces(
    tinyobj_attrib_t *attrib,
    AVD_ModelResources *resources,
    AVD_Mesh *mesh,
    uint32_t faceOffset,
    uint32_t faceCount)
{
    AVD_ASSERT(attrib != NULL);
    AVD_ASSERT(resources != NULL);
    AVD_ASSERT(mesh != NULL);
    AVD_ASSERT(faceOffset < attrib->num_face_num_verts);

    mesh->triangleCount = 0;
    mesh->indexOffset   = (uint32_t)resources->indicesList.count;

    uint32_t attribFaceOffset = 0;

    for (uint32_t faceIndex = 0; faceIndex < faceOffset; faceIndex++) {
        attribFaceOffset += (uint32_t)attrib->face_num_verts[faceIndex];
    }

    AVD_ModelVertex vertex             = {0};
    AVD_ModelVertexPacked packedVertex = {0};

    for (size_t faceIndex = faceOffset; faceIndex < faceOffset + faceCount; faceIndex++) {
        // Ensure the face has a multiple of 3 vertices (triangles)
        uint32_t vertexCount = (uint32_t)attrib->face_num_verts[faceIndex];
        AVD_CHECK(vertexCount % 3 == 0);
        mesh->triangleCount += vertexCount / 3;

        // Add the three indices for the triangle
        uint32_t currentIndex = 0;
        for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
            avdModelVertexInit(&vertex);

            tinyobj_vertex_index_t index = attrib->faces[attribFaceOffset + vertexIndex];

            vertex.position.x = attrib->vertices[3 * index.v_idx + 0];
            vertex.position.y = attrib->vertices[3 * index.v_idx + 1];
            vertex.position.z = attrib->vertices[3 * index.v_idx + 2];

            if (attrib->num_normals > 0) {
                vertex.normal.x = attrib->normals[3 * index.vn_idx + 0];
                vertex.normal.y = attrib->normals[3 * index.vn_idx + 1];
                vertex.normal.z = attrib->normals[3 * index.vn_idx + 2];
            }

            if (attrib->num_texcoords > 0) {
                vertex.texCoord.x = attrib->texcoords[2 * index.vt_idx + 0];
                vertex.texCoord.y = attrib->texcoords[2 * index.vt_idx + 1];
            }

            AVD_CHECK(avdModelVertexPack(&vertex, &packedVertex));
            avdListPushBack(&resources->verticesList, &packedVertex);

            currentIndex = (uint32_t)resources->verticesList.count - 1;
            avdListPushBack(&resources->indicesList, &currentIndex);
        }

        attribFaceOffset += (uint32_t)attrib->face_num_verts[faceIndex];
    }
    return true;
}

bool avd3DSceneLoadObj(const char *filename, AVD_3DScene *scene, AVD_ObjLoadFlags flags)
{
    AVD_ASSERT(scene != NULL);

    AVD_Model model = {0};
    AVD_CHECK(avdModelCreate(&model, 0));
    AVD_CHECK(avdModelLoadObj(filename, &model, &scene->modelResources, flags));
    avdListPushBack(&scene->modelsList, &model);
    return true;
}

bool avdModelLoadObj(const char *filename, AVD_Model *model, AVD_ModelResources *resources, AVD_ObjLoadFlags flags)
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

    int err = tinyobj_parse_obj(
        &attrib,
        &shapes,
        &shapeCount,
        &materials,
        &materialCount,
        filename,
        __avdReadFile,
        &memoryContext,
        TINYOBJ_FLAG_TRIANGULATE);

    if (err != TINYOBJ_SUCCESS) {
        AVD_LOG("Error parsing OBJ file: %d\n", err);
        return false;
    }

    snprintf(model->name, sizeof(model->name), "%s", filename);
    model->id = avdHashString(model->name);

    if (flags & AVD_OBJ_LOAD_FLAG_IGNORE_OBJECTS) {
        // Load all shapes as a single mesh
        AVD_Mesh mesh = {0};
        AVD_CHECK(avdMeshInit(&mesh));
        snprintf(mesh.name, sizeof(mesh.name), "%s/Mesh", model->name);
        mesh.id = avdHashString(mesh.name);
        AVD_CHECK(__avdMeshLoadFaces(&attrib, resources, &mesh, 0, attrib.num_face_num_verts));
        avdListPushBack(&model->meshes, &mesh);
    } else {
        // Load all shapes as separate meshes
        for (size_t i = 0; i < shapeCount; i++) {
            tinyobj_shape_t *shape = &shapes[i];
            AVD_Mesh mesh          = {0};
            AVD_CHECK(avdMeshInit(&mesh));
            snprintf(mesh.name, sizeof(mesh.name), "%s/%s", model->name, shape->name);
            mesh.id = avdHashString(mesh.name);
            AVD_CHECK(__avdMeshLoadFaces(&attrib, resources, &mesh, shape->face_offset, shape->length));
            avdListPushBack(&model->meshes, &mesh);
        }
    }

    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, shapeCount);
    tinyobj_materials_free(materials, materialCount);

    if (memoryContext.allocatedObjData != NULL) {
        free(memoryContext.allocatedObjData);
    }

    if (memoryContext.allocatedMtlData != NULL) {
        free(memoryContext.allocatedMtlData);
    }

    return true;
}