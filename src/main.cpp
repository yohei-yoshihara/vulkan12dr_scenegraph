#include "common.hpp"

#include "main.hpp"
#include "node.hpp"

#include "shapes/mesh_box.hpp"
#include "shapes/mesh_cone.hpp"
#include "shapes/mesh_plane.hpp"
#include "shapes/mesh_sphere.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <ranges>

void Engine::init_instance() {
  LOGI("Initializing Vulkan instance.");

  std::vector<const char *> required_instance_extensions{
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};

  uint32_t sdlExtensionCount = 0;
  auto sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
  std::copy(sdlExtensions, sdlExtensions + sdlExtensionCount,
            std::back_inserter(required_instance_extensions));

  vkb::InstanceBuilder builder;
  auto inst_ret = builder.set_app_name("Simple Scene Graph V1.2 + Direct Rendering")
                      .set_engine_name("No Engine")
                      .enable_extensions(required_instance_extensions)
                      .require_api_version(VK_MAKE_VERSION(1, 2, 0))
                      .build();
  if (!inst_ret) {
    throw std::runtime_error("failed to create instance");
  }
  context.instance = inst_ret.value();

  volkLoadInstance(context.instance);
}

void Engine::init_device() {
  LOGI("Initializing Vulkan device.");

  VkPhysicalDeviceSynchronization2Features enable_sync2_features = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
      .synchronization2 = VK_TRUE};
  VkPhysicalDeviceExtendedDynamicStateFeaturesEXT
      enable_extended_dynamic_state_features = {
          .sType =
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
          .pNext = &enable_sync2_features,
          .extendedDynamicState = VK_TRUE};

  VkPhysicalDeviceVulkan13Features features13 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &enable_extended_dynamic_state_features,
      .synchronization2 = VK_TRUE,
      .dynamicRendering = VK_TRUE,
  };

  VkPhysicalDeviceVulkan12Features features12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .pNext = &enable_extended_dynamic_state_features,
      .descriptorIndexing = VK_TRUE,
      .bufferDeviceAddress = VK_TRUE,
  };

  vkb::PhysicalDeviceSelector selector{context.instance};
  auto phys_ret = selector
                      .add_required_extensions({
                          VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                          VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
                          VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
                      })
                      .set_minimum_version(1, 2)
                      .set_required_features_13(features13)
                      .set_required_features_12(features12)
                      .set_surface(context.surface)
                      .select();
  if (!phys_ret) {
    LOGE("failed to select Vulkan Physical Device");
    return;
  }
  context.physicalDevice = phys_ret.value();

  vkb::DeviceBuilder device_builder{phys_ret.value()};
  auto dev_ret = device_builder.build();
  if (!dev_ret) {
    LOGE("Failed to create Vulkan device");
    throw std::runtime_error("Failed to create Vulkan device");
  }
  context.device = dev_ret.value();

  auto graphics_queue_ret = context.device.get_queue(vkb::QueueType::graphics);
  if (!graphics_queue_ret) {
    LOGE("Failed to get graphics queue");
    return;
  }
  context.graphics_queue_index =
      context.device.get_queue_index(vkb::QueueType::graphics).value();
  context.queue = graphics_queue_ret.value();

  VkCommandPoolCreateInfo cmd_pool_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
      .queueFamilyIndex = static_cast<uint32_t>(context.graphics_queue_index)};
  VK_CHECK(vkCreateCommandPool(context.device, &cmd_pool_info, nullptr,
                               &context.commandPool));

  VmaVulkanFunctions functions{
      .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
  };

  VmaAllocatorCreateInfo createInfo{
      .physicalDevice = context.physicalDevice,
      .device = context.device,
      .instance = context.instance,
      .pVulkanFunctions = &functions,
  };
  VK_CHECK(vmaCreateAllocator(&createInfo, &context.vma_allocator));
}

/**
 * Vertex Bufferの初期化
 */
