#include "stdafx.h"
#include <liborbisrender.h>
#include <png_dec.h>

#include <audioout.h>
#pragma comment(lib, "liborbisrender.a")

// We need to provide an export to force the expected stub library to be generated
__declspec (dllexport) void dummy()
{
}

extern "C" {
	__declspec(dllexport) void* _ZTINSt8ios_base7failureE = nullptr;
	__declspec(dllexport) void* _ZTVSt5ctypeIcE = nullptr;
	__declspec(dllexport) void* _ZTISt8bad_cast = nullptr;
}

extern "C"
{
	int __cdecl module_start(size_t argc, const void* args)
	{
		liborbisutil::thread t([](void*) {
			auto res = MH_Initialize();
			if (res != MH_OK)
			{
				LOG_ERROR("failed to initialize minhook: %d\n", res);
				return;
			}

			liborbisutil::http::initialize();
			liborbisutil::patcher::create("mutex on list patch", "libkernel.sprx", 0x7850, { 0xC3 }, true);

			render_context::create(HookFlip | FunctionImGui | FunctionRenderDebug | UnlockFps, [](int flipIndex) {

				if (render_context::begin_frame(flipIndex))
				{
					render_context::update_frame();

					// do render...
					ImGui::Begin("Hello, world!");
					ImGui::Text("This is some useful text.");

					static texture tex("https://cataas.com/cat");
					if (tex)
					{
						ImGui::Image(&tex, { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y });
					}

					ImGui::End();

					render_context::end_frame();

				}
				}, [](ImGuiIO& io) {
					io.Fonts->AddFontDefault();
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
		liborbisutil::thread t([](void*) {
			render_context::release();

			liborbisutil::pad::finalize();
			liborbisutil::http::finalize();

			MH_Uninitialize();
			}, "release");

		t.join();

		return SCE_OK;
	}
}