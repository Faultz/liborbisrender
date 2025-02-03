#pragma once

#include <liborbisutil.h>

template<class T>
struct base_shader
{
	base_shader() = default;

	T* m_shader;

	operator T* () const
	{
		return m_shader;
	}

	sce::Gnmx::InputResourceOffsets resourceTable;
	sce::Gnmx::InputOffsetsCache cacheTable;
};

struct PsShader : base_shader<sce::Gnmx::PsShader>
{
	PsShader(void* shaderBinary, liborbisutil::memory::direct_memory_allocator* allocator);
	PsShader(sce::Gnmx::PsShader* shader);
};

struct VsShader : base_shader<sce::Gnmx::VsShader>
{
	VsShader(void* shaderBinary, liborbisutil::memory::direct_memory_allocator* allocator);
	VsShader(sce::Gnmx::VsShader* shader);

	uint32_t* m_fetchShader;
	sce::Gnm::FetchShaderBuildState fb;
};

struct CsShader : base_shader<sce::Gnmx::CsShader>
{
	CsShader(void* shaderBinary, liborbisutil::memory::direct_memory_allocator* allocator);
	CsShader(sce::Gnmx::CsShader* shader);
};

