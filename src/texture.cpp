#include "toy2d/texture.hpp"
#include "toy2d/buffer.hpp"
#include "toy2d/context.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "toy2d/stb_image.h"



namespace toy2d {
	Texture::Texture(std::string_view filename) {
		int w, h, channel;
		stbi_uc* pixels = stbi_load(filename.data(), &w, &h, &channel, STBI_rgb_alpha);
		printf("Load picture: width: %d, height:%d, channel: %d.\n", w, h, channel);
		size_t size = w * h * 4; //RGBA

		if (!pixels || (w <=0) || (h<=0)) {
			throw std::runtime_error("Load image failed!");
		}

		std::unique_ptr<Buffer> buffer = std::make_unique<Buffer>(
			size, 
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent);

		memcpy(buffer->map, pixels, size);

		createImage(w, h);
		allocMemory();
		Context::GetInstance().device.bindImageMemory(image, memory, 0);
		transitionImageLayoutFromUndefine2Dst();
		transformData2Image(*buffer, w, h);
		transitionImageLayoutFromDst2Optimal();
		createImageView();

		stbi_image_free(pixels);
		set = DescriptorSetManager::Instance().AllocImageSet();
		updateDescriptorSet();
	}

	Texture::~Texture() {
		auto& device = Context::GetInstance().device;
		DescriptorSetManager::Instance().FreeImageSet(set);
		device.destroyImageView(imageView);
		device.destroyImage(image);
		device.freeMemory(memory);
	}

	void Texture::createImage(uint32_t w, uint32_t h) {
		vk::ImageCreateInfo image_info;
		image_info.setImageType(vk::ImageType::e2D)
			.setArrayLayers(1)
			.setMipLevels(1)
			.setExtent({ w, h, 1 })
			.setFormat(vk::Format::eR8G8B8A8Srgb)
			.setTiling(vk::ImageTiling::eOptimal)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
			.setSamples(vk::SampleCountFlagBits::e1);
		image = Context::GetInstance().device.createImage(image_info);
	}

	void Texture::allocMemory() {
		auto& device = Context::GetInstance().device;
		vk::MemoryAllocateInfo alloc_info;
		auto requirements = device.getImageMemoryRequirements(image);
		alloc_info.setAllocationSize(requirements.size);

		auto index = QueryBufferMemTypeIndex(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
		alloc_info.setMemoryTypeIndex(index);

		memory = device.allocateMemory(alloc_info);
		
	}

	void Texture::createImageView() {
		vk::ImageViewCreateInfo createInfo;
		vk::ComponentMapping mapping;
		vk::ImageSubresourceRange range;
		range.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseArrayLayer(0)
			.setLayerCount(1)
			.setLevelCount(1)
			.setBaseMipLevel(0);
		createInfo.setImage(image)
			.setViewType(vk::ImageViewType::e2D)
			.setComponents(mapping)
			.setFormat(vk::Format::eR8G8B8A8Srgb)
			.setSubresourceRange(range);
		imageView = Context::GetInstance().device.createImageView(createInfo);
	}

	//uint32_t Texture::queryImageMemoryIndex() {
	//
	//}


	void Texture::transitionImageLayoutFromUndefine2Dst() {
		auto& ctx = Context::GetInstance();
		ctx.commandManager->ExecuteCmd(ctx.graphics_queue,
			[&](vk::CommandBuffer cmdBuf) {
				vk::ImageMemoryBarrier barrier;
				vk::ImageSubresourceRange range;
				range.setLayerCount(1)
					.setBaseArrayLayer(0)
					.setLevelCount(1)
					.setBaseMipLevel(0)
					.setAspectMask(vk::ImageAspectFlagBits::eColor);
				barrier.setImage(image)
					.setOldLayout(vk::ImageLayout::eUndefined)
					.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstAccessMask((vk::AccessFlagBits::eTransferWrite))
					.setSubresourceRange(range);
				cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
					{}, {}, nullptr, barrier);
			});
	}


	void Texture::transitionImageLayoutFromDst2Optimal() {
		auto& ctx = Context::GetInstance();
		ctx.commandManager->ExecuteCmd(ctx.graphics_queue,
			[&](vk::CommandBuffer cmdBuf) {
				vk::ImageMemoryBarrier barrier;
				vk::ImageSubresourceRange range;
				range.setLayerCount(1)
					.setBaseArrayLayer(0)
					.setLevelCount(1)
					.setBaseMipLevel(0)
					.setAspectMask(vk::ImageAspectFlagBits::eColor);
				barrier.setImage(image)
					.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
					.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setSrcAccessMask((vk::AccessFlagBits::eTransferWrite))
					.setDstAccessMask((vk::AccessFlagBits::eShaderRead))
					.setSubresourceRange(range);
				cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
					{}, {}, nullptr, barrier);
			});
	}
	void Texture::transformData2Image(Buffer& buffer, uint32_t w, uint32_t h) {
		auto& ctx = Context::GetInstance();
		ctx.commandManager->ExecuteCmd(ctx.graphics_queue,
			[&](vk::CommandBuffer cmdBuf) {
				vk::BufferImageCopy region;
				vk::ImageSubresourceLayers subsource;
				subsource.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseArrayLayer(0)
					.setMipLevel(0)
					.setLayerCount(1);
				region.setBufferImageHeight(0)
					.setBufferOffset(0)
					.setImageOffset(0)
					.setImageExtent({ w, h, 1 })
					.setBufferRowLength(0)
					.setImageSubresource(subsource);
				cmdBuf.copyBufferToImage(buffer.buffer, image,
					vk::ImageLayout::eTransferDstOptimal,
					region);
			});
	}

	void Texture::updateDescriptorSet() {
		vk::WriteDescriptorSet writer;
		vk::DescriptorImageInfo imageInfo;
		//vk::DescriptorBufferInfo bufferinfo;
		imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(imageView)
			.setSampler(Context::GetInstance().sampler);
		writer.setImageInfo(imageInfo)
			.setDstBinding(0)
			.setDstArrayElement(0)
			.setDstSet(set.set)
			//.setBufferInfo(bufferinfo)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
		Context::GetInstance().device.updateDescriptorSets(writer, {});
	}


	std::unique_ptr<TextureManager> TextureManager::instance_ = nullptr;

	Texture* TextureManager::Load(const std::string& filename) {
		datas.push_back(std::unique_ptr<Texture>(new Texture(filename)));
		return datas.back().get();
	}

	void TextureManager::Clear() {
		datas.clear();
	}

	void TextureManager::Destroy(Texture* texture) {
		auto it = std::find_if(datas.begin(), datas.end(),
			[&](const std::unique_ptr<Texture>& t) {
				return t.get() == texture;
			});
		if (it != datas.end()) {
			Context::GetInstance().device.waitIdle();
			datas.erase(it);
			return;
		}
	}

}