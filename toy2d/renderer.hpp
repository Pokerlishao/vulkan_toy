#pragma once

#include "vulkan/vulkan.hpp"
#include "toy2d/vertex.hpp"
#include "toy2d/buffer.hpp"
#include "toy2d/tool.hpp"
#include "toy2d/CommandManager.hpp"
#include "toy2d/texture.hpp"
#include "glm/glm.hpp"

namespace toy2d {
	class Renderer final {
	public:

		Renderer(int maxFlightCount = 2);
		~Renderer();
		void Render(int x,int y, float rot);

		void SetProject(int right, int left, int bottom, int top, int far, int near);
		void SetDrawColor(const Color& color);

	private:
		int maxFlightCount;
		int curFrame;

		glm::mat4x4 projectMat;
		glm::mat4x4 viewMat;


		std::vector<vk::CommandBuffer> cmdBuffers;
		std::vector<vk::Fence> cmdAvailableFences;
		std::vector<vk::Semaphore> imageAvailableSems;
		std::vector<vk::Semaphore> imageDrawFinishSems;


		std::unique_ptr<Buffer> hostVertexBuffer_;
		std::unique_ptr<Buffer> deviceVertexBuffer_;
		std::unique_ptr<Buffer> hostIndicesBuffer_;
		std::unique_ptr<Buffer> deviceIndicesBuffer_;
		std::vector<std::unique_ptr<Buffer>> hostUniformBuffers_;
		std::vector<std::unique_ptr<Buffer>> deviceUniformBuffers_;
		std::vector<std::unique_ptr<Buffer>> hostColorBuffers_;
		std::vector<std::unique_ptr<Buffer>> deviceColorBuffers_;

		vk::DescriptorPool descriptorPool;
		std::vector<vk::DescriptorSet> descriptorSets;

		std::unique_ptr<Texture> texture;
		vk::Sampler sampler;

		void createSemaphores();
		void createFences();
		void createCmdBuffers();
		void createVertexBuffer();
		void bufferVertexData();
		void createIndicesBuffer();
		void bufferIndicesData();
		void createUniformBuffers();


		void createDescriptorPool();
		void allocateDescriptorSets();
		void updateDescriptorSets();

		void bufferMVPData();

		void createSampler();
		void createTexture();

		void copyBuffer(Buffer& src, Buffer& dst, size_t size, size_t srcOffset, size_t dstOffset);
	};
}