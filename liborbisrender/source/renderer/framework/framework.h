#pragma once

#include <stdio.h>

#include <gnm.h>
#include <gnmx.h>

#include "../render_context.h"
#include "../shaders.h"

namespace framework
{
	template<typename T>
	class shader_program
	{
		shader_program() = default;
		~shader_program();

		bool load_from_array(const char* shader_code)
		{
			return false;
		}
		void bind(sce::Gnmx::LightweightGfxContext* context);

		void release();

		T* shader;
	};

	class vertex_buffer
	{

	};

	class render_target
	{

	};

	class render_framework
	{
	public:
	};
}