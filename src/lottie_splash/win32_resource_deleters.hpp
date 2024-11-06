#pragma once
struct WindowDeleter final {
    inline void operator()(HWND hwnd) const {
        if(hwnd)
            DestroyWindow(hwnd);
    }
};

struct DCDeleter final {
    HWND hwnd = {};

    inline void operator()(HDC hdc) const {
        if(hdc)
            ReleaseDC(hwnd, hdc);
    }
};

struct GLContextDeleter final {
    inline void operator()(HGLRC hglrc) const {
        if(hglrc) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(hglrc);
        }
    }
};
