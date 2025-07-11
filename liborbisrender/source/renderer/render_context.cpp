#include "render_context.h"

void* allocMem(size_t size, void* userData)
{
	return render_context::onion_memory_allocator->allocate(size, 8, "");
}
void freeMem(void* ptr, void* userData)
{
	render_context::onion_memory_allocator->free(ptr);
}

bool render_context::create(uint32_t flags, std::function<void(int)> user_callback, std::function<void(ImGuiIO&)> load_fonts_cb)
{
	if (is_initialized())
	{
		return false;
	}

	memset(this, 0, sizeof(render_context));

	this->instance_obj = this;
	this->flags = flags;
	this->user_callback = user_callback;

	if (!create_garlic_allocator(25_MB))
	{
		return false;
	}

	if (!create_onion_allocator(25_MB))
	{
		return false;
	}

	texture::allocator = garlic_memory_allocator;

	debug_ctx.frame_fps_history.resize(120, 0.0f);
	debug_ctx.last_frame_time = sceKernelGetProcessTime();

	release_label = reinterpret_cast<volatile uint64_t*>(garlic_memory_allocator->allocate(sizeof(uint64_t), 8, "release label"));
	*release_label = label_active;

	auto videoOutBase = liborbisutil::resolve::get_module_address<uintptr_t>("libSceVideoOut.sprx");

	static auto sceVideoOutGet = (void* (*)(int))(videoOutBase + 0xA600);
	video_out_handle = *(int*)(videoOutBase + 0x1CAD0);
	video_out_info = *(SceVideoOutBuffers**)sceVideoOutGet(video_out_handle);

	int buffer_count = 0;
	while (true) {
		auto buffer = video_out_info->buffers[buffer_count].videoBuffer;
		if (buffer == 0)
			break;

		buffer_count++;
	}

	this->target_count = buffer_count;

	if (!create_device())
	{
		return false;
	}

	create_render_targets();
	create_contexts();
	create_event_queue();

	current_lw_context = context.get();
	prev_frame_index = 0;
	curr_frame_index = 0;
	next_frame_index = 0;
	curr_frame_context = &frame_contexts[0];
	prev_frame_context = &frame_contexts[1];

	if (flags & FunctionImGui)
	{
		ImGui::SetAllocatorFunctions(allocMem, freeMem, nullptr);

		ImGui::CreateContext();
		ImGui_ImplOrbis_Init(this);

		ImGui_ImplGnm_Init(load_fonts_cb);
	}

	this->flags |= StateRunning;

	const char* gnm_driver_libraries[] = {
		"libSceGnmDriverForNeoMode.sprx",
		"libSceGnmDriver.sprx",
	};

	bool is_gnm_driver_loaded = false;
	bool is_sce_video_out_loaded = false;

	auto gnm_driver_library = sceKernelIsNeoMode() ? gnm_driver_libraries[0] : gnm_driver_libraries[1];
	auto driver_handle = sceKernelLoadStartModule(gnm_driver_library, 0, nullptr, 0, nullptr, nullptr);
	if (driver_handle < 0)
	{
		LOG_ERROR("failed to load driver library: 0x%08X\n", driver_handle);
	}
	else
	{
		is_gnm_driver_loaded = true;
	}

	auto sce_video_out_handle = sceKernelLoadStartModule("libSceVideoOut.sprx", 0, nullptr, 0, nullptr, nullptr);
	if (sce_video_out_handle < 0)
	{
		LOG_ERROR("failed to load sce video out library: 0x%08X\n", sce_video_out_handle);
	}
	else
	{
		is_sce_video_out_loaded = true;
	}

	if (!is_gnm_driver_loaded || !is_sce_video_out_loaded)
	{
		return false;
	}

	if (is_gnm_driver_loaded)
	{
		if (this->flags & HookFlip)
		{
			detour_manager.create("sceGnmSubmitAndFlipCommandBuffers", gnm_driver_library, "sceGnmSubmitAndFlipCommandBuffers", sceGnmSubmitAndFlipCommandBuffers_h);

			sceGnmSubmitAndFlipCommandBuffer_d = *detour_manager.get("sceGnmSubmitAndFlipCommandBuffers");
			sceGnmSubmitAndFlipCommandBuffer_d.enable();
		}

		if (this->flags & HookFlipForWorkload)
		{
			detour_manager.create("sceGnmSubmitAndFlipCommandBuffersForWorkload", gnm_driver_library, "sceGnmSubmitAndFlipCommandBuffersForWorkload", sceGnmSubmitAndFlipCommandBuffersForWorkload_h);

			sceGnmSubmitAndFlipCommandBufferForWorkload_d = *detour_manager.get("sceGnmSubmitAndFlipCommandBuffersForWorkload");
			sceGnmSubmitAndFlipCommandBufferForWorkload_d.enable();
		}
	}

	if (is_sce_video_out_loaded)
	{
		if (this->flags & HookFlipVideoOut)
		{
			detour_manager.create("sceVideoOutSubmitFlip", "libSceVideoOut.sprx", "sceVideoOutSubmitFlip", sceVideoOutSubmitFlip_h);

			sceVideoOutSubmitFlip_d = *detour_manager.get("sceVideoOutSubmitFlip");
			sceVideoOutSubmitFlip_d.enable();
		}
	}

	return true;
}
void render_context::release()
{
	if (!is_initialized())
	{
		return;
	}

	if (flags & StateDestroying)
		return;

	flags |= StateDestroying;

	release_hooks();
	clear_submits();

	// release any flags (imgui)
	// ...
	//
	if (flags & FunctionImGui)
	{
		LOG_INFO("destroying imgui context\n");

		ImGui_ImplOrbis_Shutdown();
		ImGui_ImplGnm_Shutdown();
		ImGui::DestroyContext();
	}

	// release all resources
	release_device();
	release_render_targets();
	release_event_queue();
	release_contexts();
	release_garlic_allocator();
	release_onion_allocator();
}