void Engine::init_vertex_buffer() {
  for (const auto &node : nodes) {
    const auto &mesh = node->mesh();
    if (!context.meshBufferMap.contains(mesh)) {
      auto vertex = uploadBuffer(mesh->vertices().data(), mesh->vertices().size() * sizeof(Vertex),
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
      auto index = uploadBuffer(mesh->indices().data(), mesh->indices().size() * sizeof(IndexType),
                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
      MeshBuffer meshBuffer{vertex, index};
      context.meshBufferMap[mesh] = meshBuffer;
    }
  }
}

/**
 * Uniform Buffer Objectの初期化
 */
void Engine::init_ubo() {
  // DescriptorPoolの作成
  std::array<VkDescriptorPoolSize, 1> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  poolSizes[0].descriptorCount = context.swapchain.image_count;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = context.swapchain.image_count;

  VK_CHECK(vkCreateDescriptorPool(context.device, &poolInfo, nullptr,
                                  &context.descriptorPool));

  // DescriptorSetLayoutの作成
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = nullptr;

  std::array<VkDescriptorSetLayoutBinding, 1> bindings = {uboLayoutBinding};
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  VK_CHECK(vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr,
                                       &context.descriptorSetLayout));

  // Uniform Bufferの割り当て
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(context.physicalDevice, &properties);

  size_t min_ubo_alignment = properties.limits.minUniformBufferOffsetAlignment;
  std::cout << "min_ubo_alignment = " << min_ubo_alignment << std::endl;
  context.uboBufferSizePerNode = sizeof(UniformBufferObject);
  if (min_ubo_alignment > 0) {
    context.uboBufferSizePerNode =
        (context.uboBufferSizePerNode + min_ubo_alignment - 1) &
        ~(min_ubo_alignment - 1);
  }
  std::cout << "bufferSize = " << min_ubo_alignment << std::endl;
  VkDeviceSize totalBufferSize =
      MAX_NUMBER_OF_NODES * context.uboBufferSizePerNode;

  // フレーム毎のUniform Bufferを作成する
  for (std::size_t i = 0; i < context.swapchain.image_count; ++i) {
    auto &per_frame = context.per_frame[i];

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = totalBufferSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocationCreateInfo{};
    allocationCreateInfo.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VK_CHECK(vmaCreateBuffer(context.vma_allocator, &bufferCreateInfo,
                             &allocationCreateInfo, &per_frame.uniformBuffer,
                             &per_frame.uniformBufferAllocation, nullptr));
  }

  // DescriptorSetsを作成する
  std::vector<VkDescriptorSetLayout> layouts(context.swapchain.image_count,
                                             context.descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = context.descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
  allocInfo.pSetLayouts = layouts.data();

  std::vector<VkDescriptorSet> descriptorSets(context.swapchain.image_count,
                                              VK_NULL_HANDLE);
  VK_CHECK(vkAllocateDescriptorSets(context.device, &allocInfo,
                                    descriptorSets.data()));

  // DescriptorSetsを更新する
  for (size_t i = 0; i < context.swapchain.image_count; ++i) {
    auto &per_frame = context.per_frame[i];
    per_frame.descriptorSet = descriptorSets[i];

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = per_frame.uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = per_frame.descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(context.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}

/**
 * UBOの更新
 */
void Engine::update_ubo(PerFrame &per_frame) {
  for (size_t i = 0; i < nodes.size(); ++i) {
    UniformBufferObject ubo{};
    auto model = nodes[i]->worldMatrix();
    auto view = glm::lookAt(glm::vec3(1.2f, 1.2f, 1.0f), // eye
                            glm::vec3(0.0f, 0.0f, 0.0f), // center
                            glm::vec3(0.0f, 0.0f, 1.0f)  // up
    );
    auto proj =
        glm::perspective(glm::radians(60.0f), // fov
                         static_cast<float>(context.swapchain.extent.width) /
                             context.swapchain.extent.height, // aspect ratio
                         0.1f,                                // near
                         10.0f                                // far
        );
    proj[1][1] *= -1;
    ubo.mvpMatrix = proj * view * model;

    ubo.light = glm::vec4(5.0f, 5.0f, 5.0f, 1.0);

    VkDeviceSize offset = i * context.uboBufferSizePerNode;
    VK_CHECK(vmaCopyMemoryToAllocation(context.vma_allocator, &ubo,
                                       per_frame.uniformBufferAllocation,
                                       offset, sizeof(ubo)));
  }
}

void Engine::init_per_frame(PerFrame &per_frame) {
  VkFenceCreateInfo info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                         .flags = VK_FENCE_CREATE_SIGNALED_BIT};
  VK_CHECK(vkCreateFence(context.device, &info, nullptr,
                         &per_frame.queue_submit_fence));

  VkCommandPoolCreateInfo cmd_pool_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
      .queueFamilyIndex = static_cast<uint32_t>(context.graphics_queue_index)};
  VK_CHECK(vkCreateCommandPool(context.device, &cmd_pool_info, nullptr,
                               &per_frame.primary_command_pool));

  VkCommandBufferAllocateInfo cmd_buf_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = per_frame.primary_command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};
  VK_CHECK(vkAllocateCommandBuffers(context.device, &cmd_buf_info,
                                    &per_frame.primary_command_buffer));
}

void Engine::teardown_per_frame(PerFrame &per_frame) {
  if (per_frame.queue_submit_fence != VK_NULL_HANDLE) {
    vkDestroyFence(context.device, per_frame.queue_submit_fence, nullptr);

    per_frame.queue_submit_fence = VK_NULL_HANDLE;
  }

  if (per_frame.primary_command_buffer != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(context.device, per_frame.primary_command_pool, 1,
                         &per_frame.primary_command_buffer);

    per_frame.primary_command_buffer = VK_NULL_HANDLE;
  }

  if (per_frame.primary_command_pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(context.device, per_frame.primary_command_pool,
                         nullptr);

    per_frame.primary_command_pool = VK_NULL_HANDLE;
  }

  if (per_frame.swapchain_acquire_semaphore != VK_NULL_HANDLE) {
    vkDestroySemaphore(context.device, per_frame.swapchain_acquire_semaphore,
                       nullptr);

    per_frame.swapchain_acquire_semaphore = VK_NULL_HANDLE;
  }

  if (per_frame.swapchain_release_semaphore != VK_NULL_HANDLE) {
    vkDestroySemaphore(context.device, per_frame.swapchain_release_semaphore,
                       nullptr);

    per_frame.swapchain_release_semaphore = VK_NULL_HANDLE;
  }

  if (per_frame.uniformBuffer != VK_NULL_HANDLE) {
    vmaDestroyBuffer(context.vma_allocator, per_frame.uniformBuffer,
                     per_frame.uniformBufferAllocation);
    per_frame.uniformBuffer = VK_NULL_HANDLE;
    per_frame.uniformBufferAllocation = VK_NULL_HANDLE;
  }
}

void Engine::init_swapchain() {
  vkb::SwapchainBuilder swapchain_builder{context.device};
  auto swap_ret =
      swapchain_builder.set_old_swapchain(context.swapchain).build();
  if (!swap_ret) {
    LOGE("failed to create swapchain");
    return;
  }
  vkb::destroy_swapchain(context.swapchain);
  context.swapchain = swap_ret.value();
  uint32_t image_count = context.swapchain.image_count;

  context.swapchain_dimensions = {context.swapchain.extent.width,
                                  context.swapchain.extent.height,
                                  context.swapchain.image_format};

  context.swapchain_images = context.swapchain.get_images().value();

  context.per_frame.clear();
  context.per_frame.resize(image_count);
  for (size_t i = 0; i < image_count; i++) {
    init_per_frame(context.per_frame[i]);
  }

  context.swapchain_image_views = context.swapchain.get_image_views().value();
}

static std::vector<char> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

VkShaderModule Engine::load_shader_module(const char *path) {
  std::vector<char> spirv = readFile(path);
  VkShaderModuleCreateInfo module_info{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = spirv.size(),
      .pCode = reinterpret_cast<const uint32_t *>(spirv.data())};

  VkShaderModule shader_module;
  VK_CHECK(vkCreateShaderModule(context.device, &module_info, nullptr,
                                &shader_module));

  return shader_module;
}

void Engine::init_pipeline() {
  VkPipelineLayoutCreateInfo layout_info{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  layout_info.setLayoutCount = 1;
  layout_info.pSetLayouts = &context.descriptorSetLayout;
  VK_CHECK(vkCreatePipelineLayout(context.device, &layout_info, nullptr,
                                  &context.pipeline_layout));

  VkVertexInputBindingDescription binding_description{
      .binding = 0,
      .stride = sizeof(Vertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

  std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions = {{
      {.location = 0,
       .binding = 0,
       .format = VK_FORMAT_R32G32B32_SFLOAT,
       .offset = offsetof(Vertex, position)}, // position
      {.location = 1,
       .binding = 0,
       .format = VK_FORMAT_R32G32B32_SFLOAT,
       .offset = offsetof(Vertex, normal)}, // normal
      {.location = 2,
       .binding = 0,
       .format = VK_FORMAT_R32G32B32_SFLOAT,
       .offset = offsetof(Vertex, color)}, // color
  }};

  VkPipelineVertexInputStateCreateInfo vertex_input{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &binding_description,
      .vertexAttributeDescriptionCount =
          static_cast<uint32_t>(attribute_descriptions.size()),
      .pVertexAttributeDescriptions = attribute_descriptions.data()};

  VkPipelineInputAssemblyStateCreateInfo input_assembly{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE};

  VkPipelineRasterizationStateCreateInfo raster{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f};

  std::vector<VkDynamicState> dynamic_states = {
      VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
      VK_DYNAMIC_STATE_CULL_MODE, VK_DYNAMIC_STATE_FRONT_FACE,
      VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY};

  VkPipelineColorBlendAttachmentState blend_attachment{
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

  VkPipelineColorBlendStateCreateInfo blend{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &blend_attachment};

  VkPipelineViewportStateCreateInfo viewport{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1};

  VkPipelineDepthStencilStateCreateInfo depth_stencil{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS,
      .depthBoundsTestEnable = VK_FALSE,
      .minDepthBounds = 0.0f,
      .maxDepthBounds = 1.0f,
      .stencilTestEnable = VK_FALSE};

  VkPipelineMultisampleStateCreateInfo multisample{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};

  VkPipelineDynamicStateCreateInfo dynamic_state_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
      .pDynamicStates = dynamic_states.data()};

  std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {{
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .module = load_shader_module("shaders/triangle.vert.spv"),
       .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .module = load_shader_module("shaders/triangle.frag.spv"),
       .pName = "main"}
  }};

  VkPipelineRenderingCreateInfo pipeline_rendering_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &context.swapchain_dimensions.format,
      .depthAttachmentFormat = context.depthFormat};

  VkGraphicsPipelineCreateInfo pipe{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &pipeline_rendering_info,
      .stageCount = static_cast<uint32_t>(shader_stages.size()),
      .pStages = shader_stages.data(),
      .pVertexInputState = &vertex_input,
      .pInputAssemblyState = &input_assembly,
      .pViewportState = &viewport,
      .pRasterizationState = &raster,
      .pMultisampleState = &multisample,
      .pDepthStencilState = &depth_stencil,
      .pColorBlendState = &blend,
      .pDynamicState = &dynamic_state_info,
      .layout = context.pipeline_layout,
      .renderPass = VK_NULL_HANDLE,
      .subpass = 0,
  };

  VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipe,
                                     nullptr, &context.pipeline));

  vkDestroyShaderModule(context.device, shader_stages[0].module, nullptr);
  vkDestroyShaderModule(context.device, shader_stages[1].module, nullptr);
}

