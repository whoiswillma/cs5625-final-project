#ifndef CS5625_PLAPP_H
#define CS5625_PLAPP_H

#include <map>

#include "Scene.h"
#include "OceanScene.h"

#include <nanogui/screen.h>

#include <GLWrap/Program.hpp>
#include <GLWrap/Mesh.hpp>
#include <RTUtil/Camera.hpp>
#include <RTUtil/CameraController.hpp>
#include <GLWrap/Framebuffer.hpp>
#include "Tessendorf.h"
#include "Timer.h"
#include "Bird.hpp"

enum ShadingMode {
    ShadingMode_Flat,
    ShadingMode_Forward,
    ShadingMode_Deferred
};

enum TextureFilteringMode {
    TextureFilteringMode_Nearest = 0,
    TextureFilteringMode_Linear = 1
};

struct PLAppConfig {
    glm::ivec2 shadowMapResolution = {1024, 1024};
    float shadowBias = 1e-2;
    float shadowNear = 1;
    float shadowFar = 100;
    float shadowFov = 1;
    float exposure = 1;
    float thetaSun = glm::pi<float>() / 3;
    float turbidity = 4;
    int ssaoNumSamples = 15;
    bool pcfEnabled = true;
    bool pointLightsEnabled = true;
    bool convertAreaToPoint = true;
    bool ambientLightsEnabled = true;
    bool sunskyEnabled = false;
    bool bloomFilterEnabled = true;
    TextureFilteringMode textureFilteringMode = TextureFilteringMode_Linear;

    bool ocean = false;
    bool birds = false;
};

class PLApp : nanogui::Screen {
public:
    PLApp(
            const std::shared_ptr<Scene>& scene,
            const std::shared_ptr<OceanScene>& oceanScene,
            int height,
            const PLAppConfig& config = PLAppConfig()
    );

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;
    virtual bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
    virtual bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
    virtual bool scroll_event(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override;
    virtual bool resize_event(const nanogui::Vector2i &size) override;

    virtual void draw_contents() override;
	//virtual void load_texture(const char*);
private:
    void setUpNanoguiControls();
    void setUpMeshes();
    void setUpCamera();
    void setUpPrograms();
    void setUpTextures();
    void resetFramebuffers();

    std::shared_ptr<Scene> scene;
    std::shared_ptr<OceanScene> oceanScene;

    std::shared_ptr<GLWrap::Program> programFlat;
    std::shared_ptr<GLWrap::Program> programForward;
    std::shared_ptr<GLWrap::Program> programDeferredGeom;
    std::shared_ptr<GLWrap::Program> programDeferredShadow;
    std::shared_ptr<GLWrap::Program> programDeferredPoint;
    std::shared_ptr<GLWrap::Program> programDeferredAmbient;
    std::shared_ptr<GLWrap::Program> programDeferredSky;
    std::shared_ptr<GLWrap::Program> programDeferredBlur;
    std::shared_ptr<GLWrap::Program> programDeferredMerge;
    std::shared_ptr<GLWrap::Program> programSrgb;
	std::shared_ptr<GLWrap::Program> programTextureSrgb;
    std::shared_ptr<GLWrap::Program> programOceanForward;
    std::shared_ptr<GLWrap::Program> programOceanDeferredGeom;
    std::shared_ptr<GLWrap::Program> programOceanDeferredShadow;
	std::shared_ptr<GLWrap::Program> programTextureDeferred;

    std::vector<std::shared_ptr<GLWrap::Mesh>> meshes;
    std::shared_ptr<GLWrap::Mesh> fsqMesh;

    std::shared_ptr<RTUtil::PerspectiveCamera> cam;
    std::unique_ptr<RTUtil::DefaultCC> cc;
	
	int texture_width,texture_height, texture_Channels;;
	unsigned char *textureData;
    
	// size = viewport, color attachments = 3, depth attachment = yes
    std::shared_ptr<GLWrap::Framebuffer> geomBuffer;

    // size = viewport, color attachments = 1, depth attachment = no, with mipmaps
    std::shared_ptr<GLWrap::Framebuffer> accBuffer, temp1, temp2;

    // size = shadow map resolution, color attachments = 0, depth attachment = yes
    std::shared_ptr<GLWrap::Framebuffer> shadowMap;

    nanogui::Color backgroundColor;
    PLAppConfig config;

    Timer timer;

    const std::vector<std::pair<float, int>> blurLevels = {
            {6.2, 2},
            {24.9, 4},
            {81.0, 6},
            {263.0, 8}
    };

    ShadingMode shadingMode;
    void draw_contents_flat();
    void draw_contents_forward();

    RTUtil::PerspectiveCamera get_light_camera(const PointLight &light) const;
    glm::ivec2 getViewportSize();

    void deferred_geometry_pass();
	void deferred_texture_pass();
    void deferred_ocean_geometry_pass();
    void draw_contents_deferred();
    void deferred_draw_pass(const std::shared_ptr<GLWrap::Framebuffer>& accBuffer);
    void deferred_shadow_pass(const PointLight &light);
    void deferred_ocean_shadow_pass(const PointLight &light);
    void deferred_lighting_pass(
            const std::shared_ptr<GLWrap::Framebuffer> &geomBuffer,
            const GLWrap::Texture2D &shadowTexture,
            const PointLight &light
    );
    void deferred_ambient_pass(
            const std::shared_ptr<GLWrap::Framebuffer> &geomBuffer,
            const AmbientLight &light
    );
    void deferred_sky_pass(const GLWrap::Texture2D &image);
    void deferred_blur_pass(
            const GLWrap::Texture2D& image,
            glm::vec2 dir,
            float stdev,
            int level
    );
    void deferred_merge_pass(const GLWrap::Texture2D &image, const GLWrap::Texture2D &blurred);

    std::shared_ptr<GLWrap::Mesh> oceanMesh;

    // Pre-allocated buffers needed to run the ocean simulation
    struct OceanBuffers {
        tessendorf::array2d<std::complex<float>> buffer;
        tessendorf::array2d<std::complex<float>> fourierAmplitudes;
        tessendorf::array2d<std::complex<float>> gradientXAmplitudes;
        tessendorf::array2d<std::complex<float>> gradientZAmplitudes;

        tessendorf::array2d<float> displacementMap;
        float displacementA, displacementB;

        tessendorf::array2d<float> gradXMap;
        float gradXA, gradXB;

        tessendorf::array2d<float> gradZMap;
        float gradZA, gradZB;
    } oceanBuffers;

    std::shared_ptr<GLWrap::Texture2D> oceanDisplacementTexture;
    std::shared_ptr<GLWrap::Texture2D> oceanGradXTexture;
    std::shared_ptr<GLWrap::Texture2D> oceanGradZTexture;

    void update_ocean_textures(double time);

    void add_birds(std::shared_ptr<Node> curr_node);
    void animate_birds();
    std::vector<Bird> birds;
};


#endif //CS5625_PLAPP_H
