#pragma once

#include "vulkan/vulkan.hpp"
#include <functional>

namespace toy2d {
	class CommandManager final {
	public:
		CommandManager();
		~CommandManager();
		
		void resetCmds();
		void freeCmds(vk::CommandBuffer buffer);

		std::vector<vk::CommandBuffer> CreateCommandBuffers(std::uint32_t count);
		vk::CommandBuffer CreateOneCommandBuffer();

		using RecordCmdFunc = std::function<void(vk::CommandBuffer&)>;
		void ExecuteCmd(vk::Queue, RecordCmdFunc);

	private:
		vk::CommandPool commandPool;
		vk::CommandPool createCommandPool();

	};


}