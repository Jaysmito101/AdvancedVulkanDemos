#ifndef AVD_MODEL_H
#define AVD_MODEL_H

#include "model/avd_model_base.h"

#ifndef AVD_MODEL_NODE_MAX_CHILDREN
#define AVD_MODEL_NODE_MAX_CHILDREN 128
#endif

#ifndef AVD_MODEL_MAX_NODES
#define AVD_MODEL_MAX_NODES 1024
#endif

typedef struct AVD_MorphTarget {
    char name[256];
    AVD_Int32 id;
    AVD_Int32 indexOffset;
    AVD_Int32 vertexCount;
} AVD_MorphTarget;

typedef struct {
    char name[256];
    AVD_Int32 id;
    AVD_Int32 indexOffset;
    AVD_Int32 triangleCount;

    AVD_MorphTarget morphTargets[AVD_MAX_MORPH_TARGETS_PER_MESH];
    AVD_Int32 morphTargetCount;
} AVD_Mesh;

typedef struct AVD_ModelNode {
    char name[256];
    AVD_Int32 id;
    AVD_Matrix4x4 transform;
    
    AVD_Mesh* mesh;
    struct AVD_ModelNode* children[AVD_MODEL_NODE_MAX_CHILDREN];
    struct AVD_ModelNode* parent;
} AVD_ModelNode;

typedef struct {
    char name[256];
    AVD_Int32 id;
    AVD_List meshes;
    
    AVD_ModelNode* nodes;
    AVD_Int32 nodeCount;
} AVD_Model;

typedef enum {
    AVD_OBJ_LOAD_FLAG_NONE           = 0,
    AVD_OBJ_LOAD_FLAG_IGNORE_OBJECTS = 1 << 0,
} AVD_ObjLoadFlags;

typedef enum {
    AVD_GLT_LOAD_FLAG_NONE           = 0,
} AVD_GltfLoadFlags;


bool avdModelCreate(AVD_Model *model, AVD_Int32 id);
void avdModelDestroy(AVD_Model *model);

bool avdModelNodePrepare(AVD_ModelNode* node, AVD_ModelNode* parent, const char* name, AVD_Int32 id);
bool avdModelAllocNode(AVD_Model *model, AVD_ModelNode** outNode);
bool avdMeshInit(AVD_Mesh *mesh);
bool avdMeshInitWithNameId(AVD_Mesh *mesh, const char *name, AVD_Int32 id);

bool avdModelLoadGltf(const char *filename, AVD_Model *model, AVD_ModelResources *resources, AVD_GltfLoadFlags flags);

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