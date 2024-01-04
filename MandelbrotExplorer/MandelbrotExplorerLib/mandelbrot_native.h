#pragma once
#include "pch.h"
#include <vector>
#include <string>
#include <shaderc/shaderc.hpp>
#include "vertex.h"
#include <glm/glm.hpp>

struct debug_message
{
	std::string text;
	VkDebugUtilsMessageSeverityFlagBitsEXT severity;
	VkDebugUtilsMessageTypeFlagsEXT type;
};

class vulkan_renderer
{
public:

	vulkan_renderer(HINSTANCE hinstance, HWND hwnd, bool debug);
	~vulkan_renderer();
	
	void dispose();
	bool disposed() { return _disposed; }

	const std::vector<debug_message>& debug_messages() const { return _messages; };

	void load_fragment_shader(std::string code, uint32_t pushDataSize);

	void refresh_surface() { recreate_swap_chain(); }
	VkExtent2D surface_extent() { return _selectedSwapExtent; }

	void draw_frame(void* pushData = nullptr);

private:

	void setup(HINSTANCE hinstance, HWND hwnd, bool debug);
	void cleanup_pipeline();
	void cleanup_swap_chain();
	void cleanup();
	void create_vkinstance();
	void create_vkinstance_with_debugging();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	void create_surface(HWND hwnd, HINSTANCE hinstance);
	void select_physical_device();
	void create_logical_device();
	void choose_swap_surface_format();
	void choose_swap_present_mode();
	void choose_swap_extent();
	
	void recreate_swap_chain();
	void create_swap_chain();
	void create_image_views();
	
	VkShaderModule compile_shader(std::string name, std::string source, shaderc_shader_kind kind);
	void create_vertex_shader();
	void create_fragment_shader();

	void create_render_pass();
	void create_framebuffers();
	void recreate_graphics_pipeline();
	void create_graphics_pipeline();

	uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void create_vertex_buffer();
	void create_index_buffer();

	void create_command_pool();
	void create_command_buffer();
	void create_sync_objects();

	void record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, void* pushData);

	// ================================================================

	const std::vector<Vertex> _vertices = {
		{{-1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}},
		{{ 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
		{{ 1.0f,  1.0f}, {1.0f, 1.0f, 0.0f}},
		{{-1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}}
	};

	const std::vector<uint16_t> _indices = {
		0, 1, 2, 2, 3, 0
	};

	VkBuffer _vertexBuffer = nullptr;
	VkDeviceMemory _vertexBufferMemory = nullptr;
	VkBuffer _indexBuffer = nullptr;
	VkDeviceMemory _indexBufferMemory = nullptr;

	// ================================================================

	bool _disposed = false;

	VkInstance _vkinstance = nullptr;

	VkDebugUtilsMessengerEXT _debugMessenger = nullptr;
	std::vector<debug_message> _messages;

	VkSurfaceKHR _surface = nullptr;

	VkPhysicalDevice _physicalDevice = nullptr;
	uint32_t _graphicsQueueFamilyIndex = UINT32_MAX;
	uint32_t _presentQueueFamilyIndex = UINT32_MAX;

	VkDevice _logicalDevice = nullptr;
	VkQueue _graphicsQueue = nullptr;
	VkQueue _presentQueue = nullptr;

	VkSurfaceFormatKHR _selectedSurfaceFormat;
	VkPresentModeKHR _selectedPresentMode;
	VkExtent2D _selectedSwapExtent;

	VkSwapchainKHR _swapChain = nullptr;
	std::vector<VkImage> _swapChainImages;
	std::vector<VkImageView> _swapChainImageViews;

	VkShaderModule _vertexShader = nullptr;
	VkShaderModule _fragmentShader = nullptr;

	VkRenderPass _renderPass = nullptr;
	VkPipelineLayout _pipelineLayout = nullptr;
	VkPipeline _graphicsPipeline = nullptr;
	std::vector<VkFramebuffer> _swapChainFramebuffers;

	VkCommandPool _commandPool = nullptr;
	VkCommandBuffer _commandBuffer;

	VkSemaphore _imageAvailableSemaphore;
	VkSemaphore _renderFinishedSemaphore;

	// ================================================================

	uint32_t _pushDataSize = 0;
};

