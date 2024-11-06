#pragma once
#include <thorvg.h>

#include <memory>
#include <mutex>
#include <string>
#include <chrono>
#include <atomic>

#include <Windows.h>

#include "win32_resource_deleters.hpp"

class SplashWindow final {
  public:
    explicit SplashWindow(std::pair<int, int> dimensions) noexcept;
    ~SplashWindow() noexcept;

    bool init(const char * lottie_data, size_t data_size, const wchar_t * window_title) noexcept;
    bool run_message_loop() noexcept;
    void set_status_message(const char8_t * message) noexcept;
    void set_progress(float progress) noexcept;
    bool wait_until_closed(const unsigned timeout_ms = 3000) noexcept;
    void request_close() noexcept;
    void show() const noexcept;
    bool is_initialized() const noexcept;

    enum class InitError {
        None,
        WindowCreationFailed,
        OpenGLInitFailed,
        ThorVGInitFailed,
        AnimationLoadFailed,
        FontLoadFailed,
        DisplayInitFailed,
    } _last_error = InitError::None;

  private:
    bool render() noexcept;
    bool init_window(const wchar_t * window_title) noexcept;
#ifdef THORVG_GL_RASTER_SUPPORT
    bool init_opengl() noexcept;
#endif
    bool  init_thorvg(const char * lottie_data, const size_t data_size) noexcept;
    bool  init_thorvg_common(const char * lottie_data, size_t data_size) noexcept;
    bool  init_fonts() noexcept;
    void  cleanup() noexcept;
    float get_interpolated_progress() noexcept;

    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;
    LRESULT                 HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;

    std::string _loaded_font_family;
    float       _dpi_scale = 1.0f;

    mutable std::mutex _state_mutex;
    std::atomic_bool   _is_rendering = false;
    struct WindowState {
        std::u8string status_message;
        float         progress = 0.0f;
    };
    WindowState      _current_state;
    WindowState      _pending_state;
    std::atomic_bool _needs_update = false;

    struct {
        float                                 start_value      = 0.0f;
        float                                 target_value     = 0.0f;
        std::chrono::steady_clock::time_point start_time       = std::chrono::steady_clock::now();
        bool                                  is_interpolating = false;
    } _progress_state;

    struct {
        bool thorvg_initialized = false;
        bool opengl_initialized = false;
        bool window_initialized = false;
    } _init_state;

    const int                              _window_width;
    const int                              _window_height;
    std::unique_ptr<HWND__, WindowDeleter> _hwnd;
    std::unique_ptr<HDC__, DCDeleter>      _hdc;
#ifdef THORVG_GL_RASTER_SUPPORT
    std::unique_ptr<HGLRC__, GLContextDeleter> _hglrc;
    std::unique_ptr<tvg::GlCanvas>             _canvas;
#else
    std::unique_ptr<HDC__, DCDeleter> _memdc;
    std::unique_ptr<tvg::SwCanvas>    _canvas;
#endif
    std::unique_ptr<tvg::Animation>       _logo_animation;
    std::chrono::steady_clock::time_point _start_time;
    std::atomic_bool                      _close_requested = false;
};