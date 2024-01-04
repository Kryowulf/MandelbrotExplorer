#pragma once
#include "pch.h"
#include <glm/glm.hpp>

struct mandelbrot_parameter_info
{
	static const std::string MANDELBROT_FRAGMENT_SHADER;

	glm::float32 top;			
	glm::float32 left;			
	glm::float32 right;			
	glm::float32 bottom;		
	glm::float32 surface_width;		
	glm::float32 surface_height;	
	glm::float32 bailout_radius;
	glm::uint max_iterations;		
	glm::uint fill_color;		
	glm::float32 gradient_period_factor;	
	glm::uint gradient_length;

	static const int GRADIENT_CAPACITY = 21;
	glm::uint gradient[GRADIENT_CAPACITY];

	mandelbrot_parameter_info()
	{
		if (sizeof(mandelbrot_parameter_info) > 128)
		{
			throw std::runtime_error("Mandelbrot parameters size exceeds Vulkan push constant limit.");
		}
	}
};
