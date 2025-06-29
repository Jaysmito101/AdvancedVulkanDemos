#include "model/avd_3d_scene.h"
#include "model/avd_model.h"

#define CACHE_EMPTY_KEY UINT64_MAX

typedef struct {
    uint64_t key;
    uint32_t value;
} AVD_MeshGenCacheEntry;

typedef struct {
    AVD_MeshGenCacheEntry *entries;
    size_t capacity;
    size_t count;
} AVD_MeshGenMidpointCache;

static void __avdMeshGenCacheCreate(AVD_MeshGenMidpointCache *cache, size_t initialCapacity)
{
    AVD_ASSERT(cache != NULL);
    AVD_ASSERT(initialCapacity > 0);
    cache->capacity = initialCapacity;
    cache->count    = 0;
    cache->entries  = (AVD_MeshGenCacheEntry *)malloc(cache->capacity * sizeof(AVD_MeshGenCacheEntry));
    AVD_ASSERT(cache->entries != NULL);
    for (size_t i = 0; i < cache->capacity; ++i) {
        cache->entries[i].key = CACHE_EMPTY_KEY;
    }
}

static void __avdMeshGenCacheDestroy(AVD_MeshGenMidpointCache *cache)
{
    AVD_ASSERT(cache != NULL);
    if (cache->entries) {
        free(cache->entries);
        cache->entries  = NULL;
        cache->capacity = 0;
        cache->count    = 0;
    }
}

static AVD_MeshGenCacheEntry *__avdMeshGenCacheFindEntry(AVD_MeshGenCacheEntry *entries, size_t capacity, uint64_t key)
{
    uint64_t hash = key % capacity;
    size_t index  = (size_t)hash;

    for (;;) {
        AVD_MeshGenCacheEntry *entry = &entries[index];
        if (entry->key == key || entry->key == CACHE_EMPTY_KEY) {
            return entry;
        }
        index = (index + 1) % capacity;
    }
}

static void __avdMeshGenCacheResize(AVD_MeshGenMidpointCache *cache, size_t newCapacity)
{
    AVD_MeshGenCacheEntry *newEntries = (AVD_MeshGenCacheEntry *)malloc(newCapacity * sizeof(AVD_MeshGenCacheEntry));
    AVD_ASSERT(newEntries != NULL);
    for (size_t i = 0; i < newCapacity; ++i) {
        newEntries[i].key = CACHE_EMPTY_KEY;
    }

    cache->count = 0;
    for (size_t i = 0; i < cache->capacity; ++i) {
        AVD_MeshGenCacheEntry *entry = &cache->entries[i];
        if (entry->key != CACHE_EMPTY_KEY) {
            AVD_MeshGenCacheEntry *dest = __avdMeshGenCacheFindEntry(newEntries, newCapacity, entry->key);
            dest->key                   = entry->key;
            dest->value                 = entry->value;
            cache->count++;
        }
    }

    free(cache->entries);
    cache->entries  = newEntries;
    cache->capacity = newCapacity;
}

static void __avdMeshGenCacheSet(AVD_MeshGenMidpointCache *cache, uint64_t key, uint32_t value)
{
    if (cache->count + 1 > cache->capacity * 0.75) {
        __avdMeshGenCacheResize(cache, cache->capacity * 2);
    }

    AVD_MeshGenCacheEntry *entry = __avdMeshGenCacheFindEntry(cache->entries, cache->capacity, key);
    bool isNew                   = entry->key == CACHE_EMPTY_KEY;
    entry->key                   = key;
    entry->value                 = value;

    if (isNew) {
        cache->count++;
    }
}

static bool __avdMeshGenCacheGet(AVD_MeshGenMidpointCache *cache, uint64_t key, uint32_t *value)
{
    if (cache->count == 0)
        return false;

    AVD_MeshGenCacheEntry *entry = __avdMeshGenCacheFindEntry(cache->entries, cache->capacity, key);
    if (entry->key == CACHE_EMPTY_KEY) {
        return false;
    }

    *value = entry->value;
    return true;
}

