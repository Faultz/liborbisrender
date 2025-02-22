#include "../render_context.h"

#include <video_out.h>
//
//uint64_t get_current_tick() {
//    using namespace std::chrono;
//    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
//}
//
//uint64_t calculate_modded_tick()
//{
//    auto context = render_context::get_instance();
//
//    uint64_t current_tick = get_current_tick();
//    uint64_t tick_diff = current_tick - context->saved_tick_count;
//    uint64_t modded_tick_diff = static_cast<uint64_t>(tick_diff * context->speedhack_multiplier);
//    return context->saved_modded_tick_count + modded_tick_diff;
//}
//
//uint64_t render_context::sceKernelGetProcessTimeCounter()
//{
//	return calculate_modded_tick();
//}
//
//uint64_t render_context::sceKernelReadTsc()
//{
//	return calculate_modded_tick();
//}