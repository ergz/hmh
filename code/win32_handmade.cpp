#include <Windows.h>
#include <stdio.h>

#define internal_fn static 
#define local_persist static 
#define global_variable static 

// global variable for now
global_variable bool running;

global_variable BITMAPINFO bitmap_info;
global_variable void *bitmap_memory;
global_variable HBITMAP bitmap_handle;
global_variable HDC bitmap_device_context;

internal_fn void w32_resize_dib_section(int width, int height)
{

    if (bitmap_handle) 
    {
        DeleteObject(bitmap_handle);
    }
    if (!bitmap_handle) 
    {
        bitmap_device_context = CreateCompatibleDC(0);
    }

    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = width;
    bitmap_info.bmiHeader.biHeight = height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    bitmap_handle = CreateDIBSection(
        bitmap_device_context, 
        &bitmap_info, 
        DIB_RGB_COLORS, &bitmap_memory, 0, 0);

}

internal_fn void w32_update_window(HDC device_context, int x, int y, int width, int height)
{
    StretchDIBits(device_context, 
        x, y, width, height, 
        x, y, width, height, 
        bitmap_memory, 
        &bitmap_info, 
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
            RECT client_rect;
            GetClientRect(window, &client_rect);
            int width = client_rect.right - client_rect.left;
            int height = client_rect.bottom - client_rect.top;
            w32_resize_dib_section(width, height);
        } break;

        case WM_DESTROY: 
        {
            running = false;
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

            w32_update_window(device_context, x, y, width, height);
            
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

    WNDCLASS window_class = {};

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
            MSG Message;
            while (running) 
            {
                BOOL message_is_ok = GetMessage(&Message, 0, 0, 0);
                if (message_is_ok > 0) 
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                } 
                else 
                {
                    break; // error
                }
            }
        } 
        else 
        {
            // todo error handle
        }
    } 
    else 
    {
        // todo error handle
    }

    return(0);
}