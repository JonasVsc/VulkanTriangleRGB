#include "pre-compiled-header.h"
#include "vk_engine.h"

#include "configurations.h"
#include "vk_utils.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <glm/gtc/type_ptr.hpp>

void Engine::init()
{
	init_vulkan();

	init_swapchain();

	init_per_frame();

	init_pipeline();

	init_scene();

	init_imgui();
}

void Engine::run()
{
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Triangle");
		ImGui::ColorEdit3("Top", glm::value_ptr(m_colors.colors[0]));
		ImGui::ColorEdit3("Right", glm::value_ptr(m_colors.colors[1]));
		ImGui::ColorEdit3("Left", glm::value_ptr(m_colors.colors[2]));
		ImGui::End();

		ImGui::Render();

		draw();
	}

	cleanup();
}

void Engine::cleanup()
{
	vkQueueWaitIdle(context.queue);

	m_deletion_queue.flush();
}

void Engine::draw()
{
	VK_CHECK(vkWaitForFences(context.device, 1, &get_current_frame().queue_submit_fence, true, UINT64_MAX));
	VK_CHECK(vkResetFences(context.device, 1, &get_current_frame().queue_submit_fence));
	
	// acquire next image
	uint32_t image;
	VK_CHECK(vkAcquireNextImageKHR(context.device, context.swapchain, UINT64_MAX,
											get_current_frame().swapchain_acquire_semaphore, VK_NULL_HANDLE, 
											&image));
	
	
	// render triangle
	VkCommandBuffer cmd = get_current_frame().primary_command_buffer;

	vkResetCommandBuffer(cmd, 0);

	VkCommandBufferBeginInfo begin_info = { 
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	VK_CHECK(vkBeginCommandBuffer(get_current_frame().primary_command_buffer, &begin_info));

	// transition swapchain image to COLOR_ATTACHMENT_OPTIMAL
	vkutil::transition_image_layout(
		cmd,
		context.swapchain_images[image],
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		0,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
	);

	VkClearValue clear_value = {{{0.01f, 0.01f, 0.033f, 1.0f}}};
	VkRenderingAttachmentInfo color_attachment = {
		.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
	    .imageView   = context.swapchain_image_views[image],
	    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	    .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
	    .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
	    .clearValue  = clear_value
	};

	// begin rendering
	VkRenderingInfo rendering_info = {
		.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
	    .renderArea           = {                         													
			.offset 		  = {0, 0},        																
			.extent 		  = { context.swapchain_dimensions.width, context.swapchain_dimensions.height } 
		},
		.layerCount 		  = 1,
	    .colorAttachmentCount = 1,
	    .pColorAttachments    = &color_attachment
	};

	vkCmdBeginRendering(cmd, &rendering_info);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline);

	// Set dynamic states

	// Set viewport dynamically
	VkViewport vp{
	    .width    = static_cast<float>(context.swapchain_dimensions.width),
	    .height   = static_cast<float>(context.swapchain_dimensions.height),
	    .minDepth = 0.0f,
	    .maxDepth = 1.0f};

	vkCmdSetViewport(cmd, 0, 1, &vp);

	// Set scissor dynamically
	VkRect2D scissor{
	    .extent = {
	        .width  = context.swapchain_dimensions.width,
	        .height = context.swapchain_dimensions.height}};

	vkCmdSetScissor(cmd, 0, 1, &scissor);

	VkDeviceSize offSets = { 0 };

	vkCmdBindVertexBuffers(cmd, 0, 1, &m_mesh.buffer, &offSets);
	vkCmdPushConstants(cmd, context.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUMeshConstant), &m_colors);

	vkCmdDraw(cmd, 3, 1, 0, 0);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);

	// transition the swapchain image to PRESENT_SRC
	vkutil::transition_image_layout(
	    cmd,
	    context.swapchain_images[image],
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	    VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,                 // srcAccessMask
	    0,                                                      // dstAccessMask
	    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,        // srcStage
	    VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT                  // dstStage
	);
	
	VK_CHECK(vkEndCommandBuffer(cmd));

	// submit
	VkPipelineStageFlags wait_stage = { VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT };
	VkSubmitInfo submit_info = {
		.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    .waitSemaphoreCount   = 1,
	    .pWaitSemaphores      = &get_current_frame().swapchain_acquire_semaphore,
	    .pWaitDstStageMask    = &wait_stage,
	    .commandBufferCount   = 1,
	    .pCommandBuffers      = &cmd,
	    .signalSemaphoreCount = 1,
	    .pSignalSemaphores    = &get_current_frame().swapchain_release_semaphore
	};

	VK_CHECK(vkQueueSubmit(context.queue, 1, &submit_info, get_current_frame().queue_submit_fence));

	// present
	VkPresentInfoKHR present_info = {
		.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	    .waitSemaphoreCount = 1,
	    .pWaitSemaphores    = &get_current_frame().swapchain_release_semaphore,
	    .swapchainCount     = 1,
	    .pSwapchains        = &context.swapchain,
	    .pImageIndices      = &image
	};
	VK_CHECK(vkQueuePresentKHR(context.queue, &present_info));

	frame_number++;
}


