#pragma once

#include "vulkan/vulkan.hpp"

namespace toy2d {
	struct Vertex final {
		float x, y;
		float u, v;

		static std::vector<vk::VertexInputAttributeDescription> GetAttribute() {
			std::vector <vk::VertexInputAttributeDescription> descs(2);
			descs[0].setBinding(0)
				.setFormat(vk::Format::eR32G32Sfloat)
				.setLocation(0)
				.setOffset(0);
			descs[1].setBinding(0)
				.setFormat(vk::Format::eR32G32Sfloat)
				.setLocation(1)
				.setOffset(sizeof(float) * 2);
			return descs;
		}

		static vk::VertexInputBindingDescription GetBinding() {
			vk::VertexInputBindingDescription binding;
			binding.setBinding(0)
				.setInputRate(vk::VertexInputRate::eVertex)
				.setStride(sizeof(Vertex));
			return binding;
		}
	};
}