#include "model/avd_3d_scene.h"

#include "cgltf.h"

static AVD_Matrix4x4 __avdModelGltfMatrixToAvdMatrix(const cgltf_float *matrix)
{
    AVD_ASSERT(matrix != NULL);
    return avdMat4x4(
        matrix[0], matrix[1], matrix[2], matrix[3],
        matrix[4], matrix[5], matrix[6], matrix[7],
        matrix[8], matrix[9], matrix[10], matrix[11],
        matrix[12], matrix[13], matrix[14], matrix[15]);
}

static bool __avdModelLoadGltfTransform(AVD_Transform* transform, cgltf_node* node) {
    AVD_ASSERT(transform != NULL);
    AVD_ASSERT(node != NULL);

    *transform = avdTransformIdentity();

    if (node->has_matrix) {
        AVD_Matrix4x4 matrix = __avdModelGltfMatrixToAvdMatrix(node->matrix);
        *transform = avdTransformFromMatrix(&matrix);
    }

    if (node->has_translation) {
        transform->position = avdVec3(
            node->translation[0],
            node->translation[1],
            node->translation[2]);
    }

    if (node->has_rotation) {
        transform->rotation = avdQuat(
            node->rotation[0],
            node->rotation[1],
            node->rotation[2],
            node->rotation[3]);
    }

    if (node->has_scale) {
        transform->scale = avdVec3(
            node->scale[0],
            node->scale[1],
            node->scale[2]);
    }

    return true;
}

static cgltf_attribute* __avdModelGltfFindAttribute(cgltf_attribute* attributes, cgltf_size count, cgltf_attribute_type type, AVD_UInt32 index, void* scratchBuffer)
{
    AVD_Size size = attributes[0].data->count;
    for (cgltf_size i = 0; i < count; i++) {
        if (attributes[i].type == type && attributes[i].index == index) {
            switch (type) {
                case cgltf_attribute_type_position:
                case cgltf_attribute_type_normal: {
                    if (cgltf_num_components(attributes[i].data->type) != 3) {
                        AVD_LOG("Warning: %s attribute is not vec3, found type %d. This is unexpected, skipping.\n",
                                type == cgltf_attribute_type_position ? "POSITION" : "NORMAL",
                                attributes[i].data->type);
                        continue;
                    } else {
                        size *= 3;
                        break;
                    }
                }
                case cgltf_attribute_type_tangent: {
                    if (cgltf_num_components(attributes[i].data->type) != 4) {
                        AVD_LOG("Warning: TANGENT attribute is not vec4, found type %d. This is unexpected, skipping.\n", attributes[i].data->type);
                        continue;
                    } else {
                        size *= 4;
                        break;
                    }
                }
                case cgltf_attribute_type_texcoord: {
                    if (cgltf_num_components(attributes[i].data->type) != 2) {
                        AVD_LOG("Warning: TEXCOORD attribute is not vec2, found type %d. This is unexpected, skipping.\n", attributes[i].data->type);
                        continue;
                    } else {
                        size *= 2;
                        break;
                    }
                }
            }

            cgltf_accessor_unpack_floats(attributes[i].data, scratchBuffer, size);
            return scratchBuffer;
        }
    }
    return NULL;
}