void Engine::init_vulkan()
{
	if(!glfwInit())
		throw std::runtime_error("Failed to initialize glfw");
	m_deletion_queue.deletors.push_back([=]()
	{
		std::cout << "Terminating glfw" << '\n';
		glfwTerminate();
	});
	
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, APP_NAME, nullptr, nullptr);
	if(m_window == nullptr)
		throw std::runtime_error("Failed to create window");
	m_deletion_queue.deletors.push_back([this]()
	{
		std::cout << "destroying window" << '\n';
		glfwDestroyWindow(m_window);
	});


	uint32_t required_instance_extensions_count;
	const char** required_instance_extensions = glfwGetRequiredInstanceExtensions(&required_instance_extensions_count);

	// instance
	VkApplicationInfo app_info = {
		.sType 			  = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = APP_NAME,
		.pEngineName 	  = "No Engine",
		.apiVersion 	  = VK_MAKE_VERSION(1, 3, 0)
	};
	
	VkInstanceCreateInfo instance_info = {
		.sType 					 = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo 		 = &app_info,
		.enabledExtensionCount 	 = required_instance_extensions_count,
		.ppEnabledExtensionNames = required_instance_extensions
	};

	if(VALIDATION_LAYERS)
	{
		const char* validations[] = {
			"VK_LAYER_KHRONOS_validation"
		};
		instance_info.enabledLayerCount = 1,
		instance_info.ppEnabledLayerNames = validations;
	}

	VK_CHECK(vkCreateInstance(&instance_info, nullptr, &context.instance));
	m_deletion_queue.deletors.push_back([this]()
	{
		std::cout << "destroying instance" << '\n';
		vkDestroyInstance(context.instance, nullptr);
	});

	// initialize surface
	glfwCreateWindowSurface(context.instance, m_window, nullptr, &context.surface);
	m_deletion_queue.deletors.push_back([this]()
	{
		std::cout << "destroying surface" << '\n';
		vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
	});

	// select physical device
	uint32_t gpu_count;
	VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &gpu_count, nullptr));
	if(gpu_count < 1)
		throw std::runtime_error("No physical device found.");
	std::vector<VkPhysicalDevice> gpus(gpu_count);
	VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &gpu_count, gpus.data()));

	for(const auto &physical_device : gpus)
	{
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(physical_device, &device_properties);

		std::cout << "maxPerStageDescriptorUniformBuffers: " << device_properties.limits.maxPerStageDescriptorUniformBuffers << '\n';

		if(device_properties.apiVersion < VK_API_VERSION_1_3)
		{
			std::cout << "Physical device " << device_properties.deviceName << " does not support Vulkan 1.3, skipping." << std::endl;
			continue;
		}

		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties.data());

		for(uint32_t i = 0; i < queue_family_count; i++)
		{
			VkBool32 supports_present = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, context.surface, &supports_present);

			if((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supports_present)
			{
				context.graphics_queue_index = i;
				break;
			}
		}

		if(context.graphics_queue_index >= 0)
		{
			context.gpu = physical_device;
			break;
		}
	}

	if(context.graphics_queue_index < 0)
		throw std::runtime_error("Failed to find a suitable GPU with Vulkan 1.3 support.");

	// query vulkan 1.3 features
	std::vector<const char*> required_device_extensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	VkPhysicalDeviceFeatures2 query_device_features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	VkPhysicalDeviceVulkan13Features query_vulkan13_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT query_extended_dynamic_state_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT};
	query_device_features2.pNext = &query_vulkan13_features;
	query_vulkan13_features.pNext = &query_extended_dynamic_state_features;

	vkGetPhysicalDeviceFeatures2(context.gpu, &query_device_features2);

	if(!query_vulkan13_features.dynamicRendering)
		throw std::runtime_error("Dynamic Rendering feature is missing");
	if(!query_vulkan13_features.synchronization2)
		throw std::runtime_error("Synchronization2 feature is missing");
	if(!query_extended_dynamic_state_features.extendedDynamicState)
		throw std::runtime_error("Extended Dynamic State feature is missing");

	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT enable_extended_dynamic_state_features = {
	    .sType 				  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
	    .extendedDynamicState = VK_TRUE
	};

	VkPhysicalDeviceVulkan13Features enable_vulkan13_features = {
	    .sType 			  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
	    .pNext            = &enable_extended_dynamic_state_features,
	    .synchronization2 = VK_TRUE,
	    .dynamicRendering = VK_TRUE,
	};

	VkPhysicalDeviceFeatures2 enable_device_features2{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
	    .pNext = &enable_vulkan13_features
	};

	// create logical device
	float queue_priority = 1.0f;

	VkDeviceQueueCreateInfo queue_info = {
		.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
	    .queueFamilyIndex = static_cast<uint32_t>(context.graphics_queue_index),
	    .queueCount       = 1,
	    .pQueuePriorities = &queue_priority
	};

	VkDeviceCreateInfo device_info{
	    .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	    .pNext                   = &enable_device_features2,
	    .queueCreateInfoCount    = 1,
	    .pQueueCreateInfos       = &queue_info,
	    .enabledExtensionCount   = static_cast<uint32_t>(required_device_extensions.size()),
	    .ppEnabledExtensionNames = required_device_extensions.data()
	};

	VK_CHECK(vkCreateDevice(context.gpu, &device_info, nullptr, &context.device));
	m_deletion_queue.deletors.push_back([this]()
	{
		std::cout << "destroying device" << '\n';
		vkDestroyDevice(context.device, nullptr);
	});
	vkGetDeviceQueue(context.device, context.graphics_queue_index, 0, &context.queue);

	// init vma allocator
	VmaAllocatorCreateInfo allocator_info = {
		.physicalDevice = context.gpu,
		.device = context.device,
		.instance = context.instance
	};

	VK_CHECK(vmaCreateAllocator(&allocator_info, &context.allocator));
	m_deletion_queue.deletors.push_back([this]()
	{
		std::cout << "destroying allocator" << '\n';
		vmaDestroyAllocator(context.allocator);
	});
}