bool render_context::begin_scene(int flip_index)
{
	if (!is_initialized())
	{
		return false;
	}

	if (flags & StateDestroying)
		return false;

	prev_frame_index = curr_frame_index;
	curr_frame_index = flip_index;
	next_frame_index = (flip_index + 1) % target_count;
	curr_frame_context = &frame_contexts[curr_frame_index];
	prev_frame_context = &frame_contexts[prev_frame_index];
	current_lw_context = curr_frame_context->context;

	*curr_frame_context->context_label = label_active;

	current_lw_context->waitUntilSafeForRendering(video_out_handle, prev_frame_index);

	current_lw_context->reset();
	current_lw_context->initializeDefaultHardwareState();

	current_lw_context->setRenderTarget(0, curr_frame_context->target);
	current_lw_context->setRenderTargetMask(0xF);

	current_lw_context->setDepthRenderTarget(nullptr);

	current_lw_context->setScreenScissor(0, 0, video_out_info->width, video_out_info->height);
	current_lw_context->setupScreenViewport(0, 0, video_out_info->width, video_out_info->height, 0.5f, 0.5f);

	return true;
}
void render_context::update_scene()
{
	if (!is_initialized())
	{
		return;
	}

	if (flags & StateDestroying)
		return;

	if (flags & FunctionImGui)
	{
		ImGui_ImplOrbis_NewFrame();
		ImGui_ImplGnm_NewFrame();
		ImGui::NewFrame();

		if (flags & FunctionRenderDebug)
		{
			auto last_frame_time = debug_ctx.last_frame_time;
			auto current_time = sceKernelGetProcessTime();
			float delta_time = current_time - last_frame_time;
			debug_ctx.last_frame_time = current_time;

			float frame_time = delta_time / 1000.0f;
			float fps = 1000.0f / frame_time;

			ImGui::Begin("Render Info");
			ImGui::Text("Framerate: %.2f", ImGui::GetIO().Framerate);
			ImGui::Text("Frame Time: %.2f ms", frame_time);
			ImGui::Text("FPS: %.1f", fps);

			ImGui::Text("Frame Index: %d", curr_frame_index);
			ImGui::Text("Next Frame Index: %d", next_frame_index);
			ImGui::End();
		}
	}
}

