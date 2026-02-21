#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <kernel.h>

#include <gnm.h>
#include <gnmx.h>

#include <liborbisutil.h>

#include "../source/renderer/imgui/backend/imgui_impl_orbis.h"
#include "../source/renderer/imgui/backend/imgui_impl_gnm.h"

#include "../source/renderer/utils/texture.h"
#include "../source/renderer/utils/shaders.h"
#include "../source/renderer/render_context.h"

#pragma comment(lib, "libScePngDec_stub_weak.a")
#pragma comment(lib, "libSceJpegDec_stub_weak.a")
#pragma comment(lib, "libSceShaderBinary.a")
#pragma comment(lib, "liborbisutil.a")