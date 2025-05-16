#ifndef AVD_PIPELINE_UTILS_H
#define AVD_PIPELINE_UTILS_H

#include "avd_common.h"


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

bool avdWriteImageDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorImageInfo *imageInfo);
bool avdWriteBufferDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorBufferInfo *bufferInfo);

bool avdBeginSceneRenderPass(VkCommandBuffer commandBuffer, AVD_VulkanRenderer* renderer, VkClearValue* clearValues, size_t clearValueCount);
bool avdEndSceneRenderPass(VkCommandBuffer commandBuffer);

#endif // AVD_PIPELINE_UTILS_H
