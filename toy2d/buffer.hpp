#pragma once

#include "vulkan/vulkan.hpp"

namespace toy2d {
	class Buffer final {
	public:
		vk::Buffer buffer;
		vk::DeviceMemory memory;
		size_t size;

		Buffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags property);
		~Buffer();
	private:
		struct MemoryInfo final {
			size_t size;
			uint32_t index;
		};

		void createBuffer(size_t size, vk::BufferUsageFlags usage);
		void allocMemory(MemoryInfo info);
		void bindingMem2Buf();

		MemoryInfo GetMemoryInfo(vk::MemoryPropertyFlags property);
	};


}