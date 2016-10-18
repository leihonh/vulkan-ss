#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
//stdexception �����쳣��functional ����lambda����
#include <stdexcept>
#include<functional>
#include <algorithm>
#include <iostream>
#include <vector>
//#include <map>
#include <cstring>
#include <set>

using namespace std;

const int WIDTH = 800;
const int HEIGHT = 800;
//������Ҫ��layer
const std::vector<const char*> validationLayers =
{
	"VK_LAYER_LUNARG_standard_validation"
};
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef  NDEBUG
const bool enableValidationLayers = false;
#else 
const bool enableValidationLayers = true;
#endif //  NDEBUG

//ע������ֻ��windows�µ�ʵ�֡�   ����һ���ص�����(callback)����debug��Ϣ���ص����ǵĳ����У�Ȼ����ʾ����
#ifndef WIN32
#define __stdcall
#endif
static VkBool32 __stdcall debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
	std::cerr << "validation layer: " << msg << std::endl;
	return VK_FALSE;
}


// VkDebugReportCallbackEXT ������Ҫ��vkDestroyDebugReportCallbackEXT���
void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

//�����Լ���callback  ��ʹ��vkGetInstanceProcAddr����ȡ��Ӧ�ĵ�ַ
VkResult CreateDebugReportCallbackEXT(VkInstance instance, const    VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {

		//cout << "callback  ::  " << func(instance, pCreateInfo, pAllocator, pCallback) << endl;
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}



template <typename T>  //T����һ������  
class VDeleter {
public:
	VDeleter() : VDeleter([](T, VkAllocationCallbacks*) {}) {}

	VDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef) {
		this->deleter = [=](T obj) { deletef(obj, nullptr); };
	}

	VDeleter(const VDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
		this->deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
	}

	VDeleter(const VDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
		this->deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
	}

	~VDeleter() {
		cleanup();
	}

	const T* operator &() const {
		return &object;
	}

	T* replace() {
		cleanup();
		return &object;
	}

	operator T() const {
		return object;
	}

	void operator=(T rhs) {
		if (rhs != object) {
			cleanup();
			object = rhs;
		}
	}

	template<typename V>
	bool operator==(V rhs) {
		return object == T(rhs);
	}

private:
	T object{ VK_NULL_HANDLE };
	std::function<void(T)> deleter;

	void cleanup() {
		if (object != VK_NULL_HANDLE) {
			deleter(object);
		}
		object = VK_NULL_HANDLE;
	}
};

class HelloTriangleApplication {
private:

	GLFWwindow* window;

	VDeleter<VkInstance> instance{ vkDestroyInstance };
	VDeleter<VkDebugReportCallbackEXT> callback{ instance, DestroyDebugReportCallbackEXT };
	VDeleter<VkSurfaceKHR> surface{ instance, vkDestroySurfaceKHR };

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VDeleter<VkDevice> device{ vkDestroyDevice };

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VDeleter<VkSwapchainKHR> swapChain{ device, vkDestroySwapchainKHR };
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct  QueueFamilyIndices
	{
		int graphicsFamily = -1;
		int presentFamily = -1;

		bool isComplete()
		{
			return graphicsFamily >= 0 && presentFamily >= 0;
			//return graphicsFamily >= 0;
		}

	};


