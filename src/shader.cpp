#include "vulkan/vulkan.hpp"
#include "toy2d/shader.hpp"
#include "toy2d/context.hpp"


namespace toy2d {
	std::unique_ptr<Shader> Shader::instance_ = nullptr;

	void Shader::Init(const std::string& vertexSource, const std::string& fragSource) {
		instance_.reset(new Shader(vertexSource, fragSource));
	}

	void Shader::Quit() {
		instance_.reset();
	}

	Shader& Shader::GetInstance() {
		return *instance_;
	}

	Shader::Shader(const std::string& vertexSource, const std::string& fragSource) {
		vk::ShaderModuleCreateInfo create_info;
		create_info.setCodeSize(vertexSource.size())
			.setPCode((uint32_t*)(vertexSource.data()));
		vertShader = Context::GetInstance().device.createShaderModule(create_info);


		create_info.setCodeSize(fragSource.size())
			.setPCode((uint32_t*)(fragSource.data()));
		fragShader = Context::GetInstance().device.createShaderModule(create_info);

		InitStage();
	}

	Shader::~Shader() {
		auto& device = Context::GetInstance().device;
		device.destroyShaderModule(vertShader);
		device.destroyShaderModule(fragShader);
	}

	std::vector<vk::PipelineShaderStageCreateInfo> Shader::GetStage() {
		return shaderStageInfo;
	}


	void Shader::InitStage() {
		shaderStageInfo.resize(2);
		shaderStageInfo[0].setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(vertShader)
			.setPName("main");
		shaderStageInfo[1].setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(fragShader)
			.setPName("main");
	}
}
