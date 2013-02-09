#define _WIN32_WINNT 0x0500
#define WINVER 0x0500
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#define G_GL_EXT_IMPLEMENT
#include "land.h"
#include "gl/wglext.h"

GConfig conf = { NULL, 640, 480, 0, 15, NULL };


LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE ignoreMe0, LPSTR ignoreMe1, INT ignoreMe2) {
    LPCSTR szName = "MyrApp";

    g_configure( &conf );
    if( !conf.title ) conf.title = strdup("Myr default");

    WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, GetModuleHandle(0), 0, 0, 0, 0, szName, 0 };
    DWORD dwStyle = WS_SYSMENU | WS_VISIBLE | ( conf.flags & GC_FULLSCREEN ? WS_POPUP : WS_OVERLAPPEDWINDOW );
    DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    RECT rect;
    int windowWidth, windowHeight, windowLeft, windowTop;
    HWND hWnd;
    PIXELFORMATDESCRIPTOR pfd;
    HDC hDC;
    HGLRC hRC;
    int pixelFormat;
//    GLenum err;
    DWORD previousTime = GetTickCount();
    MSG msg = {0};

    wc.hCursor = LoadCursor( 0, IDC_ARROW );
    RegisterClassExA( &wc );

    SetRect( &rect, 0, 0, conf.width, conf.height );
    AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);
    windowWidth = rect.right - rect.left;
    windowHeight = rect.bottom - rect.top;
    windowLeft = GetSystemMetrics(SM_CXSCREEN) / 2 - windowWidth / 2;
    windowTop = GetSystemMetrics(SM_CYSCREEN) / 2 - windowHeight / 2;
    hWnd = CreateWindowExA(0, szName, szName, dwStyle, windowLeft, windowTop, windowWidth, windowHeight, 0, 0, 0, 0);

    // Create the GL context.
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    hDC = GetDC(hWnd);
    pixelFormat = ChoosePixelFormat(hDC, &pfd);

    SetPixelFormat(hDC, pixelFormat, &pfd);
    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);

    if( conf.flags & GC_MULTISAMPLING ) {
        int pixelAttribs[] = {
            WGL_SAMPLES_ARB, 16,
            WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_RED_BITS_ARB, 8,
            WGL_GREEN_BITS_ARB, 8,
            WGL_BLUE_BITS_ARB, 8,
            WGL_ALPHA_BITS_ARB, 8,
            WGL_DEPTH_BITS_ARB, 24,
            WGL_STENCIL_BITS_ARB, 8,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            0
        };
        int* sampleCount = pixelAttribs + 1;
        int* useSampleBuffer = pixelAttribs + 3;
        int pixelFormat = -1;
        PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC) wglGetProcAddress("wglChoosePixelFormatARB");
        unsigned int numFormats;

        if( !wglChoosePixelFormatARB )
            g_fatal_error("Could not load function pointer for 'wglChoosePixelFormatARB'.  Is your driver properly installed?");

        // Try fewer and fewer samples per pixel till we find one that is supported:
        while( pixelFormat <= 0 && *sampleCount >= 0 ) {
            wglChoosePixelFormatARB(hDC, pixelAttribs, 0, 1, &pixelFormat, &numFormats);
            (*sampleCount)--;
            if (*sampleCount <= 1) *useSampleBuffer = GL_FALSE;
        }

        // Win32 allows the pixel format to be set only once per app, so destroy and re-create the app:
        DestroyWindow( hWnd );
        hWnd = CreateWindowExA( 0, szName, szName, dwStyle, windowLeft, windowTop, windowWidth, windowHeight, 0, 0, 0, 0);
        SetWindowPos(hWnd, HWND_TOP, windowLeft, windowTop, windowWidth, windowHeight, 0);
        hDC = GetDC(hWnd);
        SetPixelFormat(hDC, pixelFormat, &pfd);
        hRC = wglCreateContext(hDC);
        wglMakeCurrent(hDC, hRC);
    }

    g_debug_str("OpenGL Version: %s\n", glGetString(GL_VERSION));

    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
    if ( !(conf.flags & GC_VERTICAL_SYNC) && wglSwapIntervalEXT ) wglSwapIntervalEXT(0);

    if( conf.flags & GC_CORE_PROFILE ) {
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");
        if (!wglCreateContextAttribsARB)
            g_fatal_error("Your platform does not support OpenGL 3.0.\nTry a configuration without the CORE_PROFILE flag.\n");

        const int contextAttribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, conf.gl_version/10,
            WGL_CONTEXT_MINOR_VERSION_ARB, conf.gl_version%10,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            0
        };

        HGLRC newRC = wglCreateContextAttribsARB(hDC, 0, contextAttribs);
        wglMakeCurrent(0, 0);
        wglDeleteContext(hRC);
        hRC = newRC;
        wglMakeCurrent(hDC, hRC);
    }

    g_init_gl_extensions();
    g_initialize( conf.width, conf.height, conf.data );
    SetWindowTextA( hWnd, conf.title );


    // -------------------
    // Start the Game Loop
    // -------------------
    while( msg.message != WM_QUIT ) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            DWORD currentTime = GetTickCount();
            DWORD deltaTime = currentTime - previousTime;
            previousTime = currentTime;
            g_update( deltaTime, conf.data );
            g_render( conf.data );
            SwapBuffers(hDC);
            if (glGetError() != GL_NO_ERROR) g_fatal_error("OpenGL error.\n");
        }
    }

    g_free( conf.title );
    g_cleanup( conf.data );
    UnregisterClassA(szName, wc.hInstance);

    return 0;
}


