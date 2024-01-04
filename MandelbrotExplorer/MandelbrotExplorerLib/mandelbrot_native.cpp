#include "pch.h"
#include "mandelbrot_native.h"
#include <set>
#include <iterator>

vulkan_renderer::vulkan_renderer(HINSTANCE hinstance, HWND hwnd, bool debug)
{
	try
	{
		setup(hinstance, hwnd, debug);
	}
	catch (const std::runtime_error& err)
	{
		cleanup();
		throw;
	}
}

vulkan_renderer::~vulkan_renderer()
{
	dispose();
}

void vulkan_renderer::dispose()
{
	if (!_disposed)
	{
		vkDeviceWaitIdle(_logicalDevice);
		cleanup();
		_disposed = true;
	}
}

void vulkan_renderer::setup(HINSTANCE hinstance, HWND hwnd, bool debug)
{
	if (debug) 
		create_vkinstance_with_debugging();
	else 
		create_vkinstance();

	create_surface(hwnd, hinstance);
	select_physical_device();
	create_logical_device();
	choose_swap_surface_format();
	choose_swap_present_mode();
	choose_swap_extent();
	create_swap_chain();
	create_image_views();
	create_vertex_shader();
	create_fragment_shader();
	create_render_pass();
	create_framebuffers();			// swapchain framebuffers depend on the render pass.
	create_graphics_pipeline();
	create_command_pool();
	create_vertex_buffer();	
	create_index_buffer();
	create_command_buffer();
	create_sync_objects();
}

void vulkan_renderer::cleanup_pipeline()
{
	if (_graphicsPipeline != nullptr)
		vkDestroyPipeline(_logicalDevice, _graphicsPipeline, nullptr);

	if (_pipelineLayout != nullptr)
		vkDestroyPipelineLayout(_logicalDevice, _pipelineLayout, nullptr);
}

void vulkan_renderer::cleanup_swap_chain()
{
	for (auto framebuffer : _swapChainFramebuffers) {
		vkDestroyFramebuffer(_logicalDevice, framebuffer, nullptr);
	}

	for (auto imageView : _swapChainImageViews) {
		vkDestroyImageView(_logicalDevice, imageView, nullptr);
	}

	if (_swapChain != nullptr)
		vkDestroySwapchainKHR(_logicalDevice, _swapChain, nullptr);
}


