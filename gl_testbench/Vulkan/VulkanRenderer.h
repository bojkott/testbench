#pragma once
#include <SDL.h>
#include <vulkan\vulkan.h>
#include "../Renderer.h"


#pragma comment(lib, "vulkan-1.lib")
#ifdef _DEBUG
	#pragma comment(lib, "shaderc_combined_debug.lib")
#else
	#pragma comment(lib, "shaderc_combined.lib")
#endif

#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2main.lib")

#include <algorithm>


class VulkanRenderer :
	public Renderer
{
public:

	static VkDevice device;
	static VkExtent2D swapChainExtent;
	static VkFormat swapChainImageFormat;
	static VkRenderPass renderPass;
	static VkPhysicalDevice physicalDevice;
	static VkPipelineLayout pipelineLayout;
	static VkDescriptorSet descriptorSet;
	static VkCommandBuffer* currentBuffer;
	static VkCommandPool commandPool;
	static VkQueue graphicsQueue;
	VulkanRenderer();
	~VulkanRenderer();

	Material* makeMaterial(const std::string& name);
	Mesh* makeMesh();
	VertexBuffer* makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage) ;
	Texture2D* makeTexture2D();
	Sampler2D* makeSampler2D();
	RenderState* makeRenderState();
	std::string getShaderPath();
	std::string getShaderExtension();
	ConstantBuffer* makeConstantBuffer(std::string NAME, unsigned int location);
	Technique* makeTechnique(Material*, RenderState*);

	int initialize(unsigned int width = 800, unsigned int height = 600);
	void setWinTitle(const char* title);
	void present();
	int shutdown();

	void setClearColor(float, float, float, float);
	void clearBuffer(unsigned int);
	void setRenderState(RenderState* ps);
	void submit(Mesh* mesh);
	void frame();

private:
	#ifdef _DEBUG
		const bool enableValidationLayers = true;
	#else
		const bool enableValidationLayers = false;
	#endif
	#define FAILED(x) x != VK_SUCCESS

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	struct QueueFamilyIndices {
		int graphicsFamily = -1;
		int presentFamily = -1;
		bool isComplete()
		{
			return graphicsFamily >= 0 && presentFamily >= 0;
		}	
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData);

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	VkResult CreateDebugReportCallbackEXT(
		VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);

	void DestroyDebugReportCallbackEXT(
		VkInstance instance, VkDebugReportCallbackEXT callback,
		const VkAllocationCallbacks* pAllocator);


	SDL_Window* window;
	VkInstance instance;
	VkDebugReportCallbackEXT callback;

	uint32_t height;
	uint32_t width;
	VkQueue presentQueue;
	VkSurfaceKHR surface;

	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;

	std::vector<VkImageView> swapChainImageViews;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	

	std::vector<VkCommandBuffer> commandBuffers;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	std::vector<Mesh*> drawList;
	VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

	void initWindow(unsigned int width, unsigned int height);
	void initVulkan();
	void createInstance();
	void createLogicalDevice();
	void createSwapChain();
	void setupDebugCallback();
	void createCommandBuffers();
	bool checkValidationLayersSupport();
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	std::vector<const char*> getRequiredExtensions();
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	void createSurface();
	//Create image views from the avalible swapchain
	void createImageViews();
	void createRenderPass();
	void createCommandPool();
	void createFrameBufffers();
	void createSemaphores();
	void createDescriptorPool();
	void createDescriptorSetLayout();
	void createDescriptorSet();
	void createPipelineLayout();


	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

};
