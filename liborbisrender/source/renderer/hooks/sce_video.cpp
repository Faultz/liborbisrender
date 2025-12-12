#include "../render_context.h"

#include <video_out.h>

int render_context::sceVideoOutSubmitFlip_h(uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg)
{
	auto context = get_instance();

	if (context->flags & UnlockFps)
		sceVideoOutSetFlipRate(videoOutHandle, 0);

	if (context->flags & FlipModeVSync)
	{
		flipMode = SCE_VIDEO_OUT_FLIP_MODE_VSYNC;
	}

	auto should_render_after_flip = context->flags & RenderBeforeFlip;
	if (should_render_after_flip && (context->flags & StateDestroying) == 0)
	{
		if (context->user_callback)
			context->user_callback((displayBufferIndex + 1) % context->get_target_count());
	}

	context->stall();

	auto res = context->sceVideoOutSubmitFlip_d.invoke<int>(videoOutHandle, displayBufferIndex, flipMode, flipArg);

	if (!should_render_after_flip && (context->flags & StateDestroying) == 0)
	{
		if (context->user_callback)
			context->user_callback((displayBufferIndex + 1) % context->get_target_count());
	}

	if (render_context::dump_dcb)
	{
		context->dump();
	}

	return res;
}