void vulkan_renderer::cleanup()
{
	vkDestroySemaphore(_logicalDevice, _renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(_logicalDevice, _imageAvailableSemaphore, nullptr);

	if (_indexBuffer != nullptr)
		vkDestroyBuffer(_logicalDevice, _indexBuffer, nullptr);

	if (_indexBufferMemory != nullptr)
		vkFreeMemory(_logicalDevice, _indexBufferMemory, nullptr);

	if (_vertexBuffer != nullptr)
		vkDestroyBuffer(_logicalDevice, _vertexBuffer, nullptr);

	if (_vertexBufferMemory != nullptr)
		vkFreeMemory(_logicalDevice, _vertexBufferMemory, nullptr);

	if (_commandPool != nullptr)
		vkDestroyCommandPool(_logicalDevice, _commandPool, nullptr);

	cleanup_pipeline();

	if (_renderPass != nullptr)
		vkDestroyRenderPass(_logicalDevice, _renderPass, nullptr);

	cleanup_swap_chain();

	if (_fragmentShader != nullptr)
		vkDestroyShaderModule(_logicalDevice, _fragmentShader, nullptr);

	if (_vertexShader != nullptr)
		vkDestroyShaderModule(_logicalDevice, _vertexShader, nullptr);

	if (_logicalDevice != nullptr)
		vkDestroyDevice(_logicalDevice, nullptr);

	if (_surface != nullptr)
		vkDestroySurfaceKHR(_vkinstance, _surface, nullptr);

	if (_debugMessenger != nullptr)
	{
		auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(_vkinstance, "vkDestroyDebugUtilsMessengerEXT");

		if (vkDestroyDebugUtilsMessengerEXT != nullptr)
			vkDestroyDebugUtilsMessengerEXT(_vkinstance, _debugMessenger, nullptr);
	}

	if (_vkinstance != nullptr)
		vkDestroyInstance(_vkinstance, nullptr);
}

void vulkan_renderer::create_vkinstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Mandelbrot Explorer Native Renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	std::vector<const char*> requiredExtensions{
		"VK_KHR_surface",
		"VK_KHR_win32_surface"
	};

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = (uint32_t)requiredExtensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();
	instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.ppEnabledLayerNames = nullptr;
	instanceCreateInfo.pNext = nullptr;

	if (vkCreateInstance(&instanceCreateInfo, nullptr, &_vkinstance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create vulkan instance.");
	}
}

void vulkan_renderer::create_vkinstance_with_debugging()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Mandelbrot Explorer Native Renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	std::vector<const char*> requiredExtensions{
		"VK_KHR_surface",
		"VK_KHR_win32_surface",
		"VK_EXT_debug_utils"
	};

	std::vector<const char*> requiredLayers{
		"VK_LAYER_KHRONOS_validation"
	};

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
	debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	
	// Subscribe to all debug messages. The UI can filter them by type & severity.
	// Maybe not the most performant approach.

	debugMessengerCreateInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT   ;

	debugMessengerCreateInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT                |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT            |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT             |
		VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT ;

	debugMessengerCreateInfo.pfnUserCallback = vulkan_renderer::debug_callback;
	debugMessengerCreateInfo.pUserData = this;

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = (uint32_t)requiredExtensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();
	instanceCreateInfo.enabledLayerCount = (uint32_t)requiredLayers.size();
	instanceCreateInfo.ppEnabledLayerNames = requiredLayers.data();
	instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreateInfo;

	if (vkCreateInstance(&instanceCreateInfo, nullptr, &_vkinstance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create vulkan instance.");
	}

	auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(_vkinstance, "vkCreateDebugUtilsMessengerEXT");

	if (vkCreateDebugUtilsMessengerEXT == nullptr) {
		throw std::runtime_error("Failed to load vkCreateDebugUtilsMessengerEXT.");
	}

	if (vkCreateDebugUtilsMessengerEXT(_vkinstance, &debugMessengerCreateInfo, nullptr, &_debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("Failed to setup vulkan instance debug messenger.");
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_renderer::debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	debug_message message;
	message.text = std::string(pCallbackData->pMessage);
	message.severity = messageSeverity;
	message.type = messageType;

	vulkan_renderer* self = static_cast<vulkan_renderer*>(pUserData);
	self->_messages.push_back(message);

	return VK_FALSE;
}

void vulkan_renderer::create_surface(HWND hwnd, HINSTANCE hinstance)
{
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = hwnd;
	createInfo.hinstance = hinstance;

	if (vkCreateWin32SurfaceKHR(_vkinstance, &createInfo, nullptr, &_surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface!");
	}
}

void vulkan_renderer::select_physical_device()
{
	uint32_t deviceCount = 0;

	vkEnumeratePhysicalDevices(_vkinstance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(_vkinstance, &deviceCount, devices.data());

	// For each physical device on the system.
	for (VkPhysicalDevice device : devices)
	{
		// Get this device's properties.
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);

		// Find this device's graphics & present queues.
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		VkBool32 hasGraphicsQueue = false;
		uint32_t graphicsIndex = UINT32_MAX;

		VkBool32 hasPresentQueue = false;
		uint32_t presentIndex = UINT32_MAX;

		for (uint32_t i = 0; i < queueFamilyCount; i++)
		{
			if (queueFamilies[i].queueFlags && VK_QUEUE_GRAPHICS_BIT)
			{
				hasGraphicsQueue = true;
				graphicsIndex = i;
				break;
			}
		}

		for (uint32_t i = 0; i < queueFamilyCount; i++)
		{
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &hasPresentQueue);

			if (hasPresentQueue)
			{
				presentIndex = i;
				break;
			}
		}

		if (hasGraphicsQueue && hasPresentQueue)
		{
			// If we've found a suitable discrete GPU, then select it and stop searching.
			// (what happens in a dual-GPU system? Would the secondary GPU have present support?)
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				_physicalDevice = device;
				_graphicsQueueFamilyIndex = graphicsIndex;
				_presentQueueFamilyIndex = presentIndex;
				break;
			}
			
			// If we've found a suitable integrated GPU, then select it but keep searching
			// in case there's a better discrete GPU.
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			{
				_physicalDevice = device;
				_graphicsQueueFamilyIndex = graphicsIndex;
				_presentQueueFamilyIndex = presentIndex;
			}
		}
	}

	if (_physicalDevice == nullptr)
	{
		throw std::runtime_error("No suitable device found.");
	}
}

void vulkan_renderer::create_logical_device()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	std::set<uint32_t> uniqueFamilyIndices = {
		_graphicsQueueFamilyIndex, 
		_presentQueueFamilyIndex
	};

	float queuePriority = 1.0f;

	for (uint32_t familyIndex : uniqueFamilyIndices) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = familyIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.shaderFloat64 = true;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	// The instance level validation layer should catch everything, but tutorial
	// enables device-specific validation layer anyway .
	const std::vector<const char*> validationLayers{
		"VK_LAYER_KHRONOS_validation"
	};

	if (_debugMessenger != nullptr)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device!");
	}

	vkGetDeviceQueue(_logicalDevice, _graphicsQueueFamilyIndex, 0, &_graphicsQueue);
	vkGetDeviceQueue(_logicalDevice, _presentQueueFamilyIndex, 0, &_presentQueue);
}

void vulkan_renderer::choose_swap_surface_format()
{
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, nullptr);

	if (formatCount != 0) 
	{
		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, surfaceFormats.data());

		for (VkSurfaceFormatKHR f : surfaceFormats)
		{
			if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				_selectedSurfaceFormat = f;
				return;
			}
		}

		_selectedSurfaceFormat = surfaceFormats[0];
		return;
	}

	throw std::runtime_error("No surface formats available.");
}

