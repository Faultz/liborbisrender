#include "texture.h"

#include <regex>
#include <libsysmodule.h>

#pragma comment(lib, "libScePngDec_stub_weak.a")
#pragma comment(lib, "libSceJpegDec_stub_weak.a")

texture::texture(const std::string& file, bool should_use_cache)
{
	std::string texture_name = fmt::format("texture_{}_{}", file, texture_count);

	if(!create(file, should_use_cache))
	{
		LOG_ERROR("Failed to create texture from file: %s\n", file.data());
	}

	sce::Gnm::registerResource(&resource_handle, texture::allocator->owner_handle, getBaseAddress(), getSizeAlign().m_size, texture_name.data(), sce::Gnm::kResourceTypeTextureBaseAddress, 0);

	texture_count++;
}

texture::texture(const void* data, int width, int height, sce::Gnm::DataFormat format)
{
	std::string texture_name = fmt::format("texture_{}_{}_{}", width, height, texture_count);

	if (!create_from_data(data, width, height, format))
	{
		LOG_ERROR("Failed to create texture from data(%lX) with %dx%d: %s\n", data, width, height, texture_name.data());
	}

	sce::Gnm::registerResource(&resource_handle, texture::allocator->owner_handle, getBaseAddress(), getSizeAlign().m_size, texture_name.data(), sce::Gnm::kResourceTypeTextureBaseAddress, 0);

	texture_count++;
}

texture::~texture()
{
	if (resource_handle) 
	{
		sce::Gnm::unregisterResource(resource_handle);
	}
}

void texture::register_resource(std::string name)
{
	sce::Gnm::registerResource(&resource_handle, texture::allocator->owner_handle, getBaseAddress(), getSizeAlign().m_size, name.data(), sce::Gnm::kResourceTypeTextureBaseAddress, 0);
}

