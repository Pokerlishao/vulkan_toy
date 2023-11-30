#include "vulkan/vulkan.hpp"
#include "toy2d/CommandManager.hpp"
#include "toy2d/context.hpp"

namespace toy2d {

	CommandManager::CommandManager() {
		commandPool = createCommandPool();
	}
	CommandManager::~CommandManager() {
		auto& device = Context::GetInstance().device;
		device.destroyCommandPool(commandPool);
	}


	vk::CommandPool CommandManager::createCommandPool() {
		auto& ctx = Context::GetInstance();
		vk::CommandPoolCreateInfo cmdPoolInfo;
		cmdPoolInfo.setQueueFamilyIndex(ctx.queueInfo.graphQueue.value())
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

		return ctx.device.createCommandPool(cmdPoolInfo);
	}

	std::vector<vk::CommandBuffer> CommandManager::CreateCommandBuffers(std::uint32_t count) {
		auto& device = Context::GetInstance().device;
		vk::CommandBufferAllocateInfo cmdBufInfo;
		cmdBufInfo.setCommandBufferCount(count)
			.setCommandPool(commandPool)
			.setLevel(vk::CommandBufferLevel::ePrimary);
		return device.allocateCommandBuffers(cmdBufInfo);
	}

	vk::CommandBuffer CommandManager::CreateOneCommandBuffer() {
		return CreateCommandBuffers(1)[0];
	}

	void CommandManager::resetCmds() {
		auto& device = Context::GetInstance().device;
		device.resetCommandPool(commandPool);
	}
	void CommandManager::freeCmds(vk::CommandBuffer buffer) {
		auto& device = Context::GetInstance().device;
		device.freeCommandBuffers(commandPool, buffer);
	}

}