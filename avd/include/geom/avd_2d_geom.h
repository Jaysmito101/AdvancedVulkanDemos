#ifndef AVD_2D_GEOM_H
#define AVD_2D_GEOM_H

#include "math/avd_math.h"

typedef enum {
    AVD_GEOM_TRIANGLE_CLIP_STATUS_INSIDE,
    AVD_GEOM_TRIANGLE_CLIP_STATUS_OUTSIDE,
    AVD_GEOM_TRIANGLE_CLIP_STATUS_CLIPPED
} AVD_GeomTriangleClipStatus;


AVD_GeomTriangleClipStatus avdGeomGetTriangleClipStatus(
    AVD_Vector2 v1, AVD_Vector2 v2, AVD_Vector2 v3,
    AVD_Vector2 clipMin, AVD_Vector2 clipMax);


#endif 