#pragma once

#include "vulkan/vulkan.hpp"

namespace toy2d {
	class CommandManager final {
	public:
		CommandManager();
		~CommandManager();
		
		void resetCmds();
		void freeCmds(vk::CommandBuffer buffer);

		std::vector<vk::CommandBuffer> CreateCommandBuffers(std::uint32_t count);
		vk::CommandBuffer CreateOneCommandBuffer();

	private:
		vk::CommandPool commandPool;
		vk::CommandPool createCommandPool();

	};


}