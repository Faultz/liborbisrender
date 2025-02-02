#include "stdafx.h"

#include "liborbisrender.h"

// We need to provide an export to force the expected stub library to be generated
__declspec (dllexport) void dummy()
{
}

renderer::render_context context;

extern "C"
{
	int __cdecl module_start(size_t argc, const void* args)
	{
		liborbisutil::thread t([]() {
			MH_Initialize();

			context.create(renderer::RenderAfterFlip | renderer::HookFlip | renderer::FunctionImGui | renderer::FunctionRenderDebug, [](int flipIndex) {

				if (context.begin_scene(flipIndex))
				{
					context.update_scene();
					context.flip_scene(flipIndex);
					context.end_scene();
				}

				});
			}, "init");

		t.join();

		return SCE_OK;
	}

	int __cdecl module_stop(size_t argc, const void* args)
	{
		liborbisutil::thread t([]() {
			context.release();

			MH_Uninitialize();
			}, "release");

		t.join();

		return SCE_OK;
	}
}