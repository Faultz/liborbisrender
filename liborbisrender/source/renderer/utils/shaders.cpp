#include "shaders.h"

shader_program::shader_program(const std::string& name, void* vertexShaderBinary, size_t vertexShaderBinarySize, void* pixelShaderBinary, size_t pixelShaderBinarySize, liborbisutil::memory::direct_memory_allocator* allocator)
{
	this->name = name;

	load_vertex_shader(vertexShaderBinary, vertexShaderBinarySize, allocator);
	load_pixel_shader(pixelShaderBinary, pixelShaderBinarySize, allocator);
}

shader_program::shader_program(const std::string& name, const std::string vertexShaderFile, const std::string pixelShaderFile, liborbisutil::memory::direct_memory_allocator* allocator)
{
	this->name = name;

	load_vertex_shader(vertexShaderFile, allocator);
	load_pixel_shader(pixelShaderFile, allocator);
}

shader_program::~shader_program()
{
	if (vertex_shader.resource_handle)
	{
		sce::Gnm::unregisterResource(vertex_shader.resource_handle);
	}
	if (pixel_shader.binary)
	{
		sce::Gnm::unregisterResource(pixel_shader.resource_handle);
	}
}

void shader_program::load_vertex_shader(void* shaderBinary, size_t shaderBinarySize, liborbisutil::memory::direct_memory_allocator* allocator)
{
	// parse shader
	sce::Gnmx::ShaderInfo shaderInfo;
	sce::Gnmx::parseShader(&shaderInfo, shaderBinary);

	void* binary = allocator->allocate(shaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes, "Vertex Binary Shader");
	void* header = allocator->allocate(shaderInfo.m_vsShader->computeSize(), sce::Gnm::kAlignmentOfBufferInBytes, "Vertex Header Shader");

	memcpy(binary, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);
	memcpy(header, shaderInfo.m_vsShader, shaderInfo.m_vsShader->computeSize());

	vertex_shader.m_shader = static_cast<sce::Gnmx::VsShader*>(header);
	vertex_shader.m_shader->patchShaderGpuAddress(binary);

	sce::Gnmx::generateInputResourceOffsetTable(&vertex_shader.resource_table, sce::Gnm::kShaderStageVs, vertex_shader.m_shader);
	sce::Gnmx::generateInputOffsetsCache(&vertex_shader.offset_cache, sce::Gnm::kShaderStageVs, vertex_shader.m_shader);

	static int shader_idx = 0;
	std::string resource_name = liborbisutil::string::format("%s_VertexShader_ResourceHandle_%d", name.c_str(), shader_idx++);
	sce::Gnm::registerResource(&vertex_shader.resource_handle, allocator->owner_handle, vertex_shader.m_shader->getBaseAddress(), shaderInfo.m_gpuShaderCodeSize,
		resource_name.data(), sce::Gnm::kResourceTypeShaderBaseAddress, 0);

	load_fetch_shader(shaderBinary, allocator);
	load_program(&vertex_shader, shaderBinary, shaderBinarySize, allocator);
}

void shader_program::load_vertex_shader(const std::string filename, liborbisutil::memory::direct_memory_allocator* allocator)
{
	liborbisutil::file_system fs(filename, "rb");
	if (!fs.is_open())
	{
		LOG_ERROR("Failed to open vertex shader file: %s\n", filename.c_str());
		return;
	}

	size_t file_size = fs.get_size();
	vertex_shader.binary = new uint8_t[file_size];
	fs.read(vertex_shader.binary, file_size);

	load_vertex_shader(vertex_shader.binary, file_size, allocator);

}

