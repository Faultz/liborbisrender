#pragma once

#include "shader_common.h"

namespace sce { namespace SampleUtil { namespace Graphics {
/*!
 * @~English
 * @brief Shader material
 * @~Japanese
 * @brief シェーダマテリアル
 */
struct ShaderMaterial
{
	/*!
	 * @~English
	 * @brief Emissive weight
	 * @~Japanese
	 * @brief エミッシブ重み
	 */
	Vector3Unaligned				m_emissiveWeights;
	/*!
	 * @~English
	 * @brief Material id
	 * @~Japanese
	 * @brief マテリアルID
	 */
	uint							m_materialId;
	/*!
	 * @~English
	 * @brief Diffuse weight
	 * @~Japanese
	 * @brief ディフューズ重み
	 */
	Vector3Unaligned				m_diffuseWeights;
	uint							__padding2__[1];
	/*!
	 * @~English
	 * @brief Specular weight
	 * @~Japanese
	 * @brief スペキュラ重み
	 */
	Vector4Unaligned				m_specularWeights;
	/*!
	 * @~English
	 * @brief Ambient weight
	 * @~Japanese
	 * @brief アンビエント重み
	 */
	Vector4Unaligned				m_ambientWeights;

	// resource definitions from here
	/*!
	 * @~English
	 * @brief Emissive map texture
	 * @~Japanese
	 * @brief エミッシブマップテクスチャ
	 */
	Texture2D<Vector3Unaligned>		m_emissiveMap;
	/*!
	 * @~English
	 * @brief Diffuse map texture
	 * @~Japanese
	 * @brief ディフューズマップテクスチャ
	 */
	Texture2D<Vector3Unaligned>		m_diffuseMap;
	/*!
	 * @~English
	 * @brief Specular map texture
	 * @~Japanese
	 * @brief スペキュラマップテクスチャ
	 */
	Texture2D<Vector4Unaligned>		m_specularMap;
	/*!
	 * @~English
	 * @brief Emissive map texture
	 * @~Japanese
	 * @brief エミッシブマップテクスチャ
	 */
	Texture2D<Vector4Unaligned>		m_normalMap;
	/*!
	 * @~English
	 * @brief Mask map texture
	 * @~Japanese
	 * @brief マスクマップテクスチャ
	 */
	Texture2D<float>				m_maskMap;
};
}}} // namespace sce::SampleUtil::Graphics
