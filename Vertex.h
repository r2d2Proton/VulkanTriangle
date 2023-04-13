#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <array>


struct Vertex
{
    glm::vec3 pos;
    glm::vec4 color;

    static VkVertexInputBindingDescription getBindingDescription();

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescription();
};
