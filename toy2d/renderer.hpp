#pragma once

#include "vulkan/vulkan.hpp"
#include "toy2d/vertex.hpp"
#include "toy2d/buffer.hpp"
#include "toy2d/CommandManager.hpp"

namespace toy2d {
	class Renderer final {
	public:

		Renderer(int maxFlightCount = 2);
		~Renderer();
		void Render();

	private:
		int maxFlightCount;
		int curFrame;


		std::vector<vk::CommandBuffer> cmdBuffers;
		std::vector<vk::Fence> cmdAvailableFences;
		std::vector<vk::Semaphore> imageAvailableSems;
		std::vector<vk::Semaphore> imageDrawFinishSems;


		std::unique_ptr<Buffer> hostVertexBuffer_;
		std::unique_ptr<Buffer> deviceVertexBuffer_;

		void createSemaphores();
		void createFences();
		void createCmdBuffers();
		void createVertexBuffer();
		void bufferVertexData();
	};
}