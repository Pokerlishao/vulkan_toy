#pragma once

#include "vulkan/vulkan.hpp"
#include "context.hpp"

namespace toy2d {

	void Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func, int W, int H);
	void Quit();
	Texture* LoadTexture(const std::string& filename);
	void DestroyTexture(Texture*);
	Renderer* GetRenderer();


}