static bool __avdModelLoadGltfAttrs(AVD_ModelResources *resources, cgltf_attribute* attributes, cgltf_size attributeCount, AVD_GltfLoadFlags flags, AVD_ModelVertexPacked* defaultVertex)
{
    AVD_ASSERT(resources != NULL);
    AVD_ASSERT(attributes != NULL);
    AVD_ASSERT(attributeCount > 0);

    AVD_UInt32 attrCount = (AVD_UInt32)attributes[0].data->count;
    AVD_Float* scratchBuffer = (AVD_Float*)calloc(attrCount * 4 * 4, sizeof(AVD_Float));
    AVD_Size indBuffSize = attrCount * 4;

    const AVD_Float* positionAttr = (const AVD_Float*)__avdModelGltfFindAttribute(attributes, attributeCount, cgltf_attribute_type_position, 0, scratchBuffer + indBuffSize * 0);
    const AVD_Float* normalAttr = (const AVD_Float*)__avdModelGltfFindAttribute(attributes, attributeCount, cgltf_attribute_type_normal, 0, scratchBuffer + indBuffSize * 1);
    const AVD_Float* tangentAttr = (const AVD_Float*)__avdModelGltfFindAttribute(attributes, attributeCount, cgltf_attribute_type_tangent, 0, scratchBuffer + indBuffSize * 2);
    const AVD_Float* texcoordAttr = (const AVD_Float*)__avdModelGltfFindAttribute(attributes, attributeCount, cgltf_attribute_type_texcoord, 0, scratchBuffer + indBuffSize * 3);

    // const AVD_UInt32* jointsAttr = (const AVD_UInt32*)__avdModelGltfFindAttribute(attributes, attributeCount, cgltf_attribute_type_joints, 0, scratchBuffer + indBuffSize * 4);
    // const AVD_Float* weightsAttr = (const AVD_Float*)__avdModelGltfFindAttribute(attributes, attributeCount, cgltf_attribute_type_weights, 0, scratchBuffer + indBuffSize * 5);

    AVD_CHECK_MSG(positionAttr != NULL, "A primitive is missing POSITION attribute which is required.\n");

    AVD_ModelVertex vertex = {0};
    AVD_ModelVertexPacked packedVertex = {0};
    for (AVD_UInt32 i = 0; i < attrCount ; i++) {
        if (defaultVertex) {
            avdModelVertexUnpack(&defaultVertex[i], &vertex);
        } else {
            avdModelVertexInit(&vertex);
        }

        if (positionAttr) {
            vertex.position = avdVec3(
                positionAttr[i * 3 + 0],
                positionAttr[i * 3 + 1],
                positionAttr[i * 3 + 2]);
        }

        if (normalAttr) {
            vertex.normal = avdVec3(
                normalAttr[i * 3 + 0],
                normalAttr[i * 3 + 1],
                normalAttr[i * 3 + 2]);
        }

        if (tangentAttr) {
            vertex.tangent = avdVec4(
                tangentAttr[i * 4 + 0],
                tangentAttr[i * 4 + 1],
                tangentAttr[i * 4 + 2],
                tangentAttr[i * 4 + 3]);
        }

        if (texcoordAttr) {
            vertex.texCoord = avdVec2(
                texcoordAttr[i * 2 + 0],
                texcoordAttr[i * 2 + 1]);
        }

        AVD_CHECK(avdModelVertexPack(&vertex, &packedVertex));
        avdListPushBack(&resources->verticesList, &packedVertex);
    }

    free(scratchBuffer);
    return true;
}

static bool __avdModelLoadGltfLoadPrimAttributes(AVD_ModelResources *resources, AVD_Mesh* mesh, cgltf_primitive *prim, AVD_GltfLoadFlags flags)
{
    AVD_ASSERT(resources != NULL);
    AVD_ASSERT(mesh != NULL);
    AVD_ASSERT(prim != NULL);

    
    mesh->triangleCount = 0;
    mesh->indexOffset = (AVD_UInt32)resources->indicesList.count;

    AVD_UInt32 indexCount = 0;
    if (prim->indices) {
        indexCount = (AVD_UInt32)prim->indices->count;
        AVD_UInt32* indices = avdListAddEmptyN(&resources->indicesList, (AVD_Size)indexCount);
        cgltf_accessor_unpack_indices(prim->indices, indices, 4, indexCount);
    } else {
        indexCount = (AVD_UInt32)prim->attributes[0].data->count;
        for (AVD_UInt32 i = 0; i < indexCount; i++) {
            avdListPushBack(&resources->indicesList, &i);
        }
    }
    AVD_CHECK_MSG(indexCount % 3 == 0, "Only triangle primitives are supported. Found a primitive with %d indices which is not a multiple of 3.\n", indexCount);
    mesh->triangleCount = indexCount / 3;

    AVD_UInt32 attrCount = (AVD_UInt32)prim->attributes[0].data->count;
    avdListEnsureCapacity(&resources->verticesList, resources->verticesList.count + attrCount * (1 + prim->targets_count));
    AVD_ModelVertexPacked *baseVertex = (AVD_ModelVertexPacked *)avdListGet(&resources->verticesList, resources->verticesList.count - attrCount * (1 + prim->targets_count));
    AVD_CHECK(__avdModelLoadGltfAttrs(resources, prim->attributes, prim->attributes_count, 0, NULL));

    for (AVD_UInt32 i = 0; i < prim->targets_count; i++) {
        AVD_CHECK(__avdModelLoadGltfAttrs(resources, prim->targets[i].attributes, prim->targets[i].attributes_count, 0, baseVertex));
    }


    return true;
}

