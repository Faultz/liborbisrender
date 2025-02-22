#include "stdafx.h"
#include <liborbisrender.h>
#include <png_dec.h>

// We need to provide an export to force the expected stub library to be generated
__declspec (dllexport) void dummy()
{
}

bool closed = false;
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

			liborbisutil::patcher::create("mutex on list patch", "libkernel.sprx", 0x7850, { 0xC3 }, true);

			context.create(RenderAfterFlip | HookFlipVideoOut | FunctionImGui | FunctionRenderDebug | UnlockFps, [](int flipIndex) {

				if(context.begin_scene(flipIndex))
				{
					context.update_scene();

					// do render...
					ImGui::Begin("Hello, world!");
					ImGui::Text("This is some useful text.");

					//static texture tex("https://cataas.com/cat");
					//if (tex)
					//{
					//	ImGui::Image(&tex, { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y });
					//}

					ImGui::End();

					context.end_scene();
				}

				}, [](ImGuiIO& io) {

					// calculate font scale based on screen resolution and if we're in neomode
					liborbisutil::math::vector2 screen_size = { static_cast<float>(render_context::video_out_info->width), static_cast<float>(render_context::video_out_info->height) };
					
					float base_scale = 1.0f;
					if (screen_size.x >= 3840 && screen_size.y >= 2160) {
						// 4K resolution
						base_scale = 2.0f;
					}
					else if (screen_size.x >= 2560 && screen_size.y >= 1440) {
						// 1440p resolution
						base_scale = 1.5f;
					}
					else {
						// 1080p or lower
						base_scale = 1.0f;
					}

					for (auto file : liborbisutil::directory_iterator("/data/ImGui Fonts"))
					{

						io.Fonts->AddFontFromFileTTF(file.data(), 16.0f);
					}

					io.FontGlobalScale = base_scale;

					});

			liborbisutil::pad::initialize(liborbisutil::pad::state::read_state, true, [](ScePadData* pad, int num) {

					ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
					if (bd)
						memcpy(&bd->m_sce_pad, pad, sizeof(ScePadData));

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