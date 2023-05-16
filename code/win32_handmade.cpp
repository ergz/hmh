#include <stdint.h>
#include <stdio.h>

#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>


#define internal_fn static 
#define local_persist static 
#define global_variable static 

#define PI32 3.14159265359f

typedef float real32;
typedef double real64;

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

struct W32_sound_output
{
    int samples_per_second;
    int tone_hz;
    int16_t tone_volume;
    uint32_t running_sample_index;
    int wave_period;
    int bytes_per_sample;
    int secondary_buffer_size;
    real32 t_sine;
    int latency_sample_count;

};

// global variable for now
global_variable bool running;
global_variable W32_offscreen_buffer GLOBAL_OFFSCREEN_BUFFER;
global_variable LPDIRECTSOUNDBUFFER GLOBAL_SECONDARY_BUFFER;
global_variable bool sound_is_playing;

global_variable int x_offset = 0;
global_variable int y_offset = 0;

// this is a macro that will expand to a function signature when passed in a value for name
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)

/*
here the macro is first expanded to be 
DWOTD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState);
the typedef here creates a new type "x_input_get_state" for 
*/
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(x_input_get_state_stub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = x_input_get_state_stub;
 
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(x_input_set_state_stub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = x_input_set_state_stub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

internal_fn void w32_load_xinput(void)
{
    // TODO(ergz) xinput load dll check versions
    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
    if (!XInputLibrary)
    {
        HMODULE XInputLibrary = LoadLibrary("xinput1_3.dll");
    }
    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}


#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN punkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal_fn void w32_fill_sound_buffer(W32_sound_output *sound_output, DWORD byte_to_lock, DWORD bytes_to_write)
{

    VOID *region_1;
    DWORD region_1_size;
    VOID *region_2;
    DWORD region_2_size;

    HRESULT lock_error = GLOBAL_SECONDARY_BUFFER->Lock(
        byte_to_lock,
        bytes_to_write,
        &region_1, &region_1_size,
        &region_2, &region_2_size,
        0
    );

    if (SUCCEEDED(lock_error)) 
    {
        DWORD region_1_sample_count = region_1_size/sound_output->bytes_per_sample;
        int16_t *sample_output = (int16_t *)region_1;
        for (DWORD sample_index = 0; sample_index < region_1_sample_count; sample_index++)
        {
            real32 sine_value = sinf(sound_output->t_sine);
            int16_t sample_value = (int16_t)(sine_value * sound_output->tone_volume);

            // assign value to sample and increase the pointer
            *sample_output++ = sample_value;
            *sample_output++ = sample_value;

            sound_output->t_sine += 2.0f*PI32*1.0f/(real32)sound_output->wave_period;
            ++sound_output->running_sample_index;
        }

        DWORD region_2_sample_count = region_2_size/sound_output->bytes_per_sample;
        sample_output = (int16_t *)region_2;
        for (DWORD sample_index = 0; sample_index < region_2_sample_count; sample_index++)
        {
            real32 sine_value = sinf(sound_output->t_sine);
            int16_t sample_value = (int16_t)(sine_value * sound_output->tone_volume);

            // assign value to sample and increase the pointer
            *sample_output++ = sample_value;
            *sample_output++ = sample_value;

            sound_output->t_sine += 2.0f*PI32*1.0f/(real32)sound_output->wave_period;
            ++sound_output->running_sample_index;
        }

        GLOBAL_SECONDARY_BUFFER->Unlock(region_1, region_1_size, region_2, region_2_size);
    }


}



internal_fn void w32_init_direct_sound(HWND window, int32_t samples_per_second, int32_t buffer_size)
{
    HMODULE direct_sound_library = LoadLibrary("dsound.dll");
 
    if (direct_sound_library)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(direct_sound_library, "DirectSoundCreate");
        LPDIRECTSOUND direct_sound;

        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &direct_sound, 0))) // direct sound gets filled here
        {
            WAVEFORMATEX wave_format = {};
            wave_format.wFormatTag = WAVE_FORMAT_PCM;
            wave_format.nChannels = 2;
            wave_format.nSamplesPerSec = samples_per_second;
            wave_format.wBitsPerSample = 16;
            wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
            wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
            wave_format.cbSize = 0;

            if (SUCCEEDED(direct_sound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC buffer_description = {};
                buffer_description.dwSize = sizeof(buffer_description);
                buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER primary_buffer;

                if (SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_description, &primary_buffer, 0)))
                {
                    if (SUCCEEDED(primary_buffer->SetFormat(&wave_format)))
                    {
                        printf("the primary sound buffer is good to go\n");
                    }
                    else
                    {
                        // TODO logging
                        printf("there was an error setting format for primary_buffer");
                    }
                }
                else
                {
                            // TODO diag logs
                }
            }
            else
            {
                // TODO diag logs
            }
            DSBUFFERDESC buffer_description = {};
            buffer_description.dwSize = sizeof(buffer_description);
            buffer_description.dwFlags = 0;
            buffer_description.dwBufferBytes = buffer_size;
            buffer_description.lpwfxFormat = &wave_format;

            // the CreateBuffer "method" here is going to load the object we want into the address
            // at GLOBAL_SECONDARY_BUFFER, once that is loaded we will call functions from this object
            // as if they were methods so like GLOBAL_SECONDARY_BUFFER->some_function();
            HRESULT ds_error = direct_sound->CreateSoundBuffer(&buffer_description, &GLOBAL_SECONDARY_BUFFER, 0);

            if (SUCCEEDED(ds_error))
            {
                printf("the secondary buffer was popualted!\n");
            }
            else
            {
                printf("there was an error create secondary sound buffer");
            }
        }
        else
        {
                    // TODO diag logs
        }
    }
    else
    {
        // TODO diag logs
    }
}

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
            if (key_was_down != key_is_down) { // NOTE(ergz) dont accept keep-holding-down key presses
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
                    x_offset += 10;
                }
                else if (vk_code == VK_RIGHT)
                {
                    x_offset -= 10;
                    
                }
                else if (vk_code == VK_UP)
                {
                    y_offset += 10;
                    
                }
                else if (vk_code == VK_DOWN)
                {
                    y_offset -= 10;
                    
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
            w32_copy_buffer_to_window(device_context, window_dims.width, window_dims.height, GLOBAL_OFFSCREEN_BUFFER);
            
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

    w32_resize_dib_section(&GLOBAL_OFFSCREEN_BUFFER, 1280, 720);

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

            HDC device_context = GetDC(window_handle);

            W32_sound_output sound_output = {};
            //
            sound_output.samples_per_second = 48000;
            sound_output.tone_hz = 256; // somewhat middle c
            sound_output.tone_volume = 3000;
            sound_output.running_sample_index = 0;

            /*
                samples_per_second/tone_hz = samples/cycle

                period by definition is the distance it takes for the wave to do an entire cycle 
            */
            sound_output.wave_period = sound_output.samples_per_second/sound_output.tone_hz;
            sound_output.bytes_per_sample = sizeof(int16_t)*2; // left and right channels each 16bits
            sound_output.secondary_buffer_size = sound_output.samples_per_second*sound_output.bytes_per_sample;

            w32_init_direct_sound(window_handle, sound_output.samples_per_second, sound_output.secondary_buffer_size);
            w32_fill_sound_buffer(&sound_output, 0, sound_output.secondary_buffer_size);
            GLOBAL_SECONDARY_BUFFER->Play(0, 0, DSBPLAY_LOOPING);

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
                    // FIXME  
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

                render_weird_gradient(&GLOBAL_OFFSCREEN_BUFFER, x_offset, y_offset);

                DWORD play_cursor = 0;
                DWORD write_cursor = 0;

                if (SUCCEEDED(GLOBAL_SECONDARY_BUFFER->GetCurrentPosition(&play_cursor, &write_cursor)))
                {
                    DWORD byte_to_lock = ((sound_output.running_sample_index*sound_output.bytes_per_sample) % sound_output.secondary_buffer_size); 
                    DWORD bytes_to_write;
                    if (byte_to_lock == play_cursor) 
                    {
                        bytes_to_write = 0;
                    }
                    else if (byte_to_lock > play_cursor)
                    {
                        bytes_to_write = (sound_output.secondary_buffer_size - byte_to_lock);
                        bytes_to_write += play_cursor;
                    }
                    else
                    {
                        bytes_to_write = play_cursor - byte_to_lock;
                    }

                    w32_fill_sound_buffer(&sound_output, byte_to_lock, bytes_to_write);

                }

                W32_window_dimensions window_dims = w32_get_window_dimensions(window_handle);
                w32_copy_buffer_to_window(device_context, window_dims.width, window_dims.height, GLOBAL_OFFSCREEN_BUFFER);
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
