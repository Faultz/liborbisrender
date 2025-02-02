#include "../render_context.h"

#include <video_out.h>

namespace renderer
{
	int render_context::sceVideoOutSubmitFlip_h(uint32_t videoOutHandle, uint32_t displayBufferIndex, uint32_t flipMode, int64_t flipArg)
	{
		auto context = get_instance();

		if (context->flags & UnlockFps)
			sceVideoOutSetFlipRate(context->video_out_handle, 0);

		auto should_render_after_flip = context->flags & RenderAfterFlip;
		if (!should_render_after_flip)
		{
			if (context->user_callback)
				context->user_callback(displayBufferIndex);
		}

		auto res = context->sceVideoOutSubmitFlip_d.invoke<int>(videoOutHandle, displayBufferIndex, flipMode, flipArg);

		if (should_render_after_flip)
		{
			if (context->user_callback)
				context->user_callback(displayBufferIndex);
		}

		return res;
	}
}