bool texture::decode_png(const std::string& file)
{
	// Decode png file
	ScePngDecParseParam		parseParam;
	ScePngDecImageInfo		imageInfo;
	ScePngDecCreateParam	createParam;
	ScePngDecHandle			handle;
	ScePngDecDecodeParam	decodeParam;
	int32_t ret;

	size_t fileSize = liborbisutil::filesystem::get_size(file);

	std::unique_ptr<uint8_t[]> pngBuffer = std::make_unique<uint8_t[]>(fileSize);
	liborbisutil::filesystem::read_file(file, pngBuffer.get(), fileSize);

	// get image info.
	parseParam.pngMemAddr = pngBuffer.get();
	parseParam.pngMemSize = fileSize;
	parseParam.reserved0 = 0;
	ret = scePngDecParseHeader(&parseParam, &imageInfo);
	if (ret < 0) {
		if ((ret == SCE_PNG_DEC_ERROR_INVALID_ADDR)
			|| (ret == SCE_PNG_DEC_ERROR_INVALID_SIZE)
			|| (ret == SCE_PNG_DEC_ERROR_INVALID_PARAM)
			) {
			return -1;
		}
		return false;
	}

	// setup texture
	sce::Gnm::TextureSpec textureSpec;
	textureSpec.init();
	textureSpec.m_textureType = sce::Gnm::kTextureType2d;
	textureSpec.m_width = imageInfo.imageWidth;
	textureSpec.m_height = imageInfo.imageHeight;
	textureSpec.m_format = sce::Gnm::kDataFormatR8G8B8A8UnormSrgb;
	textureSpec.m_minGpuMode = sce::Gnm::getGpuMode();
	ret = init(&textureSpec);

	format = sce::Gnm::kDataFormatR8G8B8A8UnormSrgb;

	// allocate memory for output image
	const sce::Gnm::SizeAlign textureSizeAlign = getSizeAlign();
	setBaseAddress(allocator->allocate(textureSizeAlign.m_size, textureSizeAlign.m_align));
	if (!getBaseAddress()) {
		return false;
	}

	// query memory size for PNG decoder
	createParam.thisSize = sizeof(createParam);
	createParam.attribute = imageInfo.bitDepth >> 4;
	createParam.maxImageWidth = imageInfo.imageWidth;
	ret = scePngDecQueryMemorySize(&createParam);
	if (ret < 0) {
		return false;
	}

	// allocate memory for PNG decoder
	int32_t decSize = ret;
	std::unique_ptr<uint8_t[]> decoderMemory = std::make_unique<uint8_t[]>(decSize);
	if (decoderMemory.get() == nullptr) {
		return false;
	}

	// create PNG decoder
	ret = scePngDecCreate(&createParam, decoderMemory.get(), decSize, &handle);
	if (ret < 0) {
		return false;
	}

	// allocate for untiled surface
	std::unique_ptr<uint8_t[]> untiledSurface = std::make_unique<uint8_t[]>(textureSizeAlign.m_size);
	if (untiledSurface.get() == nullptr) {
		return false;
	}

	// decode PNG image
	decodeParam.pngMemAddr = pngBuffer.get();
	decodeParam.pngMemSize = fileSize;
	decodeParam.imageMemAddr = untiledSurface.get();
	decodeParam.imageMemSize = textureSizeAlign.m_size;
	decodeParam.imagePitch = getWidth() * 4;
	decodeParam.pixelFormat = SCE_PNG_DEC_PIXEL_FORMAT_R8G8B8A8;
	decodeParam.alphaValue = 255;
	ret = scePngDecDecode(handle, &decodeParam, nullptr);
	if (ret < 0) {
		scePngDecDelete(handle);
		return false;
	}

	// delete PNG decoder
	ret = scePngDecDelete(handle);
	if (ret < 0) {
		if (ret == SCE_PNG_DEC_ERROR_INVALID_HANDLE) {
			return false;
		}
		return false;
	}

	sce::GpuAddress::TilingParameters tp;
	ret = tp.initFromTexture(this, 0, 0);
	if (ret != SCE_OK)
		return false;

	// tileSurface
	ret = sce::GpuAddress::tileSurface(getBaseAddress(), untiledSurface.get(), &tp);
	if (ret != SCE_OK) {
		return false;
	}

	initialized = true;
	pixels = getBaseAddress();

	return true;
}

