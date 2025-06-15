#ifndef AVD_PIPELINE_UTILS_H
#define AVD_PIPELINE_UTILS_H

#include "vulkan/avd_vulkan_base.h"

#ifndef AVD_MAX_DESCRIPTOR_SET_BINDINGS
#define AVD_MAX_DESCRIPTOR_SET_BINDINGS 32
#endif

struct AVD_VulkanRenderer;

bool avdPipelineUtilsShaderStage(VkPipelineShaderStageCreateInfo *shaderStageInfo, VkShaderModule shaderModule, VkShaderStageFlagBits stageFlags);
bool avdPipelineUtilsDynamicState(VkPipelineDynamicStateCreateInfo *dynamicStateInfo);
bool avdPipelineUtilsInputAssemblyState(VkPipelineInputAssemblyStateCreateInfo *inputAssemblyInfo);
bool avdPipelineUtilsViewportScissor(VkViewport *viewport, VkRect2D *scissor);
bool avdPipelineUtilsViewportState(VkPipelineViewportStateCreateInfo *viewportStateInfo, VkViewport *viewport, VkRect2D *scissor);
bool avdPipelineUtilsRasterizationState(VkPipelineRasterizationStateCreateInfo *rasterizerInfo);
bool avdPipelineUtilsMultisampleState(VkPipelineMultisampleStateCreateInfo *multisampleInfo);
bool avdPipelineUtilsDepthStencilState(VkPipelineDepthStencilStateCreateInfo *depthStencilInfo, bool enableDepthTest);
bool avdPipelineUtilsBlendAttachment(VkPipelineColorBlendAttachmentState *blendAttachment, bool enableBlend);
bool avdPipelineUtilsColorBlendState(VkPipelineColorBlendStateCreateInfo *colorBlendStateInfo, VkPipelineColorBlendAttachmentState *blendAttachments, size_t attachmentCount);

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
    const char *vertShaderAsset,
    const char *fragShaderAsset);

bool avdCreateDescriptorSetLayout(
    VkDescriptorSetLayout *descriptorSetLayout,
    VkDevice device,
    VkDescriptorType *descriptorTypes,
    size_t descriptorTypesCount,
    VkShaderStageFlags stageFlags);

bool avdWriteImageDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorImageInfo *imageInfo);
bool avdWriteBufferDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorBufferInfo *bufferInfo);

bool avdBeginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, const VkImageView *attachments, size_t attachmentCount, uint32_t framebufferWidth, uint32_t framebufferHeight, VkClearValue *customClearValues, size_t customClearValueCount);
bool avdEndRenderPass(VkCommandBuffer commandBuffer);
bool avdBeginSceneRenderPass(VkCommandBuffer commandBuffer, struct AVD_VulkanRenderer *renderer);
bool avdEndSceneRenderPass(VkCommandBuffer commandBuffer);

#endif // AVD_PIPELINE_UTILS_H
