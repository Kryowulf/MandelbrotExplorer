#include "pch.h"
#include "MandelbrotExplorerLib.h"
#include <vector>
#include <glm/glm.hpp>
#include <string>
#include "mandelbrot_parameters.h"

namespace MandelbrotExplorerLib
{
	using msclr::interop::marshal_as;

	MandelbrotRenderer::MandelbrotRenderer(System::IntPtr hinstance, System::IntPtr hwnd, bool debug)
	{
		try
		{
			_native_renderer = new vulkan_renderer((HINSTANCE)hinstance.ToPointer(), (HWND)hwnd.ToPointer(), debug);
			_native_renderer->load_fragment_shader(mandelbrot_parameter_info::MANDELBROT_FRAGMENT_SHADER, sizeof(mandelbrot_parameter_info));
		}
		catch (const std::runtime_error& err)
		{
			throw gcnew System::Exception(marshal_as<System::String^>(err.what()));
		}
	}

	MandelbrotRenderer::~MandelbrotRenderer()
	{
		if (!_disposed)
		{
			_native_renderer->dispose();
			_cachedMessages = GetDebugMessages();
			delete _native_renderer;
			_disposed = true;
		}
	}

	array<DebugMessage^>^ MandelbrotRenderer::GetDebugMessages()
	{
		if (_disposed)
			return _cachedMessages;

		const std::vector<debug_message>& nativeMessages = _native_renderer->debug_messages();
		int size = nativeMessages.size();
		array<DebugMessage^>^ managedMessages = gcnew array<DebugMessage^>(size);

		for (int i = 0; i < size; i++)
		{
			const debug_message& nativeMessage = nativeMessages[i];
			DebugMessage^ managedMessage = gcnew DebugMessage();
			
			managedMessage->Text = marshal_as<System::String^>(nativeMessage.text);
			managedMessage->Type = (DebugMessageType)nativeMessage.type;
			managedMessage->Severity = (DebugMessageSeverity)nativeMessage.severity;

			managedMessages[i] = managedMessage;
		}

		return managedMessages;
	}

	void MandelbrotRenderer::RefreshSurface()
	{
		_native_renderer->refresh_surface();
	}

	System::ValueTuple<System::UInt32, System::UInt32> MandelbrotRenderer::GetSurfaceExtent()
	{
		VkExtent2D extent = _native_renderer->surface_extent();
		return System::ValueTuple<System::UInt32, System::UInt32>(extent.width, extent.height);
	}

	void MandelbrotRenderer::Draw()
	{
		mandelbrot_parameter_info info;

		info.top = this->Top;
		info.left = this->Left;
		info.right = this->Right;
		info.bottom = this->Bottom;

		VkExtent2D extent = _native_renderer->surface_extent();
		info.surface_width = (float)extent.width;
		info.surface_height = (float)extent.height;

		info.bailout_radius = this->BailoutRadius;
		info.max_iterations = this->MaxIterations;
		info.fill_color = this->FillColor;
		info.gradient_period_factor = this->GradientPeriodFactor;

		int length = mandelbrot_parameter_info::GRADIENT_CAPACITY;
		length = std::min(length, this->Gradient->Length);

		info.gradient_length = length;

		for (int i = 0; i < length; i++)
			info.gradient[i] = this->Gradient[i];

		for (int i = length; i < mandelbrot_parameter_info::GRADIENT_CAPACITY; i++)
			info.gradient[i] = 0x00FF00;

		try
		{
			_native_renderer->draw_frame(&info);
		}
		catch (const std::runtime_error& err)
		{
			throw gcnew System::Exception(marshal_as<System::String^>(err.what()));
		}
	}
}