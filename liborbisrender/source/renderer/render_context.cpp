#include "render_context.h"

const constexpr liborbisutil::memory::pattern<13> sceVideoOutGet_sig{
	"E8 {????????} 48 85 C0 74 ?? 49 89 C6"
};

const constexpr liborbisutil::memory::pattern<9> video_out_handle_sig{
	"8B 3D {????????} 45 31 FF"
};

void* allocMem(size_t size, void* userData)
{
	return render_context::onion_memory_allocator->allocate(size, 8, "");
}
void freeMem(void* ptr, void* userData)
{
	render_context::onion_memory_allocator->free(ptr);
}

eqevent::~eqevent()
{
}

void eqevent::create(const std::string& name)
{
	SceKernelEqueue event_queue;
	auto res = sceKernelCreateEqueue(&event_queue, name.data());
	if (res < 0)
	{
		LOG_ERROR("failed to create event queue: 0x%08X\n", res);
		return;
	}

	this->equeue = event_queue;
	this->name = name;
}

void eqevent::release()
{
	sceKernelDeleteEqueue(equeue);
}

bool eqevent::wait()
{
	SceKernelEvent ev;
	int num;
	sceKernelWaitEqueue(equeue, &ev, 1, &num, nullptr);
	return true;
}

bool render_context::create(uint32_t flags, std::function<void(int)> user_callback, std::function<void(ImGuiIO&)> load_fonts_cb)
{
	if (is_initialized())
	{
		return false;
	}

	render_context::flags = flags;
	render_context::user_callback = user_callback;

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

	auto sceVideoOutGet_addr = sceVideoOutGet_sig.sigscan("sceVideoOutGet", "libSceVideoOut.sprx");
	if (!sceVideoOutGet_addr)
	{
		LOG_ERROR("failed to find sceVideoOutGet address\n");
		return false;
	}

	sceVideoOutGet_addr = liborbisutil::memory::read_offset(sceVideoOutGet_addr);

	auto video_out_handle_addr = video_out_handle_sig.sigscan("video_out_handle", "libSceVideoOut.sprx");
	if (!video_out_handle_addr)
	{
		LOG_ERROR("failed to find video out handle address\n");
		return false;
	}

	video_out_handle_addr = liborbisutil::memory::read_offset(video_out_handle_addr);

#if defined(SCE_VIDEO_OUT_GET_OFFSET) && defined(SCE_VIDEO_OUT_HANDLE_OFFSET)
	auto videoOutBase = resolve::get_module_address("libSceVideoOut.sprx");
	auto sceVideoOutGet = (void*(*)(int))(videoOutBase + SCE_VIDEO_OUT_GET_OFFSET);
	video_out_handle = *(int*)(videoOutBase + SCE_VIDEO_OUT_HANDLE_OFFSET);
#else
	auto sceVideoOutGet = (void*(*)(int))sceVideoOutGet_addr;
	video_out_handle = *(int*)video_out_handle_addr;
#endif

	auto video_out = sceVideoOutGet(video_out_handle);
	if(!video_out)
	{
		LOG_ERROR("failed to get video out info\n");
		return false;
	}

	video_out_info = *(SceVideoOutBuffers**)video_out;

	int buffer_count = 0;
	while (true) {
		auto buffer = video_out_info->buffers[buffer_count].videoBuffer;
		if (buffer == 0)
			break;

		buffer_count++;
	}

	target_count = buffer_count;

	if (!create_device())
	{
		return false;
	}

	create_render_targets();
	create_contexts();
	create_event_queue();

	current_lw_context = context;
	prev_frame_index = 0;
	curr_frame_index = 0;
	curr_frame_context = frame_contexts[0];
	prev_frame_context = frame_contexts[1];

	if (flags & FunctionImGui)
	{
		ImGui::SetAllocatorFunctions(allocMem, freeMem, nullptr);

		ImGui::CreateContext();
		ImGui_ImplOrbis_Init();

		ImGui_ImplGnm_Init(load_fonts_cb);
	}

	render_context::flags |= StateRunning;

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
		if (render_context::flags & HookFlip)
		{
			detour_manager.create("sceGnmSubmitAndFlipCommandBuffers", gnm_driver_library, "sceGnmSubmitAndFlipCommandBuffers", sceGnmSubmitAndFlipCommandBuffers_h);

			sceGnmSubmitAndFlipCommandBuffer_d = *detour_manager.get("sceGnmSubmitAndFlipCommandBuffers");
			sceGnmSubmitAndFlipCommandBuffer_d.enable();
		}

		if (render_context::flags & HookFlipForWorkload)
		{
			detour_manager.create("sceGnmSubmitAndFlipCommandBuffersForWorkload", gnm_driver_library, "sceGnmSubmitAndFlipCommandBuffersForWorkload", sceGnmSubmitAndFlipCommandBuffersForWorkload_h);

			sceGnmSubmitAndFlipCommandBufferForWorkload_d = *detour_manager.get("sceGnmSubmitAndFlipCommandBuffersForWorkload");
			sceGnmSubmitAndFlipCommandBufferForWorkload_d.enable();
		}
	}

	if (is_sce_video_out_loaded)
	{
		if (render_context::flags & HookFlipVideoOut)
		{
			detour_manager.create("sceVideoOutSubmitFlip", "libSceVideoOut.sprx", "sceVideoOutSubmitFlip", sceVideoOutSubmitFlip_h, false);

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

	render_thread.join();

	release_hooks();
	clear_submits();

	// release any flags (imgui)
	// ...
	//
	if (flags & FunctionImGui)
	{
		LOG_INFO("destroying imgui context\n");

		ImGui_ImplGnm_Shutdown();
		ImGui_ImplOrbis_Shutdown();
		ImGui::DestroyContext();
	}

	// release all resources
	release_device();
	release_render_targets();
	release_event_queue();
	release_contexts();
	release_garlic_allocator();
	release_onion_allocator();

	LOG_INFO("render context cleanup done!\n");
}

bool render_context::begin_frame(int flip_index)
{
	if (!is_initialized())
	{
		return false;
	}

	if (flags & StateDestroying)
		return false;


	prev_frame_index = curr_frame_index;
	curr_frame_index = flip_index;
	curr_frame_context = frame_contexts[curr_frame_index];
	prev_frame_context = frame_contexts[prev_frame_index];
	current_lw_context = curr_frame_context->context;

	*curr_frame_context->context_label = label_active;

	current_lw_context->pushMarker("Frame Start");

	current_lw_context->waitUntilSafeForRendering(video_out_handle, prev_frame_index);

	current_lw_context->reset();
	current_lw_context->initializeDefaultHardwareState();
	current_lw_context->initializeToDefaultContextState();

	current_lw_context->setRenderTarget(0, curr_frame_context->target);
	current_lw_context->setRenderTargetMask(0xF);

	current_lw_context->setDepthRenderTarget(nullptr);

	current_lw_context->setScreenScissor(0, 0, video_out_info->width, video_out_info->height);
	current_lw_context->setupScreenViewport(0, 0, video_out_info->width, video_out_info->height, 0.5f, 0.5f);

	current_lw_context->popMarker();

	return true;
}
void render_context::update_frame()
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

			if (sce::Gnm::isRazorLoaded()) 
			{
				if (ImGui::Button("Dump DCB"))
				{
					dump_dcb = true;
				}
			}

			ImGui::BeginGroup();
			{
				// display video info
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
				ImGui::Text("Video Info:");
				ImGui::Text("Width: %dx%d", video_out_info->width, video_out_info->height);
				ImGui::Text("Format Bits: 0x%X", video_out_info->formatBits);
				ImGui::PopStyleColor();
			}
			ImGui::EndGroup();

			ImGui::End();
		}
	}
}

