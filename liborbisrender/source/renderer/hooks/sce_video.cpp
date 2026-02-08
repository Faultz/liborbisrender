#include "../render_context.h"

#include <video_out.h>

int render_context::sceVideoOutSubmitFlip_h(uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg)
{
	if (flags & UnlockFps)
		sceVideoOutSetFlipRate(videoOutHandle, 0);

	if (flags & FlipModeVSync)
	{
		flipMode = SCE_VIDEO_OUT_FLIP_MODE_VSYNC;
	}

	auto should_render_after_flip = flags & RenderBeforeFlip;
	if (should_render_after_flip && (flags & StateDestroying) == 0)
	{
		if (user_callback)
			user_callback((displayBufferIndex + 1) % get_target_count());
	}

	stall();

	auto res = sceVideoOutSubmitFlip_d.invoke<int>(videoOutHandle, displayBufferIndex, flipMode, flipArg);

	if (!should_render_after_flip && (flags & StateDestroying) == 0)
	{
		if (user_callback)
			user_callback((displayBufferIndex + 1) % get_target_count());
	}

	if (render_context::dump_dcb)
	{
		dump();
	}

	return res;
}
