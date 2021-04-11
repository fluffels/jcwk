void dispatchCompute(
    Vulkan& vk,
    VulkanPipeline& pipeline,
    uint32_t x,
    uint32_t y,
    uint32_t z
) {
    VkCommandBuffer cmd = allocateCommandBuffer(vk.device, vk.cmdPoolComputeTransient);
    beginOneOffCommandBuffer(cmd);
    vkCmdBindPipeline(
        cmd,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipeline.handle
    );
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipeline.layout,
        0, 1, &pipeline.descriptorSet,
        0, nullptr
    );
    vkCmdDispatch(cmd, x, y, z);
    endCommandBuffer(cmd);
    submitCommandBuffer(cmd, vk.computeQueue);
}
