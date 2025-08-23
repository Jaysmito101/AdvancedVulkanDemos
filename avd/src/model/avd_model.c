#include "model/avd_model.h"

bool avdModelNodePrepare(AVD_ModelNode* node, AVD_ModelNode* parent, const char* name, AVD_Int32 id)
{
    AVD_ASSERT(node != NULL);
    AVD_ASSERT(name != NULL);

    memset(node, 0, sizeof(AVD_ModelNode));
    snprintf(node->name, sizeof(node->name), "%s", name);
    node->id = id;
    node->parent = parent;

    if (parent != NULL) {
        bool couldPut = false;
        for (size_t i = 0; i < AVD_MODEL_NODE_MAX_CHILDREN; i++) {
            if (parent->children[i] == NULL) {
                parent->children[i] = node;
                couldPut = true;
                break;
            }
        }
        AVD_CHECK_MSG(couldPut, "Parent node '%s' has no space for more children\n", parent->name);
    }

    return true;
}

bool avdModelAllocNode(AVD_Model *model, AVD_ModelNode** outNode) {
    AVD_ASSERT(model != NULL);
    AVD_ASSERT(outNode != NULL);
    *outNode = &model->nodes[model->nodeCount++];
    return true;
}

bool avdModelCreate(AVD_Model *model, AVD_Int32 id)
{
    AVD_ASSERT(model != NULL);
    memset(model, 0, sizeof(AVD_Model));
    model->id = id;
    avdListCreate(&model->meshes, sizeof(AVD_Mesh));
    model->nodes = calloc(AVD_MODEL_MAX_NODES, sizeof(AVD_ModelNode));
    AVD_CHECK_MSG(model->nodes != NULL, "Failed to allocate memory for model nodes\n");
    memset(model->nodes, 0, sizeof(AVD_ModelNode) * AVD_MODEL_MAX_NODES);

    // prepare the root node
    AVD_ModelNode* rootNode = NULL;
    AVD_CHECK(avdModelAllocNode(model, &rootNode));
    avdModelNodePrepare(rootNode, NULL, "__Root", 0);

    return true;
}

void avdModelDestroy(AVD_Model *model)
{
    AVD_ASSERT(model != NULL);

    model->id = -1;
    avdListDestroy(&model->meshes);
    free(model->nodes);
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