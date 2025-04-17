#ifndef _CONSTANTS_
#define _CONSTANTS_

static const float PI = 3.141592654f;
static const float PI_RCP = 1.f / 3.141592654f;
// Could add PI/2 and other often used constants...

static const float SMALL_FLOAT_EPSILON = 1e-5f;

inline float DegreesToRadians(float degrees)
{
    return degrees * (PI / 180.0f);
}

static const float MAX_FLOAT = 3.402823466e38;

// Add rotation matrices (x, y, z) here if we ever need them!

#endif