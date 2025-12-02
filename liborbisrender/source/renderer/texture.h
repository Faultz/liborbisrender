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
	texture() {}
	// loads texture from file or link.
	texture(const std::string& path, bool should_use_cache = false);
	// creates empty texture with width & height.
	texture(const void* data, int width, int height, sce::Gnm::DataFormat format = sce::Gnm::kDataFormatR8G8B8A8Unorm);

	~texture();

	operator bool() const { return initialized; }

	void register_resource(std::string name);

	bool decode_png(const std::string& file);
	bool decode_jpeg(const std::string& file);
	bool decode_gnf(const std::string& file);

	bool decode(const std::string& file);

	bool encode_png(const std::string& file);
	bool encode_jpeg(const std::string& file);
	bool encode_gnf(const std::string& file);

	bool create(const std::string& file, bool should_use_cache = false);
	bool create(int width, int height, bool tiled = true);
	bool create_from_data(const void* data, int width, int height, sce::Gnm::DataFormat format = sce::Gnm::kDataFormatR8G8B8A8Unorm, bool tiled = false);

	static inline liborbisutil::memory::direct_memory_allocator* allocator;
private:
	bool initialized = false;
	sce::Gnm::DataFormat format;
	std::string file_path;
	bool should_use_cache = false;
	void* pixels = nullptr;
	sce::Gnm::ResourceHandle resource_handle;

	static inline int texture_count = 0;
	static constexpr const char* texture_cache_path = "/data/liborbisrender/cache/textures";
};
