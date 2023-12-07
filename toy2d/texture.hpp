#pragma once

#include "toy2d/buffer.hpp"
#include "toy2d/descriptor_manager.hpp"
#include "vulkan/vulkan.hpp"

namespace toy2d {
	class TextureManager;

	class Texture final {
	public:
		friend class TextureManager;
		Texture(std::string_view filename);
		~Texture();

		vk::Image image;
		vk::ImageView imageView;
		vk::DeviceMemory memory;
		DescriptorSetManager::SetInfo set;

	private:
		void createImage(uint32_t w, uint32_t h);
		void allocMemory();
		void createImageView();
		//uint32_t queryImageMemoryIndex();
		void transitionImageLayoutFromUndefine2Dst();
		void transitionImageLayoutFromDst2Optimal();
		void transformData2Image(Buffer& buffer, uint32_t w, uint32_t h);
		void updateDescriptorSet();

	};

	class TextureManager final {
	public:
		static TextureManager& Instance() {
			if (!instance_) {
				instance_.reset(new TextureManager);
			}
			return *instance_;
		}

		Texture* Load(const std::string& filename);
		void Destroy(Texture*);
		void Clear();

	private:
		static std::unique_ptr<TextureManager> instance_;

		std::vector<std::unique_ptr<Texture>> datas;
	};

}