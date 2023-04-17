#include <windows.h>
#include <stdio.h>

#define internal_fn static 
#define local_persist static 
#define global_variable static 

// global variable for now
global_variable bool running;

internal_fn void resize_dib_section()
{

}

// Window proc call back
LRESULT CALLBACK main_window_callback(
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
            
            PatBlt(device_context, x, y, width, height, WHITENESS);
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
    window_class.lpfnWndProc = main_window_callback;
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