/* SIE CONFIDENTIAL
 * PlayStation(R)5 Programmer Tool Runtime Library Release 2.00.00.09-00.00.00.0.1
 * Copyright (C) 2019 Sony Interactive Entertainment Inc.
 */

#pragma once

#ifdef __cplusplus
#include <sys/types.h>
#include <scebase_common.h>
#if _SCE_TARGET_OS_PROSPERO
#include <agc/core/texture.h>
#include <agc/core/buffer.h>
#include <agc/core/sampler.h>
#endif
#if _SCE_TARGET_OS_ORBIS
#include <gnm/texture.h>
#include <gnm/buffer.h>
#include <gnm/sampler.h>
#endif
#include <shader/srt_types.h>
#include "vector2unaligned.h"
#include "vector3unaligned.h"
#include "vector4unaligned.h"
#include "matrix4unaligned.h"
#include "matrix4x3unaligned.h"

using namespace sce::Shader::Srt;

typedef struct _short2 { short x; short y; } short2;
typedef struct _short3 { short x; short y; short z; } short3;
typedef struct _short4 { short x; short y; short z; short w; } short4;
typedef struct _ushort2 { unsigned short x; unsigned short y; } ushort2;
typedef struct _ushort3 { unsigned short x; unsigned short y; unsigned short z; } ushort3;
typedef struct _ushort4 { unsigned short x; unsigned short y; unsigned short z; unsigned short w; } ushort4;
typedef struct _int2 { int x; int y; } int2;
typedef struct _int3 { int x; int y; int z; } int3;
typedef struct _int4 { int x; int y; int z; int w; } int4;
typedef struct _uint2 { unsigned int x; unsigned int y; } uint2;
typedef struct _uint3 { unsigned int x; unsigned int y; unsigned int z; } uint3;
typedef struct _uint4 { unsigned int x; unsigned int y; unsigned int z; unsigned int w; } uint4;
typedef struct _half2 {	uint16_t x; uint16_t y; } half2;
typedef struct _half3 { uint16_t x; uint16_t y; uint16_t z; } half3;
typedef struct _half4 { uint16_t x; uint16_t y; uint16_t z; uint16_t w; } half4;
#else // __cplusplus


static const float M_PI = 3.1415926535897932384626433832795f;
static const float M_PI_2 = 1.570796327f;

#ifdef __PSSL__
#pragma warning (disable: 5203 5206 8207 5559)
#endif

typedef float2 Vector2Unaligned;
typedef float3 Vector3Unaligned;
typedef float4 Vector4Unaligned;
typedef column_major float2x2 Matrix2Unaligned;
typedef column_major float3x3 Matrix3Unaligned;
typedef column_major matrix Matrix4Unaligned;
typedef column_major float4x3 Matrix4x3Unaligned;

#pragma argument(ttrace=1)
#pragma argument(gradientadjust=auto)
#pragma warning(default: 7577) // warn if SRT shader refers to constant buffer bug#174174
#pragma warning( error: 7577) // optionally also make it an error

#endif // __cplusplus
