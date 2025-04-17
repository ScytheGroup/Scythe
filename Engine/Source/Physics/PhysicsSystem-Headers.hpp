// Internal module partition containing the different defines and includes for the internal physics engine
// This file should be a cpp and export wouldn't be necessary but msbuild does not seem to properly support internal partitions automatically.
// module Systems.PhysicsSystem:Headers; // Uncomment if we get a buildsystem able to deduce internal partitions
#ifndef PHYSICS_HEADERS
#define PHYSICS_HEADERS

#include <Jolt/Jolt.h>
#include <Jolt/Core/Color.h>

using namespace JPH::literals; 

#endif // PHYSICS_HEADERS