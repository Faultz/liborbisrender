#include "../render_context.h"

#include <video_out.h>

int render_context::sceVideoOutSubmitFlip_h(uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg)
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

	auto res = context->sceVideoOutSubmitFlip_d.invoke<int>(videoOutHandle, displayBufferIndex, flipMode, flipArg);

	while (*context->curr_frame_context->context_label != label_free)
	{
		SceKernelEvent eop_event;
		int num;
		sceKernelWaitEqueue(context->eop_event_queue, &eop_event, 1, &num, nullptr);
	}

	return res;
}
