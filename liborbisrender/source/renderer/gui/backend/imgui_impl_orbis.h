#pragma once

#include <gnm.h>
#include <gnmx.h>
#include <pad.h>

#include "../imgui.h"
#include "../imgui_internal.h"

#include "../Shaders/imgui_srt_common.h"
#include "../../shaders.h"

class render_context;

enum ImGui_Osk_State
{
	OSK_IDLE = 0,
	OSK_OPENED = 1,
	OSK_ENTERED = 2,
	OSK_CLOSED = 3,
	OSK_CANCELLED = 4
};

struct ImGui_ImplOrbis_Osk
{
	ImGui_Osk_State State;
	bool WantTextInput;
	wchar_t Text[1024];
};

struct ImGui_ImplOrbis_Data
{
    bool											HasGamepad;
    bool											WantUpdateHasGamepad;
	bool											HasTouchpadControl;
	bool											HasMouseControl;
	render_context*									renderContext;
	liborbisutil::memory::direct_memory_allocator*  garlic_allocator;
	liborbisutil::memory::direct_memory_allocator*  onion_allocator;
	ScePadData										m_sce_pad;
	Frame::ImguiSrtData								userData;
	ImGui_ImplOrbis_Osk 							osk;
	shader_program*									shaderProgram;

    ImGui_ImplOrbis_Data() { memset(this, 0, sizeof(*this)); }
};

inline std::unique_ptr<shader_program> m_shaders;

inline sce::Gnm::Texture m_font_texture;

inline uint64_t g_time = 0;

ImGui_ImplOrbis_Data* ImGui_ImplOrbis_GetBackendData();

IMGUI_IMPL_API bool     ImGui_ImplOrbis_Init(render_context* context);
IMGUI_IMPL_API void     ImGui_ImplOrbis_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplOrbis_NewFrame();
IMGUI_IMPL_API bool     ImGui_ImplOrbis_HandleEvent();