#include "../render_context.h"

#include <video_out.h>

int render_context::sceGnmSubmitAndFlipCommandBuffers_h(uint32_t count, void* dcbGpuAddrs[], uint32_t* dcbSizesInBytes, void* ccbGpuAddrs[], uint32_t* ccbSizesInBytes, uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg)
{
	auto context = get_instance();

	if (context->flags & UnlockFps)
		sceVideoOutSetFlipRate(context->video_out_handle, 0);

	if (context->user_callback)
		context->user_callback(displayBufferIndex);

	auto res = context->sceGnmSubmitAndFlipCommandBuffer_d.invoke<int>(count, dcbGpuAddrs, dcbSizesInBytes, ccbGpuAddrs, ccbSizesInBytes, videoOutHandle, displayBufferIndex, flipMode, flipArg);

	while (*context->curr_frame_context->context_label != label_free)
	{
		SceKernelEvent eop_event;
		int num;
		sceKernelWaitEqueue(context->eop_event_queue, &eop_event, 1, &num, nullptr);
	}

	return res;
}

int render_context::sceGnmSubmitAndFlipCommandBuffersForWorkload_h(int workloadId, uint32_t count, void* dcbGpuAddrs[], uint32_t* dcbSizesInBytes, void* ccbGpuAddrs[], uint32_t* ccbSizesInBytes, uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg)
{
	auto context = get_instance();

	if (context->flags & UnlockFps)
		sceVideoOutSetFlipRate(context->video_out_handle, 0);

	if (context->user_callback)
		context->user_callback(displayBufferIndex);

	auto res = context->sceGnmSubmitAndFlipCommandBufferForWorkload_d.invoke<int>(workloadId, count, dcbGpuAddrs, dcbSizesInBytes, ccbGpuAddrs, ccbSizesInBytes, videoOutHandle, displayBufferIndex, flipMode, flipArg);

	while (*context->curr_frame_context->context_label != label_free)
	{
		SceKernelEvent eop_event;
		int num;
		sceKernelWaitEqueue(context->eop_event_queue, &eop_event, 1, &num, nullptr);
	}

	return res;
}