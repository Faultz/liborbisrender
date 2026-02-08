#include "../render_context.h"

#include <video_out.h>

int render_context::sceGnmSubmitAndFlipCommandBuffers_h(uint32_t count, void* dcbGpuAddrs[], uint32_t* dcbSizesInBytes, void* ccbGpuAddrs[], uint32_t* ccbSizesInBytes, uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg)
{
	if (flags & UnlockFps)
		sceVideoOutSetFlipRate(video_out_handle, 0);

	if (flags & FlipModeVSync)
	{
		flipMode = SCE_VIDEO_OUT_FLIP_MODE_VSYNC;
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
	{
		dump();
	}

	stall();

	return res;
}

int render_context::sceGnmSubmitAndFlipCommandBuffersForWorkload_h(int workloadId, uint32_t count, void* dcbGpuAddrs[], uint32_t* dcbSizesInBytes, void* ccbGpuAddrs[], uint32_t* ccbSizesInBytes, uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg)
{
	if (flags & UnlockFps)
		sceVideoOutSetFlipRate(video_out_handle, 0);

	if (flags & FlipModeVSync)
	{
		flipMode = SCE_VIDEO_OUT_FLIP_MODE_VSYNC;
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
	{
		dump();
	}

	stall();

	return res;
}