void Engine::init_swapchain()
{
	VkSurfaceCapabilitiesKHR surface_properties;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.gpu, context.surface, &surface_properties);

	context.surface_properties = surface_properties;
	
	VkExtent2D swapchain_size;
	if(surface_properties.currentExtent.width == 0xFFFFFFFF)
	{
		swapchain_size.width = context.swapchain_dimensions.width;
		swapchain_size.height = context.swapchain_dimensions.height;
	}
	else
	{
		swapchain_size = surface_properties.currentExtent;
	}

	VkPresentModeKHR selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;

	uint32_t surfaces_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(context.gpu, context.surface, &surfaces_count, nullptr);
	
	std::vector<VkSurfaceFormatKHR> available_surface_formats(surfaces_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(context.gpu, context.surface, &surfaces_count, available_surface_formats.data());

	VkSurfaceFormatKHR selected_format;
	for(const auto& available_format : available_surface_formats)
	{
		if(selected_format.format == VK_FORMAT_UNDEFINED)
			selected_format = available_format;

		if(available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			selected_format = available_format;
			break;
		}
	}

	uint32_t desired_swapchain_images = surface_properties.minImageCount + 1;
	if((surface_properties.maxImageCount > 0) && (desired_swapchain_images > surface_properties.maxImageCount))
		desired_swapchain_images = surface_properties.maxImageCount;
		
	VkSurfaceTransformFlagBitsKHR pre_transform;
	if(surface_properties.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	else
		pre_transform = surface_properties.currentTransform;

	VkSwapchainKHR old_swapchain = context.swapchain;

	VkCompositeAlphaFlagBitsKHR composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
		composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	else if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
		composite = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	else if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
		composite = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	else if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
		composite = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;

	VkSwapchainCreateInfoKHR swapchain_info{
	    .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
	    .surface          = context.surface,                            // The surface onto which images will be presented
	    .minImageCount    = desired_swapchain_images,                   // Minimum number of images in the swapchain (number of buffers)
	    .imageFormat      = selected_format.format,                     // Format of the swapchain images (e.g., VK_FORMAT_B8G8R8A8_SRGB)
		.imageColorSpace  = selected_format.colorSpace,                 // Color space of the images (e.g., VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
	    .imageExtent      = swapchain_size,                             // Resolution of the swapchain images (width and height)
	    .imageArrayLayers = 1,                                          // Number of layers in each image (usually 1 unless stereoscopic)
	    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,        // How the images will be used (as color attachments)
	    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,                  // Access mode of the images (exclusive to one queue family)
	    .preTransform     = pre_transform,                              // Transform to apply to images (e.g., rotation)
	    .compositeAlpha   = composite,                                  // Alpha blending to apply (e.g., opaque, pre-multiplied)
	    .presentMode      = selected_present_mode,                      // Presentation mode (e.g., vsync settings)
	    .clipped          = true,                                       // Whether to clip obscured pixels (improves performance)
	    .oldSwapchain     = old_swapchain                               // Handle to the old swapchain, if replacing an existing one
	};

	VK_CHECK(vkCreateSwapchainKHR(context.device, &swapchain_info, nullptr, &context.swapchain));
	m_deletion_queue.deletors.push_back([this]()
	{
		std::cout << "destroying swapchain" << '\n';
		vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);
	});

	if(old_swapchain != VK_NULL_HANDLE)
	{
		// TODO: destroy old swapchain 
	}

	context.swapchain_dimensions = { swapchain_size.width, swapchain_size.height, selected_format.format };

	uint32_t image_count;
	vkGetSwapchainImagesKHR(context.device, context.swapchain, &image_count, nullptr);

	std::vector<VkImage> swapchain_images(image_count);
	vkGetSwapchainImagesKHR(context.device, context.swapchain, &image_count, swapchain_images.data());

	context.swapchain_images = swapchain_images;

	for(size_t i = 0; i < image_count; i++)
	{
		VkImageViewCreateInfo view_info = {
			.sType    		  = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image    		  = swapchain_images[i],
			.viewType 		  = VK_IMAGE_VIEW_TYPE_2D,
			.format   		  = context.swapchain_dimensions.format,
			.subresourceRange = {
				.aspectMask 	= VK_IMAGE_ASPECT_COLOR_BIT, 
				.baseMipLevel 	= 0, 
				.levelCount 	= 1, 
				.baseArrayLayer = 0, 
				.layerCount 	= 1
			}
		};

		VkImageView image_view;
		VK_CHECK(vkCreateImageView(context.device, &view_info, nullptr, &image_view));
		context.swapchain_image_views.push_back(image_view);
		m_deletion_queue.deletors.push_back([this, i]()
		{
			std::cout << "destroying image view [" << i << ']' << '\n';
			vkDestroyImageView(context.device, context.swapchain_image_views[i], nullptr);
		});
	}
}

