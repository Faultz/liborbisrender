#pragma once
#include "shader_common.h"

namespace sce { namespace SampleUtil { namespace Graphics {
	/*!
	 * @~English
	 * @brief Model draw vertex definition
	 * @~Japanese
	 * @brief モデル描画用頂点定義
	 */
	struct Vertex
	{
		/*!
		 * @~English
		 * @brief Position
		 * @~Japanese
		 * @brief 位置
		 */
		Vector3Unaligned	m_position;
		/*!
		 * @~English
		 * @brief Texture coordinate
		 * @~Japanese
		 * @brief テクスチャ座標
		 */
		Vector2Unaligned	m_uv;
		/*!
		 * @~English
		 * @brief Normal
		 * @~Japanese
		 * @brief 法線
		 */
		Vector3Unaligned	m_normal;
		/*!
		 * @~English
		 * @brief Tangent
		 * @detail Binormal flip is stored in w component
		 * @~Japanese
		 * @brief 接線
		 * @details wコンポーネントにbinormal flipが格納される
		 */
		Vector4Unaligned	m_tangent;
#ifdef __cplusplus
		bool	operator==(const Vertex	&a) const
		{
#define DIFF_MARGIN .00001f
			return
				(fabs(m_position.x - a.m_position.x) < DIFF_MARGIN) &&
				(fabs(m_position.y - a.m_position.y) < DIFF_MARGIN) &&
				(fabs(m_position.z - a.m_position.z) < DIFF_MARGIN) &&
				(fabs(m_uv.x - a.m_uv.x) < DIFF_MARGIN) &&
				(fabs(m_uv.y - a.m_uv.y) < DIFF_MARGIN) &&
				(fabs(m_normal.x - a.m_normal.x) < DIFF_MARGIN) &&
				(fabs(m_normal.y - a.m_normal.y) < DIFF_MARGIN) &&
				(fabs(m_normal.z - a.m_normal.z) < DIFF_MARGIN) &&
				(fabs(m_tangent.x - a.m_tangent.x) < DIFF_MARGIN) &&
				(fabs(m_tangent.y - a.m_tangent.y) < DIFF_MARGIN) &&
				(fabs(m_tangent.z - a.m_tangent.z) < DIFF_MARGIN);
		}
#endif
	}; // struct Vertex

	/*!
	 * @~English
	 * @brief Model draw skinning vertex definition
	 * @~Japanese
	 * @brief モデル描画用スキニング頂点定義
	 */
	struct SkinVertex
	{
		/*!
		 * @~English
		 * @brief Dummy joint index
		 * @~Japanese
		 * @brief ダミージョイントインデックス
		 */
		static const	unsigned short	kDummyBoneIndex = 0xffff;
		/*!
		 * @~English
		 * @brief Joint weights
		 * @~Japanese
		 * @brief ジョイント重み
		 */
		Vector4Unaligned		m_boneWeights;
		/*!
		 * @~English
		 * @brief Joint indices
		 * @~Japanese
		 * @brief ジョイントインデックス
		 */
		uint4					m_boneIndices;
	}; // struct SkinVertex
#ifndef WIN32
	/*!
	 * @~English
	 * @brief Model draw vertex buffer definition
	 * @~Japanese
	 * @brief モデル描画用頂点バッファ定義
	 */
	struct MeshVertexBuffer
	{
		/*!
		 * @~English
		 * @brief Vertex buffer definition
		 * @~Japanese
		 * @brief 頂点バッファ定義
		 */
		RegularBuffer<Vertex>			m_vertices;
		/*!
		 * @~English
		 * @brief Skinning vertex buffer definition
		 * @~Japanese
		 * @brief スキニング頂点バッファ定義
		 */
		RegularBuffer<SkinVertex>		m_skinVertices;
	};
#endif
}}} // namespace sce::SampleUtil::Graphics
