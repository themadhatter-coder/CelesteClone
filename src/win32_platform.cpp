#include "platform.h"
#include "game_lib.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "wglext.h"

// user32.lib and gdi32 lib

// #################################################### //
// Windows Globals
// #################################################### //
static HWND window;
static HDC dc;

// #################################################### //
// Platform Implementation
// #################################################### //
LRESULT CALLBACK windows_window_callback(HWND window, UINT msg,
                                         WPARAM wParam, LPARAM lparam)
{
    LRESULT result = 0;

    switch (msg)
    {
    case WM_CLOSE:
    {
        running = false;
        break;
    }

    case WM_SIZE:
    {
        RECT rect = {};
        GetClientRect(window, &rect);
        input.screenSizeX = rect.right - rect.left;
        input.screenSizeY = rect.bottom - rect.top;
        break;
    }

    default:
        // Let windows handle default
        result = DefWindowProcA(window, msg, wParam, lparam);
    }

    return result;
}
bool platform_create_window(int width, int height, char *title)
{
    HINSTANCE instance = GetModuleHandleA(0);

    WNDCLASSA wc = {};
    wc.hInstance = instance;
    wc.hIcon = LoadIcon(instance, IDI_APPLICATION); // Application Icon
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);       // Cursor used in Application
    wc.lpszClassName = title;                       // NOT the class name, used like ID
    wc.lpfnWndProc = windows_window_callback;       // Callback for Input into the window

    if (!RegisterClassA(&wc))
    {
        return false;
    }

    int dwStyle = WS_OVERLAPPEDWINDOW; // Window Style
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

    {
        window = CreateWindowExA(0, title, // This is the reference lpszClassName from wc
                                 title,    // This is the actual title
                                 dwStyle,
                                 100,
                                 100,
                                 width,
                                 height,
                                 NULL, // Parent
                                 NULL, // Menu
                                 instance,
                                 NULL); // lpParam
        if (window == NULL)
        {
            SM_ASSERT(false, "Failed to create Windows window");
            return false;
        }

        HDC fakeDC = GetDC(window);
        if (!fakeDC)
        {
            SM_ASSERT(false, "Failed to get HDC");
            return false;
        }

        PIXELFORMATDESCRIPTOR pdf = {0};
        pdf.nSize = sizeof(pdf);
        pdf.nVersion = 1;
        pdf.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pdf.iPixelType = PFD_TYPE_RGBA;
        pdf.cColorBits = 32;
        pdf.cAlphaBits = 8;
        pdf.cDepthBits = 24;

        int pixelFormat = ChoosePixelFormat(fakeDC, &pdf);
        if (!pixelFormat)
        {
            SM_ASSERT(false, "Failed to choose pixel format");
            return false;
        }

        if (!SetPixelFormat(fakeDC, pixelFormat, &pdf))
        {
            SM_ASSERT(false, "Failed to set pixel format");
            return false;
        }

        // Create handle to a fake OpenGL rendering context
        HGLRC fakeRC = wglCreateContext(fakeDC);
        if (!fakeRC)
        {
            SM_ASSERT(false, "Failed to create render context");
            return false;
        }

        if (!wglMakeCurrent(fakeDC, fakeRC))
        {
            SM_ASSERT(false, "Failed to make current");
            return false;
        }

        wglChoosePixelFormatARB =
            (PFNWGLCHOOSEPIXELFORMATARBPROC)platform_load_gl_function("wglChoosePixelFormatARB");
        wglCreateContextAttribsARB =
            (PFNWGLCREATECONTEXTATTRIBSARBPROC)platform_load_gl_function("wglCreateContextAttribsARB");

        if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB)
        {
            SM_ASSERT(false, "Failed to load OpenGL functions");
            return false;
        }

        // Clean up the fake stuff
        wglMakeCurrent(fakeDC, 0);
        wglDeleteContext(fakeRC);
        ReleaseDC(window, fakeDC);

        // Can't reuse the same (Device)Context
        DestroyWindow(window);
    }

    // Actual OpenGL init
    {
        // Add in the border size of the window
        {
            RECT borderRect = {};
            AdjustWindowRectEx(&borderRect, dwStyle, 0, 0);

            width += borderRect.right - borderRect.left;
            height += borderRect.bottom - borderRect.top;
        }

        window = CreateWindowExA(0, title, // This is the reference lpszClassName from wc
                                 title,    // This is the actual title
                                 dwStyle,
                                 100,
                                 100,
                                 width,
                                 height,
                                 NULL, // Parent
                                 NULL, // Menu
                                 instance,
                                 NULL); // lpParam
        if (window == NULL)
        {
            SM_ASSERT(false, "Failed to create Windows window");
            return false;
        }

        dc = GetDC(window);
        if (!dc)
        {
            SM_ASSERT(false, "Failed to get DC");
            return false;
        }

        const int pixelAttribs[] = {
            WGL_DRAW_TO_WINDOW_ARB,
            GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB,
            GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,
            GL_TRUE,
            WGL_SWAP_METHOD_ARB,
            WGL_SWAP_COPY_ARB,
            WGL_PIXEL_TYPE_ARB,
            WGL_TYPE_RGBA_ARB,
            WGL_ACCELERATION_ARB,
            WGL_FULL_ACCELERATION_ARB,
            WGL_COLOR_BITS_ARB,
            32,
            WGL_ALPHA_BITS_ARB,
            8,
            WGL_DEPTH_BITS_ARB,
            24,
            0};

        UINT numPixelFormats;
        int pixelFormat = 0;
        if (!wglChoosePixelFormatARB(dc, pixelAttribs, 0, 1, &pixelFormat, &numPixelFormats))
        {
            SM_ASSERT(0, "Failed to wglChoosePixelFormatARB");
            return false;
        }

        PIXELFORMATDESCRIPTOR pfd = {0};
        DescribePixelFormat(dc, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

        if (!SetPixelFormat(dc, pixelFormat, &pfd))
        {
            SM_ASSERT(0, "Failed to SetPixelFormat");
            return false;
        }

        const int contextAttribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
            0};

        HGLRC rc = wglCreateContextAttribsARB(dc, 0, contextAttribs);
        if (!rc)
        {
            SM_ASSERT(0, "Failed to create Render Context for OpenGL");
            return false;
        }

        if (!wglMakeCurrent(dc, rc))
        {
            SM_ASSERT(0, "Failed to wglMakeCurrent");
            return false;
        }
    }

    ShowWindow(window, SW_SHOW);

    return true;
}

void platform_update_window()
{
    MSG msg;

    while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

void *platform_load_gl_function(char *funName)
{
    PROC proc = wglGetProcAddress(funName);
    if (!proc)
    {
        static HMODULE openglDLL = LoadLibraryA("opengl32.dll");
        proc = GetProcAddress(openglDLL, funName);

        if (!proc)
        {
            SM_ASSERT(false, "Failed to load OpenGL fuunctions %s", "glCreateProgram_ptr");
            return nullptr;
        }
    }

    return (void *)proc;
}

void platform_swap_buffers()
{
    SwapBuffers(dc);
}
