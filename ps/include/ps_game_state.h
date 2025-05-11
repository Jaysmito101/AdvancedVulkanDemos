#ifndef PS_GAME_STATE_H
#define PS_GAME_STATE_H

#define API_VERSION VK_API_VERSION_1_2
#include <vulkan/vulkan.h>

#include "ps_base.h"
#include "ps_font.h"

#ifndef PS_MAX_FONTS
#define PS_MAX_FONTS 128
#endif

struct GLFWwindow;

typedef struct PS_Window {
    struct GLFWwindow* window;
    int32_t width;
    int32_t height;

    int32_t framebufferWidth;
    int32_t framebufferHeight;

    bool isMinimized;
} PS_Window;

typedef struct PS_VulkanSwapchain {
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
} PS_VulkanSwapchain;

typedef struct PS_VulkanRendererResources {
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence renderFence;
} PS_VulkanRendererResources;

typedef struct PS_VulkanBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDescriptorBufferInfo descriptorBufferInfo;
} PS_VulkanBuffer;

typedef struct PS_VulkanImage {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView imageView;
    VkFormat format;
    VkImageSubresourceRange subresourceRange;
    uint32_t width;
    uint32_t height;
    VkSampler sampler;
    VkDescriptorImageInfo descriptorImageInfo;
} PS_VulkanImage;

typedef struct PS_VulkanFramebufferAttachment {
    PS_VulkanImage image;
    VkAttachmentDescription attachmentDescription;
} PS_VulkanFramebufferAttachment;

typedef struct PS_VulkanFramebuffer {
    VkFramebuffer framebuffer;
    VkRenderPass renderPass;
    bool hasDepthStencil;

    PS_VulkanFramebufferAttachment colorAttachment;
    PS_VulkanFramebufferAttachment depthStencilAttachment;

    uint32_t width;
    uint32_t height;
} PS_VulkanFramebuffer;

typedef struct PS_VulkanPresentation {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;

    VkDescriptorSetLayout iconDescriptorSetLayout;
    VkDescriptorSet iconDescriptorSet;
    PS_VulkanImage iconImage;

    float circleRadius;
} PS_VulkanPresentation;

typedef struct PS_VulkanRenderer {
    PS_VulkanRendererResources resources[16];
    uint32_t numInFlightFrames;
    uint32_t currentFrameIndex;

    PS_VulkanPresentation presentation;

    PS_VulkanFramebuffer sceneFramebuffer;
    VkDescriptorSet sceneFramebufferColorDescriptorSet;
    VkDescriptorSetLayout sceneFramebufferColorDescriptorSetLayout;
} PS_VulkanRenderer;


typedef struct PS_Vulkan {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkCommandPool graphicsCommandPool;
    VkCommandPool computeCommandPool;

    VkDescriptorPool descriptorPool;

    PS_VulkanSwapchain swapchain;
    PS_VulkanRenderer renderer;

    int32_t graphicsQueueFamilyIndex;
    int32_t computeQueueFamilyIndex;

#ifdef PS_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
    bool debugLayersEnabled;
#endif
} PS_Vulkan;


typedef struct PS_Frametime {
    double lastTime;
    double currentTime;
    double deltaTime;
    
    size_t instanteneousFrameRate;
    size_t fps;
    size_t lastSecondFrameCounter;
    double lastSecondTime;
} PS_Frametime;

typedef struct PS_Input {
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
} PS_Input;

struct PS_FontRendererVertex;


typedef struct PS_Font {
    PS_FontData fontData;   
    PS_VulkanImage fontAtlasImage;
    VkDescriptorSet fontDescriptorSet;
    VkDescriptorSetLayout fontDescriptorSetLayout;
} PS_Font;

typedef struct PS_RenderableText {
    size_t characterCount;
    size_t renderableVertexCount;
    PS_VulkanBuffer vertexBuffer;
    struct PS_FontRendererVertex* vertexBufferData;
    PS_Font* font;
    char fontName[256];
    float charHeight;
    
    float boundsMinX;
    float boundsMinY;
    float boundsMaxX;
    float boundsMaxY;
} PS_RenderableText;

typedef struct PS_FontRenderer {
    PS_Font fonts[PS_MAX_FONTS];
    size_t fontCount;
    
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout fontDescriptorSetLayout;
} PS_FontRenderer;

typedef enum PS_SceneType {
    PS_SCENE_TYPE_NONE,
    PS_SCENE_TYPE_SPLASH,
    PS_SCENE_TYPE_LOADING,
    PS_SCENE_TYPE_MAIN_MENU,
    PS_SCENE_TYPE_PROLOGUE,
} PS_SceneType;

typedef struct PS_SplashScene {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    PS_VulkanImage splashImage;

    double sceneStartTime;
    double sceneDurationLeft;
    float currentScale;
    float currentOpacity;
} PS_SplashScene;

typedef struct PS_LoadingScene {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    float sceneStartTime;
    float progress;
} PS_LoadingScene;

typedef enum PS_MainMenuButtonType {
    PS_MAIN_MENU_BUTTON_NONE,
    PS_MAIN_MENU_BUTTON_NEW_GAME,
    PS_MAIN_MENU_BUTTON_CONTINUE,
    PS_MAIN_MENU_BUTTON_OPTIONS,
    PS_MAIN_MENU_BUTTON_EXIT,
    PS_MAIN_MENU_BUTTON_COUNT,
} PS_MainMenuButtonType;

typedef struct PS_MainMenuScene {
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    // Textures
    PS_VulkanImage backgroundTexture;
    PS_VulkanImage newGameButtonTexture;
    PS_VulkanImage continueButtonTexture;
    PS_VulkanImage optionsButtonTexture;
    PS_VulkanImage exitButtonTexture;
    PS_VulkanImage mascotHopeTexture;  
    PS_VulkanImage mascotCrushTexture; 
    PS_VulkanImage mascotMonsterTexture;
    PS_VulkanImage mascotFriendTexture;

    // Descriptor Set for Textures
    VkDescriptorSetLayout textureDescriptorSetLayout;
    VkDescriptorSet textureDescriptorSet;

    PS_RenderableText titleText;

    // Interaction State
    PS_MainMenuButtonType hoveredButton;
    bool wasMouseClicked;
    bool continueDisabled;

    float debugFontScale;
    float debugFontOffsetX;
    float debugFontOffsetY;
} PS_MainMenuScene;

typedef struct PS_PrologueScene {
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
} PS_PrologueScene;

#define PS_TOTAL_SCENES 6

typedef struct PS_Scene {
    PS_SceneType previousScene;
    PS_SceneType currentScene;
    
    bool isSwitchingScene;
    double sceneSwitchStartTime;
    double sceneSwitchDuration;

    bool contentScenesLoaded[PS_TOTAL_SCENES]; 
    bool allContentScenesLoaded;
    float loadingProgress;

    PS_SplashScene splashScene;
    PS_LoadingScene loadingScene;
    PS_MainMenuScene mainMenuScene;
    PS_PrologueScene prologueScene;
} PS_Scene;

typedef struct PS_GameState {
    PS_Window window;
    PS_Vulkan vulkan;
    PS_Frametime framerate;
    PS_Scene scene;
    PS_Input input;
    PS_FontRenderer fontRenderer;

    bool running;
} PS_GameState;


#endif // PS_GAME_STATE_H