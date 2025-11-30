
#include "imgui_impl_orbis.h"
#include "../../render_context.h"

#include <ime_dialog.h>

SceVideoOutBuffers* video;

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
// FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
ImGui_ImplOrbis_Data* ImGui_ImplOrbis_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplOrbis_Data*)ImGui::GetIO().BackendPlatformUserData : NULL;
}

bool ImGui_ImplOrbis_Init(render_context* context)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == NULL && "Already initialized a platform backend!");

    // Setup backend capabilities flags
    ImGui_ImplOrbis_Data* bd = IM_NEW(ImGui_ImplOrbis_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_orbis";
    io.BackendRendererName = "imgui_impl_gnm";
    io.ConfigFlags |= (ImGuiConfigFlags_NavEnableGamepad /*| ImGuiConfigFlags_NavEnableSetMousePos*/);
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad /*| ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos*/ /*| ImGuiBackendFlags_RendererHasVtxOffset*/;

    //if (hooks::use_mouse)
    //{
    //    io.ConfigFlags |= (ImGuiConfigFlags_NavEnableSetMousePos);
    //    io.BackendFlags |= (ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos);
    //}

    bd->renderContext = context;
	bd->garlic_allocator = context->garlic_memory_allocator;
	bd->onion_allocator = context->onion_memory_allocator;

    video = context->video_out_info;

    bd->WantUpdateHasGamepad = true;
    bd->HasMouseControl = false;
    bd->HasTouchpadControl = false;

    return true;
}

void ImGui_ImplOrbis_Shutdown()
{
    ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
    IM_ASSERT(bd != NULL && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    io.BackendPlatformName = NULL;
    io.BackendPlatformUserData = NULL;

    IM_DELETE(bd);
}

// This code supports multi-viewports (multiple OS Windows mapped into different Dear ImGui viewports)
// Because of that, it is a little more complicated than your typical single-viewport binding code!
static void ImGui_ImplOrbis_UpdateMouseData()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
    if (!bd->HasMouseControl)
        return;

    constexpr float kMarginCursor = 10.0f;

    float rightStickX = liborbisutil::pad::get_right_stick_x(), rightStickY = liborbisutil::pad::get_right_stick_y();

    io.MouseDrawCursor = true;

    if (abs(rightStickX) > 0.1f)
    {
        io.MousePos.x += ((rightStickX * (float)video->width) / 80.0f);
        if (io.MousePos.x > video->width - kMarginCursor) io.MousePos.x = video->width - kMarginCursor;
        if (io.MousePos.x < 0) io.MousePos.x = 0;
    }

    if (abs(rightStickY) > 0.1f)
    {
        io.MousePos.y += ((rightStickY * (float)video->height) / 80.0f);
        if (io.MousePos.y > video->height - kMarginCursor) io.MousePos.y = video->height - kMarginCursor;
        if (io.MousePos.y < 0) io.MousePos.y = 0;
    }

    if (liborbisutil::pad::is_down(liborbisutil::pad::buttons::l1) || liborbisutil::pad::is_down(liborbisutil::pad::buttons::r1))
    {
        io.MouseDown[0] = true;
    }
    else
        io.MouseDown[0] = false;
}

static void ImGui_ImplOrbis_UpdateTouchpad()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
    if (!bd->HasTouchpadControl)
        return;

    constexpr float kMarginCursor = 10.0f;

    static auto prevTouchData = liborbisutil::pad::get_touchpad(0);
    auto touchData = liborbisutil::pad::get_touchpad(0);

    ScePadData data = bd->m_sce_pad;

    if (data.touchData.touchNum > 0)
    {
        if (prevTouchData.id != 255 && prevTouchData.id == touchData.id)
        {
            io.MousePos.x += ((touchData.input.x - prevTouchData.input.x) * (float)video->width / 2.0f);
            if (io.MousePos.x > video->width - kMarginCursor)	io.MousePos.x = video->width - kMarginCursor;
            if (io.MousePos.x < 0)								io.MousePos.x = 0;
            io.MousePos.y += ((touchData.input.y - prevTouchData.input.y) * (float)video->height / 2.0f);
            if (io.MousePos.y > video->height - kMarginCursor)	io.MousePos.y = video->height - kMarginCursor;
            if (io.MousePos.y < 0)								io.MousePos.y = 0;
        }

        //io.MousePos[0] = pad::is_down(pad::button::touchpad);

        prevTouchData = touchData;
    }
    else
    {
        prevTouchData.id = 255;
    }
}