VkResult Engine::acquire_next_swapchain_image(uint32_t *image) {
  VkSemaphore acquire_semaphore;
  if (context.recycled_semaphores.empty()) {
    VkSemaphoreCreateInfo info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VK_CHECK(
        vkCreateSemaphore(context.device, &info, nullptr, &acquire_semaphore));
  } else {
    acquire_semaphore = context.recycled_semaphores.back();
    context.recycled_semaphores.pop_back();
  }

  VkResult res =
      vkAcquireNextImageKHR(context.device, context.swapchain, UINT64_MAX,
                            acquire_semaphore, VK_NULL_HANDLE, image);

  if (res != VK_SUCCESS) {
    context.recycled_semaphores.push_back(acquire_semaphore);
    return res;
  }

  if (context.per_frame[*image].queue_submit_fence != VK_NULL_HANDLE) {
    vkWaitForFences(context.device, 1,
                    &context.per_frame[*image].queue_submit_fence, true,
                    UINT64_MAX);
    vkResetFences(context.device, 1,
                  &context.per_frame[*image].queue_submit_fence);
  }

  if (context.per_frame[*image].primary_command_pool != VK_NULL_HANDLE) {
    vkResetCommandPool(context.device,
                       context.per_frame[*image].primary_command_pool, 0);
  }

  VkSemaphore old_semaphore =
      context.per_frame[*image].swapchain_acquire_semaphore;

  if (old_semaphore != VK_NULL_HANDLE) {
    context.recycled_semaphores.push_back(old_semaphore);
  }

  context.per_frame[*image].swapchain_acquire_semaphore = acquire_semaphore;

  return VK_SUCCESS;
}

