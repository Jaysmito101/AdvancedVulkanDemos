#ifndef PS_PIPELINE_UTILS_H
#define PS_PIPELINE_UTILS_H

#include "ps_common.h"


bool psPipelineUtilsShaderStage(VkPipelineShaderStageCreateInfo *shaderStageInfo, VkShaderModule shaderModule, VkShaderStageFlagBits stageFlags);
bool psPipelineUtilsDynamicState(VkPipelineDynamicStateCreateInfo *dynamicStateInfo);
bool psPipelineUtilsInputAssemblyState(VkPipelineInputAssemblyStateCreateInfo *inputAssemblyInfo);
bool psPipelineUtilsViewportScissor(VkViewport *viewport, VkRect2D *scissor);
bool psPipelineUtilsViewportState(VkPipelineViewportStateCreateInfo *viewportStateInfo, VkViewport *viewport, VkRect2D *scissor);
bool psPipelineUtilsRasterizationState(VkPipelineRasterizationStateCreateInfo *rasterizerInfo);
bool psPipelineUtilsMultisampleState(VkPipelineMultisampleStateCreateInfo *multisampleInfo);
bool psPipelineUtilsDepthStencilState(VkPipelineDepthStencilStateCreateInfo *depthStencilInfo, bool enableDepthTest);
bool psPipelineUtilsBlendAttachment(VkPipelineColorBlendAttachmentState *blendAttachment, bool enableBlend);
bool psPipelineUtilsColorBlendState(VkPipelineColorBlendStateCreateInfo *colorBlendStateInfo, VkPipelineColorBlendAttachmentState *blendAttachments, size_t attachmentCount);

bool psWriteImageDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorImageInfo *imageInfo);
bool psWriteBufferDescriptorSet(VkWriteDescriptorSet *writeDescriptorSet, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorBufferInfo *bufferInfo);

#endif // PS_PIPELINE_UTILS_H