bool texture::decode_jpeg(const std::string& file)
{
	SceJpegDecParseParam	parseParam;
	SceJpegDecImageInfo		imageInfo;
	SceJpegDecCreateParam	createParam;
	SceJpegDecHandle		handle;
	SceJpegDecDecodeParam	decodeParam;
	int32_t ret;

	size_t fileSize = liborbisutil::filesystem::get_size(file);

	std::unique_ptr<uint8_t[]> jpegBuffer = std::make_unique<uint8_t[]>(fileSize);
	liborbisutil::filesystem::read_file(file, jpegBuffer.get(), fileSize);

	// get image info.
	parseParam.jpegMemAddr = jpegBuffer.get();
	parseParam.jpegMemSize = fileSize;
	parseParam.decodeMode = SCE_JPEG_DEC_DECODE_MODE_NORMAL;
	parseParam.downScale = 1;
	ret = sceJpegDecParseHeader(&parseParam, &imageInfo);
	if (ret < 0) {
		if ((ret == SCE_JPEG_DEC_ERROR_INVALID_ADDR)
			|| (ret == SCE_JPEG_DEC_ERROR_INVALID_SIZE)
			|| (ret == SCE_JPEG_DEC_ERROR_INVALID_PARAM)
			) {
			return false;
		}
		return false;
	}

	if (imageInfo.colorSpace == SCE_JPEG_DEC_COLOR_SPACE_UNKNOWN)
	{
		return false;
	}

	// setup texture
	sce::Gnm::TextureSpec textureSpec;
	textureSpec.init();
	textureSpec.m_textureType = sce::Gnm::kTextureType2d;
	textureSpec.m_width = imageInfo.outputImageWidth;
	textureSpec.m_height = imageInfo.outputImageHeight;
	textureSpec.m_format = sce::Gnm::kDataFormatR8G8B8A8UnormSrgb;
	textureSpec.m_minGpuMode = sce::Gnm::getGpuMode();
	ret = init(&textureSpec);

	format = sce::Gnm::kDataFormatR8G8B8A8UnormSrgb;

	// allocate memory for output image
	const sce::Gnm::SizeAlign textureSizeAlign = getSizeAlign();
	setBaseAddress(allocator->allocate(textureSizeAlign.m_size, textureSizeAlign.m_align));
	if (nullptr == getBaseAddress())
	{
		return false;
	}

	// allocate coefficient memory to be used for decoding progressive JPEG, if needed
	std::unique_ptr<uint8_t[]> coefficientMemory;
	if (imageInfo.coefficientMemSize > 0) {
		coefficientMemory = std::make_unique<uint8_t[]>(imageInfo.coefficientMemSize);
		if (coefficientMemory.get() == nullptr) {
			return false;
		}
	}

	// query memory size for JPEG decoder
	createParam.thisSize = sizeof(createParam);
	createParam.attribute = imageInfo.suitableCscAttribute;
	createParam.maxImageWidth = imageInfo.outputImageWidth;
	ret = sceJpegDecQueryMemorySize(&createParam);
	if (ret < 0) {
		if ((ret == SCE_JPEG_DEC_ERROR_INVALID_ADDR)
			|| (ret == SCE_JPEG_DEC_ERROR_INVALID_SIZE)
			|| (ret == SCE_JPEG_DEC_ERROR_INVALID_PARAM)
			) {
			return false;
		}
		return false;
	}

	// allocate memory for JPEG decoder
	size_t decoderSize = ret;
	std::unique_ptr<uint8_t[]> decoderMemory = std::make_unique<uint8_t[]>(decoderSize);
	if (decoderMemory.get() == nullptr) {
		return false;
	}

	// create JPEG decoder
	ret = sceJpegDecCreate(&createParam, decoderMemory.get(), decoderSize, &handle);
	if (ret < 0) {
		if ((ret == SCE_JPEG_DEC_ERROR_INVALID_ADDR)
			|| (ret == SCE_JPEG_DEC_ERROR_INVALID_SIZE)
			|| (ret == SCE_JPEG_DEC_ERROR_INVALID_PARAM)
			) {
			return false;
		}
		return false;
	}

	// allocate for untiled surface
	std::unique_ptr<uint8_t[]> untiledSurface = std::make_unique<uint8_t[]>(textureSizeAlign.m_size);
	if (untiledSurface.get() == nullptr) {
		return false;
	}

	// decode JPEG image
	decodeParam.jpegMemAddr = jpegBuffer.get();
	decodeParam.jpegMemSize = fileSize;
	decodeParam.imageMemAddr = untiledSurface.get();
	decodeParam.imageMemSize = textureSizeAlign.m_size;
	decodeParam.decodeMode = parseParam.decodeMode;
	decodeParam.downScale = parseParam.downScale;
	decodeParam.pixelFormat = SCE_JPEG_DEC_PIXEL_FORMAT_R8G8B8A8;
	decodeParam.imagePitch = getWidth() * 4;
	decodeParam.alphaValue = 255;
	decodeParam.coefficientMemAddr = (imageInfo.coefficientMemSize > 0) ? coefficientMemory.get() : nullptr;
	decodeParam.coefficientMemSize = imageInfo.coefficientMemSize;
	ret = sceJpegDecDecode(handle, &decodeParam, nullptr);
	if (ret < 0) {
		sceJpegDecDelete(handle);
		if ((ret == SCE_JPEG_DEC_ERROR_INVALID_ADDR)
			|| (ret == SCE_JPEG_DEC_ERROR_INVALID_SIZE)
			|| (ret == SCE_JPEG_DEC_ERROR_INVALID_PARAM)
			|| (ret == SCE_JPEG_DEC_ERROR_INVALID_HANDLE)
			|| (ret == SCE_JPEG_DEC_ERROR_INVALID_COEF_MEMORY)
			) {
			return false;
		}
		return false;
	}

	// delete JPEG decoder
	ret = sceJpegDecDelete(handle);
	if (ret < 0) {
		if (ret == SCE_JPEG_DEC_ERROR_INVALID_HANDLE) {
			return false;
		}
		return false;
	}

	sce::GpuAddress::TilingParameters tp;
	ret = tp.initFromTexture(this, 0, 0);
	if (ret != SCE_OK)
		return false;

	// tileSurface
	ret = sce::GpuAddress::tileSurface(getBaseAddress(), untiledSurface.get(), &tp);

	if (ret != SCE_OK) {
		return false;
	}

	initialized = true;
	pixels = getBaseAddress();

	return true;
}

