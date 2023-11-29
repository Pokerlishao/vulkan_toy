#include "toy2d/context.hpp"
#include "vulkan/vulkan.hpp"
#include "toy2d/CommandManager.hpp"
#include <vector>
#include <memory>

namespace toy2d {

//std::unique_ptr<Context> Context::instance_ = nullptr;
Context* Context::instance_ = nullptr;


void Context::Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func) {
    //instance_.reset(new Context(extensions, func));
    instance_ = new Context(extensions, func);
}

void Context::Quit() {

    //instance_.reset(); // to do debug
    delete instance_;
}

Context& Context::GetInstance() {
    assert(instance_);
    return *instance_;
}

void Context::InitSwapchain(int W, int H) {
    swapchain = std::make_unique<Swapchain>(W, H);
}

void Context::InitRenderProcess() {
    renderProcess = std::make_unique<RenderProcess>();
}

void Context::InitGraphicsPipeline() {
    renderProcess->recreateGraphicsPipeline();
}

void Context::InitCommandPool() {
    commandManager = std::make_unique<CommandManager>();
}

Context::Context(const std::vector<const char*>& extensions, CreateSurfaceFunc func) {
    createInstance(extensions, func);
    pickupPhysicalDevice();
    surface = func(instance);
    queryQueueInfo();
    createDevice();
    getQueues();
}

Context::~Context() {
    if (instance_ == nullptr) {
        std::cout << "ptr is null" << std::endl;  // 这里会输出 "ptr is null"
    }
    commandManager.reset();
    renderProcess.reset();
    swapchain.reset();
    instance.destroySurfaceKHR(surface);


    device.destroy();
    instance.destroy();
}

void Context::createInstance(const std::vector<const char*>& extensions, CreateSurfaceFunc func) {
    vk::InstanceCreateInfo createInfo;
    vk::ApplicationInfo appInfo;

    // validation layers
    std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };
    createInfo.setPEnabledLayerNames(layers)
        .setPEnabledExtensionNames(extensions);

    appInfo.setApiVersion(VK_VERSION_1_3)
            .setPEngineName("SDL");
    createInfo.setPApplicationInfo(&appInfo);

    //for (auto& extension : vk::enumerateInstanceExtensionProperties()) {
    //    std::cout << extension.extensionName << std::endl;
    //}
    

    //auto vlayers = vk::enumerateInstanceLayerProperties();
    //for (auto& layer : vlayers) {
    //    std::cout << layer.layerName << std::endl;
    //}

    RemoveNosupportedElems<const char*, vk::LayerProperties>(layers, vk::enumerateInstanceLayerProperties(),
                           [](const char* e1, const vk::LayerProperties& e2) {
                                return std::strcmp(e1, e2.layerName) == 0;
                           });
    instance = vk::createInstance(createInfo);
}

void Context::pickupPhysicalDevice() {
    auto devices = instance.enumeratePhysicalDevices();
    physicaldevice = devices[0];
}

void Context::createDevice() {
    std::array extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    vk::DeviceCreateInfo createinfo;
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    float priorities = 1.0;
    if (queueInfo.presentQueue.value() == queueInfo.graphQueue.value()) {
        vk::DeviceQueueCreateInfo queue_create_info;
        queue_create_info.setPQueuePriorities(&priorities)
            .setQueueCount(1)
            .setQueueFamilyIndex(queueInfo.graphQueue.value());
        queue_create_infos.push_back(std::move(queue_create_info) );
    }
    else {
        vk::DeviceQueueCreateInfo queue_create_info;
        queue_create_info.setPQueuePriorities(&priorities)
            .setQueueCount(1)
            .setQueueFamilyIndex(queueInfo.graphQueue.value());
        queue_create_infos.push_back(queue_create_info);
        queue_create_info.setPQueuePriorities(&priorities)
            .setQueueCount(1)
            .setQueueFamilyIndex(queueInfo.presentQueue.value());
        queue_create_infos.push_back(queue_create_info);
    }
    createinfo.setQueueCreateInfos(queue_create_infos)
        .setPEnabledExtensionNames(extensions);
    device = physicaldevice.createDevice(createinfo);
}

void Context::queryQueueInfo() {
    auto properties = physicaldevice.getQueueFamilyProperties();
    //for (auto &prop: properties) {

    for(int i=0;i<properties.size();i++){
        const auto& prop = properties[i];
        if (prop.queueFlags | vk::QueueFlagBits::eGraphics) {
            queueInfo.graphQueue = i;
        }

        if (physicaldevice.getSurfaceSupportKHR(i, surface)) {
            queueInfo.presentQueue = i;
        }

        if (queueInfo) break;
    }
}


void Context::getQueues() {
    graphics_queue =  device.getQueue(queueInfo.graphQueue.value(), 0);
    present_queue = device.getQueue(queueInfo.presentQueue.value(), 0);
}



}