void Engine::render(uint32_t swapchain_index) {
  VkCommandBuffer cmd =
      context.per_frame[swapchain_index].primary_command_buffer;

  VkCommandBufferBeginInfo begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

  VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

  transitionImageLayout(
      cmd, context.swapchain_images[swapchain_index], VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      0,
      VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,         
      VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,            
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT 
  );
  VkClearValue clear_value{.color = {{0.01f, 0.01f, 0.033f, 1.0f}}};

  VkRenderingAttachmentInfo color_attachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = context.swapchain_image_views[swapchain_index],
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = clear_value};

  VkClearValue depthClearValue = {{1.0f, 0.0f}};
  VkRenderingAttachmentInfo depth_attachment{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = context.depthImageView,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = depthClearValue};

  VkRenderingInfo rendering_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
      .renderArea =
          {                  
           .offset = {0, 0}, 
           .extent =
               {
                .width = context.swapchain_dimensions.width,
                .height = context.swapchain_dimensions.height}},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment,
      .pDepthAttachment = &depth_attachment};

  vkCmdBeginRenderingKHR(cmd, &rendering_info);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline);

  VkViewport vp{.width = static_cast<float>(context.swapchain_dimensions.width),
                .height =
                    static_cast<float>(context.swapchain_dimensions.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f};

  vkCmdSetViewport(cmd, 0, 1, &vp);

  VkRect2D scissor{.extent = {.width = context.swapchain_dimensions.width,
                              .height = context.swapchain_dimensions.height}};

  vkCmdSetScissor(cmd, 0, 1, &scissor);

  vkCmdSetCullModeEXT(cmd, VK_CULL_MODE_NONE);

  vkCmdSetFrontFaceEXT(cmd, VK_FRONT_FACE_CLOCKWISE);

  vkCmdSetPrimitiveTopologyEXT(cmd, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

  for (std::size_t i = 0; i < nodes.size(); ++i) {
    const auto &node = nodes[i];
    const auto &meshBuffer = context.meshBufferMap[node->mesh()];
    const auto &vertexBuffer = meshBuffer.vertexBuffer;
    VkDeviceSize offset = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer.buffer, &offset);
    const auto &indexBuffer = meshBuffer.indexBuffer;
    vkCmdBindIndexBuffer(cmd, indexBuffer.buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    uint32_t dynamic_offset =
        static_cast<uint32_t>(i * context.uboBufferSizePerNode);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline_layout, 0, 1,
        &context.per_frame[swapchain_index].descriptorSet, 1, &dynamic_offset);
    vkCmdDrawIndexed(
          cmd,
          static_cast<uint32_t>(node->mesh()->numberOfIndices()), 1, 0, 0,
          0);
  }

  vkCmdEndRenderingKHR(cmd);

  transitionImageLayout(
      cmd, context.swapchain_images[swapchain_index],
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,          // srcAccessMask
      0,                                               // dstAccessMask
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStage
      VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT           // dstStage
  );

  VK_CHECK(vkEndCommandBuffer(cmd));

  if (context.per_frame[swapchain_index].swapchain_release_semaphore ==
      VK_NULL_HANDLE) {
    VkSemaphoreCreateInfo semaphore_info{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VK_CHECK(vkCreateSemaphore(
        context.device, &semaphore_info, nullptr,
        &context.per_frame[swapchain_index].swapchain_release_semaphore));
  }

  VkPipelineStageFlags wait_stage{VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT};

  VkSubmitInfo info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores =
          &context.per_frame[swapchain_index].swapchain_acquire_semaphore,
      .pWaitDstStageMask = &wait_stage,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores =
          &context.per_frame[swapchain_index].swapchain_release_semaphore};

  VK_CHECK(
      vkQueueSubmit(context.queue, 1, &info,
                    context.per_frame[swapchain_index].queue_submit_fence));
}