void render_context::end_scene()
{
	if (!is_initialized())
	{
		return;
	}

	if (flags & StateDestroying)
		return;

	if (flags & FunctionImGui)
	{
		ImGui::Render();
		ImGui_ImplGnm_RenderDrawData(ImGui::GetDrawData(), current_lw_context);
	}

	current_lw_context->writeAtEndOfPipe(sce::Gnm::kEopFlushCbDbCaches, sce::Gnm::kEventWriteDestMemory, (void*)curr_frame_context->context_label, sce::Gnm::kEventWriteSource64BitsImmediate,
		label_free, sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyBypass);
	current_lw_context->waitOnAddress((void*)curr_frame_context->context_label, label_free, sce::Gnm::kWaitCompareFuncEqual, 1);

	current_lw_context->submit();

	if (flags & SubmitSelf)
		sce::Gnm::submitDone();
}

texture render_context::create_texture(const std::string& file, bool should_use_cache)
{
	texture tex(file, should_use_cache);
	textures.push_back(tex);

	return tex;
}

render_context* render_context::get_instance()
{
	if(render_context::instance_obj)
		return render_context::instance_obj;

	static render_context instance_obj;
	return &instance_obj;
}
const int render_context::get_target_count() const
{
	return target_count;
}
bool render_context::is_target_srgb() const
{
	return target_is_srgb;
}
bool render_context::is_initialized() const
{
	return flags & StateRunning;
}

