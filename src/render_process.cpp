#include "vulkan/vulkan.hpp"
#include "toy2d/render_process.hpp"
#include "toy2d/swapchain.hpp"
#include "toy2d/context.hpp"
#include "toy2d/shader.hpp"
#include "toy2d/uniform.hpp"

namespace toy2d {

	RenderProcess::RenderProcess() {
		setLayout = createSetLayout();
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
		auto range = Shader::GetInstance().GetPushConstantRange();
		layoutInfo.setSetLayouts(Shader::GetInstance().GetDescriptorSetLayouts())
			.setPushConstantRanges(range);
		return Context::GetInstance().device.createPipelineLayout(layoutInfo);
	}

	vk::DescriptorSetLayout RenderProcess::createSetLayout() {
		vk::DescriptorSetLayoutCreateInfo setInfo;
		auto binding = Uniform::GetBinding();
		setInfo.setBindings(binding);

		return Context::GetInstance().device.createDescriptorSetLayout(setInfo);
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
			.setCullMode(vk::CullModeFlagBits::eFront)
			.setFrontFace(vk::FrontFace::eCounterClockwise )
			.setPolygonMode(vk::PolygonMode::eFill)
			.setLineWidth(1)
			.setDepthClampEnable(false);
		createInfo.setPRasterizationState(&rasteState);

		//6. Multi-Sample
		vk::PipelineMultisampleStateCreateInfo MSStateInfo;
		MSStateInfo.setSampleShadingEnable(false) //no super sampling
			.setRasterizationSamples(vk::SampleCountFlagBits::e1);
		createInfo.setPMultisampleState(&MSStateInfo);

		//7. stencil test depth test

		//8. color blending

		vk::PipelineColorBlendAttachmentState blendstate;
		blendstate.setBlendEnable(true)
			.setColorWriteMask(vk::ColorComponentFlagBits::eR |
				vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB |
				vk::ColorComponentFlagBits::eA)
			.setSrcColorBlendFactor(vk::BlendFactor::eOne)
			.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
			.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
			.setAlphaBlendOp(vk::BlendOp::eAdd);

		vk::PipelineColorBlendStateCreateInfo ColorBlendStage;
		ColorBlendStage.setLogicOpEnable(false)
			.setAttachments(blendstate);
		createInfo.setPColorBlendState(&ColorBlendStage);

		// dynamic changing state of pipeline
		//vk::PipelineDynamicStateCreateInfo dynamicState;
		//std::array states = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		//dynamicState.setDynamicStates(states);
		//createInfo.setPDynamicState(&dynamicState);

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