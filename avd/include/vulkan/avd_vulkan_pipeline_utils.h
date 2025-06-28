#ifndef AVD_PIPELINE_UTILS_H
#define AVD_PIPELINE_UTILS_H

#include "vulkan/avd_vulkan_base.h"
#include "shader/avd_shader.h"

#ifndef AVD_MAX_DESCRIPTOR_SET_BINDINGS
#define AVD_MAX_DESCRIPTOR_SET_BINDINGS 32
#endif

struct AVD_VulkanFramebuffer;
struct AVD_VulkanRenderer;

typedef struct {
    bool enableDepthTest;
    bool enableBlend;
    VkPolygonMode polygonMode;
    VkCullModeFlags cullMode;
    VkFrontFace frontFace;
} AVD_VulkanPipelineCreationInfo;

bool avdPipelineUtilsShaderStage(VkPipelineShaderStageCreateInfo *shaderStageInfo, VkShaderModule shaderModule, VkShaderStageFlagBits stageFlags);
bool avdPipelineUtilsDynamicState(VkPipelineDynamicStateCreateInfo *dynamicStateInfo);
bool avdPipelineUtilsInputAssemblyState(VkPipelineInputAssemblyStateCreateInfo *inputAssemblyInfo);
bool avdPipelineUtilsViewportScissor(VkViewport *viewport, VkRect2D *scissor);
bool avdPipelineUtilsViewportState(VkPipelineViewportStateCreateInfo *viewportStateInfo, VkViewport *viewport, VkRect2D *scissor);
bool avdPipelineUtilsRasterizationState(VkPipelineRasterizationStateCreateInfo *rasterizerInfo, AVD_VulkanPipelineCreationInfo *creationInfo);
bool avdPipelineUtilsMultisampleState(VkPipelineMultisampleStateCreateInfo *multisampleInfo);
bool avdPipelineUtilsDepthStencilState(VkPipelineDepthStencilStateCreateInfo *depthStencilInfo, bool enableDepthTest);
bool avdPipelineUtilsBlendAttachment(VkPipelineColorBlendAttachmentState *blendAttachment, bool enableBlend);
bool avdPipelineUtilsColorBlendState(VkPipelineColorBlendStateCreateInfo *colorBlendStateInfo, VkPipelineColorBlendAttachmentState *blendAttachments, size_t attachmentCount);

void avdPipelineUtilsPipelineCreationInfoInit(AVD_VulkanPipelineCreationInfo *creationInfo);

bool avdPipelineUtilsCreateGraphicsPipelineLayout(
    VkPipelineLayout *pipelineLayout,
    VkDevice device,
    VkDescriptorSetLayout *descriptorSetLayouts,
    size_t descriptorSetLayoutCount,
    uint32_t pushConstantSize);
bool avdPipelineUtilsCreateGenericGraphicsPipeline(
    VkPipeline *pipeline,
    VkPipelineLayout layout,
    VkDevice device,
    VkRenderPass renderPass,
    uint32_t attachmentCount,
    const char *vertShaderAsset,
    const char *fragShaderAsset,
    AVD_ShaderCompilationOptions *compilationOptions,
    AVD_VulkanPipelineCreationInfo *creationInfo);

bool avdPipelineUtilsCreateGraphicsLayoutAndPipeline(
    VkPipelineLayout *pipelineLayout,
    VkPipeline *pipeline,
    VkDevice device,
    VkDescriptorSetLayout *descriptorSetLayouts,
    size_t descriptorSetLayoutCount,
    uint32_t pushConstantSize,
    VkRenderPass renderPass,
    uint32_t attachmentCount,
    const char *vertShaderAsset,
    const char *fragShaderAsset,
    AVD_ShaderCompilationOptions *compilationOptions,
    AVD_VulkanPipelineCreationInfo *creationInfo);

bool avdCreateDescriptorSetLayout(
    VkDescriptorSetLayout *descriptorSetLayout,
    VkDevice device,
    VkDescriptorType *descriptorTypes,
    size_t descriptorTypesCount,
    VkShaderStageFlags stageFlags);
bool avdAllocateDescriptorSet(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout descriptorSetLayout,
    VkDescriptorSet *descriptorSet);


bool avdWriteImageDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorImageInfo *imageInfo);
bool avdWriteBufferDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
bool avdWriteUniformBufferDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorBufferInfo *bufferInfo);

bool avdBeginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, const VkImageView *attachments, size_t attachmentCount, uint32_t framebufferWidth, uint32_t framebufferHeight, VkClearValue *customClearValues, size_t customClearValueCount);
bool avdBeginRenderPassWithFramebuffer(VkCommandBuffer commandBuffer, struct AVD_VulkanFramebuffer *framebuffer, VkClearValue *customClearValues, size_t customClearValueCount);
bool avdEndRenderPass(VkCommandBuffer commandBuffer);
bool avdBeginSceneRenderPass(VkCommandBuffer commandBuffer, struct AVD_VulkanRenderer *renderer);
bool avdEndSceneRenderPass(VkCommandBuffer commandBuffer);

#endif // AVD_PIPELINE_UTILS_H