bool render_context::create_garlic_allocator(size_t size)
{
	garlic_memory_allocator = new liborbisutil::memory::direct_memory_allocator(size, SCE_KERNEL_WC_GARLIC);
	if (!garlic_memory_allocator->Initialised)
	{
		return false;
	}

	return true;
}
bool render_context::create_onion_allocator(size_t size)
{
	onion_memory_allocator = new liborbisutil::memory::direct_memory_allocator(size, SCE_KERNEL_WB_ONION);
	if (!onion_memory_allocator->Initialised)
	{
		return false;
	}
	return true;
}
bool render_context::create_device()
{
	auto device = std::make_unique<sce::Gnmx::LightweightGfxContext>();

	constexpr int k_command_buffer_size = 4_MB;
	const uint32_t k_num_batches = 100;
	const uint32_t resource_buffer_size = sce::Gnmx::LightweightConstantUpdateEngine::computeResourceBufferSize(sce::Gnmx::LightweightConstantUpdateEngine::kResourceBufferGraphics, k_num_batches);

	void* draw_command_buffer = onion_memory_allocator->allocate(k_command_buffer_size, 4, "draw command buffer");
	if (!draw_command_buffer)
	{
		LOG_ERROR("failed to allocate draw command buffer\n");
		return false;
	}

	void* resource_buffer = onion_memory_allocator->allocate(k_command_buffer_size, 4, "resource buffer");
	if (!resource_buffer)
	{
		LOG_ERROR("failed to allocate resource buffer\n");
		return false;
	}

	device->init(draw_command_buffer, k_command_buffer_size, resource_buffer, resource_buffer_size, nullptr);

	context = std::move(device);

	return true;
}
bool render_context::create_render_targets()
{
	render_targets.resize(get_target_count());

	for (int i = 0; i < get_target_count(); i++)
	{
		auto buffer = video_out_info->buffers[i].videoBuffer;
		auto format = sce::Gnm::kDataFormatR8G8B8A8UnormSrgb;

		target_is_srgb = true;

		switch (video_out_info->formatBits)
		{
		case 0x80000000:
			format = sce::Gnm::kDataFormatB8G8R8A8UnormSrgb;
			target_is_srgb = true;
			break;
		case 0x88000000:
			format = sce::Gnm::kDataFormatB10G10R10A2Unorm;
			target_is_srgb = false;
			break;
		}

		auto& target = render_targets[i];
		sce::Gnm::RenderTarget rt;
		sce::Gnm::TileMode tileMode;
		sce::GpuAddress::computeSurfaceTileMode(sce::Gnm::getGpuMode(), &tileMode, sce::GpuAddress::kSurfaceTypeColorTargetDisplayable, format, 1);

		sce::Gnm::RenderTargetSpec spec;
		spec.init();
		spec.m_width = video_out_info->width;
		spec.m_height = video_out_info->height;
		spec.m_pitch = 0;
		spec.m_numSlices = 1;
		spec.m_colorFormat = format;
		spec.m_colorTileModeHint = tileMode;
		spec.m_minGpuMode = sce::Gnm::getGpuMode();
		spec.m_numSamples = sce::Gnm::kNumSamples1;
		spec.m_numFragments = sce::Gnm::kNumFragments1;
		spec.m_colorTileModeHint = sce::Gnm::TileMode::kTileModeDisplay_2dThin;
		rt.init(&spec);
		rt.setBaseAddress(buffer);

		target = rt;
	}

	return true;
}
bool render_context::create_event_queue()
{
	SceKernelEqueue event_queue;
	auto res = sceKernelCreateEqueue(&event_queue, "eop event queue");
	if (res < 0)
	{
		LOG_ERROR("failed to create event queue: 0x%08X\n", res);
		return false;
	}

	sce::Gnm::addEqEvent(event_queue, sce::Gnm::kEqEventGfxEop, nullptr);
	eop_event_queue = event_queue;

	return true;
}
bool render_context::create_contexts()
{
	for (int i = 0; i < target_count; i++)
	{
		auto frame = new frame_context();
		frame->context = context.get();
		frame->target = &render_targets[i];

		frame->context_label = reinterpret_cast<volatile uint64_t*>(garlic_memory_allocator->allocate(sizeof(uint64_t), 8, "context label"));
		*frame->context_label = label_free;

		frame_contexts[i] = *frame;
	}

	prev_frame_context = &frame_contexts[0];
	curr_frame_context = &frame_contexts[1];

	return true;
}

void render_context::clear_submits()
{
	context->writeAtEndOfPipe(sce::Gnm::kEopCbDbReadsDone, sce::Gnm::kEventWriteDestMemory, (void*)release_label, sce::Gnm::kEventWriteSource64BitsImmediate, label_free, sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyBypass);
	context->submit();

	// wait for the release label to be written
	while (*release_label != label_free)
	{
	}

	context.reset();
}

void render_context::release_hooks()
{
	detour_manager.clear();
}
void render_context::release_garlic_allocator()
{
	if (garlic_memory_allocator)
	{
		delete garlic_memory_allocator;
		garlic_memory_allocator = nullptr;

		LOG_DEBUG("garlic memory allocator released\n");
	}
}
void render_context::release_onion_allocator()
{
	if (onion_memory_allocator)
	{
		delete onion_memory_allocator;
		onion_memory_allocator = nullptr;

		LOG_DEBUG("onion memory allocator released\n");
	}
}
void render_context::release_device()
{
	if (context)
	{
		context.reset();

		LOG_DEBUG("device released\n");
	}
}
void render_context::release_render_targets()
{
	render_targets.clear();
}
void render_context::release_event_queue()
{
	sceKernelDeleteEqueue(eop_event_queue);
	sce::Gnm::deleteEqEvent(eop_event_queue, sce::Gnm::kEqEventGfxEop);

	LOG_DEBUG("event queue released\n");

}
void render_context::release_contexts()
{
	LOG_DEBUG("contexts released\n");

	prev_frame_context = nullptr;
	curr_frame_context = nullptr;
}