void shader_program::load_pixel_shader(void* shaderBinary, size_t shaderBinarySize, liborbisutil::memory::direct_memory_allocator* allocator)
{
	// parse shader
	sce::Gnmx::ShaderInfo shaderInfo;
	sce::Gnmx::parseShader(&shaderInfo, shaderBinary);

	if (pixel_shader.program.loadFromMemory(shaderBinary, shaderBinarySize) == sce::Shader::Binary::kStatusFail)
	{
		LOG_ERROR("Failed to load vertex shader program from memory\n");
		return;
	}

	void* binary = allocator->allocate(shaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes, "Pixel Binary Shader");
	void* header = allocator->allocate(shaderInfo.m_psShader->computeSize(), sce::Gnm::kAlignmentOfBufferInBytes, "Pixel Header Shader");

	memcpy(binary, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);
	memcpy(header, shaderInfo.m_psShader, shaderInfo.m_psShader->computeSize());

	pixel_shader.m_shader = static_cast<sce::Gnmx::PsShader*>(header);
	pixel_shader.m_shader->patchShaderGpuAddress(binary);

	sce::Gnmx::generateInputResourceOffsetTable(&pixel_shader.resource_table, sce::Gnm::kShaderStagePs, pixel_shader.m_shader);
	sce::Gnmx::generateInputOffsetsCache(&pixel_shader.offset_cache, sce::Gnm::kShaderStagePs, pixel_shader.m_shader);

	static int shader_idx = 0;
	std::string resource_name = liborbisutil::string::format("%s_PixelShader_ResourceHandle_%d", name.c_str(), shader_idx++);
	sce::Gnm::registerResource(&pixel_shader.resource_handle, allocator->owner_handle, pixel_shader.m_shader->getBaseAddress(), shaderInfo.m_gpuShaderCodeSize,
		resource_name.data(), sce::Gnm::kResourceTypeShaderBaseAddress, 0);

	load_program(&pixel_shader, shaderBinary, shaderBinarySize, allocator);
}

void shader_program::load_pixel_shader(const std::string filename, liborbisutil::memory::direct_memory_allocator* allocator)
{
	liborbisutil::file_system fs(filename, "rb");
	if (!fs.is_open())
	{
		LOG_ERROR("Failed to open pixel shader file: %s\n", filename.c_str());
		return;
	}

	size_t file_size = fs.get_size();
	pixel_shader.binary = new uint8_t[file_size];
	fs.read(pixel_shader.binary, file_size);

	load_pixel_shader(pixel_shader.binary, file_size, allocator);
}

void shader_program::load_fetch_shader(void* shaderBinary, liborbisutil::memory::direct_memory_allocator* allocator)
{
	sce::Gnmx::ShaderInfo shaderInfo;
	sce::Gnmx::parseShader(&shaderInfo, shaderBinary);

	sce::Gnm::SizeAlign calculatedSizeAlign;
	calculatedSizeAlign.m_align = sce::Gnm::kAlignmentOfBufferInBytes;
	calculatedSizeAlign.m_size = sce::Gnmx::computeVsFetchShaderSize(shaderInfo.m_vsShader);

	fetch_shader = allocator->allocate(calculatedSizeAlign, "Vertex Fetch Shader");

	uint32_t shaderModifier;
	sce::Gnmx::generateVsFetchShader(fetch_shader, &shaderModifier, vertex_shader.m_shader);

	vertex_shader.m_shader->applyFetchShaderModifier(shaderModifier);
}

void shader_program::load_compute_shader(void* shaderBinary, size_t shaderBinarySize, liborbisutil::memory::direct_memory_allocator* allocator)
{
	// parse shader
	sce::Gnmx::ShaderInfo shaderInfo;
	sce::Gnmx::parseShader(&shaderInfo, shaderBinary);

	if (compute_shader.program.loadFromMemory(shaderBinary, shaderBinarySize) == sce::Shader::Binary::kStatusFail)
	{
		LOG_ERROR("Failed to load vertex shader program from memory\n");
		return;
	}

	void* binary = allocator->allocate(shaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes, "Compute Binary Shader");
	void* header = allocator->allocate(shaderInfo.m_csShader->computeSize(), sce::Gnm::kAlignmentOfBufferInBytes, "Compute Header Shader");

	memcpy(binary, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize);
	memcpy(header, shaderInfo.m_csShader, shaderInfo.m_csShader->computeSize());

	compute_shader.m_shader = static_cast<sce::Gnmx::CsShader*>(header);
	compute_shader.m_shader->patchShaderGpuAddress(binary);

	sce::Gnmx::generateInputResourceOffsetTable(&compute_shader.resource_table, sce::Gnm::kShaderStagePs, compute_shader.m_shader);
	sce::Gnmx::generateInputOffsetsCache(&compute_shader.offset_cache, sce::Gnm::kShaderStagePs, compute_shader.m_shader);

	static int shader_idx = 0;
	std::string resource_name = liborbisutil::string::format("%s_ComputeShader_ResourceHandle_%d", name.c_str(), shader_idx++);
	sce::Gnm::registerResource(&compute_shader.resource_handle, allocator->owner_handle, compute_shader.m_shader->getBaseAddress(), shaderInfo.m_gpuShaderCodeSize,
		resource_name.data(), sce::Gnm::kResourceTypeShaderBaseAddress, 0);

	load_program(&compute_shader, shaderBinary, shaderBinarySize, allocator);
}