	std::vector<const char*> getRequestExtensions()
	{
		std::vector<const char*> extensions;

		unsigned int glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		for (unsigned int i = 0; i < glfwExtensionCount; i++)
		{
			extensions.push_back(glfwExtensions[i]);
		}

		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		return extensions;
	}
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;

		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount ,nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount , queueFamilies.data());

	
		//cout <<"queueFamilyCount  " << queueFamilyCount << endl;   1

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);   //window  surface  support

			if (queueFamily.queueCount > 0 && presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete())
			{
				break;
			}

			i++;
		}

		return indices;

	}

	//����Ƿ�֧�ָ�����layer
	bool checkValidationLayersSupport()
	{
		  uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		cout << "layerCount  " << layerCount << endl;

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
			

            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
				//cout << "ini   "<< layerProperties.layerName << endl;
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
	}

	void initVulkan() {
		createInstance();

		setUpDebugCallback();//���ûص�����

		createSurface();

		pickPhysicalDevice();

		createLogicalDevice();

		createSwapChain();
	}

	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void createLogicalDevice()
	{
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

		float queuePriority = 1.0f;
		for (int queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		//VkDeviceQueueCreateInfo queueCreateInfo = {};
		//queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		//queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
		//queueCreateInfo.queueCount = 1;

		//queueCreateInfo.pQueuePriorities = &queuePriority;

		VkPhysicalDeviceFeatures deviceFeatures = {};//����Ĭ��ֵVK_FALSE

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		//createInfo.pQueueCreateInfos = &queueCreateInfo;
		//createInfo.queueCreateInfoCount = 1;
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
		createInfo.enabledExtensionCount = 0;

		createInfo.enabledExtensionCount = deviceExtensions.size();
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = validationLayers.size();
			createInfo.ppEnabledLayerNames = validationLayers.data();
		
		
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		cout <<"device " << vkCreateDevice(physicalDevice, &createInfo, nullptr, device.replace()) << endl;

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, device.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
	
	}
	//
	void pickPhysicalDevice()
	{//ѡ���Կ�
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);

		vkEnumeratePhysicalDevices(instance,&deviceCount , devices.data());

		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {

				physicalDevice = device;
				break;
			}
		}
		
			/*for (const auto& device : devices) {

			cout << isDeviceSuitable(device) << endl;
		}*/

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
		//isDeviceSuitable(physicalDevice);//�鿴�Ƿ��к��ʵ��Կ�
		

	}
	//���Ӽ��֧��deviceExtensions��չ�ĺ���checkDeviceExtensionSupport
	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount , nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device , surface , &formatCount , details.formats.data());

		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface , &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;

	}
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		// plan A
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
			return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}
		// plan B
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}
		// plan C
		return availableFormats[0];
	}
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			VkExtent2D actualExtent = { WIDTH, HEIGHT };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}
	return VK_PRESENT_MODE_FIFO_KHR;
	}

	void createSwapChain() {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR; 
		createInfo.surface = surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format; 
		createInfo.imageColorSpace = surfaceFormat.colorSpace; 
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice); uint32_t queueFamilyIndices[] = {
			(uint32_t)indices.graphicsFamily,
			(uint32_t)indices.presentFamily
		};

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;      
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; 
		//	createInfo.queueFamilyIndexCount = 0; // Optional  
			//createInfo.pQueueFamilyIndices = nullptr; // Optional 
		}
		
												  // ����Image  �任
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		// ���Ժ�����������ɫ���ʱ��Alpha ͨ��
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE; // ��������Щ���ڸǵ����� 

		createInfo.oldSwapchain = VK_NULL_HANDLE; // ��ʱ�����ÿռ���

		//cout << vkCreateSwapchainKHR(device, &createInfo, nullptr, swapChain.replace()) << endl;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, swapChain.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr); 
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;

	}


	bool isDeviceSuitable(VkPhysicalDevice device) {
		/**VkPhysicalDeviceProperties deviceProperties;//�Կ�������
		VkPhysicalDeviceFeatures deviceFeatures;// ����֧��

		vkGetPhysicalDeviceProperties(device,&deviceProperties);
		vkGetPhysicalDeviceFeatures(device,&deviceFeatures);

		//cout << "device " << deviceProperties.deviceType << endl;

		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && deviceFeatures.geometryShader;

		//return true;
		*/
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;



		//return indices.isComplete();

	}

	void mainloop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();//�����Ƿ�����꣬�����¼�
		}

	}
	void initWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "learn vulkan ", nullptr, nullptr);
	}

	void setUpDebugCallback()
	{
		if (!enableValidationLayers) return;

		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT; //error  warings
		createInfo.pfnCallback = debugCallback;   //���ûص�����


	//	cout << CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, callback.replace()) << "  VK_SUCCESS  "<< VK_SUCCESS << endl;

		if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, callback.replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug callback!");
		}

	}


	
	void createInstance()
	{
		if (enableValidationLayers && !checkValidationLayersSupport())
		{//�ж�validationlayers�Ƿ����
			cout << "layers" <<enableValidationLayers << endl;
			throw std::runtime_error("validation layers requested , but no available");
		}


		VkApplicationInfo appInfo = {};

		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "vulkan in vs 2015";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = validationLayers.size();
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledExtensionCount = 0;
		}

		/*unsigned int glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		

		cout << "glfwExtensionCount :  " << glfwExtensionCount << endl;
		*/

		auto extensions = getRequestExtensions();

		createInfo.enabledExtensionCount = extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();

		createInfo.enabledLayerCount = 0;
		VkResult result = vkCreateInstance(&createInfo,nullptr, instance.replace());

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance");
		}

		/*
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount , nullptr);
		cout << "extensionCount" << extensionCount << endl;

		std::vector<VkExtensionProperties> extensions(extensionCount);

		vkEnumerateInstanceExtensionProperties(nullptr , &extensionCount , extensions.data());

		std::cout << "availabe extensions" << std::endl;

		for (const auto& extension : extensions)
		{
			std::cout << "\t" << extension.extensionName << std::endl;
		}
		*/

	}

	

public:
	void run() {
		initWindow();
		initVulkan();
		mainloop();//rendering frame

		
	}
};

int main() {
	/*
	glfwInit();//��ʼ��GLFW
	//GLFW�����Ϊ����OPENGL �� context��Ƶģ�Ϊ�˱���������OPENGL context����Ҫ����
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);//����GLFW(ѡ������ƣ����õ�ֵ)
	//���ڶ���
	GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

	uint32_t extensionCount = 0;
	/*
	pLayerName�� ��һ��Validation layer,��������extension ,��������Ϊnullptr��
	pPropertyCount�� ����extension ��������
	pProperties : extension�ľ���ϸ�ڣ�������ֵΪnullptr,�򣬺���ֻ����extension����������pPropertyCount�����򷵻�pPropertyCount��VkExtensionProperties

	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::cout << extensionCount << " extensions supported" << std::endl;

	glm::mat4 matrix;
	glm::vec4 vec;
	auto test = matrix * vec;

	while (!glfwWindowShouldClose(window)) {
	glfwPollEvents();//�����û�д���ʲô�¼�������������롢����ƶ��ȣ�
	}

	glfwDestroyWindow(window);

	glfwTerminate();//�ͷ�GLFW������ڴ�
	*/
	HelloTriangleApplication app;

	try
	{
		app.run();
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;

	}

	return EXIT_SUCCESS;
}