static bool __avdModelLoadGltfNodeMeshPrim(AVD_ModelResources *resources, AVD_Mesh* mesh, cgltf_primitive *prim, AVD_GltfLoadFlags flags)
{
    AVD_ASSERT(resources != NULL);
    AVD_ASSERT(mesh != NULL);
    AVD_ASSERT(prim != NULL);

    AVD_CHECK_MSG(!prim->has_draco_mesh_compression, "Draco compressed meshes are not supported yet!\n");
    AVD_CHECK_MSG(prim->type == cgltf_primitive_type_triangles, "Unsupported primitive type %d, Only triangles are supported!\n", prim->type);

    if (prim->material) {
        // TODO: Implement Material Loading...
        AVD_LOG("Material loading not implemented yet, skipping material for primitive\n");
    }
    
    AVD_CHECK(__avdModelLoadGltfLoadPrimAttributes(resources, mesh, prim, flags));

    return true;    
}

static bool __avdModelLoadGltfNodeMesh(AVD_Model *model, AVD_ModelResources *resources, AVD_ModelNode *node, cgltf_mesh *mesh, cgltf_skin *skin, AVD_GltfLoadFlags flags)
{
    AVD_ASSERT(model != NULL);
    AVD_ASSERT(resources != NULL);
    AVD_ASSERT(node != NULL);
    AVD_ASSERT(mesh != NULL);

    if (skin) {
        // TODO: Implement Skin Loading...
        AVD_LOG("Warning: Skins are not supported yet. Ignoring skin for mesh '%s'\n", mesh->name ? mesh->name : "__UnnamedMesh");
    }

    AVD_Mesh avdMesh = { 0 };
    AVD_CHECK(avdMeshInit(&avdMesh));

  
    if (mesh->target_names_count > 0) {
        AVD_CHECK_MSG(mesh->target_names_count == mesh->weights_count, "Mismatched target names and weights count");

        avdMesh.morphTargets = (AVD_MorphTargets*)avdListAddEmpty(&model->morphTargets);
        avdMesh.morphTargets->count = (AVD_Int32)mesh->target_names_count;
        for (int i = 0; i < mesh->target_names_count; i++) {
            snprintf(avdMesh.morphTargets->targets[i].name, sizeof(avdMesh.morphTargets->targets[i].name), "%s", mesh->target_names[i]);
            avdMesh.morphTargets->targets[i].weight = 0.0;
        }
    }

    const char *meshRawName = mesh->name ? mesh->name : "__UnnamedMesh";
    if (mesh->primitives_count == 1) {
        sprintf(avdMesh.name, "%s", meshRawName);
        AVD_CHECK(__avdModelLoadGltfNodeMeshPrim(resources, &avdMesh, &mesh->primitives[0], flags));

        avdMesh.id = avdHashString(avdMesh.name);
        node->mesh = avdMesh;
        node->hasMesh = true;
        avdListPushBack(&model->meshes, &avdMesh);
    } else {
        for (int i = 0; i < mesh->primitives_count; i++) {
            AVD_ModelNode *sceneNode = NULL;
            AVD_CHECK(avdModelAllocNode(model, &sceneNode));
            static char nodeName[64];
            snprintf(nodeName, sizeof(nodeName), "MeshPrim/%s/%d", meshRawName, i);
            AVD_CHECK(avdModelNodePrepare(sceneNode, node, nodeName, avdHashString(nodeName)));
            
            AVD_Mesh localMesh = {0};
            AVD_CHECK(avdMeshInit(&localMesh));
            snprintf(localMesh.name, sizeof(localMesh.name), "%s/%d", meshRawName, i);
            localMesh.id = avdHashString(nodeName);
            localMesh.morphTargets = avdMesh.morphTargets;
            AVD_CHECK(__avdModelLoadGltfNodeMeshPrim(resources, &localMesh, &mesh->primitives[i], flags));

            sceneNode->mesh = localMesh;
            sceneNode->hasMesh = true;

            avdListPushBack(&model->meshes, &localMesh);
        }
    }

    return true;
}