VkResult Engine::present_image(uint32_t index) {
  VkSwapchainKHR swapChains[] = {context.swapchain};
  VkPresentInfoKHR present{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &context.per_frame[index].swapchain_release_semaphore,
      .swapchainCount = 1,
      .pSwapchains = swapChains,
      .pImageIndices = &index,
  };
  return vkQueuePresentKHR(context.queue, &present);
}

void Engine::transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                     VkImageLayout oldLayout,
                                     VkImageLayout newLayout,
                                     VkAccessFlags2 srcAccessMask,
                                     VkAccessFlags2 dstAccessMask,
                                     VkPipelineStageFlags2 srcStage,
                                     VkPipelineStageFlags2 dstStage) {
  VkImageMemoryBarrier2 image_barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = srcStage,       
      .srcAccessMask = srcAccessMask, 
      .dstStageMask = dstStage,       
      .dstAccessMask = dstAccessMask, 

      .oldLayout = oldLayout,
      .newLayout = newLayout,

      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

      .image = image,

      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .levelCount = 1,     
          .baseArrayLayer = 0, 
          .layerCount = 1      
      }};

  VkDependencyInfo dependency_info{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .dependencyFlags = 0,        
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers =
          &image_barrier
  };
  vkCmdPipelineBarrier2KHR(cmd, &dependency_info);
}

