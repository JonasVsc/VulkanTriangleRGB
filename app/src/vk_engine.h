#pragma once

#include "vk_defines.h"
#include "vk_mesh.h"



const int FRAME_OVERLAP = 2;

struct GPUMeshConstant
{
	glm::vec4 colors[3];
};


class Engine
{
	struct DeletionQueue
	{
		std::deque<std::function<void()>> deletors;

		void flush() 
		{
			for(auto it = deletors.rbegin(); it != deletors.rend(); it++)
				(*it)();
		}
	};

	struct SwapchainDimensions
	{
		uint32_t width = 0;
	
		uint32_t height = 0;

		VkFormat format = VK_FORMAT_UNDEFINED;
	};

	struct PerFrame 
	{
		VkFence queue_submit_fence 				= VK_NULL_HANDLE;
		VkCommandBuffer primary_command_buffer  = VK_NULL_HANDLE;
		VkSemaphore swapchain_acquire_semaphore = VK_NULL_HANDLE;
		VkSemaphore swapchain_release_semaphore = VK_NULL_HANDLE;
	};

	struct Context
	{
		VkInstance instance = VK_NULL_HANDLE;

		VkSurfaceKHR surface = VK_NULL_HANDLE;

		VkSurfaceCapabilitiesKHR surface_properties;

		VkPhysicalDevice gpu = VK_NULL_HANDLE;

		uint32_t graphics_queue_index = -1;

		VkDevice device = VK_NULL_HANDLE;

		VkQueue queue;

		VmaAllocator allocator;

		VkSwapchainKHR swapchain = VK_NULL_HANDLE;

		SwapchainDimensions swapchain_dimensions;

		std::vector<VkImage> swapchain_images;

		std::vector<VkImageView> swapchain_image_views;
		
		VkCommandPool primary_command_pool;

		PerFrame per_frame[FRAME_OVERLAP];

		VkPipeline pipeline;

		VkPipelineLayout pipeline_layout;

	};

public:
	
	void init();

	void run();

private:

	void draw();

	void cleanup();

	void init_vulkan();

	void init_swapchain();

	void init_per_frame();

	void init_pipeline();

	void init_scene();

	void init_imgui();

	inline PerFrame& get_current_frame() { return context.per_frame[frame_number % FRAME_OVERLAP]; }

	// --- window ---
	GLFWwindow*	m_window;

	uint32_t frame_number {};

	Context context;

	DeletionQueue m_deletion_queue;

	// --- temp ---

	AllocatedBuffer m_mesh;

	GPUMeshConstant m_colors;

	const std::vector<Vertex> vertices = {
		{{ 0.0f,-0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }},
		{{ 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
		{{-0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }}
	}; 

};