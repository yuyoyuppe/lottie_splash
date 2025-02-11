#include "splash_window.hpp"

#include <glad/glad.h>
#include <algorithm>
#include <array>

#include "utils/unicode.hpp"
#include "utils/display.hpp"

namespace {
static constexpr std::chrono::milliseconds PROGRESS_INTERPOLATION_DURATION{500LL};

inline float px_to_pt(const float font_size_px) { return font_size_px * (72.0f / 96.0f); }

#ifdef THORVG_GL_RASTER_SUPPORT
constexpr tvg::CanvasEngine ENGINE = tvg::CanvasEngine::Gl;
#else
constexpr tvg::CanvasEngine ENGINE = tvg::CanvasEngine::Sw;
#endif
constexpr int NUM_THREADS = 2;

bool process_messages() {
    MSG msg;
    while(PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if(msg.message == WM_QUIT || (msg.message == WM_NULL && !IsWindow(msg.hwnd)))
            return true;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return false;
}

DWORD wait_for_messages(DWORD timeout_ms) {
    const DWORD start_time = GetTickCount();
    if(const DWORD result = MsgWaitForMultipleObjects(0, nullptr, FALSE, timeout_ms, QS_ALLINPUT);
       result == WAIT_TIMEOUT)
        return 0;

    const DWORD elapsed = GetTickCount() - start_time;
    return elapsed < timeout_ms ? timeout_ms - elapsed : 0;
}
}

SplashWindow::SplashWindow(std::pair<int, int> dimensions) noexcept
  : _window_width(dimensions.first), _window_height(dimensions.second) {}

SplashWindow::~SplashWindow() noexcept { cleanup(); }

bool SplashWindow::init_thorvg(const char * lottie_data, size_t data_size) noexcept {
    if(!init_fonts())
        return false;

    const int scaled_width  = static_cast<int>(_window_width * _dpi_scale);
    const int scaled_height = static_cast<int>(_window_height * _dpi_scale);

#ifdef THORVG_GL_RASTER_SUPPORT
    _canvas = tvg::GlCanvas::gen();
    if(!_canvas)
        return false;

    if(_canvas->target(0, scaled_width, scaled_height) != tvg::Result::Success)
        return false;
#else
    _hdc = decltype(_hdc){GetDC(_hwnd.get()), {_hwnd.get()}};
    if(!_hdc)
        return false;

    const BITMAPINFO bmi{.bmiHeader = {
                           .biSize        = sizeof(bmi),
                           .biWidth       = scaled_width,
                           .biHeight      = -scaled_height,
                           .biPlanes      = 1,
                           .biBitCount    = 32,
                           .biCompression = BI_RGB,
                         }};

    void *  bits   = nullptr;
    HBITMAP bitmap = CreateDIBSection(_hdc.get(), &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if(!bitmap)
        return false;

    _memdc.reset(CreateCompatibleDC(_hdc.get()));
    SelectObject(_memdc.get(), bitmap);

    _canvas = tvg::SwCanvas::gen();
    if(!_canvas)
        return false;

    if(_canvas->target(
         static_cast<uint32_t *>(bits), scaled_width, scaled_width, scaled_height, tvg::ColorSpace::ARGB8888) !=
       tvg::Result::Success)
        return false;
#endif

    return init_thorvg_common(lottie_data, data_size);
}


bool SplashWindow::init_thorvg_common(const char * lottie_data, size_t data_size) noexcept {
    _logo_animation.reset(tvg::Animation::gen());
    if(!_logo_animation)
        return false;

    auto * logo_picture = _logo_animation->picture();
    if(!logo_picture ||
       logo_picture->load(lottie_data, static_cast<uint32_t>(data_size), "application/json", "", true) !=
         tvg::Result::Success)
        return false;

    float w;
    float h;
    logo_picture->size(&w, &h);
    logo_picture->scale(_dpi_scale);

    const float shiftX = (_window_width - w) * 0.5f * _dpi_scale;
    const float shiftY = 56.f * _dpi_scale;
    logo_picture->translate(shiftX, shiftY);

    _canvas->push(logo_picture);

    _start_time = std::chrono::steady_clock::now();

    return true;
}

bool SplashWindow::init(const char * lottie_data, size_t data_size, const wchar_t * window_title) noexcept {
    cleanup();

    _dpi_scale = utils::get_dpi_scale();
    if(!init_window(window_title)) {
        _last_error = InitError::WindowCreationFailed;
        cleanup();
        return false;
    }
    _init_state.window_initialized = true;

#ifdef THORVG_GL_RASTER_SUPPORT
    if(!init_opengl()) {
        _last_error = InitError::OpenGLInitFailed;
        cleanup();
        return false;
    }
    _init_state.opengl_initialized = true;
#endif

    if(tvg::Initializer::init(NUM_THREADS, ENGINE) != tvg::Result::Success) {
        _last_error = InitError::ThorVGInitFailed;
        cleanup();
        return false;
    }
    _init_state.thorvg_initialized = true;

    if(!init_thorvg(lottie_data, data_size)) {
        _last_error = InitError::AnimationLoadFailed;
        cleanup();
        return false;
    }

    _last_error = InitError::None;
    return true;
}

bool SplashWindow::init_window(const wchar_t * window_title) noexcept {
    constexpr wchar_t WINDOW_CLASS[] = L"LottieSplashWindow";

    const WNDCLASSEXW wc = {.cbSize        = sizeof(wc),
                            .style         = CS_OWNDC,
                            .lpfnWndProc   = StaticWindowProc,
                            .hInstance     = GetModuleHandleW(nullptr),
                            .hCursor       = LoadCursorW(nullptr, IDC_ARROW),
                            .lpszClassName = WINDOW_CLASS};

    static std::once_flag classRegistered;
    bool                  classRegisteredFlag = true;
    std::call_once(classRegistered, [&] { classRegisteredFlag = RegisterClassExW(&wc); });

    if(!classRegisteredFlag)
        return false;

    const HMONITOR hmon = MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO    mi   = {.cbSize = sizeof(mi)};
    GetMonitorInfoW(hmon, &mi);

    const int x = mi.rcMonitor.left + (mi.rcMonitor.right - mi.rcMonitor.left - _window_width) / 2;
    const int y = mi.rcMonitor.top + (mi.rcMonitor.bottom - mi.rcMonitor.top - _window_height) / 2;

    DWORD ex_style = WS_EX_APPWINDOW;
#ifdef THORVG_GL_RASTER_SUPPORT
    ex_style |= WS_EX_LAYERED;
#endif
    _hwnd.reset(CreateWindowExW(ex_style,
                                WINDOW_CLASS,
                                window_title,
                                WS_POPUP | WS_VISIBLE,
                                x,
                                y,
                                static_cast<int>(_window_width * _dpi_scale),
                                static_cast<int>(_window_height * _dpi_scale),
                                nullptr,
                                nullptr,
                                GetModuleHandleW(nullptr),
                                nullptr));
    if(!_hwnd)
        return false;

    using namespace utils;

    enable_transparency(_hwnd.get());
    enable_rounded_corners(_hwnd.get());
    enable_shadow(_hwnd.get());

    return true;
}

void SplashWindow::set_status_message(const char8_t * message) noexcept {
    std::lock_guard lock{_state_mutex};
    _pending_state.status_message = message ? message : u8"";
    _needs_update                 = true;
}

float SplashWindow::get_interpolated_progress() noexcept {
    if(!_progress_state.is_interpolating)
        return _current_state.progress;

    const auto now     = std::chrono::steady_clock::now();
    const auto elapsed = duration_cast<std::chrono::milliseconds>(now - _progress_state.start_time);

    if(elapsed >= PROGRESS_INTERPOLATION_DURATION) {
        _progress_state.is_interpolating = false;
        return _progress_state.target_value;
    }

    const float t = elapsed.count() / static_cast<float>(PROGRESS_INTERPOLATION_DURATION.count());
    const float progress =
      _progress_state.start_value + (_progress_state.target_value - _progress_state.start_value) * t;

    return progress;
}


void SplashWindow::set_progress(float progress) noexcept {
    const float clamped_progress = std::clamp(progress, 0.0f, 1.0f);

    std::lock_guard lock{_state_mutex};
    _progress_state.start_value      = _current_state.progress;
    _progress_state.target_value     = clamped_progress;
    _progress_state.start_time       = std::chrono::steady_clock::now();
    _progress_state.is_interpolating = true;

    _pending_state.progress = clamped_progress;
    _needs_update           = true;
}

bool SplashWindow::wait_until_closed(const unsigned timeout_ms) noexcept {
    if(!_hwnd)
        return true;

    DWORD remaining_time = timeout_ms;
    while(remaining_time > 0 && IsWindow(_hwnd.get())) {
        if(process_messages())
            return true;

        remaining_time = wait_for_messages(remaining_time);
    }

    return !IsWindow(_hwnd.get());
}

void SplashWindow::request_close() noexcept {
    if(_hwnd) {
        _close_requested = true;
        SendMessageW(_hwnd.get(), WM_CLOSE, 0, 0);
    }
}

void SplashWindow::show() const noexcept {
    if(!_hwnd)
        return;

    ShowWindow(_hwnd.get(), SW_SHOW);
    UpdateWindow(_hwnd.get());
}

LRESULT CALLBACK SplashWindow::StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept {
    if(uMsg == WM_NCCREATE) {
        const auto * create_struct = reinterpret_cast<CREATESTRUCTW *>(lParam);
        const auto * window        = static_cast<SplashWindow *>(create_struct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else if(auto * window = reinterpret_cast<SplashWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA)))
        return window->HandleMessage(hwnd, uMsg, wParam, lParam);

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT SplashWindow::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept {
    switch(uMsg) {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

bool SplashWindow::run_message_loop() noexcept {
    constexpr int   TARGET_FPS    = 120;
    constexpr DWORD FRAME_TIME_MS = 1000 / TARGET_FPS;

    for(; !_close_requested && IsWindow(_hwnd.get());) {
        const DWORD start_time = GetTickCount();

        if(process_messages())
            break;

        render();

        const DWORD elapsed = GetTickCount() - start_time;
        if(elapsed < FRAME_TIME_MS)
            wait_for_messages(FRAME_TIME_MS - elapsed);
    }

    CloseWindow(_hwnd.get());
    _hwnd.reset();

    return _close_requested;
}

bool SplashWindow::init_fonts() noexcept {
    struct FontInfo {
        std::wstring filename;
        std::string  family_name;
    };

    const std::array<FontInfo, 3> fonts = {
      {{L"\\Fonts\\segoeui.ttf", "segoeui"}, {L"\\Fonts\\arial.ttf", "arial"}, {L"\\Fonts\\tahoma.ttf", "tahoma"}}};

    std::array<wchar_t, MAX_PATH> windows_dir;
    if(!GetWindowsDirectoryW(windows_dir.data(), MAX_PATH)) {
        _last_error = InitError::FontLoadFailed;
        return false;
    }

    for(const auto & font : fonts) {
        std::wstring full_path = std::wstring(windows_dir.data()) + font.filename;
        if(tvg::Text::load(utils::wide_to_utf8(full_path).c_str()) == tvg::Result::Success) {
            _loaded_font_family = font.family_name;
            return true;
        }
    }

    _last_error = InitError::FontLoadFailed;
    return false;
}

void SplashWindow::cleanup() noexcept {
    _canvas.reset();
    _logo_animation.reset();

    if(_init_state.thorvg_initialized) {
        tvg::Initializer::term(ENGINE);
        _init_state.thorvg_initialized = false;
    }

    if(_init_state.opengl_initialized) {
#ifdef THORVG_GL_RASTER_SUPPORT
        if(_hglrc) {
            wglMakeCurrent(nullptr, nullptr);
            _hglrc.reset();
        }
        _init_state.opengl_initialized = false;
#endif
    }

    if(_init_state.window_initialized) {
        _hwnd.reset();
        _init_state.window_initialized = false;
    }
}

bool SplashWindow::render() noexcept {
    if(!is_initialized())
        return false;

    // Prevent recursion
    bool expected = false;
    if(!_is_rendering.compare_exchange_strong(expected, true))
        return false;

    struct RenderGuard {
        std::atomic<bool> & flag;
        explicit RenderGuard(std::atomic<bool> & f) : flag{f} {}
        ~RenderGuard() { flag = false; }
    } render_guard{_is_rendering};


    {
        std::lock_guard lock{_state_mutex};
        if(_needs_update) {
            _current_state = _pending_state;
            _needs_update  = false;
        }
    }

    _canvas->remove();

    auto scene = tvg::Scene::gen();
    if(!scene)
        return false;

    const auto now     = std::chrono::steady_clock::now();
    const auto elapsed = duration_cast<std::chrono::milliseconds>(now - _start_time).count();

    if(_logo_animation->duration() <= 0.0f)
        return false;

    const float animation_progress =
      (elapsed % static_cast<int>(_logo_animation->duration() * 1000)) / (_logo_animation->duration() * 1000.0f);

    if(_logo_animation->frame(_logo_animation->totalFrame() * animation_progress) != tvg::Result::Success)
        return false;

    auto logo_picture = _logo_animation->picture();
    if(!logo_picture)
        return false;

    auto dup_logo_picture = logo_picture->duplicate();
    if(!dup_logo_picture)
        return false;

    if(scene->push(std::move(dup_logo_picture)) != tvg::Result::Success)
        return false;

    if(_current_state.progress > 0.0f || !_current_state.status_message.empty()) {
        auto overlay_scene = tvg::Scene::gen();
        if(!overlay_scene)
            return false;

        constexpr float BASE_BAR_WIDTH         = 245.f;
        constexpr float BASE_BAR_HEIGHT        = 6.f;
        constexpr float BASE_BAR_CORNER_RADIUS = 3.f;
        constexpr float BASE_BAR_Y             = 243.f;
        constexpr float BASE_STATUS_MESSAGE_Y  = 257.f;
        constexpr float BASE_GETTING_READY_Y   = 200.f;
        constexpr float BASE_X                 = 110.f;

        const float BAR_WIDTH         = BASE_BAR_WIDTH * _dpi_scale;
        const float BAR_HEIGHT        = BASE_BAR_HEIGHT * _dpi_scale;
        const float BAR_CORNER_RADIUS = BASE_BAR_CORNER_RADIUS * _dpi_scale;
        const float bar_x             = (_window_width * _dpi_scale - BAR_WIDTH) * .5f;
        const float bar_y             = BASE_BAR_Y * _dpi_scale;

        if(!_current_state.status_message.empty()) {
            auto text = tvg::Text::gen();
            auto fill = tvg::LinearGradient::gen();
            if(!text || !fill)
                return false;

            text->font(_loaded_font_family.c_str(), px_to_pt(12.f * _dpi_scale));

            constexpr tvg::Fill::ColorStop STATUS_MESSAGE_COLOR = {.offset = 0, .r = 255, .g = 255, .b = 255, .a = 182};
            const std::array<tvg::Fill::ColorStop, 2> colorStops = {STATUS_MESSAGE_COLOR, STATUS_MESSAGE_COLOR};
            fill->linear(0, 0, 1, 1);
            fill->colorStops(colorStops.data(), static_cast<uint32_t>(colorStops.size()));
            text->fill(std::move(fill));

            text->text(reinterpret_cast<const char *>(_current_state.status_message.c_str()));

            const float status_message_x = bar_x;
            const float status_message_y = BASE_STATUS_MESSAGE_Y * _dpi_scale;
            text->translate(status_message_x, status_message_y);
            overlay_scene->push(std::move(text));
        }

        const float progress_bar_progress = get_interpolated_progress();
        auto        bg                    = tvg::Shape::gen();
        if(!bg)
            return false;

        bg->appendRect(bar_x, bar_y, BAR_WIDTH, BAR_HEIGHT, BAR_CORNER_RADIUS, BAR_CORNER_RADIUS);
        bg->fill(128, 128, 128, static_cast<uint8_t>(255 * 0.18f));
        overlay_scene->push(std::move(bg));

        auto fill = tvg::Shape::gen();
        if(!fill)
            return false;

        const float filled_width = BAR_WIDTH * progress_bar_progress;
        fill->appendRect(bar_x, bar_y, filled_width, BAR_HEIGHT, BAR_CORNER_RADIUS, BAR_CORNER_RADIUS);
        fill->fill(255, 255, 255);
        overlay_scene->push(std::move(fill));

        const char8_t * GETTING_READY_MESSAGE = u8"Getting Ready…";
        if(auto getting_ready_text = tvg::Text::gen()) {
            getting_ready_text->font(_loaded_font_family.c_str(), px_to_pt(15.f * _dpi_scale));
            getting_ready_text->text(reinterpret_cast<const char *>(GETTING_READY_MESSAGE));
            getting_ready_text->fill(255, 255, 255);
            getting_ready_text->translate(BASE_X * _dpi_scale, BASE_GETTING_READY_Y * _dpi_scale);
            overlay_scene->push(std::move(getting_ready_text));
        }

        scene->push(std::move(overlay_scene));
    }

    _canvas->push(std::move(scene));
    _canvas->update();
    _canvas->draw();
    _canvas->sync();

#ifdef THORVG_GL_RASTER_SUPPORT
    SwapBuffers(_hdc.get());
#else
    BitBlt(_hdc.get(),
           0,
           0,
           static_cast<int>(_window_width * _dpi_scale),
           static_cast<int>(_window_height * _dpi_scale),
           _memdc.get(),
           0,
           0,
           SRCCOPY);
#endif

    return true;
}

bool SplashWindow::is_initialized() const noexcept { return !!_hwnd && !!_logo_animation && !!_canvas; }

#ifdef THORVG_GL_RASTER_SUPPORT
bool SplashWindow::init_opengl() noexcept {
    _hdc = decltype(_hdc){GetDC(_hwnd.get()), {_hwnd.get()}};
    if(!_hdc)
        return false;

    const PIXELFORMATDESCRIPTOR pfd = {
      .nSize           = sizeof(pfd),
      .nVersion        = 1,
      .dwFlags         = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
      .iPixelType      = PFD_TYPE_RGBA,
      .cColorBits      = 32,
      .cRedBits        = 8,
      .cGreenBits      = 8,
      .cBlueBits       = 8,
      .cAlphaBits      = 8,
      .cAccumAlphaBits = 24,
      .cDepthBits      = 8,
    };

    if(!SetPixelFormat(_hdc.get(), ChoosePixelFormat(_hdc.get(), &pfd), &pfd))
        return false;

    _hglrc.reset(wglCreateContext(_hdc.get()));
    if(!_hglrc)
        return false;

    if(!wglMakeCurrent(_hdc.get(), _hglrc.get()))
        return false;

    static std::once_flag gladInitialized;
    bool                  gladInitializedFlag = true;
    std::call_once(gladInitialized, [&] { gladInitializedFlag = gladLoadGL(); });
    if(!gladInitializedFlag)
        return false;
    return true;
}
#endif