void Engine::init_per_frame()
{

	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	VkSemaphoreCreateInfo semaphore_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	VkCommandPoolCreateInfo cmd_pool_info = {
		.sType 			  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags 			  = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = context.graphics_queue_index
	};

	vkCreateCommandPool(context.device, &cmd_pool_info, nullptr, &context.primary_command_pool);
	m_deletion_queue.deletors.push_back([this]()
	{
		std::cout << "destroying command_pool" << '\n';
		vkDestroyCommandPool(context.device, context.primary_command_pool, nullptr);
	});

	for(int i = 0; i < FRAME_OVERLAP; i++)
	{

		// initialize sync objects
		vkCreateSemaphore(context.device, &semaphore_info, nullptr, &context.per_frame[i].swapchain_acquire_semaphore);
		vkCreateSemaphore(context.device, &semaphore_info, nullptr, &context.per_frame[i].swapchain_release_semaphore);
		vkCreateFence(context.device, &fence_info, nullptr, &context.per_frame[i].queue_submit_fence);
		m_deletion_queue.deletors.push_back([this, i]()
		{
			std::cout << "destroying fence [" << i << ']' << '\n';
			std::cout << "destroying swapchain_acquire_semaphore [" << i << ']' << '\n';
			std::cout << "destroying swapchain_release_semaphore [" << i << ']' << '\n';
			vkDestroyFence(context.device, context.per_frame[i].queue_submit_fence, nullptr);

			vkDestroySemaphore(context.device, context.per_frame[i].swapchain_acquire_semaphore, nullptr);
			vkDestroySemaphore(context.device, context.per_frame[i].swapchain_release_semaphore, nullptr);
		});

		// initialize commands objects
		

		VkCommandBufferAllocateInfo cmd_buffer_info = {
			.sType 				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool 		= context.primary_command_pool,
			.level 				= VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		vkAllocateCommandBuffers(context.device, &cmd_buffer_info, &context.per_frame[i].primary_command_buffer);
	}
}

void Engine::init_pipeline()
{
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {{
	{
		.sType 	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage 	= VK_SHADER_STAGE_VERTEX_BIT,
		.module = vkutil::load_shader_module(context.device, "assets/shaders/spirv/default_mesh_vert.spv"),
		.pName 	= "main"
	},
	{
		.sType 	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage 	= VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = vkutil::load_shader_module(context.device, "assets/shaders/spirv/default_mesh_frag.spv"),
		.pName 	= "main"
	}
	}};

	VertexInputDescription vertexDescription = Vertex::get_vertex_description();

	// vertex input state
	VkPipelineVertexInputStateCreateInfo vertex_input = {
		.sType 							 = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount   = static_cast<uint32_t>(vertexDescription.bindings.size()),
		.pVertexBindingDescriptions 	 = vertexDescription.bindings.data(),
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexDescription.attributes.size()),
		.pVertexAttributeDescriptions 	 = vertexDescription.attributes.data()
	};

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {
		.sType 					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology 				= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	VkPipelineViewportStateCreateInfo viewport = {
		.sType 		   = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount  = 1
	};

	VkPipelineRasterizationStateCreateInfo raster = {
		.sType 					 = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable 		 = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode 			 = VK_POLYGON_MODE_FILL,
		.depthBiasEnable 		 = VK_FALSE,
		.lineWidth 				 = 1.0f
	};

	VkPipelineMultisampleStateCreateInfo multisample = {
		.sType 				  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
	};

	VkPipelineDepthStencilStateCreateInfo depth_stencil = {
		.sType 			= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthCompareOp = VK_COMPARE_OP_ALWAYS
	};

	VkPipelineColorBlendAttachmentState blend_attachment = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo blend = {
		.sType 			 = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments 	 = &blend_attachment
	};

	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state_info = {
		.sType 			   = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
		.pDynamicStates    = dynamic_states.data()
	};

	VkPushConstantRange push_constant = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = offsetof(GPUMeshConstant, colors),
		.size = sizeof(GPUMeshConstant)
	};

	VkPipelineLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push_constant
	};

	VK_CHECK(vkCreatePipelineLayout(context.device, &layout_info, nullptr, &context.pipeline_layout));
	m_deletion_queue.deletors.push_back([this]()
	{
		vkDestroyPipelineLayout(context.device, context.pipeline_layout, nullptr);
	});

	// required for dynamic rendering
	VkPipelineRenderingCreateInfo pipeline_rendering_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount 	 = 1,
		.pColorAttachmentFormats = &context.swapchain_dimensions.format
	};

	VkGraphicsPipelineCreateInfo pipeline_graphics_info = {
		.sType 				 = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext 				 = &pipeline_rendering_info,
		.stageCount 		 = static_cast<uint32_t>(shader_stages.size()),
		.pStages 			 = shader_stages.data(),
		.pVertexInputState 	 = &vertex_input,
		.pInputAssemblyState = &input_assembly,
		.pViewportState 	 = &viewport,
		.pRasterizationState = &raster,
		.pMultisampleState 	 = &multisample,
		.pColorBlendState 	 = &blend,
		.pDynamicState 		 = &dynamic_state_info,
		.layout 			 = context.pipeline_layout,
		.renderPass 		 = VK_NULL_HANDLE,
		.subpass			 = 0
	};

	VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipeline_graphics_info, nullptr, &context.pipeline));
	m_deletion_queue.deletors.push_back([this]()
	{
		vkDestroyPipeline(context.device, context.pipeline, nullptr);
	});

	vkDestroyShaderModule(context.device, shader_stages[0].module, nullptr);
	vkDestroyShaderModule(context.device, shader_stages[1].module, nullptr);
}

