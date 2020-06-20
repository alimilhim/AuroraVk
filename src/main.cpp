

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <vector>
#define _DEBUG
#define VK_CHECK(call)        \
  do {                        \
    VkResult _r = call;       \
    assert(_r == VK_SUCCESS); \
  } while (0)

void keyCallBack(GLFWwindow* win, int key, int scancode, int actions,
                 int mods) {
  if (key == GLFW_KEY_ESCAPE && actions == GLFW_PRESS) {
    glfwSetWindowShouldClose(win, GLFW_TRUE);
  }
}

std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",

};

std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamilyIndex;
  std::optional<uint32_t> presentFamilyIndex;

  bool isReady() {
    return graphicsFamilyIndex.has_value() && presentFamilyIndex.has_value();
  }
};
struct PhysicalDeviceInfo {
  QueueFamilyIndices queuefamilyindices;
  VkPhysicalDevice phyDevice;
};


GLFWwindow* win;
bool is_resized = false;
VkInstance instance = 0;

VkSwapchainKHR swapChain;
PhysicalDeviceInfo deviceInfo;
VkDevice logicalDevice;
VkSurfaceKHR surface;
VkQueue graphicsQueue;
VkQueue presentQueue;
std::vector<VkImage> swapChainImages;
std::vector<VkImageView> swapChainImageViews;
std::vector<VkFramebuffer> swapChainFramebuffers;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;
VkCommandPool commandPool;
std::vector<VkCommandBuffer> commandBuffers;

size_t currentFrame = 0;
const int MAX_FRAMES_IN_FLIGHT = 2;
std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinshedSemaphores;

std::vector<VkFence> inFlightFences; 
std::vector<VkFence> imagesInFlight;

bool checkValidationLayerSupport() {
  uint32_t layerCount;

  vkEnumerateInstanceLayerProperties(&layerCount, 0);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
  for (const char* layerName : validationLayers) {
    bool layerFound = false;

    for (const auto& layerProprties : availableLayers) {
      if (strcmp(layerName, layerProprties.layerName) == 0) {
        return true;
      }
    }
  }

  return false;
}

std::vector<const char*> getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;

  const char** glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<const char*> extensions(glfwExtensions,
                                      glfwExtensions + glfwExtensionCount);
#ifdef _DEBUG
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData) {
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}

void fillDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
  createInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

void initInstance(VkInstance& instance,
                  VkDebugUtilsMessengerEXT& debugMessenger) {
  VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.apiVersion = VK_API_VERSION_1_1;

  VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  createInfo.pApplicationInfo = &appInfo;
#ifdef _DEBUG

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
  if (checkValidationLayerSupport()) {
    createInfo.ppEnabledLayerNames = validationLayers.data();
    createInfo.enabledLayerCount = validationLayers.size();
    fillDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
  }
#endif

  auto extensions = getRequiredExtensions();
  createInfo.ppEnabledExtensionNames = extensions.data();
  createInfo.enabledExtensionCount = extensions.size();

  VK_CHECK(vkCreateInstance(&createInfo, 0, &instance));

#ifdef _DEBUG
  VK_CHECK(CreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, 0,
                                        &debugMessenger));
#endif  // _DEBUG
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);
  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                           deviceExtensions.end());

  for (const auto& extension : availableExtensions) {
    // printf("deviceExtenionName: %s\n ", extension.extensionName);
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

QueueFamilyIndices getPhysicalDeviceQueueFamilies(VkPhysicalDevice device,
                                                  VkSurfaceKHR surface) {
  uint32_t queueFamilyCount = 0;
  QueueFamilyIndices indices;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, 0);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());
  uint32_t i = 0;
  for (const auto& queueFamily : queueFamilies) {
    VkBool32 presentIsSUpported = false;

    if (queueFamily.queueCount > 0 &&
        queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamilyIndex = i;
    }
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                         &presentIsSUpported);
    if (queueFamily.queueCount > 0 && presentIsSUpported) {
      indices.presentFamilyIndex = i;
    }

    if (indices.isReady()) {
      return indices;
    }

    i++;
  }
  return indices;
}

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface) {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
  assert(formatCount);

  details.formats.resize(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                       details.formats.data());

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            nullptr);
  assert(presentModeCount);
  details.presentModes.resize(presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            details.presentModes.data());

  return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
      return availablePresentMode;
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width != UINT32_MAX)
    return capabilities.currentExtent;
  else {
      int w, h;
      glfwGetFramebufferSize(win, &w, &h);
      return { (uint32_t)w,(uint32_t)h };

  }
  assert(0);
}