Engine::~Engine() {
  if (context.device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(context.device);
  }

  for (auto &per_frame : context.per_frame) {
    teardown_per_frame(per_frame);
  }

  context.per_frame.clear();

  for (auto semaphore : context.recycled_semaphores) {
    vkDestroySemaphore(context.device, semaphore, nullptr);
  }

  if (context.pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(context.device, context.pipeline, nullptr);
  }

  if (context.pipeline_layout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(context.device, context.pipeline_layout, nullptr);
  }

  for (VkImageView image_view : context.swapchain_image_views) {
    vkDestroyImageView(context.device, image_view, nullptr);
  }

  vkb::destroy_swapchain(context.swapchain);

  if (context.surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
    context.surface = VK_NULL_HANDLE;
  }

  if (context.descriptorSetLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(context.device, context.descriptorSetLayout,
                                 nullptr);
    context.descriptorSetLayout = VK_NULL_HANDLE;
  }
  if (context.descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(context.device, context.descriptorPool, nullptr);
    context.descriptorPool = VK_NULL_HANDLE;
  }

  vkDestroyImageView(context.device, context.depthImageView, nullptr);
  vmaDestroyImage(context.vma_allocator, context.depthImage,
                  context.depthAllocation);

  for (auto &pair: context.meshBufferMap) {
    vmaDestroyBuffer(context.vma_allocator, pair.second.vertexBuffer.buffer, pair.second.vertexBuffer.allocation);
    vmaDestroyBuffer(context.vma_allocator, pair.second.indexBuffer.buffer, pair.second.indexBuffer.allocation);
  }

  vmaDestroyAllocator(context.vma_allocator);

  if (context.commandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(context.device, context.commandPool, nullptr);
  }

  if (context.device != VK_NULL_HANDLE) {
    vkb::destroy_device(context.device);
  }

  SDL_DestroyWindow(context.window);
  SDL_Quit();
}

