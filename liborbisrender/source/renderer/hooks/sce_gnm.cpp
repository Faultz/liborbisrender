#include "../render_context.h"

#include <video_out.h>

int render_context::sceGnmSubmitAndFlipCommandBuffers_h(uint32_t count, void* dcbGpuAddrs[], uint32_t* dcbSizesInBytes, void* ccbGpuAddrs[], uint32_t* ccbSizesInBytes, uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg)
{
	auto context = get_instance();

	if (context->flags & UnlockFps)
		sceVideoOutSetFlipRate(context->video_out_handle, 0);

	auto should_render_after_flip = context->flags & RenderAfterFlip;
	if (!should_render_after_flip && (context->flags & StateDestroying) == 0)
	{
		if (context->user_callback)
			context->user_callback(displayBufferIndex);
	}

	auto res = context->sceGnmSubmitAndFlipCommandBuffer_d.invoke<int>(count, dcbGpuAddrs, dcbSizesInBytes, ccbGpuAddrs, ccbSizesInBytes, videoOutHandle, displayBufferIndex, flipMode, flipArg);

	if (should_render_after_flip && (context->flags & StateDestroying) == 0)
	{
		if (context->user_callback)
			context->user_callback(displayBufferIndex);
	}

	return res;
}

int render_context::sceGnmSubmitAndFlipCommandBuffersForWorkload_h(int workloadId, uint32_t count, void* dcbGpuAddrs[], uint32_t* dcbSizesInBytes, void* ccbGpuAddrs[], uint32_t* ccbSizesInBytes, uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg)
{
	auto context = get_instance();

	if (context->flags & UnlockFps)
		sceVideoOutSetFlipRate(context->video_out_handle, 0);

	auto should_render_after_flip = context->flags & RenderAfterFlip;
	if (!should_render_after_flip && (context->flags & StateDestroying) == 0)
	{
		if (context->user_callback)
			context->user_callback(displayBufferIndex);
	}

	auto res = context->sceGnmSubmitAndFlipCommandBufferForWorkload_d.invoke<int>(workloadId, count, dcbGpuAddrs, dcbSizesInBytes, ccbGpuAddrs, ccbSizesInBytes, videoOutHandle, displayBufferIndex, flipMode, flipArg);

	if (should_render_after_flip && (context->flags & StateDestroying) == 0)
	{
		if (context->user_callback)
			context->user_callback(displayBufferIndex);
	}

	return res;
}