PhysicalDeviceInfo pickPhysicalDevice(VkInstance& instance,
                                      VkSurfaceKHR surface) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, 0);
  assert(deviceCount);
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  int i = 0;
  PhysicalDeviceInfo chosen;
  for (const auto& device : devices) {
    VkPhysicalDeviceProperties deviceprop;
    vkGetPhysicalDeviceProperties(device, &deviceprop);
    printf("GPU[%d]:%s Vram: %dmb\n", i, deviceprop.deviceName,
           deviceprop.limits.maxMemoryAllocationCount);
    QueueFamilyIndices queueIndices =
        getPhysicalDeviceQueueFamilies(device, surface);
    bool isDeviceextAvailable = checkDeviceExtensionSupport(device);
    SwapChainSupportDetails swapChainDetails =
        querySwapChainSupport(device, surface);
    bool swapChainIsAdequate = !swapChainDetails.formats.empty() &&
                               !swapChainDetails.presentModes.empty();
    if (queueIndices.isReady() && isDeviceextAvailable && swapChainIsAdequate) {
      chosen.phyDevice = device;
      chosen.queuefamilyindices = queueIndices;
    }
    i++;
  }

  VkPhysicalDeviceProperties deviceprop;
  vkGetPhysicalDeviceProperties(chosen.phyDevice, &deviceprop);
  printf("chosen device: %s\n", deviceprop.deviceName);
  return chosen;
}

void createLogicalDeviceAndQueueFamilies(VkInstance instance,
                                         PhysicalDeviceInfo& phydeviceInfo,
                                         VkDevice& device) {
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
      phydeviceInfo.queuefamilyindices.graphicsFamilyIndex.value(),
      phydeviceInfo.queuefamilyindices.presentFamilyIndex.value()};
  float queuePriority = 1.0f;

  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueCreateInfo.queueFamilyIndex =
        phydeviceInfo.queuefamilyindices.graphicsFamilyIndex.value();
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures = {};

  VkDeviceCreateInfo deviceInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceInfo.queueCreateInfoCount = queueCreateInfos.size();
  deviceInfo.pEnabledFeatures = &deviceFeatures;
  deviceInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
  VK_CHECK(vkCreateDevice(phydeviceInfo.phyDevice, &deviceInfo, 0, &device));
}

void createSurface(const VkInstance& instance, GLFWwindow* window,
                   VkSurfaceKHR& surface) {
  VK_CHECK(glfwCreateWindowSurface(instance, window, 0, &surface));
}

void createImageViews() {
  swapChainImageViews.resize(swapChainImages.size());
  for (size_t i = 0; i < swapChainImages.size(); i++) {
    VkImageViewCreateInfo createInfo = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.image = swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapChainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    VK_CHECK(vkCreateImageView(logicalDevice, &createInfo, nullptr,
                               &swapChainImageViews[i]));
  }
}

void createSwapChain(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
  SwapChainSupportDetails swapChainSupport =
      querySwapChainSupport(physicalDevice, surface);
  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode =
      chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  QueueFamilyIndices indices =
      getPhysicalDeviceQueueFamilies(physicalDevice, surface);

  uint32_t queueFamilyIndices[] = {indices.graphicsFamilyIndex.value(),
                                   indices.presentFamilyIndex.value()};
  if (indices.graphicsFamilyIndex != indices.presentFamilyIndex) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;
  VK_CHECK(
      vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain));

  VK_CHECK(
      vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr));
  assert(imageCount);
  swapChainImages.resize(imageCount);
  VK_CHECK(vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount,
                                   swapChainImages.data()));

  swapChainExtent = extent;
  swapChainImageFormat = surfaceFormat.format;
}

static std::vector<char> readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    printf("failed to open file:%s \n", filename.c_str());
    assert(0);
  }
  size_t fsize = file.tellg();
  std::vector<char> buffer(fsize);
  file.seekg(0);
  file.read(buffer.data(), fsize);
  file.close();
  return buffer;
}

VkShaderModule createShaderModule(const std::vector<char>& code) {
  VkShaderModuleCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
  VkShaderModule shaderModule;
  VK_CHECK(
      vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule));
  return shaderModule;
}

void createGraphicsPipeline() {
  auto vertShaderCode = readFile("shaders/vert.spv");
  auto fragShaderCode = readFile("shaders/frag.spv");

  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr;
  vertexInputInfo.pVertexBindingDescriptions = nullptr;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = swapChainExtent.width;
  viewport.height = swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;

  VkPipelineViewportStateCreateInfo viewportState = {
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewportState.pScissors = &scissor;
  viewportState.pViewports = &viewport;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer = {
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0;

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending = {
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_LINE_WIDTH};

  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = 2;
  dynamicState.pDynamicStates = dynamicStates;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  VK_CHECK(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr,
                                  &pipelineLayout));

  VkGraphicsPipelineCreateInfo pipelineInfo = {
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr;
  pipelineInfo.pDynamicState = nullptr;
  pipelineInfo.pColorBlendState = &colorBlending;

  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  VK_CHECK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1,
                                     &pipelineInfo, nullptr,
                                     &graphicsPipeline));

  vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
  vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void createRenderPass() {
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;




  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;

  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


  VkRenderPassCreateInfo renderPassInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;
  VK_CHECK(
      vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass));
}

void createFramebuffers() {
  swapChainFramebuffers.resize(swapChainImageViews.size());
  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    VkImageView attachments[] = {swapChainImageViews[i]};
    VkFramebufferCreateInfo framebufferInfo = {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    VK_CHECK(vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr,
                                 &swapChainFramebuffers[i]));
  }
}