static bool __avdModelLoadGltfNode(AVD_Model *model, AVD_ModelResources *resources, AVD_ModelNode *parent, cgltf_node *node, AVD_GltfLoadFlags flags)
{
    AVD_ModelNode *sceneNode = NULL;
    AVD_CHECK(avdModelAllocNode(model, &sceneNode));
    const char *nodeName = node->name ? node->name : "__UnnamedNode";
    AVD_CHECK(avdModelNodePrepare(sceneNode, parent, nodeName, avdHashString(nodeName)));
    AVD_CHECK(__avdModelLoadGltfTransform(&sceneNode->transform, node));

    if (node->mesh) {
        AVD_CHECK(__avdModelLoadGltfNodeMesh(model, resources, sceneNode, node->mesh, node->skin, flags));
    }

    for (int i = 0; i < node->children_count; i++) {
        AVD_CHECK(__avdModelLoadGltfNode(model, resources, sceneNode, node->children[i], flags));
    }

    return true;
}


static bool __avdModelLoadGltfScene(AVD_Model *model, AVD_ModelResources* resources, cgltf_scene *scene, AVD_GltfLoadFlags flags, bool mainScene)
{
    AVD_ModelNode *sceneNode = NULL;
    AVD_CHECK(avdModelAllocNode(model, &sceneNode));
    const char *sceneName = scene->name ? scene->name : "__UnnamedScene";
    AVD_CHECK(avdModelNodePrepare(sceneNode, model->rootNode, sceneName, avdHashString(sceneName)));
    
    for (int i = 0; i < scene->nodes_count; i++) {
        AVD_CHECK(__avdModelLoadGltfNode(model, resources, sceneNode, scene->nodes[i], flags));
    }

    if (mainScene) {
        model->mainScene = sceneNode;
    }
    
    return true;
}

bool avd3DSceneLoadGltf(const char *filename, AVD_3DScene *scene, AVD_GltfLoadFlags flags)
{
    AVD_ASSERT(scene != NULL);

    AVD_Model model = {0};
    AVD_CHECK(avdModelCreate(&model, 0));
    AVD_CHECK(avdModelLoadGltf(filename, &model, &scene->modelResources, flags));
    avdListPushBack(&scene->modelsList, &model);
    return true;
}

bool avdModelLoadGltf(const char *filename, AVD_Model *model, AVD_ModelResources *resources, AVD_GltfLoadFlags flags)
{
    AVD_ASSERT(model != NULL);
    AVD_ASSERT(resources != NULL);
    AVD_ASSERT(filename != NULL);

    AVD_CHECK_MSG(avdPathExists(filename), "The specified GLTF file does not exist: %s", filename);

    cgltf_options options = {0};
    cgltf_data* data = NULL;
    AVD_CHECK_MSG(cgltf_parse_file(&options, filename, &data) == cgltf_result_success, "Failed to parse GLTF file: %s", filename);
    AVD_CHECK_MSG(cgltf_load_buffers(&options, data, filename) == cgltf_result_success, "Failed to parse GLTF file: %s", filename);
    AVD_CHECK_MSG(cgltf_validate(data) == cgltf_result_success, "Failed to validate GLTF file: %s", filename);

    snprintf(model->name, sizeof(model->name), "%s", filename);
    model->id = avdHashString(model->name);
    
    for (int i = 0; i < data->scenes_count; i++) {
        AVD_CHECK(__avdModelLoadGltfScene(model, resources, &data->scenes[i], flags, &data->scenes[i] == data->scene));
    }

    cgltf_free(data);
    return true;
}