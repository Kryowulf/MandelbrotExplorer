#pragma once
#include "mandelbrot_native.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Drawing;

namespace MandelbrotExplorerLib 
{
	public enum class DebugMessageSeverity : System::UInt32
	{
		Verbose = 0x00000001,
		Info = 0x00000010,
		Warning = 0x00000100,
		Error = 0x00001000
	};

	public enum class DebugMessageType : System::UInt32
	{
		General = 0x00000001,
		Validation = 0x00000002,
		Performance = 0x00000004,
		DeviceAddressBinding = 0x00000008
	};

	public ref class DebugMessage
	{
	public:
		property String^ Text;
		property DebugMessageSeverity Severity;
		property DebugMessageType Type;
	};

	public ref class MandelbrotRenderer : System::IDisposable
	{

	public:

		MandelbrotRenderer(System::IntPtr hinstance, System::IntPtr hwnd)
			: MandelbrotRenderer(hinstance, hwnd, false)
		{
		}

		MandelbrotRenderer(System::IntPtr hinstance, System::IntPtr hwnd, bool debug);
		~MandelbrotRenderer();

		void RefreshSurface();
		System::ValueTuple<System::UInt32, System::UInt32> GetSurfaceExtent();
		void Draw();

		array<DebugMessage^>^ GetDebugMessages();

		property float Top;
		property float Left;
		property float Right;
		property float Bottom;
		property float BailoutRadius;
		property System::UInt32 MaxIterations;
		property System::UInt32 FillColor;
		property float GradientPeriodFactor;
		property array<System::UInt32>^ Gradient;

	private:

		bool _disposed = false;
		vulkan_renderer* _native_renderer = nullptr;
		array<DebugMessage^>^ _cachedMessages = nullptr;
	};
}
