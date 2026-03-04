#pragma once
#include "vector4unaligned.h"
#include <vectormath.h>

// Matrix4x3Unaligned is meant to store a Transform3, except that padding and alignment are
// 4-byte granular (GPU), rather than 16-byte (SSE). This is to bridge
// the SSE math library with GPU structures that assume 4-byte granularity.
// While a Transform3 has many operations, Matrix4x3Unaligned can only be converted to and from Transform3.
struct Matrix4x3Unaligned
{
	Vector4Unaligned x;
	Vector4Unaligned y;
	Vector4Unaligned z;
	Matrix4x3Unaligned &operator=(sce::Vectormath::Simd::Aos::Transform3_arg &rhs)
	{
		x = ToVector4Unaligned(rhs.getRow(0));
		y = ToVector4Unaligned(rhs.getRow(1));
		z = ToVector4Unaligned(rhs.getRow(2));
		return *this;
	}
};

inline Matrix4x3Unaligned ToMatrix4x3Unaligned(sce::Vectormath::Simd::Aos::Transform3_arg &r)
{
	const Matrix4x3Unaligned result = { ToVector4Unaligned(r.getRow(0)), ToVector4Unaligned(r.getRow(1)), ToVector4Unaligned(r.getRow(2)) };
	return result;
}

inline sce::Vectormath::Simd::Aos::Transform3 ToTransform3(const Matrix4x3Unaligned &r)
{
	return sce::Vectormath::Simd::Aos::Transform3(sce::Vectormath::Simd::Aos::Vector3(r.x.x, r.y.x, r.z.x), sce::Vectormath::Simd::Aos::Vector3(r.x.y, r.y.y, r.z.y), sce::Vectormath::Simd::Aos::Vector3(r.x.z, r.y.z, r.z.z), sce::Vectormath::Simd::Aos::Vector3(r.x.w, r.y.w, r.z.w));
}
