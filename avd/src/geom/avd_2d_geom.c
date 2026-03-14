#include "geom/avd_2d_geom.h"
#include "core/avd_input.h"

static AVD_UInt32 PRIV_avdGeomGetOutcode(AVD_Vector2 v, AVD_Vector2 clipMin, AVD_Vector2 clipMax)
{
    AVD_UInt32 code = 0;
    if (v.x < clipMin.x) {
        code |= 1;
    } else if (v.x > clipMax.x) {
        code |= 2;
    }

    if (v.y < clipMin.y) {
        code |= 4;
    } else if (v.y > clipMax.y) {
        code |= 8;
    }

    return code;
}

AVD_GeomTriangleClipStatus avdGeomGetTriangleClipStatus(
    AVD_Vector2 v1, AVD_Vector2 v2, AVD_Vector2 v3,
    AVD_Vector2 clipMin, AVD_Vector2 clipMax)
{
    int outcode1 = PRIV_avdGeomGetOutcode(v1, clipMin, clipMax);
    int outcode2 = PRIV_avdGeomGetOutcode(v2, clipMin, clipMax);
    int outcode3 = PRIV_avdGeomGetOutcode(v3, clipMin, clipMax);

    if ((outcode1 | outcode2 | outcode3) == 0) {
        return AVD_GEOM_TRIANGLE_CLIP_STATUS_INSIDE;
    }

    if ((outcode1 & outcode2 & outcode3) != 0) {
        return AVD_GEOM_TRIANGLE_CLIP_STATUS_OUTSIDE;
    }

    return AVD_GEOM_TRIANGLE_CLIP_STATUS_CLIPPED;
}