static uint32_t __avdMeshGenGetMidpointIndex(AVD_MeshGenMidpointCache *cache, uint32_t i1, uint32_t i2, AVD_List *vertices)
{
    uint64_t key = (i1 < i2) ? (((uint64_t)i1 << 32) | i2) : (((uint64_t)i2 << 32) | i1);

    uint32_t cachedIndex;
    if (__avdMeshGenCacheGet(cache, key, &cachedIndex)) {
        return cachedIndex;
    }

    AVD_Vector3 *v1 = (AVD_Vector3 *)avdListGet(vertices, i1);
    AVD_Vector3 *v2 = (AVD_Vector3 *)avdListGet(vertices, i2);

    AVD_Vector3 midpoint = avdVec3Add(*v1, *v2);
    midpoint             = avdVec3Normalize(midpoint);

    uint32_t newIndex = (uint32_t)vertices->count;
    avdListPushBack(vertices, &midpoint);

    __avdMeshGenCacheSet(cache, key, newIndex);
    return newIndex;
}

bool avdModelAddOctaSphere(
    AVD_Model *model,
    AVD_ModelResources *resources,
    const char *name,
    AVD_Int32 id,
    AVD_Float radius,
    AVD_UInt32 subdivisions)
{
    AVD_ASSERT(model != NULL);
    AVD_ASSERT(resources != NULL);
    AVD_ASSERT(name != NULL);
    AVD_ASSERT(radius > 0.0f);

    AVD_Mesh mesh = {0};
    snprintf(mesh.name, sizeof(mesh.name), "%s", name);
    mesh.id          = id;
    mesh.indexOffset = (AVD_Int32)resources->indicesList.count;

    AVD_List localVertices; // Stores AVD_Vector3
    AVD_List localIndices;  // Stores uint32_t
    avdListCreate(&localVertices, sizeof(AVD_Vector3));
    avdListCreate(&localIndices, sizeof(uint32_t));

    avdListPushBack(&localVertices, &(AVD_Vector3){0.0f, 1.0f, 0.0f});  // 0: Top
    avdListPushBack(&localVertices, &(AVD_Vector3){0.0f, -1.0f, 0.0f}); // 1: Bottom
    avdListPushBack(&localVertices, &(AVD_Vector3){-1.0f, 0.0f, 0.0f}); // 2: Left
    avdListPushBack(&localVertices, &(AVD_Vector3){1.0f, 0.0f, 0.0f});  // 3: Right
    avdListPushBack(&localVertices, &(AVD_Vector3){0.0f, 0.0f, 1.0f});  // 4: Front
    avdListPushBack(&localVertices, &(AVD_Vector3){0.0f, 0.0f, -1.0f}); // 5: Back

    const uint32_t baseIndices[] = {
        0, 3, 4, 0, 5, 3, 0, 2, 5, 0, 4, 2, // Top hemisphere
        1, 4, 3, 1, 3, 5, 1, 5, 2, 1, 2, 4  // Bottom hemisphere
    };
    for (size_t i = 0; i < AVD_ARRAY_COUNT(baseIndices); ++i) {
        avdListPushBack(&localIndices, &baseIndices[i]);
    }

    for (uint32_t i = 0; i < subdivisions; ++i) {
        AVD_MeshGenMidpointCache cache;
        __avdMeshGenCacheCreate(&cache, localIndices.count);
        AVD_List nextIndices;
        avdListCreate(&nextIndices, sizeof(uint32_t));

        for (size_t j = 0; j < localIndices.count; j += 3) {
            uint32_t i1 = *(uint32_t *)avdListGet(&localIndices, j);
            uint32_t i2 = *(uint32_t *)avdListGet(&localIndices, j + 1);
            uint32_t i3 = *(uint32_t *)avdListGet(&localIndices, j + 2);

            uint32_t m12 = __avdMeshGenGetMidpointIndex(&cache, i1, i2, &localVertices);
            uint32_t m23 = __avdMeshGenGetMidpointIndex(&cache, i2, i3, &localVertices);
            uint32_t m31 = __avdMeshGenGetMidpointIndex(&cache, i3, i1, &localVertices);

            uint32_t newFaces[] = {
                i1, m12, m31, // Top corner
                i2, m23, m12, // Right corner
                i3, m31, m23, // Left corner
                m12, m31, m23 // Center
            };
            for (size_t k = 0; k < AVD_ARRAY_COUNT(newFaces); ++k) {
                avdListPushBack(&nextIndices, &newFaces[k]);
            }
        }

        avdListDestroy(&localIndices);
        localIndices = nextIndices;
        __avdMeshGenCacheDestroy(&cache);
    }

    uint32_t baseVertexIndex = (uint32_t)resources->verticesList.count;
    for (size_t i = 0; i < localVertices.count; ++i) {
        AVD_Vector3 *posNorm = (AVD_Vector3 *)avdListGet(&localVertices, i);

        AVD_ModelVertex vertex = {0};
        avdModelVertexInit(&vertex);
        vertex.position   = avdVec3Scale(*posNorm, radius);
        vertex.normal     = *posNorm;
        vertex.texCoord.x = 0.5f + atan2f(posNorm->z, posNorm->x) / (2.0f * (float)AVD_PI);
        vertex.texCoord.y = 0.5f - asinf(posNorm->y) / (float)AVD_PI;

        AVD_ModelVertexPacked packedVertex;
        AVD_CHECK(avdModelVertexPack(&vertex, &packedVertex));
        avdListPushBack(&resources->verticesList, &packedVertex);
    }

    for (size_t i = 0; i < localIndices.count; ++i) {
        uint32_t *pIndex    = (uint32_t *)avdListGet(&localIndices, i);
        uint32_t finalIndex = *pIndex + baseVertexIndex;
        avdListPushBack(&resources->indicesList, &finalIndex);
    }

    mesh.triangleCount = (AVD_Int32)(localIndices.count / 3);
    avdListPushBack(&model->meshes, &mesh);

    avdListDestroy(&localVertices);
    avdListDestroy(&localIndices);

    return true;
}