static void ImGui_ImplOrbis_UpdateOsk()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;

    switch (bd->osk.State)
    {
    case OSK_IDLE:
    {
        bool should_spawn_osk = io.WantTextInput && (bd->osk.WantTextInput != io.WantTextInput);

        if (should_spawn_osk)
        {
            wcscpy(bd->osk.Text, L"");

            sceSysmoduleLoadModule(SCE_SYSMODULE_IME_DIALOG);

            SceImeDialogParam param;
            memset(&param, 0, sizeof(SceImeDialogParam));

            param.inputTextBuffer = bd->osk.Text;
            param.maxTextLength = 1024;
            param.enterLabel = SCE_IME_ENTER_LABEL_DEFAULT;
            param.type = SCE_IME_TYPE_DEFAULT;
            param.title = L"Enter text";
            param.placeholder = L"Enter text";
            param.userId = SCE_USER_SERVICE_USER_ID_EVERYONE;

            param.posx = (video->width / 2);
            param.posy = (video->height / 2);
            param.horizontalAlignment = SCE_IME_HALIGN_CENTER;
            param.verticalAlignment = SCE_IME_VALIGN_CENTER;
            sceImeDialogInit(&param, nullptr);

            bd->osk.State = OSK_OPENED;
        }
        break;
    }
    case OSK_OPENED:
    {
        auto dialogStatus = sceImeDialogGetStatus();
        if (dialogStatus == SCE_IME_DIALOG_STATUS_FINISHED)
        {
            bd->osk.State = OSK_CLOSED;
        }
        break;
    }
    case OSK_CLOSED:
    {
        // get result and convert to utf8 std::string
        SceImeDialogResult result;
        memset(&result, 0, sizeof(result));

        sceImeDialogGetResult(&result);

        bd->osk.State = result.endstatus == SCE_IME_DIALOG_END_STATUS_OK ? OSK_ENTERED : OSK_CANCELLED;
        break;
    }
    case OSK_ENTERED:
    {
        std::string str(bd->osk.Text, bd->osk.Text + wcslen(bd->osk.Text));

        io.ClearInputCharacters();
        io.AddInputCharactersUTF8(str.c_str());

        sceImeDialogTerm();
        bd->osk.State = OSK_IDLE;
        break;
    }
    case OSK_CANCELLED:
    {
        sceImeDialogTerm();
        bd->osk.State = OSK_IDLE;
        break;
    }
    }

    bd->osk.WantTextInput = io.WantTextInput;
}

