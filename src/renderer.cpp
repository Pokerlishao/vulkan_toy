#include "toy2d/renderer.hpp"
#include "toy2d/context.hpp"
#include "toy2d/buffer.hpp"
#include "toy2d/CommandManager.hpp"
#include "toy2d/uniform.hpp"
#include "toy2d/tool.hpp"
#include "toy2d/shader.hpp"
#include "toy2d/texture.hpp"
#include "toy2d/descriptor_manager.hpp"
#include "vulkan/vulkan.hpp"
#include "glm/glm.hpp"
#include "glm/common.hpp"
#include "glm/gtc/matrix_transform.hpp"

//todo  renderer


namespace toy2d {

	static Vertex vertices[] = {
		Vertex{-0.5, -0.5, 0, 0},
		Vertex{0.5, -0.5, 1, 0},
		Vertex{0.5, 0.5, 1 , 1},
		Vertex{-0.5, 0.5, 0, 1}
	};

	static std::uint32_t indices[] = {
		0, 1, 3,
		1, 2, 3,
	};


	Renderer::Renderer(int maxFlightCount) : maxFlightCount(maxFlightCount), curFrame(0) {
		createSemaphores();
		createFences();
		createCmdBuffers();

		createVertexBuffer();
		bufferVertexData();
		createIndicesBuffer();
		bufferIndicesData();

		createUniformBuffers();


		descriptorManagers = DescriptorSetManager::Instance().AllocBufferSets(maxFlightCount);

		updateDescriptorSets();

		projectMat = glm::mat4x4(1.0f);
		viewMat = glm::mat4x4(1.0f);

		SetDrawColor(Color{ 0, 0, 0 });
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

	void Renderer::StartRender() {
		auto& ctx = Context::GetInstance();
		auto& device = ctx.device;
		auto& render_process = ctx.renderProcess;
		auto& swapchain = ctx.swapchain;
		auto& cmdMag = ctx.commandManager;
		auto& layout = ctx.renderProcess->layout;

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
		imageIndex = result.value;

		cmdBuffers[curFrame].reset();
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdBuffers[curFrame].begin(beginInfo);

		vk::RenderPassBeginInfo passbeginInfo;
		vk::Rect2D area({ 0,0 }, { swapchain->info.imageExtent });
		vk::ClearValue clearValue;
		clearValue.setColor(vk::ClearColorValue({ 0.1f, 0.1f, 0.1f, 1.0f }));
		passbeginInfo.setRenderPass(render_process->renderPass)
			.setRenderArea(area)
			.setFramebuffer(swapchain->frameBuffers[imageIndex])
			.setClearValues(clearValue);
		cmdBuffers[curFrame].beginRenderPass(&passbeginInfo, vk::SubpassContents::eInline);
		cmdBuffers[curFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, render_process->graphicsPipeline);
	}


	void Renderer::DrawTexture(int x, int y, float rot, Texture& texture) {
		auto& ctx = Context::GetInstance();
		auto& device = ctx.device;
		auto& layout = ctx.renderProcess->layout;

		vk::DeviceSize offset = 0;
		cmdBuffers[curFrame].bindVertexBuffers(0, hostVertexBuffer_->buffer, offset);
		cmdBuffers[curFrame].bindIndexBuffer(hostIndicesBuffer_->buffer, 0, vk::IndexType::eUint32);
		cmdBuffers[curFrame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			layout,
			0, { descriptorManagers[curFrame].set, texture.set.set }, {});
		glm::mat4x4 modelMat(1.0f);
		modelMat = glm::translate(modelMat,{ float(x), float(y), 0 });
		modelMat = glm::scale(modelMat, { 400.0, 400.0, 0 });
		modelMat = glm::rotate(modelMat, glm::radians(rot),{ 0, 0, 1 });
		cmdBuffers[curFrame].pushConstants(layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4x4), (void*)&modelMat);
		cmdBuffers[curFrame].drawIndexed(6, 1, 0, 0, 0);

	}

