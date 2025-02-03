#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <kernel.h>

#include <gnm.h>
#include <gnmx.h>

#include <liborbisutil.h>

#include "../source/renderer/gui/backend/imgui_impl_orbis.h"
#include "../source/renderer/gui/backend/imgui_impl_gnm.h"

namespace renderer
{
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
		label_inuse = 0x0,
		label_free = 0x1,
	};

	enum render_flags
	{
		RenderAfterFlip = 1 << 0,

		FunctionImGui = 1 << 1,
		FunctionRenderDebug = 1 << 2,

		HookFlip = 1 << 3,
		HookFlipForWorkload = 1 << 4,
		HookFlipVideoOut = 1 << 5,

		StateRunning = 1 << 6,
		StateDestroying = 1 << 7,

		UnlockFps = 1 << 8,
	};

	struct frame_context
	{
		sce::Gnmx::LightweightGfxContext* context;
		sce::Gnm::RenderTarget* target;
	};

	class render_context
	{
	public:
		render_context() = default;
		~render_context();

		bool create(uint32_t flags, std::function<void(int)> user_callback, std::function<void(ImGuiIO&)> load_fonts_cb = nullptr);
		void release();

		bool begin_scene(int flip_index);
		void update_scene();
		void flip_scene(int flip_index);
		void end_scene();

		static render_context* get_instance();

		const int get_target_count() const;
		bool is_target_srgb() const;
		bool is_initialized() const;

		// per frame context
		uint32_t prev_frame_index;
		uint32_t curr_frame_index;
		uint32_t next_frame_index;

		frame_context* prev_frame_context;
		frame_context* curr_frame_context;
		sce::Gnmx::LightweightGfxContext* current_lw_context;

		inline static liborbisutil::memory::direct_memory_alloactor* garlic_memory_allocator;
		inline static liborbisutil::memory::direct_memory_alloactor* onion_memory_allocator;
		inline static SceVideoOutBuffers* video_out_info;

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
		std::vector<frame_context*> frame_contexts;
		std::vector<sce::Gnm::RenderTarget> render_targets;
		volatile uint64_t* context_label;
		SceKernelEqueue eop_event_queue;
		int target_count;

		// user callback (supplied to the renderer, used in hooks)
		std::function<void(int)> user_callback;

		uint32_t flags;

		// release objects (called in destructor)
		volatile uint64_t* release_label;
		const uint64_t release_value = 1;

		uint32_t video_out_handle;
		uint64_t video_out_module_base;

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
}