// Gamepad navigation mapping
static void ImGui_ImplOrbis_UpdateGamepads()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;

    // Calling XInputGetState() every frame on disconnected gamepads is unfortunately too slow.
    // Instead we refresh gamepad availability by calling XInputGetCapabilities() _only_ after receiving WM_DEVICECHANGE.
    if (bd->WantUpdateHasGamepad)
    {
        bd->HasGamepad = true;
        bd->WantUpdateHasGamepad = false;
    }

    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    ScePadData data = bd->m_sce_pad;

    float leftStickX = liborbisutil::pad::get_left_stick_x(), leftStickY = liborbisutil::pad::get_left_stick_y();
    float rightStickX = liborbisutil::pad::get_right_stick_x(), rightStickY = liborbisutil::pad::get_right_stick_y();

    io.AddKeyEvent(ImGuiKey_GamepadStart, data.buttons & SCE_PAD_BUTTON_START);
    io.AddKeyEvent(ImGuiKey_GamepadBack, data.buttons & SCE_PAD_BUTTON_TOUCH_PAD);
    io.AddKeyEvent(ImGuiKey_GamepadFaceLeft, data.buttons & SCE_PAD_BUTTON_SQUARE);
    io.AddKeyEvent(ImGuiKey_GamepadFaceRight, data.buttons & SCE_PAD_BUTTON_CIRCLE);
    io.AddKeyEvent(ImGuiKey_GamepadFaceUp, data.buttons & SCE_PAD_BUTTON_TRIANGLE);
    io.AddKeyEvent(ImGuiKey_GamepadFaceDown, data.buttons & SCE_PAD_BUTTON_CROSS);
    io.AddKeyEvent(ImGuiKey_GamepadDpadLeft, data.buttons & SCE_PAD_BUTTON_LEFT);
    io.AddKeyEvent(ImGuiKey_GamepadDpadRight, data.buttons & SCE_PAD_BUTTON_RIGHT);
    io.AddKeyEvent(ImGuiKey_GamepadDpadUp, data.buttons & SCE_PAD_BUTTON_UP);
    io.AddKeyEvent(ImGuiKey_GamepadDpadDown, data.buttons & SCE_PAD_BUTTON_DOWN);
    io.AddKeyEvent(ImGuiKey_GamepadL1, data.buttons & SCE_PAD_BUTTON_L1);
    io.AddKeyEvent(ImGuiKey_GamepadR1, data.buttons & SCE_PAD_BUTTON_R1);
    io.AddKeyEvent(ImGuiKey_GamepadL2, data.buttons & SCE_PAD_BUTTON_L2);
    io.AddKeyEvent(ImGuiKey_GamepadR2, data.buttons & SCE_PAD_BUTTON_R2);
    io.AddKeyEvent(ImGuiKey_GamepadL3, data.buttons & SCE_PAD_BUTTON_L3);
    io.AddKeyEvent(ImGuiKey_GamepadR3, data.buttons & SCE_PAD_BUTTON_R3);
    io.AddKeyEvent(ImGuiKey_GamepadR3, data.buttons & SCE_PAD_BUTTON_R3);

    io.AddKeyAnalogEvent(ImGuiKey_GamepadL2, data.buttons & SCE_PAD_BUTTON_L2, liborbisutil::pad::l2_pressure);
    io.AddKeyAnalogEvent(ImGuiKey_GamepadR2, data.buttons & SCE_PAD_BUTTON_R2, liborbisutil::pad::r2_pressure);
    io.AddKeyAnalogEvent(ImGuiKey_GamepadLStickLeft, leftStickX != 0.0f, -leftStickX);
    io.AddKeyAnalogEvent(ImGuiKey_GamepadLStickRight, leftStickX != 0.0f, leftStickX);
    io.AddKeyAnalogEvent(ImGuiKey_GamepadLStickUp, leftStickY != 0.0f, -leftStickY);
    io.AddKeyAnalogEvent(ImGuiKey_GamepadLStickDown, leftStickY != 0.0f, leftStickY);

    io.AddKeyAnalogEvent(ImGuiKey_GamepadRStickLeft, rightStickX != 0.0f, -rightStickX);
    io.AddKeyAnalogEvent(ImGuiKey_GamepadRStickRight, rightStickX != 0.0f, rightStickX);
    io.AddKeyAnalogEvent(ImGuiKey_GamepadRStickUp, rightStickY != 0.0f, -rightStickY);
    io.AddKeyAnalogEvent(ImGuiKey_GamepadRStickDown, rightStickY != 0.0f, rightStickY);
}

void ImGui_ImplOrbis_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplOrbis_Data* bd = ImGui_ImplOrbis_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplOrbis_Init()?");

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    int viewport[4];
    viewport[2] = video->width;
    viewport[3] = video->height;
    w = display_w = viewport[2];
    h = display_h = viewport[3];
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

    // Setup time step
    uint64_t current_time;
    current_time = sceKernelGetProcessTime();
    io.DeltaTime = (float)(current_time - g_time) / 1000000.f;
    g_time = current_time;

    // Update OS mouse position
    ImGui_ImplOrbis_UpdateMouseData();

    // Update game controllers (if enabled and available)
    ImGui_ImplOrbis_UpdateGamepads();

    // Update touchpad controls
    ImGui_ImplOrbis_UpdateTouchpad();

	// Update osk controls
    ImGui_ImplOrbis_UpdateOsk();
}

bool ImGui_ImplOrbis_HandleEvent()
{

    return true;
}