bool texture::decode_gnf(const std::string& file)
{
	struct GnfHeader : public sce::Gnf::Header
	{
		uint8_t	 m_version;
		uint8_t  m_count;
		uint8_t  m_alignment;
		uint8_t  m_unused;
		uint32_t m_streamSize;
	} header{};
	int index = 0;

	liborbisutil::file_system fs(file, "rb");
	if (!fs.is_open())
	{
		LOG_ERROR("Failed to open file %s\n", file.c_str());
		return false;
	}

	size_t file_size = fs.get_size();
	if (file_size < sizeof(GnfHeader))
	{
		LOG_ERROR("File is too small to be a GNF\n");
		return false;
	}

	fs.read(&header, sizeof(GnfHeader), 1);
	if(header.m_magicNumber != sce::Gnf::kMagic)
	{
		LOG_ERROR("Invalid magic number\n");
		return false;
	}

	fs.seek(sizeof(sce::Gnm::Texture) * index, 1);
	fs.read(reinterpret_cast<char*>(this), sizeof(*this));
	fs.seek(sizeof(sce::Gnm::Texture) * (header.m_count - (index + 1)), 1);

	void* texture_buffer = allocator->allocate(getSizeAlign());
	if (!texture_buffer)
	{
		LOG_ERROR("Failed to allocate texture buffer\n");
		return false;
	}

	int32_t data_offset = header.m_contentsSize + sizeof(sce::Gnf::Header);
	fs.seek(data_offset, 0);
	fs.read(static_cast<char*>(texture_buffer), getSizeAlign().m_size);
	setBaseAddress(texture_buffer);

	fs.close();

	format = getDataFormat();

	initialized = true;
	pixels = texture_buffer;

	return true;
}

bool texture::decode(const std::string& file)
{
	if (liborbisutil::string::ends_with(file, ".png"))
		return decode_png(file);

	if (liborbisutil::string::ends_with(file, ".jpg") || liborbisutil::string::ends_with(file, ".jpeg"))
		return decode_jpeg(file);

	if (liborbisutil::string::ends_with(file, ".gnf"))
		return decode_gnf(file);

	LOG_ERROR("Unsupported file format\n");
	return false;
}

bool texture::encode_png(const std::string& file)
{
	return false;
}

bool texture::encode_jpeg(const std::string& file)
{
	return false;
}

bool texture::encode_gnf(const std::string& file)
{
	return false;
}