void render_context::end_frame()
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

	if (flags & SubmitSelf)
	{
		current_lw_context->writeAtEndOfPipe(sce::Gnm::kEopFlushCbDbCaches, sce::Gnm::kEventWriteDestMemory, (void*)curr_frame_context->context_label, sce::Gnm::kEventWriteSource64BitsImmediate,
			label_free, sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyBypass);
	}
	else
	{
		current_lw_context->writeAtEndOfPipeWithInterrupt(sce::Gnm::kEopFlushCbDbCaches, sce::Gnm::kEventWriteDestMemory, (void*)curr_frame_context->context_label, sce::Gnm::kEventWriteSource64BitsImmediate,
			label_free, sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyLru);
	}

	current_lw_context->submit();

	if (flags & SubmitSelf)
		sce::Gnm::submitDone();
}

void render_context::stall()
{
	while (static_cast<uint64_t>(*curr_frame_context->context_label) != label_free)
		eop_event_queue.wait();
}

void render_context::advance_frame()
{
	prev_frame_index = curr_frame_index;
	curr_frame_index = (curr_frame_index + 1) % target_count;

	prev_frame_context = frame_contexts[prev_frame_index];
	curr_frame_context = frame_contexts[curr_frame_index];
}

texture render_context::create_texture(const std::string& file, bool should_use_cache)
{
	texture tex(file, should_use_cache);
	textures.push_back(tex);

	return tex;
}

const int render_context::get_target_count()
{
	return target_count;
}
bool render_context::is_target_srgb()
{
	return target_is_srgb;
}
bool render_context::is_initialized()
{
	return flags & StateRunning;
}

