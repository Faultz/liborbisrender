
#include "imgui_impl_gnm.h"
#include "../../render_context.h"

bool        ImGui_ImplGnm_Init(std::function<void(ImGuiIO& io)> loadFontsCb)
{
	ImGui_ImplGnm_CreateDeviceObjects();
	ImGui_ImplGnm_CreateFontsTexture(loadFontsCb);

	return true;
}

void        ImGui_ImplGnm_Shutdown()
{
	// we'll do this later
	ImGui_ImplGnm_DestroyDeviceObjects();
}

void        ImGui_ImplGnm_NewFrame()
{
}

void ImGui_ImplGnm_SetupRenderState(sce::Gnmx::LightweightGfxContext* dcb, ImDrawData* draw_data)
{
	ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
	IM_ASSERT(bd != NULL && "Did you call ImGui_ImplOrbis_Init()?");

	{
		Frame::ImguiSrtData userData = bd->userData;

		float L = draw_data->DisplayPos.x;
		float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
		float T = draw_data->DisplayPos.y;
		float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
		float mvp[4][4] =
		{
			{ 2.0f / (R - L),		0.0f,				0.0f,       0.0f },
			{ 0.0f,					2.0f / (T - B),     0.0f,       0.0f },
			{ 0.0f,					0.0f,				0.5f,       0.0f },
			{ (R + L) / (L - R),	(T + B) / (B - T),	0.5f,       1.0f },
		};
		memcpy(&userData.m_frameData->m_mvp, mvp, sizeof(mvp));
	}

	dcb->setVsShader(bd->shaderProgram->getVertexShader(), &bd->shaderProgram->vertex_shader.resource_table);
	dcb->setPsShader(bd->shaderProgram->getPixelShader(), &bd->shaderProgram->pixel_shader.resource_table);

	dcb->setPrimitiveType(sce::Gnm::kPrimitiveTypeTriList);

	// set index data
	dcb->setIndexSize(sizeof(ImDrawIdx) == 2 ? sce::Gnm::kIndexSize16 : sce::Gnm::kIndexSize32);

	// mask
	dcb->setRenderTargetMask(0xf);

	// culling
	sce::Gnm::PrimitiveSetup ps;
	ps.init();
	ps.setCullFace(sce::Gnm::kPrimitiveSetupCullFaceNone);
	ps.setFrontFace(sce::Gnm::kPrimitiveSetupFrontFaceCw);
	ps.setPolygonMode(sce::Gnm::kPrimitiveSetupPolygonModeFill, sce::Gnm::kPrimitiveSetupPolygonModeFill);
	dcb->setPrimitiveSetup(ps);

	// depth stencil control
	sce::Gnm::DepthStencilControl dsc;
	dsc.init();
	{
		dsc.setDepthControl(sce::Gnm::kDepthControlZWriteDisable, sce::Gnm::kCompareFuncAlways);
	}

	dcb->setDepthStencilControl(dsc);

	// blend control
	sce::Gnm::BlendControl bc;
	bc.init();
	bc.setBlendEnable(true);
	bc.setColorEquation(sce::Gnm::kBlendMultiplierSrcAlpha, sce::Gnm::kBlendFuncAdd, sce::Gnm::kBlendMultiplierOneMinusSrcAlpha);
	dcb->setBlendControl(0, bc);
}

