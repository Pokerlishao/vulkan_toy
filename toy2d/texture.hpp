#pragma once

#include "toy2d/buffer.hpp"
#include "vulkan/vulkan.hpp"

namespace toy2d {
	class Texture final {
	public:
		Texture(std::string_view filename);
		~Texture();

		vk::Image image;
		vk::ImageView imageView;
		vk::DeviceMemory memory;

	private:
		void createImage(uint32_t w, uint32_t h);
		void allocMemory();
		void createImageView();
		//uint32_t queryImageMemoryIndex();
		void transitionImageLayoutFromUndefine2Dst();
		void transitionImageLayoutFromDst2Optimal();
		void transformData2Image(Buffer& buffer, uint32_t w, uint32_t h);



	};


}