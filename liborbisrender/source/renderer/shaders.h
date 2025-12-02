#pragma once

#include <liborbisutil.h>
#include <shader.h>
#include <gnmx/shader_parser.h>

struct srv_binding_mask
{
	uint64_t tex;
	uint64_t buf;

	srv_binding_mask()
	{
		clear();
	}

	void clear()
	{
		tex = 0;
		buf = 0;
	}

	void set_all()
	{
		tex = UINT64_MAX;
		buf = UINT64_MAX;
	}

	void set_texture(uint32_t slot)
	{
		tex |= (1ULL << slot);
	}

	void set_buffer(uint32_t slot)
	{
		buf |= (1ULL << slot);
	}

	srv_binding_mask& operator &=(const srv_binding_mask& o)
	{
		tex &= o.tex;
		buf &= o.buf;
		return *this;
	}
	srv_binding_mask& operator ^=(const srv_binding_mask& o)
	{
		tex ^= o.tex;
		buf ^= o.buf;
		return *this;
	}
};

struct shader_parameter
{
	std::string name;
	sce::Shader::Binary::Buffer* buffer;
	uint32_t slot;
	size_t size;
	size_t offset;
};

const uint32_t MAX_CONSTANT_BUFFERS = 14;
const uint32_t MAX_SRV_BINDING = 64;
const uint32_t MAX_SHADER_PARAMS = 128;

template<class T>
struct shader
{
	shader() = default;
	~shader() = default;

	operator T* () 
	{
		return m_shader;
	}

	T* m_shader;

	void* binary;
	sce::Shader::Binary::Program program;
	sce::Gnmx::InputResourceOffsets resource_table;
	sce::Gnmx::InputOffsetsCache offset_cache;

	uint32_t constant_buffer_mask;
	srv_binding_mask srv_mask;
	uint32_t srt_buffer_mask;

	shader_parameter parameters[MAX_SHADER_PARAMS];
	sce::Gnm::ResourceHandle resource_handle;
};

class shader_program
{
public:
	shader_program() {};
	shader_program(const std::string& name, void* vertexShaderBinary, size_t vertexShaderBinarySize, void* pixelShaderBinary, size_t pixelShaderBinarySize, liborbisutil::memory::direct_memory_allocator* allocator);
	shader_program(const std::string& name, const std::string vertexShaderFile, const std::string pixelShaderFile, liborbisutil::memory::direct_memory_allocator* allocator);

	void load_vertex_shader(void* shaderBinary, size_t shaderBinarySize, liborbisutil::memory::direct_memory_allocator* allocator);
	void load_vertex_shader(const std::string filename, liborbisutil::memory::direct_memory_allocator* allocator);
	void load_pixel_shader(void* shaderBinary, size_t shaderBinarySize, liborbisutil::memory::direct_memory_allocator* allocator);
	void load_pixel_shader(const std::string filename, liborbisutil::memory::direct_memory_allocator* allocator);
	void load_fetch_shader(void* shaderBinary, liborbisutil::memory::direct_memory_allocator* allocator);
	
	template<class T>
	void load_program(shader<T>* shader, void* shaderBinary, size_t shaderBinarySize, liborbisutil::memory::direct_memory_allocator* allocator);

	void bind(sce::Gnmx::LightweightGfxContext& ctx);

	sce::Shader::Binary::Buffer* get_constant_buffer(sce::Gnm::ShaderStage stage, const char* name);
	std::pair<sce::Gnm::ShaderStage, int> get_constant_buffer_slot(const char* name);

	void set_uniform_data(sce::Gnmx::LightweightGfxContext& ctx, sce::Gnm::ShaderStage stage, const char* name, const void* data, size_t size);

	template<typename T>
	void set_uniform_data(sce::Gnmx::LightweightGfxContext& ctx, sce::Gnm::ShaderStage stage, const char* name, const T& data)
	{
		set_uniform_data(stage, name, &data, sizeof(T));
	}

	sce::Gnmx::VsShader* getVertexShader()
	{
		return vertex_shader.m_shader;
	}

	sce::Gnmx::PsShader* getPixelShader()
	{
		return pixel_shader.m_shader;
	}

	std::string name;
	shader<sce::Gnmx::VsShader> vertex_shader;
	shader<sce::Gnmx::PsShader> pixel_shader;
	void* fetch_shader;
};