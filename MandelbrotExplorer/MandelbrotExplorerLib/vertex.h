#pragma once
#include "pch.h"
#include <glm/glm.hpp>
#include <array>

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		// All our per-vertex data is packed together in one array,
		// so we're only going to have one binding.

		// The binding parameter specified the index of the binding
		// in the array of bindings.

		// The stride parameter specifies the number of bytes from one 
		// entry to the next.

		// The inputRate parameter can have one of the following values:
		//	VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
		//	VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance.
		//
		// Tutorial does not use instanced rendering.

		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};