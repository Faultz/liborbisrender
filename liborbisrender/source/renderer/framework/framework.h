#pragma once

#include <stdio.h>

#include <gnm.h>
#include <gnmx.h>

#include "../render_context.h"
#include "../shaders.h"

namespace render
{
	template<typename T>
	class shader_program
	{
		shader_program() = default;
		~shader_program();

		bool load_from_array(const char* shader_code)
		{
			return shader->load_from_array(shader_code);
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

	class framework
	{
	public:
		framework();
		~framework();

		inline render_context* get_context() { return context; }

		void initialize(render_context* ctx);
		void release();

		

		static inline render_context* context;
	};
}