static int vk_to_key( DWORD key ) {
    switch( key )   {
        case 'Q': return GK_Q;
        case 'W': return GK_W;
        case 'E': return GK_E;
        case 'A': return GK_A;
        case 'S': return GK_S;
        case 'D': return GK_D;
        case VK_UP: return GK_UP;
        case VK_DOWN: return GK_DOWN;
        case VK_LEFT: return GK_LEFT;
        case VK_RIGHT: return GK_RIGHT;
        case VK_RETURN: return GK_RETURN;
        case VK_ESCAPE: return GK_ESCAPE;
        case VK_SPACE:  return GK_SPACE;
        default: return GK_UNKNOWN;
    }
};


LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    GEvent e;
    switch (msg) {
        case WM_CLOSE: PostQuitMessage(0); break;
        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
            e.type = (msg == WM_LBUTTONDOWN ? GE_MOUSEDOWN : GE_MOUSEUP);
            e.x = x;
            e.y = y;
            g_handle_event( &e, conf.data );
            break;

        case WM_MOUSEMOVE:
            e.type = GE_MOUSEMOVE;
            e.value = 0;
            e.x = x;
            e.y = y;
            g_handle_event( &e, conf.data );
            break;

        case WM_KEYUP:
        case WM_KEYDOWN:
            // no key repeat 'if previous state = PRESSED and is not being RELEASED'
            if( (conf.flags & GC_IGNORE_KEYREPEAT) && (lParam & 0x40000000) && msg != WM_KEYUP ) break;
            e.type = (msg == WM_KEYDOWN ? GE_KEYDOWN : GE_KEYUP);
            e.value = vk_to_key(wParam);
            if( !g_handle_event(&e, conf.data) ) PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void g_debug_wstr( const wchar_t* wstr, ...) {
    wchar_t msg[1024] = {0};

    va_list a;
    va_start(a, wstr);

    _vsnwprintf( msg, 1024, wstr, a );
    OutputDebugStringW( msg );
}

void g_debug_str( const char* str, ...) {
    char msg[1024] = {0};

    va_list a;
    va_start( a, str );

    _vsnprintf( msg, 1024, str, a );
    printf(msg);
    OutputDebugStringA( msg );
}

void g_fatal_werror( const wchar_t* wstr, ... ) {
    wchar_t msg[1024] = {0};

    va_list a;
    va_start( a, wstr );

    _vsnwprintf( msg, 1024, wstr, a );
    OutputDebugStringW( msg );
#ifdef _DEBUG
    __debugbreak();
#endif
    exit(1);
}

void g_fatal_error( const char* str, ... ) {
    char msg[1024] = {0};

    va_list a;
    va_start( a, str );

    _vsnprintf( msg, 1024, str, a );
    printf(msg);
    OutputDebugStringA( msg );
#ifdef _DEBUG
    __debugbreak();
#endif
    exit(1);
}