void createCommandPool() {
  QueueFamilyIndices queueFamilyIndices =
      getPhysicalDeviceQueueFamilies(deviceInfo.phyDevice, surface);

  VkCommandPoolCreateInfo poolInfo = {
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamilyIndex.value();

  VK_CHECK(
      vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool));
}

void createCommandBuffers() {
  commandBuffers.resize(swapChainFramebuffers.size());
  VkCommandBufferAllocateInfo allocInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
  VK_CHECK(vkAllocateCommandBuffers(logicalDevice, &allocInfo,
                                    commandBuffers.data()));

  for (size_t i = 0; i < commandBuffers.size(); i++) {
    VkCommandBufferBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr;
	
    VK_CHECK(vkBeginCommandBuffer(commandBuffers[i], &beginInfo));

    VkRenderPassBeginInfo renderPassInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[i];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;
    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphicsPipeline);
    vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffers[i]);
    VK_CHECK(vkEndCommandBuffer(commandBuffers[i]));
  }
}



void createSyncObjects() {
    
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinshedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainImages.size(),VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreInfo = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    
  VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      VK_CHECK(vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr,
          &imageAvailableSemaphores[i]));
      VK_CHECK(vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr,
          &renderFinshedSemaphores[i]));
      VK_CHECK(vkCreateFence(logicalDevice, &fenceInfo, nullptr,
          &inFlightFences[i]));
  }
}


void cleanupSwapChain() {

    vkFreeCommandBuffers(logicalDevice, commandPool, commandBuffers.size(),commandBuffers.data());

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
    }
    vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
    vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(logicalDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
}


void recreateSwapChain() {
    
    // to handle minimization
    int width = 0, height = 0;
    glfwGetFramebufferSize(win, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(win ,&width,&height);
        glfwPollEvents();
    }
    
    
    vkDeviceWaitIdle(logicalDevice);

    cleanupSwapChain();
    createSwapChain(deviceInfo.phyDevice, surface);
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers();

}
static int l = 0;
void drawFrame() {



    VkResult fence_state =vkGetFenceStatus(logicalDevice, inFlightFences[currentFrame]);
    if (fence_state == VK_NOT_READY)
    std::cout << fence_state << std::endl;
      VK_CHECK(vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame],VK_FALSE, UINT64_MAX));

	uint32_t imageIndex;
VkResult img_result = 	vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX,
		imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    
    if (img_result == VK_ERROR_OUT_OF_DATE_KHR) {
        std::cout << "recreating SwapChain "<< "\n";
        recreateSwapChain();
        return;
    }

    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    VK_CHECK(vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]));


	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { renderFinshedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]));

	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {swapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

    VkResult queue_result = (vkQueuePresentKHR(presentQueue, &presentInfo));


    if (is_resized || queue_result==VK_SUBOPTIMAL_KHR || queue_result ==VK_ERROR_OUT_OF_DATE_KHR) {
        std::cout << "is resized!" <<"\n";
        is_resized = false;
        recreateSwapChain();
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}


static void framebufferResizeCallback(GLFWwindow* window , int width , int hegiht) {


    is_resized = true;
}


int main() {
	int rc = glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	assert(rc == GLFW_TRUE);
    win = glfwCreateWindow(1024, 768, "vulkan!", NULL, NULL);

    glfwSetFramebufferSizeCallback(win, framebufferResizeCallback);

	assert(win);
	
	VkDebugUtilsMessengerEXT debugMessenger;
	initInstance(instance, debugMessenger);

	createSurface(instance, win, surface);
	deviceInfo = pickPhysicalDevice(instance, surface);

	createLogicalDeviceAndQueueFamilies(instance, deviceInfo, logicalDevice);

	vkGetDeviceQueue(logicalDevice,
		deviceInfo.queuefamilyindices.graphicsFamilyIndex.value(), 0,
		&graphicsQueue);
	vkGetDeviceQueue(logicalDevice,
		deviceInfo.queuefamilyindices.presentFamilyIndex.value(), 0,
		&presentQueue);

	VkPhysicalDeviceProperties dp = {};

	vkGetPhysicalDeviceProperties(deviceInfo.phyDevice, &dp);
	printf("vulkan api version:%d\n", dp.apiVersion);
  
  createSwapChain(deviceInfo.phyDevice, surface);
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandPool();
  createCommandBuffers();
  createSyncObjects();
  glfwSetKeyCallback(win, keyCallBack);

  while (!glfwWindowShouldClose(win)) {
    glfwPollEvents();
    drawFrame();
  }

  vkDeviceWaitIdle(logicalDevice);
  // clean up

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(logicalDevice, renderFinshedSemaphores[i], nullptr);
      vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
      vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
  }
  
  vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

  for (auto framebuffer : swapChainFramebuffers) {
    vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
  }
  vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
  vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

  for (auto imageView : swapChainImageViews) {
    vkDestroyImageView(logicalDevice, imageView, nullptr);
  }
  vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
  vkDestroySurfaceKHR(instance, surface, 0);
  vkDestroyDevice(logicalDevice, 0);
  DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  vkDestroyInstance(instance, 0);

  glfwDestroyWindow(win);
  glfwTerminate();

  system("pause");
  return 0;
}