#pragma once

#include "vulkan/vulkan.hpp"
#include <memory>

namespace toy2d {
	class RenderProcess final {
	public:
		vk::Pipeline graphicsPipeline;
		vk::PipelineLayout layout;
		vk::RenderPass renderPass;

		void recreateGraphicsPipeline();
		void recreateRenderPass();


		RenderProcess();
		~RenderProcess();

	private:
		vk::Pipeline createGraphicsPipeline();
		vk::PipelineLayout createLayout();
		vk::RenderPass createRenderPass();

	};
}