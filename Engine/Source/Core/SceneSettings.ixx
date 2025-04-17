module;

#include "Common/Macros.hpp"

export module Core:SceneSettings;

import Common;
import Generated;
import Serialization;

export struct SceneSettings
{
    std::string worldName = "Default World";
    
    struct Graphics
    {
        bool enableAtmosphere = true;
#if VXGI_ENABLED
        // Field will do nothing. We don't remove it to avoid problems with serialization.
#endif
        bool enableVoxelGI = false;
        struct DirectionalLight
        {
            Vector3 color = { 1, 1, 1 };
            float intensity = 5;
            Vector3 direction = { -1, -1, -1 };
        } directionalLight;

        struct AtmosphereSettings
        {
            // Rayleigh scattering occurs when light wavelengths are much bigger than the size of particles in the participating medium.
            // This is responsible for the blue color of the daytime sky and for the reddening of the sun at the sunset.
            // This is because light coming from the sun during a sunset traverses a greater portion of the atmosphere which in turn means it collides with more particles
            // (and thus more scattering occurs which leaves only the red wavelengths, aka the longest wavelengths, remain).
            // This density factory changes how light scatters when hitting these particles. Higher values will result in more scattering (and vis-versa).
            float rayleighScatteringDensity = 7.994f;
            // Mie scattering occurs when light wavelengths are approximately equal to the diameter of particles in the participating medium.
            // This generally happens in the lower 4.5km of the atmosphere because of the distrbitution of particles in the atmosphere.
            // This density factor changes how light scatters when hitting these particles. Higher values will result in more scattering (and vis-versa).
            float mieScatteringDensity = 1.200f;
            // Controls the average color of the planet's surface that is reflected back into the atmosphere.
            // Generally recommended to be kept low (under .1f) as otherwise it greatly inflates the image's brightness.
            Vector3 groundAlbedo = {0.1f, 0.1f, 0.1f};
            // Adds a multiplicative factor to the atmosphere's brightness.
            // The default value of 4 is the intended look.
            float atmosphereIntensity = 4.0f;
        } atmosphereSettings;

        std::filesystem::path skyboxPath = "Resources/brown_photostudio_02_4k.hdr";

        struct PostProcessPass
        {
            std::filesystem::path passPath;
            bool enabled;
            int priority;
        };

        std::unordered_map<std::string, PostProcessPass> customPostProcessPasses;

        std::vector<std::pair<const std::string*, PostProcessPass*>> GetSortedPostProcessPasses();
        std::vector<std::pair<const std::string*, const PostProcessPass*>> GetSortedPostProcessPasses() const;
    } graphics;

    struct Physics
    {
        Vector3 gravity = {0, -9.81, 0};
    } physics;
    
#ifdef IMGUI_ENABLED
    bool ImGuiDraw(bool* opened);
    bool ImGuiDrawContent();
#endif
};

DECLARE_SERIALIZABLE(SceneSettings)

DEFINE_SERIALIZATION_AND_DESERIALIZATION(SceneSettings::Graphics::PostProcessPass, passPath, enabled, priority)
DEFINE_SERIALIZATION_AND_DESERIALIZATION(SceneSettings::Graphics::DirectionalLight, color, direction, intensity)
DEFINE_SERIALIZATION_AND_DESERIALIZATION(SceneSettings::Graphics::AtmosphereSettings, atmosphereIntensity, groundAlbedo, mieScatteringDensity, rayleighScatteringDensity)
DEFINE_SERIALIZATION_AND_DESERIALIZATION(SceneSettings::Physics, gravity)
DEFINE_SERIALIZATION_AND_DESERIALIZATION(SceneSettings::Graphics, atmosphereSettings, directionalLight, enableAtmosphere, skyboxPath, enableVoxelGI, customPostProcessPasses)
DEFINE_SERIALIZATION_AND_DESERIALIZATION(SceneSettings, graphics, physics, worldName)