bool avdModelAddUnitCube(AVD_Model *model, AVD_ModelResources *resources, const char *name, AVD_Int32 id)
{
    AVD_ASSERT(model != NULL);
    AVD_ASSERT(resources != NULL);
    AVD_ASSERT(name != NULL);

    // A unit cube has vertices at +/- 0.5.
    // Each face has 4 vertices with unique normals and UVs.
    static const AVD_ModelVertex CUBE_VERTICES[24] = {
        // Front face (+Z)
        {.position = {-0.5f, 0.5f, 0.5f}, .normal = {0.0f, 0.0f, 1.0f}, .texCoord = {0.0f, 0.0f}},  // 0: Top-left
        {.position = {-0.5f, -0.5f, 0.5f}, .normal = {0.0f, 0.0f, 1.0f}, .texCoord = {0.0f, 1.0f}}, // 1: Bottom-left
        {.position = {0.5f, -0.5f, 0.5f}, .normal = {0.0f, 0.0f, 1.0f}, .texCoord = {1.0f, 1.0f}},  // 2: Bottom-right
        {.position = {0.5f, 0.5f, 0.5f}, .normal = {0.0f, 0.0f, 1.0f}, .texCoord = {1.0f, 0.0f}},   // 3: Top-right
        // Back face (-Z)
        {.position = {0.5f, 0.5f, -0.5f}, .normal = {0.0f, 0.0f, -1.0f}, .texCoord = {0.0f, 0.0f}},   // 4: Top-left
        {.position = {0.5f, -0.5f, -0.5f}, .normal = {0.0f, 0.0f, -1.0f}, .texCoord = {0.0f, 1.0f}},  // 5: Bottom-left
        {.position = {-0.5f, -0.5f, -0.5f}, .normal = {0.0f, 0.0f, -1.0f}, .texCoord = {1.0f, 1.0f}}, // 6: Bottom-right
        {.position = {-0.5f, 0.5f, -0.5f}, .normal = {0.0f, 0.0f, -1.0f}, .texCoord = {1.0f, 0.0f}},  // 7: Top-right
        // Right face (+X)
        {.position = {0.5f, 0.5f, 0.5f}, .normal = {1.0f, 0.0f, 0.0f}, .texCoord = {0.0f, 0.0f}},   // 8: Top-left
        {.position = {0.5f, -0.5f, 0.5f}, .normal = {1.0f, 0.0f, 0.0f}, .texCoord = {0.0f, 1.0f}},  // 9: Bottom-left
        {.position = {0.5f, -0.5f, -0.5f}, .normal = {1.0f, 0.0f, 0.0f}, .texCoord = {1.0f, 1.0f}}, // 10: Bottom-right
        {.position = {0.5f, 0.5f, -0.5f}, .normal = {1.0f, 0.0f, 0.0f}, .texCoord = {1.0f, 0.0f}},  // 11: Top-right
        // Left face (-X)
        {.position = {-0.5f, 0.5f, -0.5f}, .normal = {-1.0f, 0.0f, 0.0f}, .texCoord = {0.0f, 0.0f}},  // 12: Top-left
        {.position = {-0.5f, -0.5f, -0.5f}, .normal = {-1.0f, 0.0f, 0.0f}, .texCoord = {0.0f, 1.0f}}, // 13: Bottom-left
        {.position = {-0.5f, -0.5f, 0.5f}, .normal = {-1.0f, 0.0f, 0.0f}, .texCoord = {1.0f, 1.0f}},  // 14: Bottom-right
        {.position = {-0.5f, 0.5f, 0.5f}, .normal = {-1.0f, 0.0f, 0.0f}, .texCoord = {1.0f, 0.0f}},   // 15: Top-right
        // Top face (+Y)
        {.position = {-0.5f, 0.5f, -0.5f}, .normal = {0.0f, 1.0f, 0.0f}, .texCoord = {0.0f, 0.0f}}, // 16: Top-left
        {.position = {-0.5f, 0.5f, 0.5f}, .normal = {0.0f, 1.0f, 0.0f}, .texCoord = {0.0f, 1.0f}},  // 17: Bottom-left
        {.position = {0.5f, 0.5f, 0.5f}, .normal = {0.0f, 1.0f, 0.0f}, .texCoord = {1.0f, 1.0f}},   // 18: Bottom-right
        {.position = {0.5f, 0.5f, -0.5f}, .normal = {0.0f, 1.0f, 0.0f}, .texCoord = {1.0f, 0.0f}},  // 19: Top-right
        // Bottom face (-Y)
        {.position = {-0.5f, -0.5f, 0.5f}, .normal = {0.0f, -1.0f, 0.0f}, .texCoord = {0.0f, 0.0f}},  // 20: Top-left
        {.position = {-0.5f, -0.5f, -0.5f}, .normal = {0.0f, -1.0f, 0.0f}, .texCoord = {0.0f, 1.0f}}, // 21: Bottom-left
        {.position = {0.5f, -0.5f, -0.5f}, .normal = {0.0f, -1.0f, 0.0f}, .texCoord = {1.0f, 1.0f}},  // 22: Bottom-right
        {.position = {0.5f, -0.5f, 0.5f}, .normal = {0.0f, -1.0f, 0.0f}, .texCoord = {1.0f, 0.0f}}    // 23: Top-right
    };

    // Indices to form 12 triangles (2 per face) from the 24 vertices.
    // Each quad (0,1,2,3) is triangulated as (0,1,2) and (0,2,3).
    static const uint32_t CUBE_INDICES[36] = {
        0, 1, 2, 0, 2, 3,       // Front
        4, 5, 6, 4, 6, 7,       // Back
        8, 9, 10, 8, 10, 11,    // Right
        12, 13, 14, 12, 14, 15, // Left
        16, 17, 18, 16, 18, 19, // Top
        20, 21, 22, 20, 22, 23  // Bottom
    };

    AVD_Mesh mesh = {0};
    snprintf(mesh.name, sizeof(mesh.name), "%s", name);
    mesh.id            = id;
    mesh.indexOffset   = (AVD_Int32)resources->indicesList.count;
    mesh.triangleCount = AVD_ARRAY_COUNT(CUBE_INDICES) / 3;

    uint32_t baseVertexIndex = (uint32_t)resources->verticesList.count;

    AVD_ModelVertexPacked packedVertex = {0};
    for (size_t i = 0; i < AVD_ARRAY_COUNT(CUBE_VERTICES); ++i) {
        AVD_CHECK(avdModelVertexPack(&CUBE_VERTICES[i], &packedVertex));
        avdListPushBack(&resources->verticesList, &packedVertex);
    }

    for (size_t i = 0; i < AVD_ARRAY_COUNT(CUBE_INDICES); ++i) {
        uint32_t finalIndex = CUBE_INDICES[i] + baseVertexIndex;
        avdListPushBack(&resources->indicesList, &finalIndex);
    }

    avdListPushBack(&model->meshes, &mesh);

    return true;
}