void        ImGui_ImplGnm_RenderDrawData(ImDrawData* draw_data, sce::Gnmx::LightweightGfxContext* context)
{
	ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
	IM_ASSERT(bd != NULL && "Did you call ImGui_ImplOrbis_Init()?");

	ImGuiIO& io = ImGui::GetIO();

	//float scaleX = GraphicsContext::videoData->width / 1920;
	//float scaleY = GraphicsContext::videoData->height / 1080;
	//ImGui::GetStyle().ScaleAllSizes(std::min(scaleX, scaleY));

	draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	static int target = 0;

	auto& vertex_buffer = m_vtx_buffer[target];
	auto& index_buffer = m_idx_buffer[target];

	auto allocator = bd->garlic_allocator;

	sce::Gnmx::LightweightGfxContext* dcb = context;

	if (vertex_buffer.m_size < draw_data->TotalVtxCount * sizeof(ImDrawVert))
	{
		if (vertex_buffer.m_vb != nullptr)
		{
			allocator->free(vertex_buffer.m_vb);
		}
		combined_vertex_buffer pVB;
		pVB.m_size = (draw_data->TotalVtxCount + 5000) * sizeof(ImDrawVert);
		pVB.m_vb = (ImDrawVert*)allocator->allocate(pVB.m_size, 4, liborbisutil::string::format("Vertex Buffers %i", target));

		printf("vertex buffer allocated %i\n", pVB.m_size);
		vertex_buffer = pVB;
	}
	if (index_buffer.m_size < draw_data->TotalIdxCount * sizeof(ImDrawIdx))
	{
		if (index_buffer.m_ib != nullptr)
		{
			allocator->free(index_buffer.m_ib);
		}
		combined_index_buffer pIB;
		pIB.m_size = (draw_data->TotalIdxCount + 10000) * sizeof(ImDrawIdx);
		pIB.m_ib = (ImDrawIdx*)allocator->allocate(pIB.m_size, 4, liborbisutil::string::format("Index Buffers %i", target));

		printf("index buffer allocated %i\n", pIB.m_size);
		index_buffer = pIB;
	}

	// Copy and convert all vertices into a single contiguous buffer
	ImDrawVert* vtx_dst = vertex_buffer.m_vb;
	ImDrawIdx* idx_dst = index_buffer.m_ib;

	_mm_sfence();
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}
	_mm_sfence();

	Frame::ImguiSrtData& userData = bd->userData;
	// Setup orthographic projection matrix into our constant buffer
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). 

	userData.m_frameData = (Frame::PerFrameData*)dcb->allocateFromCommandBuffer(sizeof(Frame::PerFrameData), sce::Gnm::kEmbeddedDataAlignment16);
	{
		sce::Gnm::Buffer buf;
		buf.initAsRegularBuffer(vertex_buffer.m_vb, sizeof(ImDrawVert), draw_data->TotalVtxCount);

		userData.m_frameData->m_vertex = buf;

		userData.m_frameData->m_hdr = false;
		userData.m_frameData->srgb = bd->renderContext->target_is_srgb;
	}

	dcb->pushMarker("ImGui Render");

	ImGui_ImplGnm_SetupRenderState(dcb, draw_data);

	int vtx_offset = 0;
	int idx_offset = 0;
	ImVec2 pos = draw_data->DisplayPos;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				// User callback (registered via ImDrawList::AddCallback)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					ImGui_ImplGnm_SetupRenderState(dcb, draw_data);
				else
					pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				// Apply scissor/clipping rectangle
				dcb->setScreenScissor((uint32_t)(pcmd->ClipRect.x - pos.x), (uint32_t)(pcmd->ClipRect.y - pos.y), (uint32_t)(pcmd->ClipRect.z - pos.x), (uint32_t)(pcmd->ClipRect.w - pos.y));
			
				userData.m_drawData = (Frame::PerDrawData*)dcb->allocateFromCommandBuffer(sizeof(Frame::PerDrawData), sce::Gnm::kEmbeddedDataAlignment16);
				userData.m_drawData->m_texture = *((sce::Gnm::Texture*)pcmd->TextureId);
				userData.m_drawData->m_vtxOffset = vtx_offset;

				// bind SRT
				dcb->setUserSrtBuffer(sce::Gnm::ShaderStage::kShaderStageVs, &userData, sizeof(userData) / sizeof(uint32_t));
				dcb->setTextures(sce::Gnm::ShaderStage::kShaderStagePs, 0, 1, &userData.m_drawData->m_texture);

				dcb->drawIndex(pcmd->ElemCount, index_buffer.m_ib + idx_offset + pcmd->IdxOffset);
			}
		}
		idx_offset += cmd_list->IdxBuffer.Size;
		vtx_offset += cmd_list->VtxBuffer.Size;
	}

	dcb->popMarker();

	target = (target + 1) % bd->renderContext->get_target_count();
}

bool		ImGui_ImplGnm_CreateFontsTexture(std::function<void(ImGuiIO& io)> loadFontsCb)
{
	ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
	IM_ASSERT(bd != NULL && "Did you call ImGui_ImplOrbis_Init()?");

	auto allocator = bd->garlic_allocator;

	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;

	if(loadFontsCb)
		loadFontsCb(io);
	else
	{
		for (auto& file : liborbisutil::directory_iterator("/data/ImGui Fonts/"))
		{
			io.Fonts->AddFontFromFileTTF(file.data(), 15.0f);
		}
	}

	io.FontDefault = io.Fonts->Fonts[0];
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	m_font_texture = texture();

	sce::Gnm::TextureSpec spec;
	spec.init();
	spec.m_textureType = sce::Gnm::kTextureType2d;
	spec.m_width = width;
	spec.m_height = height;
	spec.m_format = sce::Gnm::kDataFormatR8G8B8A8UnormSrgb;
	int32_t status = m_font_texture.init(&spec);
	if (status != 0) 
		return -1;

	void* buffer = allocator->allocate(m_font_texture.getSizeAlign(), "Texture Allocate");
	if (buffer == nullptr)
		return 0;

	// tileSurface
	sce::GpuAddress::TilingParameters tp;
	int ret = tp.initFromTexture(&m_font_texture, 0, 0);
	if (ret != SCE_OK)
		return 0;

	ret = sce::GpuAddress::tileSurface(buffer, pixels, &tp);
	if (ret)
		return 0;

	m_font_texture.setBaseAddress(buffer);
	m_font_texture.register_resource("ImGui Font Texture");

	/* switch */
	io.Fonts->TexID = (void*)&m_font_texture;
	return true;
}

// Use if you want to reset your rendering device without losing ImGui state.
void        ImGui_ImplGnm_DestroyDeviceObjects()
{
	ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
	IM_ASSERT(bd != NULL && "Did you call ImGui_ImplOrbis_Init()?");
	auto allocator = bd->garlic_allocator;
	if (m_font_texture.getBaseAddress())
	{
		allocator->free((void*)m_font_texture.getBaseAddress());
		m_font_texture.setBaseAddress(nullptr);
	}
	if (m_shaders)
	{
		m_shaders.reset();
	}
}

void        ImGui_ImplGnm_InvalidateDeviceObjects()
{

}

bool        ImGui_ImplGnm_CreateDeviceObjects()
{
	ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
	IM_ASSERT(bd != NULL && "Did you call ImGui_ImplOrbis_Init()?");

	auto& m_garlic = *bd->renderContext->garlic_memory_allocator;

	extern char _binary_imgui_p_sb_start[], _binary_imgui_vv_sb_start[];
	extern char _binary_imgui_p_sb_size[], _binary_imgui_vv_sb_size[];

	m_shaders = std::make_unique<shader_program>("ImGui PS Shader", 
		_binary_imgui_vv_sb_start, (size_t)_binary_imgui_vv_sb_size,
		_binary_imgui_p_sb_start, (size_t)_binary_imgui_p_sb_size, &m_garlic);
	if (m_shaders)
	{
		bd->shaderProgram = m_shaders.get();
		return true;
	}
	return false;
}