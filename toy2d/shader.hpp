#pragma once

#include "vulkan/vulkan.hpp"
#include <memory>

namespace toy2d {
	class Shader final {
	public:
		static void Init(const std::string& vertexSource, const std::string& fragSource);
		static void Quit();
		static Shader& GetInstance();
		Shader(const std::string& vertexSource, const std::string& fragSource);
		~Shader();

		vk::ShaderModule vertShader;
		vk::ShaderModule fragShader;

		std::vector<vk::PipelineShaderStageCreateInfo> GetStage();
		
	private:
		static std::unique_ptr<Shader> instance_;
		std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfo;
		void InitStage();
	};
}