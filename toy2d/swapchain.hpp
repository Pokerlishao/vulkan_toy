 #pragma once

#include "vulkan\vulkan.hpp"

namespace toy2d {
	class Swapchain final {
	public:
		vk::SwapchainKHR swapchain;

		Swapchain(int W,int H);
		~Swapchain();

		struct SwapchainInfo {
			vk::Extent2D imageExtent;
			uint32_t imageCount;
			vk::SurfaceFormatKHR format;
			vk::SurfaceTransformFlagBitsKHR transform;
			vk::PresentModeKHR presentMode;
		};

		void InitFramebuffers();


		SwapchainInfo info;
		std::vector<vk::Image> images;
		std::vector<vk::ImageView> imageViews;
		std::vector<vk::Framebuffer> frameBuffers;
		void queryInfo(int W, int H);
		void getImages();
		void createImageViews();
		void createFramebuffers(int W, int H);
	};
}