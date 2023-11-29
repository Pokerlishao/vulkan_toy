#include "vulkan/vulkan.hpp"
#include "toy2d/buffer.hpp"
#include "toy2d/context.hpp"


namespace toy2d {
	Buffer::Buffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags property) : size(size) {
		createBuffer(size, usage);
		auto info = GetMemoryInfo(property);
		allocMemory(info);
		bindingMem2Buf();
	}
	Buffer::~Buffer() {
		auto& device = Context::GetInstance().device;
		device.destroyBuffer(buffer);
		device.freeMemory(memory);
	}

	void Buffer::createBuffer(size_t size, vk::BufferUsageFlags usage) {
		vk::BufferCreateInfo bufferInfo;
		bufferInfo.setSize(size)
			.setUsage(usage)
			.setSharingMode(vk::SharingMode::eExclusive);

		buffer = Context::GetInstance().device.createBuffer(bufferInfo);
	}
	void Buffer::allocMemory(MemoryInfo info) {
		vk::MemoryAllocateInfo allocInfo;
		allocInfo.setMemoryTypeIndex(info.index)
			.setAllocationSize(info.size);

		memory = Context::GetInstance().device.allocateMemory(allocInfo);
	}
	void Buffer::bindingMem2Buf() {
		Context::GetInstance().device.bindBufferMemory(buffer, memory, 0);
	}

	Buffer::MemoryInfo Buffer::GetMemoryInfo(vk::MemoryPropertyFlags property) {
		Buffer::MemoryInfo info;
		auto& device = Context::GetInstance().device;
		auto requirements = device.getBufferMemoryRequirements(buffer);
		info.size = requirements.size;
		
		auto properties = Context::GetInstance().physicaldevice.getMemoryProperties();
		for (int i = 0; i < properties.memoryTypeCount; i++) {
			if ( ((i << 1) & requirements.memoryTypeBits) &&
				(properties.memoryTypes[i].propertyFlags & property) ) {
				info.index = i;
				break;
			}
		}

		return info;
	}

}