VkFormat Engine::findSupportedFormat(const std::vector<VkFormat> &candidates,
                                     VkImageTiling tiling,
                                     VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(context.physicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  throw std::runtime_error("failed to find supported format!");
}

VkFormat Engine::findDepthFormat() {
  return findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void Engine::init_depth() {
  context.depthFormat = findDepthFormat();

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent = {context.swapchain_dimensions.width,
                      context.swapchain_dimensions.height, 1};
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = context.depthFormat;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocCreateInfo{};
  allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  allocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  VK_CHECK(vmaCreateImage(context.vma_allocator, &imageInfo, &allocCreateInfo,
                          &context.depthImage, &context.depthAllocation,
                          nullptr));

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = context.depthImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = context.depthFormat;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VK_CHECK(vkCreateImageView(context.device, &viewInfo, nullptr,
                             &context.depthImageView));
}

bool Engine::prepare() {
  if (volkInitialize() != VK_SUCCESS) {
    throw std::runtime_error("failed to initialize volk");
  }
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    throw std::runtime_error("failed to initialize SDL");
  }
  context.window = SDL_CreateWindow("Triangle13", 800, 600, SDL_WINDOW_VULKAN);
  if (context.window == nullptr) {
    throw std::runtime_error("failed to create window");
  }

  init_instance();

  if (!SDL_Vulkan_CreateSurface(context.window, context.instance, nullptr,
                                &context.surface)) {
    throw std::runtime_error("failed to create surface");
  }

  context.swapchain_dimensions.width = 800;
  context.swapchain_dimensions.height = 600;

  if (!context.surface) {
    throw std::runtime_error("Failed to create window surface.");
  }

  init_device();

  init_vertex_buffer();

  init_swapchain();

  init_ubo();

  init_depth();

  init_pipeline();

  return true;
}

void Engine::mainLoop() {
  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (event.type == SDL_EVENT_WINDOW_RESIZED) {
      }
    }
    update();
  }
  vkDeviceWaitIdle(context.device);
}

void Engine::update() {
  auto res = acquire_next_swapchain_image(&context.currentIndex);

  if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR) {
    if (!resize(context.swapchain_dimensions.width,
                context.swapchain_dimensions.height)) {
      LOGI("Resize failed");
    }
    res = acquire_next_swapchain_image(&context.currentIndex);
  }

  if (res != VK_SUCCESS) {
    vkQueueWaitIdle(context.queue);
    return;
  }

  update_ubo(context.per_frame[context.currentIndex]);
  render(context.currentIndex);
  res = present_image(context.currentIndex);

  if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR) {
    if (!resize(context.swapchain_dimensions.width,
                context.swapchain_dimensions.height)) {
      LOGI("Resize failed");
    }
  } else if (res != VK_SUCCESS) {
    LOGE("Failed to present swapchain image.");
  }
}