void Engine::init_scene()
{
	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
	};

	VkFence copy_fence;
	VK_CHECK(vkCreateFence(context.device, &fence_info, nullptr, &copy_fence));

	// staging buffer
	VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

	AllocatedBuffer staging = vkrsc::create_buffer(context.allocator, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

	m_mesh = vkrsc::create_buffer(context.allocator, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0);
	m_deletion_queue.deletors.push_back([this]()
	{
		vmaDestroyBuffer(context.allocator, m_mesh.buffer, m_mesh.allocation);
	});

	// map staging buffer <- mesh
	void* data;
	vmaMapMemory(context.allocator, staging.allocation, &data);
		memcpy(data, vertices.data(), (size_t)buffer_size);
	vmaUnmapMemory(context.allocator, staging.allocation);

	// copy staging buffer to mesh buffer
	VkCommandBufferAllocateInfo cmd_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = context.primary_command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer cmd_copy;
	vkAllocateCommandBuffers(context.device, &cmd_info, &cmd_copy);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkBeginCommandBuffer(cmd_copy, &begin_info);

	VkBufferCopy copy_info = {
		.size = static_cast<size_t>(buffer_size),
	};

	vkCmdCopyBuffer(cmd_copy, staging.buffer, m_mesh.buffer, 1, &copy_info);

	vkEndCommandBuffer(cmd_copy);

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd_copy,
	};

	vkQueueSubmit(context.queue, 1, &submit_info, copy_fence);

	vkWaitForFences(context.device, 1, &copy_fence, VK_TRUE, UINT64_MAX);
	vkDestroyFence(context.device, copy_fence, nullptr);

	std::cout << "Destroying staging buffer" << '\n';
	vmaDestroyBuffer(context.allocator, staging.buffer, staging.allocation);

	m_colors.colors[0] = glm::vec4(1.0f);
	m_colors.colors[1] = glm::vec4(1.0f);
	m_colors.colors[2] = glm::vec4(1.0f);
}

void Engine::init_imgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// required for dynamic rendering
	VkPipelineRenderingCreateInfo pipeline_rendering_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount 	 = 1,
		.pColorAttachmentFormats = &context.swapchain_dimensions.format
	};

	auto check_imgui_init = [](VkResult err)
	{
		std::cout << "imgui init \n";
		VK_CHECK(err);
	};

	ImGui_ImplGlfw_InitForVulkan(m_window, true);
	ImGui_ImplVulkan_InitInfo init_info = {
		.Instance = context.instance,
		.PhysicalDevice = context.gpu,
		.Device = context.device,
		.QueueFamily = context.graphics_queue_index,
		.Queue = context.queue,
		.MinImageCount = context.surface_properties.minImageCount,
		.ImageCount = static_cast<uint32_t>(context.swapchain_images.size()),
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.DescriptorPoolSize = 2,
		.UseDynamicRendering = true,
		.PipelineRenderingCreateInfo = pipeline_rendering_info,
	};

	ImGui_ImplVulkan_Init(&init_info);

	m_deletion_queue.deletors.push_back([]()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	});
	
}
