#pragma once
#include "vector4unaligned.h"
#include <vectormath.h>

// Matrix4Unaligned is meant to store a Matrix4, except that padding and alignment are
// 4-byte granular (GPU), rather than 16-byte (SSE). This is to bridge
// the SSE math library with GPU structures that assume 4-byte granularity.
// While a Matrix4 has many operations, Matrix4Unaligned can only be converted to and from Matrix4.
struct Matrix4Unaligned
{
	Vector4Unaligned x;
	Vector4Unaligned y;
	Vector4Unaligned z;
	Vector4Unaligned w;
	Matrix4Unaligned &operator=(sce::Vectormath::Simd::Aos::Matrix4_arg &rhs)
	{
		memcpy(this, &rhs, sizeof(*this));
		return *this;
	}
};

inline Matrix4Unaligned ToMatrix4Unaligned(sce::Vectormath::Simd::Aos::Matrix4_arg &r)
{
	const Matrix4Unaligned result = { ToVector4Unaligned(r.getCol0()), ToVector4Unaligned(r.getCol1()), ToVector4Unaligned(r.getCol2()), ToVector4Unaligned(r.getCol3()) };
	return result;
}

inline sce::Vectormath::Simd::Aos::Matrix4 ToMatrix4(const Matrix4Unaligned &r)
{
	using namespace sce::Vectormath::Simd::Aos;
	return Matrix4(Vector4(r.x), Vector4(r.y), Vector4(r.z), Vector4(r.w));
}
