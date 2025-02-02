#pragma once

#include "../imconfig.h"
#include "../imgui.h"

#include "imgui_impl_orbis.h"
#include "../../Shaders.h"

#include <array>

struct CombinedVertexBuffer
{
	ImDrawVert* m_vb;
	uint32_t	m_size;

	CombinedVertexBuffer()
	{
		m_vb = nullptr;
		m_size = 0;
	}
};

struct CombinedIndexBuffer
{
	ImDrawIdx* m_ib;
	uint32_t	m_size;

	CombinedIndexBuffer()
	{
		m_ib = nullptr;
		m_size = 0;
	}
};

inline std::array<CombinedVertexBuffer, 3> m_vtx_buffer;
inline std::array<CombinedIndexBuffer, 3>  m_idx_buffer;

IMGUI_API bool        ImGui_ImplGnm_Init(std::function<void(ImGuiIO& io)> loadFontsCb = nullptr);
IMGUI_API void        ImGui_ImplGnm_Shutdown();
IMGUI_API void        ImGui_ImplGnm_NewFrame();
IMGUI_API void		  ImGui_ImplGnm_SetupRenderState(sce::Gnmx::LightweightGfxContext* dcb, ImDrawData* draw_data);
IMGUI_API void        ImGui_ImplGnm_RenderDrawData(ImDrawData* draw_data, sce::Gnmx::LightweightGfxContext* context);

IMGUI_API bool		  ImGui_ImplGnm_CreateFontsTexture(std::function<void(ImGuiIO& io)> loadFontsCb = nullptr);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplGnm_DestroyDeviceObjects();
IMGUI_API void        ImGui_ImplGnm_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplGnm_CreateDeviceObjects();