#pragma warning(disable: 4018)
#pragma warning(disable: 4267)

#include "SPIRV-Reflect/spirv_reflect.h"

#include <assert.h>
#include <io.h>
#include <map>

#include "FileSystem.h"
#include "Vulkan.h"

using std::map;

void createDescriptorLayout(
    Vulkan& vk,
    vector<VulkanShader>& shaders,
    VulkanPipeline& pipeline
) {
    vector<VkDescriptorSetLayoutBinding> bindings;
    vector<VkDescriptorBindingFlags> flags;

    map<uint32_t, VkDescriptorSetLayoutBinding*> bindingDescMap;

    for (auto& shader: shaders) {
        for (auto& set: shader.sets) {
            for (uint32_t i = set->set; i < set->binding_count; i++) {
                auto& spirv = *(set->bindings[i]);

                auto existingDesc = bindingDescMap[spirv.binding];
                if (existingDesc) {
                    existingDesc->stageFlags |= shader.reflect.shader_stage;
                } else {
                    auto& desc = bindings.emplace_back();
                    bindingDescMap[spirv.binding] = &desc;
                    desc.binding = spirv.binding;
                    desc.descriptorCount = spirv.count;
                    desc.descriptorType = (VkDescriptorType)spirv.descriptor_type;
                    desc.stageFlags = shader.reflect.shader_stage;

// TODO(jan): this flag only applies to combined image samplers, does it break
// anything to enable it for everything?
                    auto& flag = flags.emplace_back();
                    flag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
                }
            }
        }
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagCI = {};
    flagCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flagCI.bindingCount = (uint32_t)bindings.size();
    flagCI.pBindingFlags = flags.data();

    VkDescriptorSetLayoutCreateInfo descriptors = {};
    descriptors.pNext = (void*)(&flagCI);
    descriptors.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptors.bindingCount = (uint32_t)bindings.size();
    descriptors.pBindings = bindings.data();
    
    checkSuccess(vkCreateDescriptorSetLayout(
        vk.device,
        &descriptors,
        nullptr,
        &pipeline.descriptorLayout
    ));
}

void createDescriptorPool(
    Vulkan& vk,
    vector<VulkanShader>& shaders,
    VulkanPipeline& pipeline
) {
    vector<VkDescriptorPoolSize> sizes;

    for (auto& shader: shaders) {
        for (auto& set: shader.sets) {
            for (uint32_t i = set->set; i < set->binding_count; i++) {
                auto& spirv = *(set->bindings[i]);
                auto type = (VkDescriptorType)spirv.descriptor_type;

                VkDescriptorPoolSize* size = nullptr;
                for (auto& candidate: sizes) {
                    if (candidate.type == type) {
                        size = &candidate;
                        break;
                    }
                }

                if (size == nullptr) {
                    auto& size = sizes.emplace_back();
                    size.descriptorCount = spirv.count;
                    size.type = type;
                } else {
                    size->descriptorCount++;
                }
            }
        }
    }

    if (sizes.size() == 0) {
        pipeline.descriptorPool = VK_NULL_HANDLE;
    } else {
        VkDescriptorPoolCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.maxSets = 1;
        createInfo.poolSizeCount = sizes.size();
        createInfo.pPoolSizes = sizes.data();

        checkSuccess(vkCreateDescriptorPool(
            vk.device,
            &createInfo,
            nullptr,
            &pipeline.descriptorPool
        ));
    }
}

void allocateDescriptorSet(Vulkan& vk, VulkanPipeline& pipeline) {
    if (pipeline.descriptorPool == VK_NULL_HANDLE) {
        pipeline.descriptorSet = VK_NULL_HANDLE;
    } else {
        VkDescriptorSetAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = pipeline.descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &pipeline.descriptorLayout;
        checkSuccess(vkAllocateDescriptorSets(
            vk.device,
            &allocateInfo,
            &pipeline.descriptorSet
        ));
    }
}

void createShaderModule(
    Vulkan& vk,
    const vector<char>& code,
    VulkanShader& shader
) {
    SpvReflectResult result = spvReflectCreateShaderModule(
        code.size(),
        code.data(),
        &shader.reflect
    );
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    
    uint32_t setCount = 0;
    spvReflectEnumerateDescriptorSets(&shader.reflect, &setCount, nullptr);
    shader.sets.resize(setCount);
    spvReflectEnumerateDescriptorSets(
        &shader.reflect,
        &setCount,
        shader.sets.data()
    );

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    checkSuccess(vkCreateShaderModule(
        vk.device,
        &createInfo,
        nullptr,
        &shader.module
    ));
}

void createShaderModule(Vulkan& vk, const string& path, VulkanShader& shader) {
    auto accessResult = _access_s(path.c_str(), 4);
    if (accessResult == EACCES) {
        LOG(ERROR) << "file '" << path << "': access denied";
        exit(-1);
    } else if (accessResult == ENOENT) {
        LOG(ERROR) << "file '" << path << "': file not found";
        exit(-1);
    }
    auto code = readFile(path);
    createShaderModule(vk, code, shader);
}

void createPipelineLayout(Vulkan& vk, vector<VulkanShader>& shaders, VulkanPipeline& pipeline) {
    vector<VkPushConstantRange> pushConstantRanges;
    for (auto& shader: shaders) {
        for (int i = 0; i < shader.reflect.push_constant_block_count; i++) {
            auto& block = shader.reflect.push_constant_blocks[i];
            auto& range = pushConstantRanges.emplace_back();
            range.stageFlags = shader.reflect.shader_stage;
            range.offset = block.offset;
            range.size = block.padded_size;
        }
    }

    VkPipelineLayoutCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = 1;
    createInfo.pSetLayouts = &pipeline.descriptorLayout;
    createInfo.pushConstantRangeCount = pushConstantRanges.size();
    createInfo.pPushConstantRanges = pushConstantRanges.data();
    checkSuccess(vkCreatePipelineLayout(
        vk.device,
        &createInfo,
        nullptr,
        &pipeline.layout
    ));
}

bool compareInputAttributes(
    SpvReflectInterfaceVariable* lhs,
    SpvReflectInterfaceVariable* rhs
) {
    return lhs->location < rhs->location;
}

void describeInputAttributes(
    VulkanPipeline& pipeline,
    VulkanShader& shader
) {
    uint32_t count = 0;
    spvReflectEnumerateInputVariables(&shader.reflect, &count, nullptr);
    vector<SpvReflectInterfaceVariable*> inputs(count);
    spvReflectEnumerateInputVariables(&shader.reflect, &count, inputs.data());

    // NOTE(jan): input attributes may be enumerated out of order, so
    // they need to be sorted when calculating the offset
    std::sort(inputs.begin(), inputs.end(), compareInputAttributes);

    pipeline.inputBinding.stride = 0;
    for (auto input: inputs) {
        if (strcmp("inUV", input->name) == 0) {
            pipeline.needsTexCoords = true;
        } else if (strcmp("inNormal", input->name) == 0) {
            pipeline.needsNormals = true;
        } else if (strcmp("inColor", input->name) == 0) {
            pipeline.needsColor = true;
        }

        auto& desc = pipeline.inputAttributes.emplace_back();
        desc.binding = 0;
        desc.location = input->location;
        desc.format = (VkFormat)input->format;
        desc.offset = pipeline.inputBinding.stride;
        auto componentCount = max(
            input->numeric.vector.component_count,
            1
        );
        pipeline.inputBinding.stride +=
            input->numeric.scalar.width / 8 *
            componentCount;
    }
}

void createPipeline(
    Vulkan& vk,
    vector<VulkanShader>& shaders,
    VkPrimitiveTopology topology,
    VkFrontFace frontFace,
    VulkanPipeline& pipeline
) {
    bool isMeshPipeline = false;

    vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (auto& shader: shaders) {
        auto& shaderStage = shaderStages.emplace_back();
        shaderStage = {};
        shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.stage = (VkShaderStageFlagBits)shader.reflect.shader_stage;
        shaderStage.module = shader.module;
        shaderStage.pName = "main";

        if (shaderStage.stage == VK_SHADER_STAGE_MESH_BIT_NV) {
            isMeshPipeline = true;
        }
    }

    pipeline.inputBinding.binding = 0;
    pipeline.inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    describeInputAttributes(
        pipeline,
        // NOTE(jan): presumably, the first shader will always be the input
        // stage
        shaders[0]
    );

    VkPipelineVertexInputStateCreateInfo vertexInput = {};
    vertexInput.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    if (!isMeshPipeline) {
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &pipeline.inputBinding;
        vertexInput.vertexAttributeDescriptionCount =
            (uint32_t)pipeline.inputAttributes.size();
        vertexInput.pVertexAttributeDescriptions = pipeline.inputAttributes.data();
    }
    
    VkPipelineInputAssemblyStateCreateInfo assembly = {};
    assembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = topology;
    assembly.primitiveRestartEnable = VK_FALSE;
    if (topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP) {
        assembly.primitiveRestartEnable = VK_TRUE;
    }

    VkViewport viewport = {};
    viewport.height = (float)vk.swap.extent.height;
    viewport.width = (float)vk.swap.extent.width;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f; 
    viewport.x = 0.f;
    viewport.y = 0.f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = vk.swap.extent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo raster = {};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.frontFace = frontFace;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.lineWidth = 1.f;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.rasterizerDiscardEnable = VK_FALSE;
    raster.depthClampEnable = VK_FALSE;
    raster.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo msample = {};
    msample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    msample.sampleShadingEnable = VK_FALSE;
    msample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    msample.minSampleShading = 1.0f;
    msample.pSampleMask = nullptr;
    msample.alphaToCoverageEnable = VK_FALSE;
    msample.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    // TODO(jan): Experiment with enabling this for better performance.
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlend = {};
    colorBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT |
                                VK_COLOR_COMPONENT_A_BIT;
    colorBlend.blendEnable = VK_FALSE;
    colorBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlend.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlend.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blending = {};
    blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blending.logicOpEnable = VK_FALSE;
    blending.logicOp = VK_LOGIC_OP_COPY;
    blending.attachmentCount = 1;
    blending.pAttachments = &colorBlend;
    blending.blendConstants[0] = 0.0f;
    blending.blendConstants[1] = 0.0f;
    blending.blendConstants[2] = 0.0f;
    blending.blendConstants[3] = 0.0f;

    VkGraphicsPipelineCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.stageCount = (uint32_t)shaderStages.size();
    createInfo.pStages = shaderStages.data();
    createInfo.pVertexInputState = &vertexInput;
    createInfo.pInputAssemblyState = &assembly;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &raster;
    createInfo.pMultisampleState = &msample;
    createInfo.pColorBlendState = &blending;
    createInfo.pDepthStencilState = &depthStencilCreateInfo;
    createInfo.renderPass = vk.renderPass;
    createInfo.layout = pipeline.layout;
    createInfo.subpass = 0;
    
    checkSuccess(vkCreateGraphicsPipelines(
        vk.device,
        VK_NULL_HANDLE,
        1,
        &createInfo,
        nullptr,
        &pipeline.handle
    ));
}

void initVKPipeline(
    Vulkan& vk,
    char* name,
    VkPrimitiveTopology topology,
    VkFrontFace frontFace,
    VulkanPipeline& pipeline
) {
    pipeline = {};

    vector<VulkanShader> shaders(2);

    char vertFile[255];
    sprintf_s(vertFile, "shaders/%s.vert.spv", name);
    char meshFile[255];
    sprintf_s(meshFile, "shaders/%s.mesh.spv", name);
    char fragFile[255];
    sprintf_s(fragFile, "shaders/%s.frag.spv", name);

    if (fexists(vertFile)) {
        createShaderModule(vk, vertFile, shaders[0]);
    } else if (fexists(meshFile)) {
        createShaderModule(vk, meshFile, shaders[0]);
    } else {
        LOG(ERROR) << "pipeline '" << name << "' has no vert/mesh shader";
        exit(-1);
    }

    createShaderModule(vk, fragFile, shaders[1]);

    createDescriptorLayout(vk, shaders, pipeline);
    createDescriptorPool(vk, shaders, pipeline);
    allocateDescriptorSet(vk, pipeline);
    createPipelineLayout(vk, shaders, pipeline);
    createPipeline(
        vk,
        shaders,
        topology,
        frontFace,
        pipeline
    );
}

void initVKPipeline(
    Vulkan& vk,
    char* name,
    VulkanPipeline& pipeline
) {
    initVKPipeline(
        vk,
        name,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_FRONT_FACE_CLOCKWISE,
        pipeline
    );
}

void initVKPipelineCCW(
    Vulkan& vk,
    char* name,
    VulkanPipeline& pipeline
) {
    initVKPipeline(
        vk,
        name,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_FRONT_FACE_COUNTER_CLOCKWISE,
        pipeline
    );
}