	void Renderer::EndRender() {
		auto& ctx = Context::GetInstance();
		auto& device = ctx.device;
		auto& swapchain = ctx.swapchain;

		cmdBuffers[curFrame].endRenderPass();
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
			vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
		deviceVertexBuffer_.reset(new Buffer(sizeof(vertices),
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal));
	}

	void Renderer::bufferVertexData() {
		memcpy(hostVertexBuffer_->map, vertices, sizeof(vertices));
	}

	void Renderer::createIndicesBuffer() {
		hostIndicesBuffer_.reset(new Buffer(sizeof(indices),
			vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
		deviceIndicesBuffer_.reset(new Buffer(sizeof(indices),
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal));
	}

	void Renderer::bufferIndicesData() {
		memcpy(hostIndicesBuffer_->map, indices, sizeof(indices));
	}

	void Renderer::createUniformBuffers() {
		hostUniformBuffers_.resize(maxFlightCount);
		deviceUniformBuffers_.resize(maxFlightCount);
		hostColorBuffers_.resize(maxFlightCount);
		deviceColorBuffers_.resize(maxFlightCount);

		size_t size = sizeof(glm::mat4x4) * 2;
		for (auto& buffer : hostUniformBuffers_) {
			buffer.reset(new Buffer(size,
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostCoherent|vk::MemoryPropertyFlagBits::eHostVisible));
		}

		for (auto& buffer : deviceUniformBuffers_) {
			buffer.reset(new Buffer(size,
				vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
				vk::MemoryPropertyFlagBits::eDeviceLocal));
		}

		for (auto& buffer : hostColorBuffers_) {
			buffer.reset(new Buffer(sizeof(Color),
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
		}

		for (auto& buffer : deviceColorBuffers_) {
			buffer.reset(new Buffer(sizeof(Color),
				vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
				vk::MemoryPropertyFlagBits::eDeviceLocal));
		}
	}



	void Renderer::copyBuffer(Buffer& src, Buffer& dst, size_t size, size_t srcOffset, size_t dstOffset) {
		auto& ctx = Context::GetInstance();
		
		auto cmdBuf = ctx.commandManager->CreateOneCommandBuffer();

		vk::CommandBufferBeginInfo begin;
		begin.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdBuf.begin(begin); {
			vk::BufferCopy region;
			region.setSize(size)
				.setSrcOffset(srcOffset)
				.setDstOffset(dstOffset);
			cmdBuf.copyBuffer(src.buffer, dst.buffer, region);
		} cmdBuf.end();

		vk::SubmitInfo submit;
		submit.setCommandBuffers(cmdBuf);
		ctx.graphics_queue.submit(submit);
		ctx.graphics_queue.waitIdle();
		ctx.device.waitIdle();
		ctx.commandManager->freeCmds(cmdBuf);
	}


	//void Renderer::createDescriptorPool() {
	//	vk::DescriptorPoolCreateInfo poolInfo;
	//	std::vector<vk::DescriptorPoolSize> poolSizes(2);
	//	poolSizes[0].setDescriptorCount(maxFlightCount * 2)
	//		.setType(vk::DescriptorType::eUniformBuffer);
	//	poolSizes[1].setDescriptorCount(maxFlightCount)
	//		.setType(vk::DescriptorType::eCombinedImageSampler);
	//	poolInfo.setPoolSizes(poolSizes)
	//		.setMaxSets(maxFlightCount);
	//	descriptorPool = Context::GetInstance().device.createDescriptorPool(poolInfo);
	//}


	// the layout need to be destroyed
	//void Renderer::allocateDescriptorSets() {
	//	std::vector<vk::DescriptorSetLayout> layouts(maxFlightCount, Shader::GetInstance().GetDescriptorSetLayouts()[0]);
	//	vk::DescriptorSetAllocateInfo allocInfo;
	//	allocInfo.setDescriptorPool(descriptorPool)
	//		.setSetLayouts(layouts);
	//	descriptorSets = Context::GetInstance().device.allocateDescriptorSets(allocInfo);
	//}

	void Renderer::updateDescriptorSets() {
		for (size_t i = 0; i < descriptorManagers.size();i++) {
			
			// bind MVP buffer
			vk::DescriptorBufferInfo bufferInfo1;
			bufferInfo1.setBuffer(deviceUniformBuffers_[i]->buffer)
				.setRange(sizeof(glm::mat4) * 2) // sizeof(glm::mat4) * 2
				.setOffset(0);

			std::vector<vk::WriteDescriptorSet> writeInfos(2);
			writeInfos[0].setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setBufferInfo(bufferInfo1)
				.setDstBinding(0)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setDescriptorCount(1)
				.setDstArrayElement(0)
				.setDstSet(descriptorManagers[i].set);


			// bind Color buffer
			vk::DescriptorBufferInfo bufferInfo2;
			bufferInfo2.setBuffer(deviceColorBuffers_[i]->buffer)
				.setOffset(0)
				.setRange(sizeof(Color));

			writeInfos[1].setBufferInfo(bufferInfo2)
				.setDstBinding(1)
				.setDstArrayElement(0)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setDstSet(descriptorManagers[i].set);

			//// bind image
			//vk::DescriptorImageInfo imageInfo;
			//imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			//	.setImageView(texture->imageView)
			//	.setSampler(sampler);

			//writeInfos[2].setImageInfo(imageInfo)
			//	.setDstBinding(2)
			//	.setDstArrayElement(0)
			//	.setDescriptorCount(1)
			//	.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			//	.setDstSet(descriptorSets[i]);

			Context::GetInstance().device.updateDescriptorSets(writeInfos, {});
		}
	}

	void Renderer::SetDrawColor(const Color& color) {
		for (int i = 0; i < hostColorBuffers_.size(); i++) {
			auto& buffer = hostColorBuffers_[i];
			auto& device = Context::GetInstance().device;
			memcpy(buffer->map, (void*)&color, sizeof(float) * 3);

			copyBuffer(*buffer, *deviceColorBuffers_[i], buffer->size, 0, 0);
		}
	}

	void Renderer::SetProject(int right, int left, int bottom, int top, int far, int near) {
		//projectMat = glm::ortho(left, right, bottom, top, near, far);
		projectMat = glm::mat4x4(1.0f);

		projectMat[0][0] = 2.0 / (right - left);
		projectMat[1][1] = 2.0 / (top - bottom);
		projectMat[2][2] = 2.0 / (near - far);
		projectMat[3][0] = (float)(left + right) / (left - right);
		projectMat[3][1] = (float)(top + bottom) / (bottom - top);
		projectMat[3][2] = (float)(near + far) / (far - near);

		bufferMVPData();
	}

	void Renderer::bufferMVPData() {
		for (size_t i = 0; i < hostUniformBuffers_.size(); i++) {
			auto& hostBuffer = hostUniformBuffers_[i];
			memcpy(hostBuffer->map, (void*)&projectMat, sizeof(glm::mat4x4));
			memcpy((float*)hostBuffer->map + 4*4, (void*)&viewMat, sizeof(glm::mat4x4));
			copyBuffer(*hostBuffer, *deviceUniformBuffers_[i], hostBuffer->size, 0, 0);
		}
	}

	void Renderer::createSampler() {
		vk::SamplerCreateInfo createInfo;
		createInfo.setMagFilter(vk::Filter::eLinear)
			.setMinFilter(vk::Filter::eLinear)
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat)
			.setAnisotropyEnable(false)
			.setBorderColor(vk::BorderColor::eIntOpaqueWhite)
			.setUnnormalizedCoordinates(false)
			.setCompareEnable(false)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear);
		sampler = Context::GetInstance().device.createSampler(createInfo);
	}
	//void Renderer::createTexture() {
	//	texture = std::make_unique<Texture>("G:\\code\\toy2d\\resources\\nahida.png");
	//}
}