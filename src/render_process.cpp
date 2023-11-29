#include "vulkan/vulkan.hpp"
#include "toy2d/render_process.hpp"
#include "toy2d/swapchain.hpp"
#include "toy2d/context.hpp"
#include "toy2d/shader.hpp"

namespace toy2d {

	RenderProcess::RenderProcess() {
		layout = createLayout();
		renderPass = createRenderPass();
		graphicsPipeline = nullptr;
	}

	RenderProcess::~RenderProcess() {
		auto device = Context::GetInstance().device;
		device.destroyPipelineLayout(layout);
		device.destroyRenderPass(renderPass);
		device.destroyPipeline(graphicsPipeline);
	}

	vk::PipelineLayout RenderProcess::createLayout() {
		vk::PipelineLayoutCreateInfo layoutInfo;
		layoutInfo.setPushConstantRangeCount(0)
			.setSetLayoutCount(0);
		return Context::GetInstance().device.createPipelineLayout(layoutInfo);
	}

	vk::Pipeline RenderProcess::createGraphicsPipeline() {
		auto& ctx = Context::GetInstance();
		vk::GraphicsPipelineCreateInfo createInfo;

		//1 Vertex
		vk::PipelineVertexInputStateCreateInfo vertexInputState;
		auto attr = Vertex::GetAttribute();
		auto binding = Vertex::GetBinding();
		vertexInputState.setVertexAttributeDescriptions(attr)
			.setVertexBindingDescriptions(binding);
		createInfo.setPVertexInputState(&vertexInputState);

		//2 Vertex Assembly
		vk::PipelineInputAssemblyStateCreateInfo assemblyState;
		assemblyState.setPrimitiveRestartEnable(false)
			.setTopology(vk::PrimitiveTopology::eTriangleList);
		createInfo.setPInputAssemblyState(&assemblyState);

		//3. Shader
		auto stage = Shader::GetInstance().GetStage();
		createInfo.setStages(stage);
		//createInfo.setStages(Shader::GetInstance().GetStage());

		//4. viewport & scissor
		vk::PipelineViewportStateCreateInfo viewState;
		vk::Viewport viewport;
		viewport.setX(0).setY(0)
			.setHeight(ctx.swapchain->info.imageExtent.height).setWidth(ctx.swapchain->info.imageExtent.width)
			.setMinDepth(0).setMaxDepth(1);
		vk::Rect2D scissor({ 0, 0 },  ctx.swapchain->info.imageExtent );
		viewState.setViewports(viewport)
			.setScissors(scissor); //绘制一部分
		createInfo.setPViewportState(&viewState);

		//5. Rasterization
		vk::PipelineRasterizationStateCreateInfo rasteState;
		rasteState.setRasterizerDiscardEnable(false)
			.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eClockwise )
			.setPolygonMode(vk::PolygonMode::eFill)
			.setLineWidth(1);
		createInfo.setPRasterizationState(&rasteState);

		//6. Multi-Sample
		vk::PipelineMultisampleStateCreateInfo MSStateInfo;
		MSStateInfo.setSampleShadingEnable(false) //no super sampling
			.setRasterizationSamples(vk::SampleCountFlagBits::e1);
		createInfo.setPMultisampleState(&MSStateInfo);

		//7. stencil test depth test

		//8. color blending
		vk::PipelineColorBlendStateCreateInfo ColorBlendStage;
		vk::PipelineColorBlendAttachmentState attaches;
		attaches.setBlendEnable(false)
			.setColorWriteMask(vk::ColorComponentFlagBits::eR |
				vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB |
				vk::ColorComponentFlagBits::eA);
		ColorBlendStage.setLogicOpEnable(false)
			.setAttachments(attaches);
		createInfo.setPColorBlendState(&ColorBlendStage);

		//9. renderPass & layout
		createInfo.setLayout(layout)
			.setRenderPass(renderPass);

		auto result = Context::GetInstance().device.createGraphicsPipeline(nullptr, createInfo);
		if (result.result != vk::Result::eSuccess) {
			throw std::runtime_error("Create graphics pipeline failed!");
		}
		return result.value;
	}

	vk::RenderPass RenderProcess::createRenderPass() {
		auto& device = Context::GetInstance().device;
		vk::RenderPassCreateInfo passInfo;
		vk::AttachmentDescription attachDesc;
		attachDesc.setFormat(Context::GetInstance().swapchain->info.format.format)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setSamples(vk::SampleCountFlagBits::e1);

		vk::SubpassDescription passDesc;
		vk::AttachmentReference reference;

		reference.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setAttachment(0);
		passDesc.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachments(reference);

		vk::SubpassDependency dependency;
		dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			.setDstSubpass(0)
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);


		passInfo.setSubpasses(passDesc)
			.setAttachments(attachDesc)
			.setDependencies(dependency);
		return device.createRenderPass(passInfo);
	}

	void RenderProcess::recreateGraphicsPipeline() {
		if (graphicsPipeline)
			Context::GetInstance().device.destroyPipeline(graphicsPipeline);
		graphicsPipeline = createGraphicsPipeline();
	}
	void RenderProcess::recreateRenderPass() {
		if (renderPass)
			Context::GetInstance().device.destroyRenderPass(renderPass);
		renderPass = createRenderPass();
	}



}