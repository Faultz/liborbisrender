#include "graphics/shader_common.h"

namespace Frame
{
	struct ImDrawVert
	{
		Vector2Unaligned	m_pos;
		Vector2Unaligned	m_uv;
		uint				m_col;
		float				m_depth;
	};

	struct PerFrameData
	{
		Matrix4Unaligned	m_mvp;
		uint				m_hdr;
		RegularBuffer<ImDrawVert>	m_vertex;
		bool srgb;
	};

	struct PerDrawData
	{
		Texture2D<Vector4Unaligned>	m_texture;
		uint						m_vtxOffset;
	};

	struct ImguiSrtData
	{
		PerFrameData* m_frameData;
		PerDrawData* m_drawData;
	};
};