void vulkan_renderer::choose_swap_present_mode()
{
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) 
	{
		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, presentModes.data());		

		for (VkPresentModeKHR mode : presentModes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				_selectedPresentMode = mode;
				return;
			}
		}

		_selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
		return;
	}

	throw std::runtime_error("No present modes available.");
}

void vulkan_renderer::choose_swap_extent()
{
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &capabilities);

	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		_selectedSwapExtent = capabilities.currentExtent;
	}
	else
	{
		throw std::runtime_error("Invalid extent.");
	}
}

void vulkan_renderer::recreate_swap_chain()
{
	do
	{
		choose_swap_extent();

	} while (_selectedSwapExtent.width == 0 || _selectedSwapExtent.height == 0);

	vkDeviceWaitIdle(_logicalDevice);

	cleanup_swap_chain();
	create_swap_chain();
	create_image_views();
	create_framebuffers();
}

void vulkan_renderer::create_swap_chain()
{
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &capabilities);

	// We need to decide how many images we would like to have in the swap chain.
	// The implementation specifies the minimum number that it requires to function.

	// Sticking to the minimum means that we may sometimes have to wait 
	// on the driver to complete internal operations before we can acquire 
	// another image to render to. Therefore, it is recommended to request
	// at least one more image than the minimum.

	// Make sure that this +1 doesn't exceed the allowed maximum.
	// (0 is a special value that means no maximum).

	uint32_t imageCount = capabilities.minImageCount + 1;

	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
	{
		imageCount = capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = _surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = _selectedSurfaceFormat.format;
	createInfo.imageColorSpace = _selectedSurfaceFormat.colorSpace;
	createInfo.imageExtent = _selectedSwapExtent;

	// Each swap chain image can have many layers to it?
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[]{ _graphicsQueueFamilyIndex, _presentQueueFamilyIndex };

	if (_graphicsQueueFamilyIndex != _presentQueueFamilyIndex)
	{
		// If the graphics family and the present family aren't the same,
		// make it so the swap chain images can be shared across queue families.
		// Apparently, this option is less performant.
		// Later sections of the tutorial show how to explicitly do
		// transfers from the graphics family to the present family.

		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
		//             ^
		// How does Vulkan know which index is for the graphics family
		// and which index is for the present family?
		// Oh, it doesn't matter - it just needs a list of which
		// queue families need to be able to access this image.
	}
	else
	{
		// Vulkan tutorial says that the present family and the
		// graphics family are the same on most hardware.

		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = _selectedPresentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	// When resizing the window, we'll need to create a new swap chain
	// but then link back to the old one?

	if (vkCreateSwapchainKHR(_logicalDevice, &createInfo, nullptr, &_swapChain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(_logicalDevice, _swapChain, &imageCount, nullptr);
	_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(_logicalDevice, _swapChain, &imageCount, _swapChainImages.data());
}

void vulkan_renderer::create_image_views()
{
	_swapChainImageViews.resize(_swapChainImages.size());

	for (size_t i = 0; i < _swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = _swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = _selectedSurfaceFormat.format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// Our images will be used as color targets without any
		// mipmapping levels or multiple layers.
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		// If we were working on a stereographic 3D application,
		// then you would create a swap chain with multiple layers.
		// You could then create multiple image views for each image
		// representing the views for the left and right eyes
		// by accessing different layers.

		if (vkCreateImageView(_logicalDevice, &createInfo, nullptr, &_swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image views!");
		}
	}
}

VkShaderModule vulkan_renderer::compile_shader(std::string name, std::string source, shaderc_shader_kind kind)
{
	shaderc::Compiler compiler;
	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, kind, name.c_str());

	if (result.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success)
	{
		std::string message;
		message = "Failed to compile shader: " + name + "\n";
		message += result.GetErrorMessage() + "\n";

		throw std::runtime_error(message);
	}

	std::vector<uint32_t> spirv;
	std::copy(result.begin(), result.end(), std::back_inserter(spirv));

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spirv.size() * sizeof(uint32_t);
	createInfo.pCode = spirv.data();

	VkShaderModule shaderModule;

	if (vkCreateShaderModule(_logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error(std::string("Failed to create shader module: ") + name);
	}

	return shaderModule;
}

void vulkan_renderer::create_vertex_shader()
{
	std::string vertexShaderSource =
		"#version 450                                   \n"
		"                                               \n"
		"layout(location = 0) in vec2 inPosition;       \n"
		"layout(location = 1) in vec3 inColor;          \n"
		"                                               \n"
		"layout(location = 0) out vec3 fragColor;       \n"
		"                                               \n"
		"void main()                                    \n"
		"{                                              \n"
		"    gl_Position = vec4(inPosition, 0.0, 1.0);  \n"
		"    fragColor = inColor;                       \n"
		"}                                              \n";


	_vertexShader = compile_shader("default_vertex_shader", vertexShaderSource, shaderc_shader_kind::shaderc_vertex_shader);
}

void vulkan_renderer::create_fragment_shader()
{
	std::string fragmentShaderSource =
		"#version 450                               \n"
		"layout(location = 0) in vec3 fragColor;    \n"
		"layout(location = 0) out vec4 outColor;    \n"
		"                                           \n"
		"void main() {                              \n"
		"                                           \n"
		"    outColor = vec4(fragColor, 1.0f);      \n"
		"}                                          \n";

	_fragmentShader = compile_shader("default_fragment_shader", fragmentShaderSource, shaderc_shader_kind::shaderc_fragment_shader);
}

void vulkan_renderer::create_render_pass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = _selectedSurfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	// Clear the frame buffer to black before drawing a new frame.
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

	// Rendered contents will be stored in memory and can be read later. 
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	// We're not doing anything with stencils. 
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// After rendering, we want to present the swap chain image to the screen.
	// Curious, wouldn't the image need to be in the COLOR_ATTACHMENT_OPTIMAL layout
	// for rendering to it? Perhaps Vulkan already knows that. 
	// When we created the swap chain, we did signifiy 
	// imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT. 

	// Texturing chapter goes more into how layouts work.
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Subpasses and attachment references.
	// A single render pass can consist of multiple subpasses.
	// Subpasses are subsequent rendering operations that depend
	// on the contents of the framebuffers in previous passes.
	// Used mostly for post-processing effects. 

	// If you group these rendering operations into one render pass,
	// then Vulkan is able to reorder the operations to better
	// optimize them.

	// For now, we'll just have a single subpass.

	// Every subpass references one or more of the attachments 
	// created earlier.

	VkAttachmentReference colorAttachmentRef{};

	// Index into the array of VkAttachmentDescription. 
	// Our array consists only of a single attachment, so this index is 0.
	colorAttachmentRef.attachment = 0;

	// The layout we want the attachment to automatically transition to 
	// when the subpass begins. This is the optimal setting for an 
	// attachment to be used as a color buffer.
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Vulkan may support compute subpasses in the future,
	// so we have to be explicit that this is a graphics subpass.
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	// The index of the attachment in this array is directly 
	// referenced from the fragment shader with the 
	// layout(location = 0) out vec4 outColor directive.
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	// The following other types of attachments can be referenced by a subpass:
	//	pInputAttachments: Attachments that are read from a shader.
	//	pResolveAttachments: Attachments used for multisampling color attachments.
	//	pDepthStencilAttachment: Attachment for depth and stencil data.
	//	pPreserveAttachments: Attachments that are not used by this subpass,
	//						  but for which data must be preserved.

	// === Render Pass ===
	// Now that the attachment and basic subpass referencing it have been described,
	// we can create the render pass itself.

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	// I'm rather confused as to how this dependency works?
	// https://www.reddit.com/r/vulkan/comments/s80reu/subpass_dependencies_what_are_those_and_why_do_i/

	// We only have a single subpass right now, but the operations right before and right
	// after this subpass are also counted as implicit "subpasses".

	// At the start of the render pass, Vulkan will attempt to transition the image layout.
	// Unfortunately, it assumes this will occur at the start of the pipeline.

	// The graphics queue is programmed to wait at the color output stage for the
	// imageAvailable semaphore to signal, before it will run the command buffer.

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;	// implicit "begin" subpass.
	dependency.dstSubpass = 0;						// index of our real subpass. 

	// We need to wait for the swap chain to finish reading from the image 
	// before we can access it. This can be accomplished by waiting on the 
	// color attachment output stage itself (according to the tutorial).

	// According to the comment, dstSubpass will only run the stages in dstStageMask 
	// after srcSubpass has completed the stages in srcStageMask.
	// Does it work the same when srcStageMask is the implicit "begin" subpass rather than a real one?
	//
	// According to ChatGPT, making the srcSubpass VK_SUBPASS_EXTERNAL 
	// allows for operations that must occur before the render pass begins.
	// Thus, wait for the completion of the first color attachment output
	// (after the semaphore will have been signaled, indicating the image is ready).
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	// then run the color attachment output stage again in the real subpass. 
	// Tell Vulkan we need to write to the color attachment on the real subpass.

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(_logicalDevice, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");
}

void vulkan_renderer::recreate_graphics_pipeline()
{
	vkDeviceWaitIdle(_logicalDevice);
	cleanup_pipeline();
	create_graphics_pipeline();
}

void vulkan_renderer::create_graphics_pipeline()
{
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = _vertexShader;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = _fragmentShader;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// The geometry of the vertices. 
	// Whether they're to be interpreted as mere lists of triangles,
	// or as a triangle strip/fan, or as a list of points, or as a 
	// list of lines/line strip for rendering wireframe models.
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Tutorial was changed to use dynamic state for the viewport & scissor.
	// 
	// Viewport: Region of the frame buffer that the output will be rendered to.
	// Will almost always be (0, 0) to (width, height) of the swap chain images.
	// These define the transformation from the image to the frame buffer.
	// 
	// Scissor: Effectively "clips" the frame buffer. Sets the region of pixels 
	// on the frame buffer that will be active, that we can draw to. The rest will 
	// remain blank.

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// Rasterizer. 
	// Takes the geometry shaped by the vertices from the vertex shader
	// and turns it into fragments to be colored by the fragment shader.
	// Performs depth testing, face culling, scissor test, and can be 
	// configured to output fragments that fill the entire polygon 
	// or just the edges (wireframe rendering). 
	//		-> odd. If I wanted wireframes, would I use a different rasterization
	//		   setting or a different input assembly setting?

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;		// FALSE will discard fragments outside of the near & far depth planes.
	rasterizer.rasterizerDiscardEnable = VK_FALSE;	// TRUE will stop geometry from ever passing through the rasterizer, disabling any output to the frame buffer.
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;	// Any other mode requires enabling a GPU feature.
	rasterizer.lineWidth = 1.0f;	// Anything thicker than 1.0 requires enabling a GPU feature.
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;		// Back face culling.
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;		// Depth biasing (and clamping) is sometimes 
	// used for shadow maps. I wonder how that works?

// Multisampling.
// One of the ways to perform anti-aliasing. 
// Works by combining the fragment shader results of multiple polygons 
// that rasterize to the same pixel. This mainly occurs along edges of polygons,
// where aliasing is most noticeable. Because it doesn't need to run the fragment shader
// multiple times if only one polygon maps to a pixel, it is significantly less
// expensive than simply renering to a higher resolution then downscaling.
// Enabling it requires enabling a GPU feature.
// (disabled here - I can't imagine the MandelbrotExplorer will require it).
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


	// Color blending.
	// After a fragment shader has returned a color, it needs to be combined with the color
	// that is already in the framebuffer. Two possibilities:
	//		1) Mix the old and new value to produce a final color
	//		2) Combine the old and new value using a bitwise operation.
	// Two structs:
	//		VkPipelineColorBlendAttachmentState
	//			- contains configuration per attached frame buffer
	//		VkPipelineColorBlendStateCreateInfo
	//			- contains the global color blending settings

	// Curious. What if you only care about the global settings. Are the per-framebuffer
	// settings still required? It seems we create it in the "disabled" state. 

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};

	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	colorBlendAttachment.blendEnable = VK_FALSE;

	// References an array of per-framebuffer settings, and sets the blend constants.
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;		// Enable if we want bitwise blending.
	colorBlending.logicOp = VK_LOGIC_OP_COPY;	// Optional. 
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;	// Optional	  Ahh, the constants are RGB factors?
	colorBlending.blendConstants[1] = 0.0f; // Optional	  Yup. I can set them to ONE, ZERO, 
	colorBlending.blendConstants[2] = 0.0f; // Optional   or CONSTANT
	colorBlending.blendConstants[3] = 0.0f;	// Optional

	// We've disabled both blending modes, so the fragment colors are 
	// written to the buffer unmodified.

	// Setup the viewport and scissor to be dynamic state
	// (able to be modified after pipeline creation, at draw time).
	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	// Pipeline Layout
	// Used to describe the uniforms (dynamic global variables accessible to the shaders,
	// whose values can be changed at drawing time... I think?).
	// Push constants... are another way of passing dynamic data into shaders.
	// What's the difference then between a uniform and a push constant?

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;

	if (_pushDataSize > 0)
	{
		VkPushConstantRange push_constant;
		push_constant.offset = 0;
		push_constant.size = _pushDataSize;
		push_constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pipelineLayoutInfo.pPushConstantRanges = &push_constant;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
	}
	else
	{
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
	}

	if (vkCreatePipelineLayout(_logicalDevice, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	// Finally we create our pipeline.
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	// First put our vertex & fragment shaders onto this pipeline.
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	// Reference all the structures described in the fixed-function pipeline.
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;

	// Set the pipeline layout. 
	// Tutorial says that other functions will need to refer to the layout
	// after pipeline creation, hence why it's a member variable.
	pipelineInfo.layout = _pipelineLayout;

	// Reference to the render pass and the index of the subpass 
	// where this graphics pipeline will be used. 
	// Wait... a single graphics pipeline cannot use more than one render subpass?
	// Does that mean the "post-processing effects" described before require multiple
	// graphics pipelines?
	pipelineInfo.renderPass = _renderPass;
	pipelineInfo.subpass = 0;

	// Vulkan allows you to create a new graphics pipeline by deriving
	// from an existing pipeline. This is more efficient than creating
	// & using two completely separate pipelines, if they're similar. 
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	// Separate chapter on pipeline caches. For now we're not using them.
	// Caches can save information for more efficient pipeline creation later.
	if (vkCreateGraphicsPipelines(_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}

void vulkan_renderer::create_framebuffers()
{
	_swapChainFramebuffers.resize(_swapChainImageViews.size());

	for (size_t i = 0; i < _swapChainImageViews.size(); i++)
	{
		// Why can a single frame buffer have many image views attached to it?
		// This doesn't seem to be the same thing as the number of layers.

		// You can only use a framebuffer with the render passes that 
		// it is compatible with, which roughly means that they
		// use the same number and type of attachments.

		// Oh, our render pass only has a single attachment, for color.
		// The attachmentCount and pAttachments parameters specify the 
		// VkImageView objects that should be bound to the 
		// respective attachment descriptions in the render pass 
		// pAttachment array.

		VkImageView attachments[] = {
			_swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = _renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = _selectedSwapExtent.width;
		framebufferInfo.height = _selectedSwapExtent.height;

		// Our swap chain images are single images, so the number of layers is 1.
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(_logicalDevice, &framebufferInfo, nullptr, &_swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

uint32_t vulkan_renderer::find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

	// VkPhysicalDeviceMemoryProperties has two arrays: memoryTypes and memoryHeaps.

	// Memory heaps are distinct memory resources like dedicated VRAM
	// and swap space in RAM for when VRAM runs out.

	// The different types of memory exist within these heaps. 

	// The typeFilter parameter will be used to specify the bit field 
	// of memory types that are suitable. That means that we can find the 
	// index of a suitable memory type by simply iterating over them and 
	// checking if the corresponding bit is set to 1.

	// However, we're not just interested in a memory type that is suitable
	// for the vertex buffer. We also need to be able to write our 
	// vertex data to that memory. 

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && 
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");

}

void vulkan_renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;

	// Just like the images in the swap chain, buffers can also be owned
	// by a specific queue family or shared between multiple at the same time.
	// The buffer will only be used by the graphics queue, so stick to exclusive access.
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(_logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements{};
	vkGetBufferMemoryRequirements(_logicalDevice, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(_logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	if (vkBindBufferMemory(_logicalDevice, buffer, bufferMemory, 0) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to bind buffer memory!");
	}
}

void vulkan_renderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	// Memory transfer operations are executed using command buffers, 
	// just like drawing commands. 

	// Therefore, we must first allocate a temporary command buffer.
	// You may wish to create a separate command pool for these kinds of 
	// short-lived buffers, because the implementation may be able to apply
	// memory allocation optimizations. 

	// You should use the VK_COMMAND_POOL_TRANSIENT_BIT flag during command
	// pool generation in that case.

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = _commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(_logicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	// We're only going to use the command buffer once and wait with returning
	// from the function until the copy operation has finished executing. 
	// It's good practice to tell the driver our intent using 
	// VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT.

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	// Wait for the copy operation to finish.
	// We could have used a fence to do the same thing. 
	// That would allow us to schedule multiple transfers simultaneously
	// and wait for them all to complete, instead of executing one at a time. 

	vkQueueWaitIdle(_graphicsQueue);

	vkFreeCommandBuffers(_logicalDevice, _commandPool, 1, &commandBuffer);
}

void vulkan_renderer::create_vertex_buffer()
{
	VkDeviceSize bufferSize = sizeof(_vertices[0]) * _vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Step 1) Create a CPU-visible staging buffer.
	createBuffer(
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	// Step 2) Write the vertex data into the staging buffer.
	void* data;
	vkMapMemory(_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, _vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(_logicalDevice, stagingBufferMemory);

	// Step 3) Create a device-local buffer for rendering vertices from.
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		_vertexBuffer,
		_vertexBufferMemory);

	// Step 4) Copy from the staging buffer to the vertex buffer.
	copyBuffer(stagingBuffer, _vertexBuffer, bufferSize);

	vkDestroyBuffer(_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(_logicalDevice, stagingBufferMemory, nullptr);
}

void vulkan_renderer::create_index_buffer()
{
	VkDeviceSize bufferSize = sizeof(_indices[0]) * _indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	void* data;
	vkMapMemory(_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, _indices.data(), (size_t)bufferSize);
	vkUnmapMemory(_logicalDevice, stagingBufferMemory);

	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		_indexBuffer,
		_indexBufferMemory);

	copyBuffer(stagingBuffer, _indexBuffer, bufferSize);

	vkDestroyBuffer(_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(_logicalDevice, stagingBufferMemory, nullptr);
}

void vulkan_renderer::create_command_pool()
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

	// Two possible flags for command pools:
	// 
	// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
	//	- Hint that command buffers are recorded with new commands very often.
	//    May change memory allocation behavior.
	// 
	// VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
	//	- Allow command buffers to be recorded individually.
	//	  Without this flag they all have to be reset together.

	// We will be recording a command buffer every frame, so we want 
	// to be able to reset and rerecord over it. Thus, we need to 
	// set the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag
	// for our command pool.

	// Actually, this is rather curious. Why should the command buffer change?
	// Aren't we always drawing the same scene - the same triangle using the same
	// vertex and fragment shaders?

	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = _graphicsQueueFamilyIndex;

	// Command buffers are executed by submitting them on one of the device queues.
	// Each command pool can only allocate command buffers that are 
	// submitted on a single type of queue. 
	//	-> Ahh, need a different command pool for graphics as for presentation?
	// 
	// We're going to record commands for drawing, which is why we've chosen
	// the graphics queue family.

	if (vkCreateCommandPool(_logicalDevice, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void vulkan_renderer::create_command_buffer()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = _commandPool;

	// The level parameter specifies if the allocated command buffers are
	// primary or secondary command buffers.
	//	VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution,
	//	  but cannot be called from other command buffers.
	//  VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can
	//    be called from primary command buffers. 

	// Secondary command buffers are helpful for reusing common operations
	// from primary command buffers.

	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(_logicalDevice, &allocInfo, &_commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffer!");
	}
}

void vulkan_renderer::create_sync_objects()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(_logicalDevice, &semaphoreInfo, nullptr, &_imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(_logicalDevice, &semaphoreInfo, nullptr, &_renderFinishedSemaphore) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create semaphores!");
	}
}

void vulkan_renderer::draw_frame(void* pushData)
{
	/*
	Rendering a frame in Vulkan consists of a common set of steps:

		* Wait for the previous frame to finish.
	      (if rendering multiple frames in flight, this entails waiting
		   on the fence for the current frame to be ready, allowing the GPU
		   to catch up to the CPU).

		* Acquire an image from the swap chain.

		* Record a command buffer which draws the scene onto that image.

		* Submit the recorded command buffer.

		* Present the swap chain image.

	=== Semaphores ===

	A semaphore is used to add order between queue operations.
	Queue operations refer to the work we submit to a queue, either in
	a command buffer or from within a function.

	Two kinds of semaphores in Vulkan: Binary and Timeline.
	Tutorial only covers Binary.

	The way we use a semaphore to order queue operations is by providing
	the same semaphore as a "signal" semaphore in one queue operation,
	and as a "wait" semaphore in another queue operation.

		A signals S when it completes.
		B waits on S to begin.

		After operation B begins executing, semaphore S is automatically
		reset back to being unsignaled, allowing it to be used again.

	=== Fences ===

	A fence has a similar purpose, in that it is used to synchronize execution,
	but it is for ordering the execution on the CPU, otherwise known as the host.
	If the host needs to know when the GPU has finished something, we use a fence.

	Unlike semaphores, fences must be reset manually to put them back
	into the unsignaled state. This is because fences are used to control
	the execution of the host, and so the host gets to decide when to reset the fence.

	Semaphores: Control execution order of operations on the GPU
	Fences: Used to keep CPU and GPU in sync with each other.

	====
	
	The Mandelbrot Explorer isn't animated, so the "Frames in Flight" stuff isn't needed.

	For simplicity's sake, drawing a frame will be a blocking operation.
	Leave it to .NET to run this operation in a Task, if it needs to be async.

	*/

	// Acquire an image from the swap chain.
	uint32_t imageIndex;

	// The swap chain is an extension feature, hence the *KHR suffix.

	// According to Vulkan documentation, it seems _imageAvailableSemaphores 
	// is signaled when the next image is acquired?

	// Tutorial says it's signaled when the presentation engine is finished
	// using the image - that's the point in time where we can start drawing to it.

	// Ahh, vkAcquireNextImageKHR immediately returns the index of an image that 
	// might not yet be ready to be written to. The semaphore will let us know
	// when we can draw to this image.

	//  _imageAvailableSemaphore must be unsignaled. Will be signaled when the image is acquired.
	VkResult result = vkAcquireNextImageKHR(
		_logicalDevice,
		_swapChain,
		UINT64_MAX,
		_imageAvailableSemaphore,	
		VK_NULL_HANDLE,
		&imageIndex);

	// If the swap extent is invalid, try to recreate it and acquire a swapchain image again.
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreate_swap_chain();

		result = vkAcquireNextImageKHR(
			_logicalDevice,
			_swapChain,
			UINT64_MAX,
			_imageAvailableSemaphore,
			VK_NULL_HANDLE,
			&imageIndex);

		// If the swap extent is still invalid (e.g., application is minimized),
		// then don't bother rendering this frame.
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			return;
		}
	}

	// Throw an exception if acquiring the swap chain image failed for any other reason.
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	// Reset the command buffer to make sure it's able to be recorded.
	vkResetCommandBuffer(_commandBuffer, 0);

	// Now record the command buffer.
	record_command_buffer(_commandBuffer, imageIndex, pushData);

	// Now submit the command buffer to the graphics queue.

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// The pipeline should stop & wait on writing colors to the image until
	// the image acquired from the swap chain becomes available to write to.
	// (no problem running the prior steps in the pipeline before then - 
	//  I guess that includes the vertex shader? How about the fragment shader?)

	// Slight problem: 
	// The render pass will attempt to do a layout transition on the selected image
	// at the start of the pipeline, before it's time to write colors 
	// and before the image acquired from the swap chain is available.

	// Two options:
	// 1) Wait at the beginning of the pipeline for the image to be available,
	//    rather than at the color output stage
	//    (i.e, use VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT in our waitStages). 
	//    
	// 2) Make the render pass wait for the color output stage
	//    to begin the layout transition.
	//
	// Tutorial uses the second approach.

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &_imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &_renderFinishedSemaphore;

	// The last parameter references an optional fence that will be signaled 
	// when the command buffer finished execution. This allows us to know 
	// when it is safe for the command buffer to be reused.

	// Waiting on a fence is the preferred approach when doing animation.
	// I won't worry about that here and will use the (relatively inefficient) 
	// vkDeviceWaitIdle

	if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	// When the command buffer finishes executing, then present the image.
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &_renderFinishedSemaphore;

	// Specify the swap chain(s) to present the images to and the index of the image
	// for each swap chain. This will almost always be a single one.
	// Curious - is it possible to present multiple images at the same time?
	// Perhaps, if different swapchains are connected to different surfaces. Interesting.
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &_swapChain;
	presentInfo.pImageIndices = &imageIndex;

	// If presenting many swap chains, this can tell us which one went wrong.
	// Since we're using only one, the return value of the present function is enough.
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(_presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		// Did the image get presented?
		// Should the image be re-rendered, given that the fragment shader
		// may be a lengthy operation (e.g., if the iteration count is high)?
		// Should draw_frame() return something indicating success or failure?
		recreate_swap_chain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	// Crude form of synchronization. 
	// Wait for the image to be presented before returning.
	vkDeviceWaitIdle(_logicalDevice);
}

void vulkan_renderer::record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, void* pushData)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	// The flags parameter specifies how we're going to use the command buffer.

	// Available flags:
	//	VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	//	  The command buffer will be rerecorded right after executing it once.
	//	VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
	//	  This is a secondary command buffer that will be entirely 
	//    within a single render pass. 
	//  VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
	//	  The command buffer can be resubmitted while it is also 
	//    already pending execution.

	// None of these flags are applicable to us right now.

	beginInfo.flags = 0;

	// pInheritanceInfo is only relevant for secondary command buffers.
	// It specifies which state to inherit from the calling primary command buffers.

	beginInfo.pInheritanceInfo = nullptr;

	// If the command buffer was already recorded once, then a call to 
	// vkBeginCommandBuffer will implicitly reset it. 
	// It's not possible to append commands to a buffer at a later time.

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	// Drawing starts by beginning the render pass with vkCmdBeginRenderPass.

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	// The first parameters are the render pass itself and the attachments to bind.
	renderPassInfo.renderPass = _renderPass;
	renderPassInfo.framebuffer = _swapChainFramebuffers[imageIndex];

	// These define the size of the render area. 
	// The render area defines where the shader loads and stores take place.
	// The pixels outside this region will have undefined values.
	// It should match the size of the attachments for best performance.
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = _selectedSwapExtent;

	// These last two parameters define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR,
	// which we used as the load operation for the color attachment. 
	// This is black with 100% opacity.

	// Why the weird initializer?
	// 
	// VkClearValue is a union - either a VkClearColorValue or a VkClearDepthStencilValue.

	// VkClearColorValue is itself a union:
	// either an array of 4 float, an array of 4 int32_t, or an array of 4 uint32_t. 

	// VkClearDepthStencilValue is a struct having a float depth and uint32_t stencil. 

	// The inner {} represents the array.
	// The middle {} represents... selecting the VkClearColorValue field?
	// Using just a single {} seems to work as well.
	VkClearValue clearColor = { {{ 0.0f, 0.0f, 0.0f, 1.0f }} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	// We now begin recording the render pass.
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// All functions that record commands can be recognized by their vkCmd prefix.
	// They all return void, so there will be no error handling until we're finished recording.

	// The final parameter controls how the drawing commands within the render pass 
	// will be provided. It can have one of two values:
	//	VK_SUBPASS_CONTENTS_INLINE
	//	  The render pass commands will be embedded in the primary command buffer itself
	//	  and no secondary command buffers will be executed.
	//  VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
	//	  The render pass commands will be executed from secondary command buffers.

	// We now bind the graphics pipeline. 
	// The second parameter specifies if the pipeline object is a graphics or compute pipeline.
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

	VkBuffer vertexBuffers[]{ _vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, _indexBuffer, 0, VK_INDEX_TYPE_UINT16);

	// The viewport and scissor state for this pipeline were set to be dynamic.
	// So we need to set them in the command buffer before issuing our draw command:

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(_selectedSwapExtent.width);
	viewport.height = static_cast<float>(_selectedSwapExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// We can set many viewports and scissors?
	// What does it mean to have many viewports?

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = _selectedSwapExtent;

	// We can set multiple scissors?
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	if (_pushDataSize > 0)
	{
		vkCmdPushConstants(commandBuffer, _pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, _pushDataSize, pushData);
	}

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	// We now finish recording the command buffer.
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void vulkan_renderer::load_fragment_shader(std::string code, uint32_t pushDataSize)
{
	VkShaderModule shaderModule = compile_shader("custom_fragment_shader", code, shaderc_shader_kind::shaderc_fragment_shader);

	vkDeviceWaitIdle(_logicalDevice);

	// Cleanup the old fragment shader module.
	vkDestroyShaderModule(_logicalDevice, _fragmentShader, nullptr);

	// Set the new fragment shader.
	_fragmentShader = shaderModule;
	_pushDataSize = pushDataSize;

	// Finally, recreate the graphics pipeline with the new shader.
	recreate_graphics_pipeline();
}
