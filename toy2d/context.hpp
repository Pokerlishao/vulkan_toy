#pragma once

#include "vulkan/vulkan.hpp"
#include "tool.hpp"
#include "toy2d/swapchain.hpp"
#include "toy2d/render_process.hpp"
#include "toy2d/renderer.hpp"
#include "toy2d/CommandManager.hpp"
#include <memory>
#include <iostream>
#include <assert.h>
#include <optional>

namespace toy2d {
	class Context final {
	public:
		static void Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func);
		static void Quit();
		static Context& GetInstance();
		~Context();


		struct QueueFamilyIndices final {
			std::optional<uint32_t> graphQueue;
			std::optional<uint32_t> presentQueue;
			operator bool() const {
				return graphQueue.has_value() && presentQueue.has_value();
			}
		};



		vk::Instance instance;
		vk::PhysicalDevice physicaldevice; //物理设备
		vk::Device device;	//逻辑设备
		vk::Queue graphics_queue;
		vk::Queue present_queue;
		vk::SurfaceKHR surface;
		std::unique_ptr<Swapchain> swapchain;
		std::unique_ptr<RenderProcess> renderProcess;
		std::unique_ptr<Renderer> renderer;
		std::unique_ptr<CommandManager> commandManager;
		QueueFamilyIndices queueInfo;

		void InitSwapchain(int W, int H);
		void InitRenderProcess();
		void InitGraphicsPipeline();
		void InitCommandPool();



		void InitRenderer() {
			renderer.reset(new Renderer());
		}



	private:
		//static std::unique_ptr<Context> instance_;
		static Context* instance_;
		Context(const std::vector<const char*>& extensions, CreateSurfaceFunc func);
		void createInstance(const std::vector<const char*>& extensions, CreateSurfaceFunc func);
		void pickupPhysicalDevice();
		void createDevice();
		void getQueues();

		void queryQueueInfo();

	};
}