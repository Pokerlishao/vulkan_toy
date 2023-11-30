#include "toy2d/renderer.hpp"
#include "toy2d/context.hpp"
#include "toy2d/buffer.hpp"
#include "toy2d/CommandManager.hpp"
#include "toy2d/uniform.hpp"
#include "vulkan/vulkan.hpp"


namespace toy2d {

	static std::array<Vertex, 3> vertices = {
		Vertex{0.0, -0.2},
		Vertex{0.5, 0.5},
		Vertex{-0.5, 0.5}
	};

	const Uniform uniform{ Color{1, 0, 0} };

	Renderer::Renderer(int maxFlightCount) : maxFlightCount(maxFlightCount), curFrame(0) {
		createSemaphores();
		createFences();
		createCmdBuffers();

		createVertexBuffer();
		bufferVertexData();

		createUniformBuffers();
		bufferUniformData();

		createDescriptorPool();
		allocateDescriptorSets();
		updateDescriptorSets();
	}

	Renderer::~Renderer() {
		hostVertexBuffer_.reset();
		deviceVertexBuffer_.reset();
		hostUniformBuffers_.clear();
		deviceUniformBuffers_.clear();
		auto& device = Context::GetInstance().device;
		for (auto& sem : imageAvailableSems) {
			device.destroySemaphore(sem);
		}
		for (auto& sem : imageDrawFinishSems) {
			device.destroySemaphore(sem);
		}
		for (auto& fence : cmdAvailableFences) {
			device.destroyFence(fence);
		}
		device.destroyDescriptorPool(descriptorPool);
	}

	void Renderer::createFences() {
		vk::FenceCreateInfo fenceInfo;
		fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
		cmdAvailableFences.resize(maxFlightCount, nullptr);
		for(auto &fence: cmdAvailableFences)
			fence = Context::GetInstance().device.createFence(fenceInfo);
	}

	void Renderer::createSemaphores() {
		vk::SemaphoreCreateInfo semInfo;
		auto& device = Context::GetInstance().device;
		imageAvailableSems.resize(maxFlightCount);
		imageDrawFinishSems.resize(maxFlightCount);
		for(auto &sem:imageAvailableSems)
			sem = device.createSemaphore(semInfo);
		for (auto& sem : imageDrawFinishSems)
			sem = device.createSemaphore(semInfo);
	}



	void Renderer::createCmdBuffers() {
		cmdBuffers.resize(maxFlightCount);
		for (auto& cmdbuffer : cmdBuffers)
			cmdbuffer = Context::GetInstance().commandManager->CreateOneCommandBuffer();
	}


	void Renderer::Render() {
		auto& ctx = Context::GetInstance();
		auto& device = ctx.device;
		auto& render_process = ctx.renderProcess;
		auto& swapchain = ctx.swapchain;
		auto& cmdMag = ctx.commandManager;

		//CPU GPU Í¬²½
		if (device.waitForFences(cmdAvailableFences[curFrame], true, std::numeric_limits<std::uint64_t>::max()) != vk::Result::eSuccess) {
			throw std::runtime_error("Wait for fence failed!");
		}
		device.resetFences(cmdAvailableFences[curFrame]);


		auto& result = device.acquireNextImageKHR(swapchain->swapchain, 
			std::numeric_limits<uint64_t>::max(), imageAvailableSems[curFrame]); //time out
		if (result.result != vk::Result::eSuccess) {
			throw std::runtime_error("Acquire next image in swapchain failed!");
		}
		auto imageIndex = result.value;

		cmdBuffers[curFrame].reset();
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdBuffers[curFrame].begin(beginInfo);
		{
			vk::RenderPassBeginInfo beginInfo;
			vk::Rect2D area({ 0,0 }, {swapchain->info.imageExtent});
			vk::ClearValue clearValue;
			clearValue.setColor(vk::ClearColorValue({0.1f, 0.0f, 0.3f, 1.0f}));
			beginInfo.setRenderPass(render_process->renderPass)
				.setRenderArea(area)
				.setFramebuffer(swapchain->frameBuffers[imageIndex])
				.setClearValues(clearValue);
			cmdBuffers[curFrame].beginRenderPass(&beginInfo, vk::SubpassContents::eInline);
			{
				cmdBuffers[curFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, render_process->graphicsPipeline);
				vk::DeviceSize offset = 0;
				cmdBuffers[curFrame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
					Context::GetInstance().renderProcess->layout, 0, descriptorSets[curFrame], {});
				cmdBuffers[curFrame].bindVertexBuffers(0, deviceVertexBuffer_->buffer, offset);
				cmdBuffers[curFrame].draw(3, 1, 0, 0);
			}
			cmdBuffers[curFrame].endRenderPass();
		}
		cmdBuffers[curFrame].end();

		vk::SubmitInfo submitInfo;
		vk::PipelineStageFlags stagemask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		submitInfo.setCommandBuffers(cmdBuffers[curFrame])
			.setWaitSemaphores(imageAvailableSems[curFrame])
			.setWaitDstStageMask(stagemask)
			.setSignalSemaphores(imageDrawFinishSems[curFrame]);
		ctx.graphics_queue.submit(submitInfo, cmdAvailableFences[curFrame]);


		vk::PresentInfoKHR presentInfo;
		presentInfo.setImageIndices(imageIndex)
			.setSwapchains(swapchain->swapchain)
			.setWaitSemaphores(imageDrawFinishSems[curFrame]);
		
		if (ctx.present_queue.presentKHR(presentInfo) != vk::Result::eSuccess) {
			throw std::runtime_error("Present queue execute failed");
		}

		curFrame = (curFrame + 1) % maxFlightCount;
	}


