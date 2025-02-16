#include "stdafx.h"
#include <liborbisrender.h>
#include <png_dec.h>

// We need to provide an export to force the expected stub library to be generated
__declspec (dllexport) void dummy()
{
}

render_context context;

extern "C"
{
	int __cdecl module_start(size_t argc, const void* args)
	{
		liborbisutil::thread t([]() {
			auto res = MH_Initialize();
			if (res != MH_OK)
			{
				LOG_ERROR("failed to initialize minhook: %d\n", res);
				return;
			}

			liborbisutil::http::initialize();

			context.create(RenderAfterFlip | HookFlip | FunctionImGui | FunctionRenderDebug | UnlockFps, [](int flipIndex) {

				if(context.begin_scene(flipIndex))
				{
					context.update_scene();

					// do render...
					ImGui::Begin("Hello, world!");
					ImGui::Text("This is some useful text.");

					static texture tex("https://cataas.com/cat");
					if (tex)
					{
						ImGui::Image(&tex, { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y });
					}

					ImGui::End();

					context.flip_scene(flipIndex);
					context.end_scene();
				}

				});

			liborbisutil::pad::initialize(liborbisutil::pad::state::read_state, true, [](ScePadData* pad, int num) {

				if (num == 1)
				{
					ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
					if (bd)
						memcpy(&bd->m_sce_pad, pad, sizeof(ScePadData));
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

			liborbisutil::pad::finalize();
			liborbisutil::http::finalize();

			MH_Uninitialize();
			}, "release");

		t.join();

		return SCE_OK;
	}
}