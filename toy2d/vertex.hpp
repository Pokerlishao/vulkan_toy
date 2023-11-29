#pragma once

#include "vulkan/vulkan.hpp"

namespace toy2d {
	struct Vertex final {
		float x, y;

		static vk::VertexInputAttributeDescription GetAttribute() {
			vk::VertexInputAttributeDescription desc;
			desc.setBinding(0)
				.setFormat(vk::Format::eR32G32Sfloat)
				.setLocation(0)
				.setOffset(0);
			return desc;
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