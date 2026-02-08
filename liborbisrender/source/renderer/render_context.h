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

	FlipModeVSync = 1 << 9,

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
	// Delete constructors to prevent instantiation
	render_context() = delete;
	render_context(const render_context&) = delete;
	render_context& operator=(const render_context&) = delete;

	static bool create(uint32_t flags, std::function<void(int)> user_callback = nullptr, std::function<void(ImGuiIO&)> load_fonts_cb = nullptr);
	static void release();

	static bool begin_frame(int flip_index);
	static void update_frame();
	static void end_frame();

	static void stall();
	static void advance_frame();

	static texture create_texture(const std::string& file, bool should_use_cache = false);

	static const int get_target_count();
	static bool is_target_srgb();
	static bool is_initialized();

	static uint32_t get_video_handle() { return video_out_handle; }

	// game theory
	inline static liborbisutil::thread submit_thread;
	inline static liborbisutil::thread render_thread;

	// per frame context
	inline static uint32_t prev_frame_index;
	inline static uint32_t curr_frame_index;

	inline static frame_context* prev_frame_context;
	inline static frame_context* curr_frame_context;

	inline static sce::Gnmx::LightweightGfxContext* current_lw_context;

	inline static liborbisutil::memory::direct_memory_allocator* garlic_memory_allocator;
	inline static liborbisutil::memory::direct_memory_allocator* onion_memory_allocator;
	inline static SceVideoOutBuffers* video_out_info;
	inline static debug_context debug_ctx;

	inline static std::vector<sce::Gnm::RenderTarget> render_targets;
	inline static bool target_is_srgb;
private:
	static bool create_garlic_allocator(size_t size);
	static bool create_onion_allocator(size_t size);
	static bool create_device();
	static bool create_render_targets();
	static bool create_event_queue();
	static bool create_contexts();

	static void clear_submits();

	static void release_hooks();
	static void release_garlic_allocator();
	static void release_onion_allocator();
	static void release_device();
	static void release_render_targets();
	static void release_event_queue();
	static void release_contexts();

	static void dump();

	// per instance context
	inline static sce::Gnmx::LightweightGfxContext* context;
	inline static frame_context* frame_contexts[3];
	inline static eqevent eop_event_queue;
	inline static int target_count;

	// user callback (supplied to the renderer, used in hooks)
	inline static std::function<void(int)> user_callback;

	inline static uint32_t flags;

	// release objects (used when releasing)
	inline static volatile uint64_t* release_label;
	inline static const uint64_t release_value = 1; // (label_free)

	inline static uint32_t video_out_handle;
	inline static uint64_t video_out_module_base;

	inline static std::vector<texture> textures;

	inline static bool dump_dcb;

	// detours...
	inline static liborbisutil::hook::manager detour_manager;

	// hooks...
	inline static liborbisutil::hook::detour sceGnmSubmitAndFlipCommandBuffer_d;
	inline static liborbisutil::hook::detour sceGnmSubmitAndFlipCommandBufferForWorkload_d;
	inline static liborbisutil::hook::detour sceVideoOutSubmitFlip_d;

	static int sceGnmSubmitAndFlipCommandBuffers_h(uint32_t count, void* dcbGpuAddrs[], uint32_t* dcbSizesInBytes, void* ccbGpuAddrs[], uint32_t* ccbSizesInBytes, uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg);
	static int sceGnmSubmitAndFlipCommandBuffersForWorkload_h(int workloadId, uint32_t count, void* dcbGpuAddrs[], uint32_t* dcbSizesInBytes, void* ccbGpuAddrs[], uint32_t* ccbSizesInBytes, uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg);
	static int sceVideoOutSubmitFlip_h(uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg);
};