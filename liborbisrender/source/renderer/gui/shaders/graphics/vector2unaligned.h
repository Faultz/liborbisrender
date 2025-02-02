/* SIE CONFIDENTIAL
 * PlayStation(R)5 Programmer Tool Runtime Library Release 2.00.00.09-00.00.00.0.1
 * Copyright (C) 2019 Sony Interactive Entertainment Inc. 
 */
#pragma once

#include <vectormath.h>
#include <string.h>

// Vector2Unaligned is meant to store a Vector2, except that padding and alignment are
// 4-byte granular (GPU), rather than 16-byte (SSE). This is to bridge
// the SSE math library with GPU structures that assume 4-byte granularity.
// While a Vector2 has many operations, Vector2Unaligned can only be converted to and from Vector2.
struct Vector2Unaligned
{
	float x;
	float y;
	Vector2Unaligned& operator=(const sce::Vectormath::Simd::Aos::Vector2_arg &rhs)
	{
		memcpy(this, &rhs, sizeof(*this));
		return *this;
	}
	float	operator[](unsigned int	i) const
	{
		return i == 0 ? x : y;
	}
};

inline Vector2Unaligned ToVector2Unaligned(const sce::Vectormath::Simd::Aos::Vector2_arg &r)
{
	const Vector2Unaligned result = { r.getX(), r.getY() };
	return result;
}
