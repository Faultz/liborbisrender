# liborbisrender

A rendering library for PlayStation 4 (Orbis) that provides an easy-to-use graphics context with integrated ImGui support.

## Features

- **GNM/GNMX Integration** - Built on Sony's low-level graphics API
- **ImGui Support** - Full Dear ImGui integration with custom Orbis backend
- **Texture Management** - Load textures from files, URLs, or raw data (PNG, JPEG, GNF)
- **Render Hooks** - Hook into the game's flip/submit pipeline for overlay rendering
- **FPS Unlock** - Optional frame rate unlocking
- **Debug Utilities** - Built-in frame time tracking and debug rendering

## Dependencies

- [liborbisutil](https://github.com/Faultz/liborbisutil) - Utility library for Orbis
- [minhook](https://github.com/Faultz/orbis-minhook) - Hooking library for Orbis
- [fmtlib](https://github.com/Faultz/fmtlib) - Formatting library
- [StubMaker](https://github.com/OSM-Made/StubMaker) - SDK stub generation

## Building

1. Open `liborbisrender.sln` in Visual Studio
2. Ensure all dependencies are properly linked
3. Build for ORBIS Debug or ORBIS Release configuration

## Usage

### Basic Setup

```cpp
#include <liborbisrender.h>

int module_start(size_t argc, const void* args)
{
    // Initialize minhook
    MH_Initialize();

    // Get render context instance
    auto& context = *render_context::get_instance();

    // Create the render context with desired flags
    context.create(HookFlip | FunctionImGui | FunctionRenderDebug, [&](int flipIndex) {

        if (context.begin_scene(flipIndex))
        {
            context.update_scene();

            // Your ImGui rendering here
            ImGui::Begin("My Window");
            ImGui::Text("Hello, PS4!");
            ImGui::End();

            context.end_scene();
        }
    });

    return SCE_OK;
}

int module_stop(size_t argc, const void* args)
{
    auto& context = *render_context::get_instance();
    context.release();
    MH_Uninitialize();

    return SCE_OK;
}
```

### Render Flags

| Flag | Description |
|------|-------------|
| `RenderBeforeFlip` | Render before the flip operation |
| `SubmitSelf` | Submit command buffers independently |
| `FunctionImGui` | Enable ImGui functionality |
| `FunctionRenderDebug` | Enable debug rendering features |
| `HookFlip` | Hook `sceGnmSubmitAndFlipCommandBuffer` |
| `HookFlipForWorkload` | Hook workload-based flip |
| `HookFlipVideoOut` | Hook `sceVideoOutSubmitFlip` |
| `FlipModeVSync` | Enable VSync flip mode |
| `UnlockFps` | Unlock frame rate |

### Loading Textures

```cpp
// Load from file
texture tex("/data/my_image.png");

// Load from URL
texture tex("https://example.com/image.png");

// Create from raw data
texture tex(pixel_data, width, height, sce::Gnm::kDataFormatR8G8B8A8Unorm);

// Use in ImGui
if (tex)
{
    ImGui::Image(&tex, ImVec2(256, 256));
}
```

### Texture Encoding/Decoding

```cpp
texture tex;

// Decode various formats
tex.decode_png("/data/image.png");
tex.decode_jpeg("/data/image.jpg");
tex.decode_gnf("/data/image.gnf");

// Encode to various formats
tex.encode_png("/data/output.png");
tex.encode_jpeg("/data/output.jpg");
tex.encode_gnf("/data/output.gnf");
```

### Custom Fonts

```cpp
context.create(flags, render_callback, [](ImGuiIO& io) {
    // Load custom fonts
    io.Fonts->AddFontFromFileTTF("/data/fonts/MyFont.ttf", 16.0f);
});
```

## Project Structure

```
liborbisrender/
├── include/
│   └── liborbisrender.h      # Main include header
├── source/
│   └── renderer/
│       ├── gui/              # ImGui implementation
│       │   ├── backend/      # Orbis & GNM backends
│       │   └── shaders/      # ImGui shaders
│       ├── render_context.h  # Core render context
│       ├── render_context.cpp
│       ├── texture.h         # Texture management
│       ├── texture.cpp
│       ├── shaders.h         # Shader utilities
│       └── shaders.cpp
└── sample/
    └── prx.cpp               # Example usage
```

## License
This project is provided as-is for educational and development purposes. Ensure compliance with all applicable laws and platform terms of service when using this software.