	void Renderer::createVertexBuffer() {
		hostVertexBuffer_.reset(new Buffer(sizeof(vertices), 
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
		deviceVertexBuffer_.reset(new Buffer(sizeof(vertices),
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal));
	}

	void Renderer::bufferVertexData() {
		auto& ctx = Context::GetInstance();
		void* ptr = ctx.device.mapMemory(hostVertexBuffer_->memory, 0, hostVertexBuffer_->size);
		memcpy(ptr, vertices.data(), sizeof(vertices));
		ctx.device.unmapMemory(hostVertexBuffer_->memory);

		copyBuffer(hostVertexBuffer_->buffer, deviceVertexBuffer_->buffer, hostVertexBuffer_->size, 0, 0);
	}

	void Renderer::createUniformBuffers() {
		hostUniformBuffers_.resize(maxFlightCount);
		deviceUniformBuffers_.resize(maxFlightCount);

		for (auto& buffer : hostUniformBuffers_) {
			buffer.reset(new Buffer(sizeof(uniform),
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostCoherent|vk::MemoryPropertyFlagBits::eHostVisible));
		}

		for (auto& buffer : deviceUniformBuffers_) {
			buffer.reset(new Buffer(sizeof(uniform),
				vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
				vk::MemoryPropertyFlagBits::eDeviceLocal));
		}
	}
	void Renderer::bufferUniformData() {
		for (size_t i=0; i < hostUniformBuffers_.size(); i++) {
			auto& hostBuffer = hostUniformBuffers_[i];
			void* ptr = Context::GetInstance().device.mapMemory(hostBuffer->memory, 0, hostBuffer->size);
			memcpy(ptr, &uniform, sizeof(uniform));
			Context::GetInstance().device.unmapMemory(hostBuffer->memory);

			copyBuffer(hostBuffer->buffer, deviceUniformBuffers_[i]->buffer, hostBuffer->size, 0, 0);
		}
	}


	void Renderer::copyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size, size_t srcOffset, size_t dstOffset) {
		auto& ctx = Context::GetInstance();
		
		auto cmdBuf = ctx.commandManager->CreateOneCommandBuffer();

		vk::CommandBufferBeginInfo begin;
		begin.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdBuf.begin(begin); {
			vk::BufferCopy region;
			region.setSize(size)
				.setSrcOffset(srcOffset)
				.setDstOffset(dstOffset);
			cmdBuf.copyBuffer(src, dst, region);
		} cmdBuf.end();

		vk::SubmitInfo submit;
		submit.setCommandBuffers(cmdBuf);
		ctx.graphics_queue.submit(submit);
		ctx.device.waitIdle();
		ctx.commandManager->freeCmds(cmdBuf);
	}


	void Renderer::createDescriptorPool() {
		vk::DescriptorPoolCreateInfo poolInfo;
		vk::DescriptorPoolSize poolSize;
		poolSize.setDescriptorCount(maxFlightCount)
			.setType(vk::DescriptorType::eUniformBuffer);
		poolInfo.setPoolSizes(poolSize)
			.setMaxSets(maxFlightCount);
		descriptorPool = Context::GetInstance().device.createDescriptorPool(poolInfo);
	}

	void Renderer::allocateDescriptorSets() {
		std::vector<vk::DescriptorSetLayout> layouts(maxFlightCount, Context::GetInstance().renderProcess->setLayout);
		vk::DescriptorSetAllocateInfo allocInfo;
		allocInfo.setDescriptorPool(descriptorPool)
			.setDescriptorSetCount(maxFlightCount)
			.setSetLayouts(layouts);
		descriptorSets = Context::GetInstance().device.allocateDescriptorSets(allocInfo);
		//for (auto& desc : layouts) {
		//	Context::GetInstance().device.destroyDescriptorSetLayout(desc);
		//}
	}

	void Renderer::updateDescriptorSets() {
		for (size_t i = 0; i < descriptorSets.size();i++) {
			auto& set = descriptorSets[i];
			vk::WriteDescriptorSet desc;
			vk::DescriptorBufferInfo bufferInfo;
			bufferInfo.setBuffer(deviceUniformBuffers_[i]->buffer)
				.setRange(deviceUniformBuffers_[i]->size)
				.setOffset(0);
			desc.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setBufferInfo(bufferInfo)
				.setDstBinding(0)
				.setDstSet(set)
				.setDstArrayElement(0)
				.setDescriptorCount(1);


			Context::GetInstance().device.updateDescriptorSets(desc, {});
		}
	}
}