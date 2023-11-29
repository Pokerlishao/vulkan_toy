#include "toy2d/swapchain.hpp"
#include "toy2d/context.hpp"

namespace toy2d {
	Swapchain::Swapchain(int W, int H) {
		queryInfo(W,H);
		vk::SwapchainCreateInfoKHR create_info;
		create_info.setClipped(true)
			.setImageArrayLayers(1)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
			.setSurface(Context::GetInstance().surface)
			.setImageColorSpace(info.format.colorSpace)
			.setImageFormat(info.format.format)
			.setImageExtent(info.imageExtent)
			.setMinImageCount(info.imageCount)
			.setPreTransform(info.transform)
			.setPresentMode(info.presentMode);

		auto &queueIndices = Context::GetInstance().queueInfo;
		if (queueIndices.graphQueue.value() == queueIndices.presentQueue.value()) {
			create_info
				.setImageSharingMode(vk::SharingMode::eExclusive);
		}
		else {
			std::array indices = { queueIndices.graphQueue.value() ,queueIndices.presentQueue.value() };
			create_info.setQueueFamilyIndices(indices)
				.setImageSharingMode(vk::SharingMode::eConcurrent);
		}

		swapchain = Context::GetInstance().device.createSwapchainKHR(create_info);

		getImages();
		createImageViews();
	}

	void Swapchain::queryInfo(int W, int H) {
		auto& phyDevice = Context::GetInstance().physicaldevice;
		auto& surface = Context::GetInstance().surface;
		auto formats = phyDevice.getSurfaceFormatsKHR(surface);
		info.format = formats[0];
		for (auto& format : formats) {
			if ((format.format == vk::Format::eR8G8B8A8Srgb) &&
				(format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)) {
				info.format = format;
				break;
			}
		}

		auto capabilities = phyDevice.getSurfaceCapabilitiesKHR(surface);
		info.imageCount = std::clamp<uint32_t>(2,capabilities.minImageCount, capabilities.maxImageCount);
		info.imageExtent.width = std::clamp<uint32_t>(W, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		info.imageExtent.height = std::clamp<uint32_t>(H, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		info.transform = capabilities.currentTransform;


		auto presents = phyDevice.getSurfacePresentModesKHR(surface);
		info.presentMode = vk::PresentModeKHR::eFifo;
		for (const auto& present: presents) {
			if (present == vk::PresentModeKHR::eMailbox) {
				info.presentMode = present;
				break; 
			}
		}
	}
	void Swapchain::getImages() {
		images = Context::GetInstance().device.getSwapchainImagesKHR(swapchain);
	}


	void Swapchain::createImageViews() {
		imageViews.resize(images.size());
		for (uint32_t i = 0; i < images.size(); i++) {
			vk::ImageViewCreateInfo create_info;
			vk::ComponentMapping mapping;  //ÑÕÉ«Í¨µÀÓ³Éä
			vk::ImageSubresourceRange range;
			range.setBaseMipLevel(0)
				.setLevelCount(1)
				.setBaseArrayLayer(0)
				.setLayerCount(1)
				.setAspectMask(vk::ImageAspectFlagBits::eColor);
			create_info.setImage(images[i])
				.setViewType(vk::ImageViewType::e2D)
				.setComponents(mapping)
				.setFormat(info.format.format)
				.setSubresourceRange(range);
			imageViews[i] = Context::GetInstance().device.createImageView(create_info);
		}
	}


	void Swapchain::InitFramebuffers() {
		createFramebuffers(info.imageExtent.width, info.imageExtent.height);
	}

	Swapchain::~Swapchain() {
		for (auto& x : imageViews) {
			Context::GetInstance().device.destroyImageView(x);
		}
		for (auto& x : frameBuffers) {
			Context::GetInstance().device.destroyFramebuffer(x);
		}
		Context::GetInstance().device.destroySwapchainKHR(swapchain);
	}


	void Swapchain::createFramebuffers(int W, int H) {
		frameBuffers.resize(images.size());
		for (uint32_t i = 0; i < frameBuffers.size();i++) {
			vk::FramebufferCreateInfo frameBufferInfo;
			frameBufferInfo.setAttachments(imageViews[i])
				.setWidth(W).setHeight(H)
				.setRenderPass(Context::GetInstance().renderProcess->renderPass)
				.setLayers(1);
			frameBuffers[i] = Context::GetInstance().device.createFramebuffer(frameBufferInfo);
		}
	}
}