template<class T>
void shader_program::load_program(shader<T>* shader, void* shaderBinary, size_t shaderBinarySize, liborbisutil::memory::direct_memory_allocator* allocator)
{
	if (shader->program.loadFromMemory(shaderBinary, shaderBinarySize) == sce::Shader::Binary::kStatusFail)
	{
		LOG_ERROR("Failed to load vertex shader program from memory\n");
		return;
	}
	for (auto i = 0; i < shader->program.m_numBuffers; i++)
	{
		auto& buffer = shader->program.m_buffers[i];

		switch (buffer.m_langType)
		{
		case sce::Shader::Binary::kBufferTypeConstantBuffer:
			shader->constant_buffer_mask |= (1 << buffer.m_resourceIndex);
			break;
		case sce::Shader::Binary::kBufferTypeDataBuffer:
		case sce::Shader::Binary::kBufferTypeRegularBuffer:
		case sce::Shader::Binary::kBufferTypeByteBuffer:
			shader->srv_mask.buf |= (1 << buffer.m_resourceIndex);
			break;
		case sce::Shader::Binary::kBufferTypeSRTBuffer:
			shader->srt_buffer_mask |= (1 << buffer.m_resourceIndex);
		case sce::Shader::Binary::kBufferTypeTexture1d:
		case sce::Shader::Binary::kBufferTypeTexture2d:
		case sce::Shader::Binary::kBufferTypeTexture3d:
		case sce::Shader::Binary::kBufferTypeTextureCube:
		case sce::Shader::Binary::kBufferTypeTexture1dArray:
		case sce::Shader::Binary::kBufferTypeTexture2dArray:
		case sce::Shader::Binary::kBufferTypeTextureCubeArray:
		case sce::Shader::Binary::kBufferTypeMsTexture2d:
		case sce::Shader::Binary::kBufferTypeMsTexture2dArray:
		case sce::Shader::Binary::kBufferTypeTexture1dR128:
		case sce::Shader::Binary::kBufferTypeTexture2dR128:
		case sce::Shader::Binary::kBufferTypeMsTexture2dR128:
			shader->srv_mask.tex |= (1 << buffer.m_resourceIndex);
			break;
		}
	}

}

void shader_program::bind(sce::Gnmx::LightweightGfxContext& ctx)
{
	ctx.setVsShader(vertex_shader.m_shader, &vertex_shader.resource_table);
	ctx.setPsShader(pixel_shader.m_shader, &pixel_shader.resource_table);
}

void shader_program::compute(sce::Gnmx::LightweightGfxContext& ctx)
{
	ctx.setCsShader(compute_shader.m_shader, &compute_shader.resource_table);
}

sce::Shader::Binary::Buffer* shader_program::get_constant_buffer(sce::Gnm::ShaderStage stage, const char* name)
{
	if (stage == sce::Gnm::kShaderStageVs)
	{
		return vertex_shader.program.getBufferResourceByName(name);
	}
	else if (stage == sce::Gnm::kShaderStagePs)
	{
		return pixel_shader.program.getBufferResourceByName(name);
	}
	

	return nullptr;
}

std::pair<sce::Gnm::ShaderStage, int> shader_program::get_constant_buffer_slot(const char* name)
{
	sce::Shader::Binary::Buffer* constant_buffer = vertex_shader.program.getBufferResourceByName(name);
	if (constant_buffer)
		return { sce::Gnm::kShaderStageVs, constant_buffer->m_resourceIndex };

	constant_buffer = pixel_shader.program.getBufferResourceByName(name);
	if (constant_buffer)
		return { sce::Gnm::kShaderStagePs, constant_buffer->m_resourceIndex };

	return { sce::Gnm::kShaderStagePs, -1 };
}

void shader_program::set_uniform_data(sce::Gnmx::LightweightGfxContext& ctx, sce::Gnm::ShaderStage stage, const char* name, const void* data, size_t size)
{
	auto [s, slot] = get_constant_buffer_slot(name);
	if (slot == -1)
		return;

	if (s != stage)
		return;

	auto* buffer = ctx.allocateFromCommandBuffer(size, sce::Gnm::kEmbeddedDataAlignment4);
	memcpy(buffer, data, size);

	sce::Gnm::Buffer constant_buffer;
	constant_buffer.initAsConstantBuffer(buffer, size);
	constant_buffer.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeRO);

	ctx.setConstantBuffers(stage, slot, 1, &constant_buffer);
}


