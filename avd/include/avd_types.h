#ifndef AVD_GAME_STATE_H
#define AVD_GAME_STATE_H

#define API_VERSION VK_API_VERSION_1_2
#include <vulkan/vulkan.h>

#include "avd_base.h"
#include "avd_font.h"

#ifndef AVD_MAX_FONTS
#define AVD_MAX_FONTS 128
#endif

struct GLFWwindow;

typedef struct AVD_Window {
    struct GLFWwindow* window;
    int32_t width;
    int32_t height;

    int32_t framebufferWidth;
    int32_t framebufferHeight;

    bool isMinimized;
} AVD_Window;

typedef struct AVD_VulkanSwapchain {
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkImage images[64];
    VkImageView imageViews[64];
    VkFramebuffer framebuffers[64];
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    uint32_t imageCount;
    VkExtent2D extent;
    VkRenderPass renderPass;
    bool swapchainRecreateRequired;
    bool swapchainReady;
} AVD_VulkanSwapchain;

typedef struct AVD_VulkanRendererResources {
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence renderFence;
} AVD_VulkanRendererResources;

typedef struct AVD_VulkanBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDescriptorBufferInfo descriptorBufferInfo;
} AVD_VulkanBuffer;

typedef struct AVD_VulkanImage {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView imageView;
    VkFormat format;
    VkImageSubresourceRange subresourceRange;
    uint32_t width;
    uint32_t height;
    VkSampler sampler;
    VkDescriptorImageInfo descriptorImageInfo;
} AVD_VulkanImage;

typedef struct AVD_VulkanFramebufferAttachment {
    AVD_VulkanImage image;
    VkAttachmentDescription attachmentDescription;
    
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
} AVD_VulkanFramebufferAttachment;

typedef struct AVD_VulkanFramebuffer {
    VkFramebuffer framebuffer;
    VkRenderPass renderPass;
    bool hasDepthStencil;

    AVD_VulkanFramebufferAttachment colorAttachment;
    AVD_VulkanFramebufferAttachment depthStencilAttachment;

    uint32_t width;
    uint32_t height;
} AVD_VulkanFramebuffer;

typedef struct AVD_VulkanPresentation {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;

    VkDescriptorSetLayout iconDescriptorSetLayout;
    VkDescriptorSet iconDescriptorSet;
    AVD_VulkanImage iconImage;

    float circleRadius;
} AVD_VulkanPresentation;

typedef struct AVD_VulkanRenderer {
    AVD_VulkanRendererResources resources[16];
    uint32_t numInFlightFrames;
    uint32_t currentFrameIndex;

    AVD_VulkanPresentation presentation;

    AVD_VulkanFramebuffer sceneFramebuffer;
} AVD_VulkanRenderer;


typedef struct AVD_Vulkan {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkCommandPool graphicsCommandPool;
    VkCommandPool computeCommandPool;

    VkDescriptorPool descriptorPool;

    AVD_VulkanSwapchain swapchain;
    AVD_VulkanRenderer renderer;

    int32_t graphicsQueueFamilyIndex;
    int32_t computeQueueFamilyIndex;

#ifdef AVD_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
    bool debugLayersEnabled;
#endif
} AVD_Vulkan;


typedef struct AVD_Frametime {
    double lastTime;
    double currentTime;
    double deltaTime;
    
    size_t instanteneousFrameRate;
    size_t fps;
    size_t lastSecondFrameCounter;
    double lastSecondTime;
} AVD_Frametime;

typedef struct AVD_Input {
    bool keyState[1024];
    bool mouseButtonState[1024];

    float lastMouseX;
    float lastMouseY;

    double rawMouseX;
    double rawMouseY;

    float mouseX;
    float mouseY;

    float mouseDeltaX;
    float mouseDeltaY;

    float mouseScrollX; 
    float mouseScrollY;
} AVD_Input;

struct AVD_FontRendererVertex;


typedef struct AVD_Font {
    AVD_FontData fontData;   
    AVD_VulkanImage fontAtlasImage;
    VkDescriptorSet fontDescriptorSet;
    VkDescriptorSetLayout fontDescriptorSetLayout;
} AVD_Font;

