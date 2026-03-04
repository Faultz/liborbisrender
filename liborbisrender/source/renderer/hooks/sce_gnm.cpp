#include "../render_context.h"

#include <video_out.h>

int render_context::sceGnmSubmitAndFlipCommandBuffers_h(uint32_t count, void* dcbGpuAddrs[], uint32_t* dcbSizesInBytes, void* ccbGpuAddrs[], uint32_t* ccbSizesInBytes, uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg)
{
	if (flags & UnlockFps)
		sceVideoOutSetFlipRate(video_out_handle, 0);

	if (flags & FlipModeVSync)
		flipMode = SCE_VIDEO_OUT_FLIP_MODE_VSYNC;

	if ((flags & InjectDCB) && (flags & StateDestroying) == 0 && user_callback)
	{
		// Record our overlay into our DCB — end_frame will NOT submit it separately
		user_callback(displayBufferIndex);

		// Only inject if end_frame() fully completed (dcb_frame_ready guards against
		// stale DCBs from the previous frame when user_callback returns early during cleanup)
		if (!dcb_frame_ready)
		{
			return sceGnmSubmitAndFlipCommandBuffer_d.invoke<int>(count, dcbGpuAddrs, dcbSizesInBytes, ccbGpuAddrs, ccbSizesInBytes, videoOutHandle, displayBufferIndex, flipMode, flipArg);
		}

		// Consume the flag — one DCB per inject
		dcb_frame_ready = false;

		// Grab the recorded DCB pointer and size (same calc as dump())
		void*    our_dcb  = (void*)current_lw_context->m_dcb.m_beginptr;
		uint32_t our_size = (uint32_t)((uintptr_t)current_lw_context->m_dcb.m_cmdptr -
		                               (uintptr_t)current_lw_context->m_dcb.m_beginptr);

		constexpr uint32_t kMaxDCBs = 16;
		void*    new_dcbs     [kMaxDCBs] = {};
		uint32_t new_dcb_sizes[kMaxDCBs] = {};
		void*    new_ccbs     [kMaxDCBs] = {};
		uint32_t new_ccb_sizes[kMaxDCBs] = {};

		memcpy(new_dcbs,      dcbGpuAddrs,    count * sizeof(void*));
		memcpy(new_dcb_sizes, dcbSizesInBytes, count * sizeof(uint32_t));
		if (ccbGpuAddrs)    memcpy(new_ccbs,      ccbGpuAddrs,    count * sizeof(void*));
		if (ccbSizesInBytes) memcpy(new_ccb_sizes, ccbSizesInBytes, count * sizeof(uint32_t));

		new_dcbs     [count] = our_dcb;
		new_dcb_sizes[count] = our_size;
		// new_ccbs[count] / new_ccb_sizes[count] stay nullptr/0

		auto res = sceGnmSubmitAndFlipCommandBuffer_d.invoke<int>(
			count + 1, new_dcbs, new_dcb_sizes, new_ccbs, new_ccb_sizes,
			videoOutHandle, displayBufferIndex, flipMode, flipArg);

		if (render_context::dump_dcb)
			dump();

		stall();
		return res;
	}

	auto should_render_after_flip = flags & RenderBeforeFlip;
	if (should_render_after_flip && (flags & StateDestroying) == 0)
	{
		if (user_callback)
			user_callback(displayBufferIndex);
	}

	auto res = sceGnmSubmitAndFlipCommandBuffer_d.invoke<int>(count, dcbGpuAddrs, dcbSizesInBytes, ccbGpuAddrs, ccbSizesInBytes, videoOutHandle, displayBufferIndex, flipMode, flipArg);

	if (!should_render_after_flip && (flags & StateDestroying) == 0)
	{
		if (user_callback)
			user_callback(displayBufferIndex);
	}

	if (render_context::dump_dcb)
		dump();

	stall();
	return res;
}

int render_context::sceGnmSubmitAndFlipCommandBuffersForWorkload_h(int workloadId, uint32_t count, void* dcbGpuAddrs[], uint32_t* dcbSizesInBytes, void* ccbGpuAddrs[], uint32_t* ccbSizesInBytes, uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg)
{
	if (flags & UnlockFps)
		sceVideoOutSetFlipRate(video_out_handle, 0);

	if (flags & FlipModeVSync)
		flipMode = SCE_VIDEO_OUT_FLIP_MODE_VSYNC;

	if ((flags & InjectDCB) && (flags & StateDestroying) == 0 && user_callback)
	{
		user_callback(displayBufferIndex);

		if (!dcb_frame_ready)
		{
			return sceGnmSubmitAndFlipCommandBufferForWorkload_d.invoke<int>(workloadId, count, dcbGpuAddrs, dcbSizesInBytes, ccbGpuAddrs, ccbSizesInBytes, videoOutHandle, displayBufferIndex, flipMode, flipArg);
		}

		dcb_frame_ready = false;

		void*    our_dcb  = (void*)current_lw_context->m_dcb.m_beginptr;
		uint32_t our_size = (uint32_t)((uintptr_t)current_lw_context->m_dcb.m_cmdptr -
		                               (uintptr_t)current_lw_context->m_dcb.m_beginptr);

		constexpr uint32_t kMaxDCBs = 16;
		void*    new_dcbs     [kMaxDCBs] = {};
		uint32_t new_dcb_sizes[kMaxDCBs] = {};
		void*    new_ccbs     [kMaxDCBs] = {};
		uint32_t new_ccb_sizes[kMaxDCBs] = {};

		memcpy(new_dcbs,      dcbGpuAddrs,    count * sizeof(void*));
		memcpy(new_dcb_sizes, dcbSizesInBytes, count * sizeof(uint32_t));
		if (ccbGpuAddrs)     memcpy(new_ccbs,      ccbGpuAddrs,    count * sizeof(void*));
		if (ccbSizesInBytes) memcpy(new_ccb_sizes, ccbSizesInBytes, count * sizeof(uint32_t));

		new_dcbs     [count] = our_dcb;
		new_dcb_sizes[count] = our_size;

		auto res = sceGnmSubmitAndFlipCommandBufferForWorkload_d.invoke<int>(
			workloadId, count + 1, new_dcbs, new_dcb_sizes, new_ccbs, new_ccb_sizes,
			videoOutHandle, displayBufferIndex, flipMode, flipArg);

		if (render_context::dump_dcb)
			dump();

		stall();
		return res;
	}

	auto should_render_after_flip = flags & RenderBeforeFlip;
	if (should_render_after_flip && (flags & StateDestroying) == 0)
	{
		if (user_callback)
			user_callback(displayBufferIndex);
	}

	auto res = sceGnmSubmitAndFlipCommandBufferForWorkload_d.invoke<int>(workloadId, count, dcbGpuAddrs, dcbSizesInBytes, ccbGpuAddrs, ccbSizesInBytes, videoOutHandle, displayBufferIndex, flipMode, flipArg);

	if (!should_render_after_flip && (flags & StateDestroying) == 0)
	{
		if (user_callback)
			user_callback(displayBufferIndex);
	}

	if (render_context::dump_dcb)
		dump();

	stall();
	return res;
}