#include "shaders.h"

#define BASE_ALIGN( x, a )					(((x) + ((a) - 1)) & ~((a) - 1))

#include <gnm.h>
#include <gnmx/shader_parser.h>

PsShader::PsShader(void* shaderBinary, liborbisutil::memory::direct_memory_allocator* allocator)
{
	sce::Gnmx::ShaderInfo shaderInfo;
	sce::Gnmx::parseShader(&shaderInfo, shaderBinary);

	void* binary = allocator->allocate(shaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes, "Pixel Binary Shader");
	void* header = allocator->allocate(shaderInfo.m_psShader->computeSize(), sce::Gnm::kAlignmentOfBufferInBytes, "Pixel Header Shader");

	memcpy(binary, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);
	memcpy(header, shaderInfo.m_psShader, shaderInfo.m_psShader->computeSize());

	m_shader = static_cast<sce::Gnmx::PsShader*>(header);
	m_shader->patchShaderGpuAddress(binary);

	static int shader_counter = 0;

	if(sce::Gnm::isUserPaEnabled())
		sce::Gnm::registerResource(&handle, allocator->owner_handle, m_shader->getBaseAddress(), shaderInfo.m_gpuShaderCodeSize, liborbisutil::string::format("Pixel Shader %d", shader_counter++).data(), sce::Gnm::kResourceTypeShaderBaseAddress, 0);

	sce::Gnmx::generateInputResourceOffsetTable(&resourceTable, sce::Gnm::kShaderStagePs, m_shader);
	sce::Gnmx::ConstantUpdateEngine::initializeInputsCache(&cacheTable, m_shader->getInputUsageSlotTable(), m_shader->m_common.m_numInputUsageSlots);
}

PsShader::PsShader(sce::Gnmx::PsShader* shader)
{
	m_shader = shader;

	//sce::Gnmx::generateInputOffsetsCache(cacheTable, Gnm::kShaderStagePs, m_shader);
	sce::Gnmx::generateInputResourceOffsetTable(&resourceTable, sce::Gnm::kShaderStagePs, m_shader);
}

VsShader::VsShader(void* shaderBinary, liborbisutil::memory::direct_memory_allocator* allocator)
{
	sce::Gnmx::ShaderInfo shaderInfo;
	sce::Gnmx::parseShader(&shaderInfo, shaderBinary);

	void* binary = allocator->allocate(shaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes, "Vertex Binary Shader");
	void* header = allocator->allocate(shaderInfo.m_psShader->computeSize(), sce::Gnm::kAlignmentOfBufferInBytes, "Vertex Header Shader");

	memcpy(binary, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);
	memcpy(header, shaderInfo.m_psShader, shaderInfo.m_psShader->computeSize());

	m_shader = static_cast<sce::Gnmx::VsShader*>(header);
	m_shader->patchShaderGpuAddress(binary);

	static int shader_counter = 0;

	if (sce::Gnm::isUserPaEnabled())
		sce::Gnm::registerResource(&handle, allocator->owner_handle, m_shader->getBaseAddress(), shaderInfo.m_gpuShaderCodeSize, liborbisutil::string::format("Vertex Shader %d", shader_counter++).data(), sce::Gnm::kResourceTypeShaderBaseAddress, 0);

	fb = { 0 };
	sce::Gnm::generateVsFetchShaderBuildState(&fb, (const sce::Gnm::VsStageRegisters*)&m_shader->m_vsStageRegisters, m_shader->m_numInputSemantics, nullptr, 0, fb.m_vertexBaseUsgpr, fb.m_instanceBaseUsgpr);

	const sce::Gnm::InputUsageSlot* inputUsageSlots = m_shader->getInputUsageSlotTable();
	fb.m_numInputSemantics = m_shader->m_numInputSemantics;
	fb.m_inputSemantics = m_shader->getInputSemanticTable();
	fb.m_numInputUsageSlots = m_shader->m_common.m_numInputUsageSlots;
	fb.m_inputUsageSlots = inputUsageSlots;
	fb.m_numElementsInRemapTable = 0;
	fb.m_semanticsRemapTable = 0;

	m_fetchShader = (uint32_t*)allocator->allocate(fb.m_fetchShaderBufferSize, sce::Gnm::kAlignmentOfFetchShaderInBytes, "Vertex Fetch Shader");
	sce::Gnm::generateFetchShader(m_fetchShader, &fb);

	sce::Gnmx::generateInputResourceOffsetTable(&resourceTable, sce::Gnm::kShaderStageVs, m_shader);
	sce::Gnmx::ConstantUpdateEngine::initializeInputsCache(&cacheTable, m_shader->getInputUsageSlotTable(), m_shader->m_common.m_numInputUsageSlots);
}

VsShader::VsShader(sce::Gnmx::VsShader* shader)
{
	m_shader = shader;

	//sce::Gnmx::generateInputOffsetsCache(cacheTable, Gnm::kShaderStageVs, m_shader);
	sce::Gnmx::generateInputResourceOffsetTable(&resourceTable, sce::Gnm::kShaderStageVs, m_shader);
}

CsShader::CsShader(void* shaderBinary, liborbisutil::memory::direct_memory_allocator* allocator)
{
	sce::Gnmx::ShaderInfo shaderInfo;
	sce::Gnmx::parseShader(&shaderInfo, shaderBinary);

	void* binary = allocator->allocate(shaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes, "Compute Binary Shader");
	void* header = allocator->allocate(shaderInfo.m_csShader->computeSize(), sce::Gnm::kAlignmentOfBufferInBytes, "Compute Header Shader");

	memcpy(binary, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);
	memcpy(header, shaderInfo.m_csShader, shaderInfo.m_csShader->computeSize());

	m_shader = static_cast<sce::Gnmx::CsShader*>(header);
	m_shader->patchShaderGpuAddress(binary);

	static int shader_counter = 0;

	if (sce::Gnm::isUserPaEnabled())
		sce::Gnm::registerResource(&handle, allocator->owner_handle, m_shader->getBaseAddress(), shaderInfo.m_gpuShaderCodeSize, liborbisutil::string::format("Compute Shader %d", shader_counter++).data(), sce::Gnm::kResourceTypeShaderBaseAddress, 0);

	sce::Gnmx::generateInputResourceOffsetTable(&resourceTable, sce::Gnm::kShaderStageCs, m_shader);
	sce::Gnmx::ConstantUpdateEngine::initializeInputsCache(&cacheTable, m_shader->getInputUsageSlotTable(), m_shader->m_common.m_numInputUsageSlots);
}

CsShader::CsShader(sce::Gnmx::CsShader* shader)
{
	m_shader = shader;

	//sce::Gnmx::generateInputOffsetsCache(cacheTable, Gnm::kShaderStagePs, m_shader);
	sce::Gnmx::generateInputResourceOffsetTable(&resourceTable, sce::Gnm::kShaderStageCs, m_shader);
}
