/* SIE CONFIDENTIAL
 * PlayStation(R)5 Programmer Tool Runtime Library Release 2.00.00.09-00.00.00.0.1
 * Copyright (C) 2019 Sony Interactive Entertainment Inc. 
 */
#pragma once

#include <vectormath.h>
#include <string.h>

// Vector3Unaligned is meant to store a Vector3, except that padding and alignment are
// 4-byte granular (GPU), rather than 16-byte (SSE). This is to bridge
// the SSE math library with GPU structures that assume 4-byte granularity.
// While a Vector3 has many operations, Vector3Unaligned can only be converted to and from Vector3.
struct Vector3Unaligned
{
	float x;
	float y;
	float z;
	Vector3Unaligned& operator=(const sce::Vectormath::Simd::Aos::Vector3_arg &rhs)
	{
		memcpy(this, &rhs, sizeof(*this));
		return *this;
	}
	Vector3Unaligned& operator=(const sce::Vectormath::Simd::Aos::Vector4_arg &rhs)
	{
		memcpy(this, &rhs, sizeof(*this));
		return *this;
	}
	float	operator[](unsigned int	i) const
	{
		return (i == 0) ? x : (i == 1) ? y : z;
	}
};

inline Vector3Unaligned ToVector3Unaligned(const sce::Vectormath::Simd::Aos::Vector3_arg &r)
{
	const Vector3Unaligned result = { r.getX(), r.getY(), r.getZ() };
	return result;
}

