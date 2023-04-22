#include <Windows.h>
#include <stdint.h>

#define internal_fn static 
#define local_persist static 
#define global_variable static 

// global variable for now
global_variable bool running;

// TODO(refactor) remove these globals
global_variable BITMAPINFO bitmap_info;
global_variable void *BITMAP_MEMORY;
global_variable int BITMAP_WIDTH;
global_variable int BITMAP_HEIGHT;
global_variable int bytes_per_pixel = 4;


internal_fn void render_weird_gradient(int x_offset, int y_offset) 
{
    int width = BITMAP_WIDTH;
    int height = BITMAP_HEIGHT;

    int pitch = width * bytes_per_pixel;        // how big the row is
    uint8_t *row = (uint8_t *)BITMAP_MEMORY;

    for (int y = 0; y < height; ++y) 
    {
        uint8_t *pixel = (uint8_t *)row;      // once in the for loop we can start thinking pixel by pixel
        for (int x = 0; x < width; ++x) 
        {
            *pixel = (uint8_t)(x + x_offset);
            ++pixel;
        
            *pixel = (uint8_t)(y + y_offset);
            ++pixel;
        
            *pixel = 0;
            ++pixel;

            *pixel = 0;
            ++pixel;
        
        }

        row += pitch;
    }
}

internal_fn void w32_resize_dib_section(int width, int height)
{

    if (BITMAP_MEMORY)
    {
        VirtualFree(BITMAP_MEMORY, NULL, MEM_RELEASE);
    }

    BITMAP_WIDTH = width;
    BITMAP_HEIGHT = height;

    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = BITMAP_WIDTH;
    bitmap_info.bmiHeader.biHeight = -BITMAP_HEIGHT;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    int bitmap_mem_size = (BITMAP_WIDTH * BITMAP_HEIGHT) * bytes_per_pixel; 
    BITMAP_MEMORY = VirtualAlloc(NULL, bitmap_mem_size, MEM_COMMIT, PAGE_READWRITE);

    


}

internal_fn void w32_update_window(HDC device_context, RECT *window_rect, int x, int y, int width, int height)
{

    int window_width = window_rect->right - window_rect->left;
    int window_height = window_rect->bottom - window_rect->top;

    StretchDIBits(device_context, 
        NULL, NULL, BITMAP_WIDTH, BITMAP_HEIGHT, 
        NULL, NULL, window_width, window_height, 
        BITMAP_MEMORY, 
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

            RECT client_rect;
            GetClientRect(window, &client_rect);

            // get x, y, height and width to passed to the PatBlt
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;

            w32_update_window(device_context, &client_rect, x, y, width, height);
            
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
            int x_offset = 0;
            int y_offset = 0;
            while (running) 
            {

                MSG message_incoming;

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

                render_weird_gradient(x_offset, y_offset);
                HDC device_context = GetDC(window_handle);
                RECT window_rect;
                GetClientRect(window_handle, &window_rect);
                int window_width = window_rect.right - window_rect.left;
                int window_height = window_rect.bottom - window_rect.top;
                w32_update_window(device_context, &window_rect, 0, 0, window_width, window_height);
                ReleaseDC(window_handle, device_context);

                ++x_offset;
                ++y_offset;
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