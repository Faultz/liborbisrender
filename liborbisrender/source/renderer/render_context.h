#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <kernel.h>

#include <gnm.h>
#include <gnmx.h>
#include <shader_liveediting.h>

#include <liborbisutil.h>
#include "gui/backend/imgui_impl_orbis.h"
#include "gui/backend/imgui_impl_gnm.h"

#include "texture.h"

struct SceVideoOutBuffer
{
	void* videoBuffer;
	size_t idk;
};

struct SceVideoOutBuffers
{
	char _0x00[0x48];
	SceVideoOutBuffer buffers[5];	// 0x48
	char _0x98[0xF8];
	unsigned int formatBits;		// 0x190
	char _0x194[0x08];
	int width;						// 0x19C
	int height;						// 0x1A0
};

enum render_label
{
	label_active = 0x0,
	label_free = 0x1,
};

enum render_flags
{
	RenderBeforeFlip = 1 << 0,

	SubmitSelf = 1 << 1,

	FunctionImGui = 1 << 2,
	FunctionRenderDebug = 1 << 3,

	HookFlip = 1 << 4,
	HookFlipForWorkload = 1 << 5,
	HookFlipVideoOut = 1 << 6,

	StateRunning = 1 << 7,
	StateDestroying = 1 << 8,

	UnlockFps = 1 << 10
};

struct frame_context
{
	sce::Gnmx::LightweightGfxContext* context;
	sce::Gnm::RenderTarget* target;
	volatile uint64_t* context_label;
};

struct debug_context
{
	uint64_t last_frame_time;
	std::vector<float> frame_fps_history;
};

class eqevent
{
public:
	eqevent() = default;
	~eqevent();
	void create(const std::string& name);
	void release();

	bool wait();

	SceKernelEqueue equeue;
	std::string name;
};

class render_context
{
public:
	bool create(uint32_t flags, std::function<void(int)> user_callback = nullptr, std::function<void(ImGuiIO&)> load_fonts_cb = nullptr);
	void release();

	bool begin_frame(int flip_index);
	void update_frame();
	void end_frame();

	void stall();
	void advance_frame();

	texture create_texture(const std::string& file, bool should_use_cache = false);

	static render_context* get_instance();

	const int get_target_count() const;
	bool is_target_srgb() const;
	bool is_initialized() const;

	uint32_t get_video_handle() const { return video_out_handle; }

	// game theory
	liborbisutil::thread submit_thread;
	liborbisutil::thread render_thread;

	// per frame context
	uint32_t prev_frame_index;
	uint32_t curr_frame_index;

	frame_context* prev_frame_context;
	frame_context* curr_frame_context;

	sce::Gnmx::LightweightGfxContext* current_lw_context;

	inline static liborbisutil::memory::direct_memory_allocator* garlic_memory_allocator;
	inline static liborbisutil::memory::direct_memory_allocator* onion_memory_allocator;
	inline static SceVideoOutBuffers* video_out_info;
	inline static debug_context debug_ctx;

	std::vector<sce::Gnm::RenderTarget> render_targets;
	bool target_is_srgb;
private:
	bool create_garlic_allocator(size_t size);
	bool create_onion_allocator(size_t size);
	bool create_device();
	bool create_render_targets();
	bool create_event_queue();
	bool create_contexts();

	void clear_submits();

	void release_hooks();
	void release_garlic_allocator();
	void release_onion_allocator();
	void release_device();
	void release_render_targets();
	void release_event_queue();
	void release_contexts();

	// per instance context
	sce::Gnmx::LightweightGfxContext* context;
	frame_context* frame_contexts[3];
	eqevent eop_event_queue;
	int target_count;

	// user callback (supplied to the renderer, used in hooks)
	std::function<void(int)> user_callback;

	uint32_t flags;

	// release objects (used when releasing)
	volatile uint64_t* release_label;
	const uint64_t release_value = 1; // (label_free)

	uint32_t video_out_handle;
	uint64_t video_out_module_base;

	std::vector<texture> textures;

	// detours...
	inline static liborbisutil::hook::manager detour_manager;

	// hooks...
	liborbisutil::hook::detour sceGnmSubmitAndFlipCommandBuffer_d;
	liborbisutil::hook::detour sceGnmSubmitAndFlipCommandBufferForWorkload_d;
	liborbisutil::hook::detour sceVideoOutSubmitFlip_d;

	static int sceGnmSubmitAndFlipCommandBuffers_h(uint32_t count, void* dcbGpuAddrs[], uint32_t* dcbSizesInBytes, void* ccbGpuAddrs[], uint32_t* ccbSizesInBytes, uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg);
	static int sceGnmSubmitAndFlipCommandBuffersForWorkload_h(int workloadId, uint32_t count, void* dcbGpuAddrs[], uint32_t* dcbSizesInBytes, void* ccbGpuAddrs[], uint32_t* ccbSizesInBytes, uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg);
	static int sceVideoOutSubmitFlip_h(uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg);

	inline static render_context* instance_obj;
};