bool Engine::resize(const uint32_t, const uint32_t) {
  if (context.device == VK_NULL_HANDLE) {
    return false;
  }

  VkSurfaceCapabilitiesKHR surface_properties;
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      context.physicalDevice, context.surface, &surface_properties));

  if (surface_properties.currentExtent.width ==
          context.swapchain_dimensions.width &&
      surface_properties.currentExtent.height ==
          context.swapchain_dimensions.height) {
    return false;
  }

  vkDeviceWaitIdle(context.device);

  init_swapchain();
  return true;
}

VkSurfaceFormatKHR
Engine::selectSurfaceFormat(VkPhysicalDevice gpu, VkSurfaceKHR surface,
                              std::vector<VkFormat> const &preferred_formats) {
  uint32_t surface_format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &surface_format_count,
                                       nullptr);
  assert(0 < surface_format_count);
  std::vector<VkSurfaceFormatKHR> supported_surface_formats(
      surface_format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &surface_format_count,
                                       supported_surface_formats.data());

  auto it = std::ranges::find_if(
      supported_surface_formats,
      [&preferred_formats](VkSurfaceFormatKHR surface_format) {
        return std::ranges::any_of(preferred_formats,
                                   [&surface_format](VkFormat format) {
                                     return format == surface_format.format;
                                   });
      });

  return it != supported_surface_formats.end() ? *it
                                               : supported_surface_formats[0];
}

VkCommandBuffer Engine::beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = context.commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(context.device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void Engine::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(context.queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(context.queue);

  vkFreeCommandBuffers(context.device, context.commandPool, 1,
                       &commandBuffer);
}

AllocatedBuffer Engine::createBuffer(VkDeviceSize size,
                                     VkBufferUsageFlags usage,
                                     VmaMemoryUsage memoryUsage) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocationInfo{};
  allocationInfo.usage = memoryUsage;

  VkBuffer buffer;
  VmaAllocation allocation;
  VK_CHECK(vmaCreateBuffer(context.vma_allocator, &bufferInfo, &allocationInfo,
                           &buffer, &allocation, nullptr));

  return {buffer, allocation};
}

void Engine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                        VkDeviceSize size) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}

AllocatedBuffer Engine::uploadBuffer(const void *srcData, VkDeviceSize size,
                                     VkBufferUsageFlags usage) {
  auto staging = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VMA_MEMORY_USAGE_CPU_ONLY);
  VK_CHECK(vmaCopyMemoryToAllocation(context.vma_allocator, srcData,
                                     staging.allocation, 0, size));
  auto gpu = createBuffer(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY);
  copyBuffer(staging.buffer, gpu.buffer, size);
  vmaDestroyBuffer(context.vma_allocator, staging.buffer, staging.allocation);
  return gpu;
}

void Engine::addNode(const std::shared_ptr<Node> &node) {
  nodes.push_back(node);
}

int main() {

  Engine engine;
  {
    auto mesh = Sphere::generate(0.5, 32, 32);
    mesh->setColor(glm::vec3(0, 0, 1));
    auto node = std::make_shared<Node>(mesh);
    node->setPosition(glm::vec3(0, 0, 0));
    node->setEulerAngle(glm::vec3(0, 0, 0));
    engine.addNode(node);
  }
  {
    auto mesh = Plane::generate(2, 2, UpAxis::Z, 1, 1);
    mesh->setColor(glm::vec3(0, 1, 0));
    auto node = std::make_shared<Node>(mesh);
    node->setPosition(glm::vec3(0, 0, -0.5));
    node->setEulerAngle(glm::vec3(0, 0, 0));
    engine.addNode(node);
  }
  engine.prepare();
  engine.mainLoop();
  return 0;
}