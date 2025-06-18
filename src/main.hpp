#pragma once

#include <VkBootstrap.h>

#include <glm/glm.hpp>
#include <vulkan/vk_enum_string_helper.h>
// #include <vulkan/vulkan.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "common.hpp"
#include "types.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

class Node;
class Mesh;

struct AllocatedBuffer {
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation allocation = VK_NULL_HANDLE;
};

struct MeshBuffer {
  AllocatedBuffer vertexBuffer;
  AllocatedBuffer indexBuffer;
};

class Engine {
  const int MAX_NUMBER_OF_NODES = 32;

  struct UniformBufferObject {
    glm::vec4 light;
    glm::mat4 mvpMatrix;
  };

  struct SwapchainDimensions {
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
  };

  struct PerFrame {
    VkFence queue_submit_fence = VK_NULL_HANDLE;
    VkCommandPool primary_command_pool = VK_NULL_HANDLE;
    VkCommandBuffer primary_command_buffer = VK_NULL_HANDLE;
    VkSemaphore swapchain_acquire_semaphore = VK_NULL_HANDLE;
    VkSemaphore swapchain_release_semaphore = VK_NULL_HANDLE;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkBuffer uniformBuffer = VK_NULL_HANDLE;
    VmaAllocation uniformBufferAllocation = VK_NULL_HANDLE;
  };

  struct Context {
    // Base resources
    vkb::Instance instance;
    SDL_Window *window = nullptr;
    vkb::PhysicalDevice physicalDevice;
    vkb::Device device;
    VkQueue queue = VK_NULL_HANDLE;
    vkb::Swapchain swapchain;
    SwapchainDimensions swapchain_dimensions;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    int32_t graphics_queue_index = -1;
    std::vector<VkImageView> swapchain_image_views;
    std::vector<VkImage> swapchain_images;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    std::vector<VkSemaphore> recycled_semaphores;
    std::vector<PerFrame> per_frame;
    uint32_t currentIndex = -1;

    // command pool for transfer
    VkCommandPool commandPool = VK_NULL_HANDLE;

    // VMA
    VmaAllocator vma_allocator = VK_NULL_HANDLE;

    // Vertex Buffer
    std::unordered_map<std::shared_ptr<Mesh>, MeshBuffer> meshBufferMap;

    // UBO
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    // ubo buffer size for each node
    std::size_t uboBufferSizePerNode = 0;

    // depth resources
    VkFormat depthFormat;
    VkImage depthImage;
    VmaAllocation depthAllocation;
    VkImageView depthImageView;
  };

public:
  Engine() = default;

  ~Engine();

  bool prepare();

  void mainLoop();

  void update();

  bool resize(const uint32_t width, const uint32_t height);

  void init_instance();

  void init_device();

  void init_vertex_buffer();

  void init_ubo();

  void update_ubo(PerFrame &per_frame);

  void init_per_frame(PerFrame &per_frame);

  void teardown_per_frame(PerFrame &per_frame);

  void init_swapchain();

  VkShaderModule load_shader_module(const char *path);

  void init_pipeline();

  void init_depth();

  VkResult acquire_next_swapchain_image(uint32_t *image);

  void render(uint32_t swapchain_index);

  VkResult present_image(uint32_t index);

  // Utility Methos
  void transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                             VkImageLayout oldLayout, VkImageLayout newLayout,
                             VkAccessFlags2 srcAccessMask,
                             VkAccessFlags2 dstAccessMask,
                             VkPipelineStageFlags2 srcStage,
                             VkPipelineStageFlags2 dstStage);

  VkSurfaceFormatKHR
  selectSurfaceFormat(VkPhysicalDevice gpu, VkSurfaceKHR surface,
                      std::vector<VkFormat> const &preferred_formats = {
                          VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB,
                          VK_FORMAT_A8B8G8R8_SRGB_PACK32});

  VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features);
  VkFormat findDepthFormat();

  // ワンショットコマンドバッファの開始
  VkCommandBuffer beginSingleTimeCommands();

  // ワンショットコマンドバッファの終了
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);

  // バッファの作成
  AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                               VmaMemoryUsage memoryUsage);

  // バッファーのコピー
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

  // バッファの作成と初期データの設定
  AllocatedBuffer uploadBuffer(const void *data, VkDeviceSize size,
                               VkBufferUsageFlags usage = 0);

  // add a node to scene graph
  void addNode(const std::shared_ptr<Node> &node);

private:
  Context context;
  std::vector<std::shared_ptr<Node>> nodes;
};
