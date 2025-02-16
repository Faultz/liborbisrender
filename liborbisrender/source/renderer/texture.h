#pragma once

#include <liborbisutil.h>

#include <string>

#include <gnf.h>

#include <png_dec.h>
#include <png_enc.h>

#include <jpeg_dec.h>
#include <jpeg_enc.h>

#include <video_out.h>

class render_context;

class texture : public sce::Gnm::Texture
{
public:
	texture() = default;
	texture(const std::string& file, bool should_use_cache = false);
	texture(int width, int height);
	texture(const void* data, int width, int height);

	~texture();

	operator bool() const { return initialized; }

	bool decode_png(const std::string& file);
	bool decode_jpeg(const std::string& file);
	bool decode_gnf(const std::string& file);

	bool decode(const std::string& file);

	bool encode_png(const std::string& file);
	bool encode_jpeg(const std::string& file);
	bool encode_gnf(const std::string& file);

	bool create(const std::string& file);
	bool create(int width, int height, bool tiled = true);
	bool create_from_data(const void* data, int width, int height, bool tiled = false);

	static inline liborbisutil::memory::direct_memory_allocator* allocator;
private:
	bool initialized = false;
	std::string file_path;
	bool should_use_cache = false;
	void* data = nullptr;
	
	static constexpr const char* texture_cache_path = "/data/liborbisrender/cache/textures";
};