typedef struct AVD_RenderableText {
    size_t characterCount;
    size_t renderableVertexCount;
    AVD_VulkanBuffer vertexBuffer;
    struct AVD_FontRendererVertex* vertexBufferData;
    AVD_Font* font;
    char fontName[256];
    float charHeight;
    
    float boundsMinX;
    float boundsMinY;
    float boundsMaxX;
    float boundsMaxY;
} AVD_RenderableText;

typedef struct AVD_FontRenderer {
    AVD_Font fonts[AVD_MAX_FONTS];
    size_t fontCount;
    
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout fontDescriptorSetLayout;

    AVD_Vulkan* vulkan;
} AVD_FontRenderer;

typedef enum AVD_SceneType {
    AVD_SCENE_TYPE_NONE,
    AVD_SCENE_TYPE_SPLASH,
    AVD_SCENE_TYPE_LOADING,
    AVD_SCENE_TYPE_MAIN_MENU,
    AVD_SCENE_TYPE_PROLOGUE,
} AVD_SceneType;

typedef struct AVD_SplashScene {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    AVD_VulkanImage splashImage;

    double sceneStartTime;
    double sceneDurationLeft;
    float currentScale;
    float currentOpacity;
} AVD_SplashScene;

typedef struct AVD_LoadingScene {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    float sceneStartTime;
    float progress;
} AVD_LoadingScene;

typedef enum AVD_MainMenuButtonType {
    AVD_MAIN_MENU_BUTTON_NONE,
    AVD_MAIN_MENU_BUTTON_NEW_GAME,
    AVD_MAIN_MENU_BUTTON_CONTINUE,
    AVD_MAIN_MENU_BUTTON_OPTIONS,
    AVD_MAIN_MENU_BUTTON_EXIT,
    AVD_MAIN_MENU_BUTTON_COUNT,
} AVD_MainMenuButtonType;

typedef struct AVD_MainMenuScene {
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    // Textures
    AVD_VulkanImage backgroundTexture;
    AVD_VulkanImage newGameButtonTexture;
    AVD_VulkanImage continueButtonTexture;
    AVD_VulkanImage optionsButtonTexture;
    AVD_VulkanImage exitButtonTexture;
    AVD_VulkanImage mascotHopeTexture;  
    AVD_VulkanImage mascotCrushTexture; 
    AVD_VulkanImage mascotMonsterTexture;
    AVD_VulkanImage mascotFriendTexture;

    // Descriptor Set for Textures
    VkDescriptorSetLayout textureDescriptorSetLayout;
    VkDescriptorSet textureDescriptorSet;

    AVD_RenderableText titleText;

    // Interaction State
    AVD_MainMenuButtonType hoveredButton;
    bool wasMouseClicked;
    bool continueDisabled;

    float debugFontScale;
    float debugFontOffsetX;
    float debugFontOffsetY;
} AVD_MainMenuScene;

typedef struct AVD_PrologueScene {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkShaderModule vertexShaderModule;
    VkShaderModule fragmentShaderModule;
    
    float sceneStartTime;
    float sphereRadius;
    float spherePosition[3];
    float cameraPosition[3];
    float rotation;
    float time;
} AVD_PrologueScene;

#define AVD_TOTAL_SCENES 6

typedef struct AVD_Scene {
    AVD_SceneType previousScene;
    AVD_SceneType currentScene;
    
    bool isSwitchingScene;
    double sceneSwitchStartTime;
    double sceneSwitchDuration;

    bool contentScenesLoaded[AVD_TOTAL_SCENES]; 
    bool allContentScenesLoaded;
    float loadingProgress;

    AVD_SplashScene splashScene;
    AVD_LoadingScene loadingScene;
    AVD_MainMenuScene mainMenuScene;
    AVD_PrologueScene prologueScene;
} AVD_Scene;

typedef struct AVD_Bloom {
    AVD_VulkanFramebuffer bloomPasses[5];
    uint32_t passCount;

    uint32_t width;
    uint32_t height;
} AVD_Bloom;

typedef struct AVD_GameState {
    AVD_Window window;
    AVD_Vulkan vulkan;
    AVD_Frametime framerate;
    AVD_Scene scene;
    AVD_Input input;
    AVD_FontRenderer fontRenderer;
    AVD_Bloom bloom;

    bool running;
} AVD_GameState;


#endif // AVD_GAME_STATE_H