bool texture::create(const std::string& path, bool should_use_cache)
{
	// make sure png dec & jpeg dec is loaded
	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_PNG_DEC) != SCE_OK)
	{
		if (sceSysmoduleLoadModule(SCE_SYSMODULE_PNG_DEC) != SCE_OK)
		{
			LOG_ERROR("Failed to load PNG dec\n");
			return false;
		}
	}

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_JPEG_DEC) != SCE_OK)
	{
		if (sceSysmoduleLoadModule(SCE_SYSMODULE_JPEG_DEC) != SCE_OK)
		{
			LOG_ERROR("Failed to load JPEG dec\n");
			return false;
		}
	}

	if (allocator == nullptr)
	{
		LOG_ERROR("Allocator is not set, are you sure you have created the render context?\n");
		return false;
	}

	// check if texture_cache_path exists.
	if (!liborbisutil::filesystem::exists(texture_cache_path))
	{
		// create texture_cache_path.
		if (!liborbisutil::filesystem::create_directory(texture_cache_path, true))
		{
			LOG_ERROR("Failed to create texture cache directory\n");
			return false;
		}
	}

	this->should_use_cache = should_use_cache;

	// check if the file is a link.
	std::smatch match;
	std::regex link_regex("^https?://\\S+$");

	if (std::regex_search(path, match, link_regex))
	{
		std::string link_file = match[0].str();
		size_t last_slash = link_file.find_last_of("/");

		std::string full_file_path = liborbisutil::string::format("%s/%s", texture_cache_path, link_file.substr(last_slash + 1, link_file.size() - last_slash).c_str());
	
		if (should_use_cache)
		{
			if (liborbisutil::filesystem::exists(full_file_path))
			{
				file_path = full_file_path;

				auto result = decode(full_file_path);
				if (!result)
				{
					LOG_ERROR("Failed to decode texture from cache: %s\n", full_file_path.data());
				}

				return result;
			}
		}

		auto response = liborbisutil::http::download(path, full_file_path);

		std::string original_file = full_file_path;
		if (response.success)
		{
			// check if the content type is an image.
			if (liborbisutil::string::contains(response.response_headers["Content-Type"], "image/png"))
			{
				if(!liborbisutil::string::ends_with(full_file_path, ".png"))
					full_file_path.append(".png");

				liborbisutil::filesystem::rename_file(original_file, full_file_path);

				return decode_png(full_file_path);
			}
			else if (liborbisutil::string::contains(response.response_headers["Content-Type"], "image/jpeg"))
			{
				if (!liborbisutil::string::ends_with(full_file_path, ".jpg") && !liborbisutil::string::ends_with(full_file_path, ".jpeg"))
					full_file_path.append(".jpg");

				liborbisutil::filesystem::rename_file(original_file, full_file_path);

				return decode_jpeg(full_file_path);
			}

			auto result = decode(full_file_path);
			if (!result)
			{
				LOG_ERROR("Failed to decode texture from link: %s\n", path.data());
			}
			 
			file_path = full_file_path;
			return result;
		}
	}

	auto result = decode(path);
	if (!result)
	{
		LOG_ERROR("Failed to decode texture from file: %s\n", path.data());
	}

	file_path = path;

	return result;
}

bool texture::create_from_data(const void* data, int width, int height, sce::Gnm::DataFormat format, bool tiled)
{
	sce::Gnm::TextureSpec spec;
	spec.init();
	spec.m_textureType = sce::Gnm::kTextureType2d;
	spec.m_width = width;
	spec.m_height = height;
	spec.m_format = format;

	this->format = format;

	int32_t status = init(&spec);
	if (status != 0)
		return false;

	size_t size = width * height * 4;
	void* texture_buffer = allocator->allocate(size);
	if (!texture_buffer)
	{
		LOG_ERROR("Failed to allocate texture buffer\n");
		return false;
	}

	if(data)
		memcpy(texture_buffer, data, size);

	if (tiled) 
	{
		sce::GpuAddress::TilingParameters tp;
		status = tp.initFromTexture(this, 0, 0);
		if (status != SCE_OK)
			return false;

		status = sce::GpuAddress::tileSurface(getBaseAddress(), texture_buffer, &tp);
		if (status != SCE_OK)
			return false;
	}

	initialized = true;
	this->pixels = texture_buffer;

	return true;
}