bool render_context::create_garlic_allocator(size_t size)
{

	garlic_memory_allocator = new liborbisutil::memory::direct_memory_allocator(size, SCE_KERNEL_WC_GARLIC, "render context garlic", SCE_KERNEL_MAIN_DMEM_SIZE + 500_MB);
	if (!garlic_memory_allocator->initialised)
	{
		auto buffer = liborbisutil::resolve::get_module_address<off_t*>("direct-memory-plugin.sprx", "garlic_address");
		if (buffer)
		{
			garlic_memory_allocator = new liborbisutil::memory::direct_memory_allocator(reinterpret_cast<void*>(*buffer), size, SCE_KERNEL_WC_GARLIC, "render context garlic");
			if (garlic_memory_allocator->initialised)
			{
				LOG_DEBUG("using existing garlic allocator from address\n");
				return true;
			}
		}
		
		return false;
	}

	return true;
}
bool render_context::create_onion_allocator(size_t size)
{

	onion_memory_allocator = new liborbisutil::memory::direct_memory_allocator(size, SCE_KERNEL_WB_ONION, "render context onion", SCE_KERNEL_MAIN_DMEM_SIZE + 500_MB);
	if (!onion_memory_allocator->initialised)
	{
		auto buffer = liborbisutil::resolve::get_module_address<off_t*>("direct-memory-plugin.sprx", "onion_address");
		if (buffer)
		{
			onion_memory_allocator = new liborbisutil::memory::direct_memory_allocator(reinterpret_cast<void*>(*buffer), size, SCE_KERNEL_WB_ONION, "render context onion");
			if (onion_memory_allocator->initialised)
			{
				LOG_DEBUG("using existing onion allocator from address\n");
				return true;
			}
		}

		return false;
	}
	return true;
}
bool render_context::create_device()
{
	auto device = new sce::Gnmx::LightweightGfxContext();

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

	context = device;

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
		rt.init(&spec);
		rt.setBaseAddress(buffer);

		target = rt;
	}

	return true;
}
bool render_context::create_event_queue()
{
	eop_event_queue.create("render context eop event queue");
	sce::Gnm::addEqEvent(eop_event_queue.equeue, sce::Gnm::kEqEventGfxEop, nullptr);

	return true;
}
bool render_context::create_contexts()
{
	for (int i = 0; i < target_count; i++)
	{
		auto frame = new frame_context();
		frame->context = context;
		frame->target = &render_targets[i];

		frame->context_label = reinterpret_cast<volatile uint64_t*>(garlic_memory_allocator->allocate(sizeof(uint64_t), 8, "context label"));
		*frame->context_label = label_free;

		frame_contexts[i] = frame;
	}

	prev_frame_context = frame_contexts[0];
	curr_frame_context = frame_contexts[1];

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

	garlic_memory_allocator->free((void*)release_label);
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
		delete context;

		LOG_DEBUG("device released\n");
	}
}
void render_context::release_render_targets()
{
	render_targets.clear();
}
void render_context::release_event_queue()
{
	eop_event_queue.release();
	LOG_DEBUG("event queue released\n");

}
void render_context::release_contexts()
{
	LOG_DEBUG("contexts released\n");

	prev_frame_context = nullptr;
	curr_frame_context = nullptr;

	for (auto& frame : frame_contexts)
	{
		if (frame)
		{
			garlic_memory_allocator->free((void*)frame->context_label);

			delete frame;
			frame = nullptr;
		}
	}
}

void render_context::dump()
{
	LOG_INFO("dumping render context state...\n");
	for (int i = 0; i < target_count; i++)
	{
		auto frame = frame_contexts[i];
		LOG_INFO("Frame Context %d:\n", i);
		LOG_INFO("  Target: %p\n", frame->target);
		LOG_INFO("  Context Label: %p (Value: %llu)\n", frame->context_label, static_cast<uint64_t>(*frame->context_label));
	}
	LOG_INFO("Render Targets:\n");
	for (int i = 0; i < get_target_count(); i++)
	{
		auto& target = render_targets[i];
		LOG_INFO("  Target %d: Width: %d, Height: %d, Format: 0x%X\n", i, target.getWidth(), target.getHeight(), target.getDataFormat());
	}

	if (sce::Gnm::isRazorLoaded())
	{
		LOG_INFO("Razor is loaded. Dumping DCB...\n");

		void* dcb_gpu_addr = (void*)current_lw_context->m_dcb.m_beginptr;
		auto size_in_dwords = (uint32_t)((size_t)current_lw_context->m_dcb.m_cmdptr - (size_t)current_lw_context->m_dcb.m_beginptr);
		sce::Gnm::captureImmediate("/data/gpu_capture.rzsr", 1, &dcb_gpu_addr, &size_in_dwords, nullptr, nullptr);
	}

	LOG_INFO("Render context dump complete.\n");

	dump_dcb = false;
}
