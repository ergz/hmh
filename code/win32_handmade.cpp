#include <stdint.h>

#include <windows.h>
#include <xinput.h>

#define internal_fn static 
#define local_persist static 
#define global_variable static 

struct W32_offscreen_buffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
};

struct W32_window_dimensions 
{
    int width;
    int height;
};


#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(x_input_get_state_stub)
{
    return(0);
}
global_variable x_input_get_state *XInputGetState_ = x_input_get_state_stub;

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(x_input_set_state_stub)
{
    return(0);
}
global_variable x_input_set_state *XInputSetState_ = x_input_set_state_stub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

internal_fn void w32_load_xinput(void)
{
    HMODULE XInputLibrary = LoadLibrary("xinput1_3.dll");
    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

// global variable for now
global_variable bool running;
global_variable W32_offscreen_buffer offscreen_buffer;

global_variable int x_offset = 0;
global_variable int y_offset = 0;

W32_window_dimensions w32_get_window_dimensions(HWND window)
{
    W32_window_dimensions result;

    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;

    return(result);

}

internal_fn void render_weird_gradient(W32_offscreen_buffer *buffer, int x_offset, int y_offset) 
{

    uint8_t *row = (uint8_t *)buffer->memory;

    for (int y = 0; y < buffer->height; ++y) 
    {
        uint32_t *pixel = (uint32_t *)row;      // once in the for loop we can start thinking pixel by pixel
        for (int x = 0; x < buffer->width; ++x) 
        {
            uint8_t blue = (x + x_offset);
            uint8_t green = (y + y_offset);

            *pixel++ = ((green << 8) | blue);
        }

        row += buffer->pitch;
    }
}

internal_fn void w32_resize_dib_section(W32_offscreen_buffer *buffer, int width, int height)
{

    if (buffer->memory)
    {
        VirtualFree(buffer->memory, NULL, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    buffer->bytes_per_pixel = 4;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmap_mem_size = (buffer->width * buffer->height) * buffer->bytes_per_pixel; 
    buffer->memory = VirtualAlloc(NULL, bitmap_mem_size, MEM_COMMIT, PAGE_READWRITE);

    buffer->pitch = buffer->width * buffer->bytes_per_pixel;        // how big the row is

}

internal_fn void w32_copy_buffer_to_window(HDC device_context, int window_width, int window_height, 
                                            W32_offscreen_buffer buffer)
{



    StretchDIBits(device_context, 
        NULL, NULL, window_width, window_height, 
        NULL, NULL, buffer.width, buffer.height, 
        buffer.memory, 
        &buffer.info, 
        DIB_RGB_COLORS,
        SRCCOPY);
}


// Window proc call back
LRESULT CALLBACK w32_main_window_callback(
    HWND window,
    UINT message,
    WPARAM w_param,
    LPARAM l_param)
{
    LRESULT result = 0;

    switch(message) 
    {
        case WM_SIZE: 
        {

        } break;

        case WM_DESTROY: 
        {   
            running = false;
        } break; 

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32_t vk_code = w_param;
            bool key_was_down = ((l_param & (1 << 30)) != 0);
            bool key_is_down = ((l_param & (1 << 31)) == 0);
            if (key_was_down != key_is_down) {
                if (vk_code == 'W')
                {

                }
                else if (vk_code == 'A')
                {

                }
                else if (vk_code == 'S')
                {
                    
                }
                else if (vk_code == 'D')
                {
                    
                }
                else if (vk_code == 'Q')
                {
                    
                }
                else if (vk_code == 'E')
                {
                    
                }
                else if (vk_code == VK_LEFT)
                {
                    x_offset += 50;
                }
                else if (vk_code == VK_RIGHT)
                {
                    x_offset -= 50;
                    
                }
                else if (vk_code == VK_UP)
                {
                    y_offset += 50;
                    
                }
                else if (vk_code == VK_DOWN)
                {
                    y_offset -= 50;
                    
                }
                else if (vk_code == VK_ESCAPE)
                {
                    running = false;
                }            
                else if (vk_code == VK_SPACE)
                {
                    
                }
            }

        } break;

        case WM_CLOSE: 
        {
            running = false;
        } break;

        case WM_ACTIVATEAPP: 
        {

        } break;

        case WM_PAINT: 
        {
            // paint a window
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);

            // get x, y, height and width to passed to the PatBlt
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;

            W32_window_dimensions window_dims = w32_get_window_dimensions(window);
            w32_copy_buffer_to_window(device_context, window_dims.width, window_dims.height, offscreen_buffer);
            
            EndPaint(window, &paint);
        } break;

        default: 
        {
            result = DefWindowProc(window, message, w_param, l_param);
        }
    }

    return(result);
}

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE prev_instance,
    LPSTR command_line,
    int show_code)
{   

    w32_load_xinput();

    WNDCLASS window_class = {};

    w32_resize_dib_section(&offscreen_buffer, 1280, 720);

    window_class.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = w32_main_window_callback;
    window_class.hInstance = instance;
    // window_class.hIcon = ;
    window_class.lpszClassName = "hmh_window_class";

    if (RegisterClass(&window_class)) 
    {
        HWND window_handle = CreateWindowEx(
            0, 
            window_class.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            instance, 
            0);
        if (window_handle) 
        {
            running = true;

            MSG message_incoming;
            while (running) 
            {


                while (PeekMessage(&message_incoming, 0, 0, 0, PM_REMOVE)) 
                {
                    if (message_incoming.message == WM_QUIT) 
                    {
                        running = false;
                    }
                    // FIXME(fixme) 
                    TranslateMessage(&message_incoming);
                    DispatchMessage(&message_incoming);

                }

                for (DWORD controller_index = 0; controller_index < XUSER_MAX_COUNT; controller_index++)
                {
                    XINPUT_STATE controller_state;
                    if (XInputGetState(controller_index, &controller_state) == ERROR_SUCCESS)
                    {
                        // controller is plugged in
                        XINPUT_GAMEPAD *gamepad = &controller_state.Gamepad;

                        bool gamepad_up = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool gamepad_down = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool gamepad_left = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool gamepad_right = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool gamepad_start = (gamepad->wButtons & XINPUT_GAMEPAD_START);
                        bool gamepad_back = (gamepad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool gamepad_left_shoulder = (gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool gamepad_right_shoulder = (gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool gamepad_A = (gamepad->wButtons & XINPUT_GAMEPAD_A);
                        bool gamepad_B = (gamepad->wButtons & XINPUT_GAMEPAD_B);
                        bool gamepad_X = (gamepad->wButtons & XINPUT_GAMEPAD_X);
                        bool gamepad_Y = (gamepad->wButtons & XINPUT_GAMEPAD_Y);

                        uint16_t gamepad_stick_X = gamepad->sThumbLX; 
                        uint16_t gamepad_stick_Y = gamepad->sThumbLY; 

                    }
                    else 
                    {
                        // controller not plugged in
                    }

                }

                render_weird_gradient(&offscreen_buffer, x_offset, y_offset);
                HDC device_context = GetDC(window_handle);
                
                W32_window_dimensions window_dims = w32_get_window_dimensions(window_handle);
                w32_copy_buffer_to_window(device_context, window_dims.width, window_dims.height, offscreen_buffer);
                ReleaseDC(window_handle, device_context);

                // ++x_offset;
                // ++y_offset;
            }
        } 
        else 
        {
            // TODO(refactor) add error logging
        }
    } 
    else 
    {
        // TODO(refactor) add error logging
    }

    return(0);
}
