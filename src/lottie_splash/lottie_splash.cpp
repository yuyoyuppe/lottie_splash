#include "lottie_splash.h"
#include "splash_window.hpp"
#include "utils/display.hpp"
#include "utils/unicode.hpp"
#include <memory>
#include <thread>
#include <optional>

namespace {
lottie_splash_error convert_init_error(SplashWindow::InitError err) {
    switch(err) {
    case SplashWindow::InitError::None:
        return LOTTIE_SPLASH_SUCCESS;
    case SplashWindow::InitError::WindowCreationFailed:
        return LOTTIE_SPLASH_ERROR_WINDOW_CREATION_FAILED;
    case SplashWindow::InitError::OpenGLInitFailed:
        return LOTTIE_SPLASH_ERROR_OPENGL_INIT_FAILED;
    case SplashWindow::InitError::ThorVGInitFailed:
        return LOTTIE_SPLASH_ERROR_THORVG_INIT_FAILED;
    case SplashWindow::InitError::AnimationLoadFailed:
        return LOTTIE_SPLASH_ERROR_ANIMATION_LOAD_FAILED;
    case SplashWindow::InitError::FontLoadFailed:
        return LOTTIE_SPLASH_ERROR_FONT_LOAD_FAILED;
    case SplashWindow::InitError::DisplayInitFailed:
        return LOTTIE_SPLASH_ERROR_DISPLAY_INIT_FAILED;
    default:
        return LOTTIE_SPLASH_ERROR_INVALID_ARGUMENT;
    }
}
}

struct lottie_splash_context {
    std::unique_ptr<SplashWindow>  window;
    std::optional<std::thread::id> window_message_loop_thread_id;
};

extern "C" {
LOTTIE_SPLASH_API lottie_splash_context * lottie_splash_create(const char *          lottie_animation_buf,
                                                               size_t                buf_size,
                                                               const char8_t *       utf8_window_title,
                                                               const unsigned        window_width,
                                                               const unsigned        window_height,
                                                               lottie_splash_error * out_error) {
    auto set_error = [&](lottie_splash_error err) {
        if(out_error)
            *out_error = err;
    };

    if(!lottie_animation_buf || buf_size == 0 || !utf8_window_title) {
        set_error(LOTTIE_SPLASH_ERROR_INVALID_ARGUMENT);
        return nullptr;
    }

    // Some system APIs (e.g. setting the DPI awareness) fail when called from multiple threads. This mutex is used to ensure that only one thread calls them.
    static std::mutex create_mutex;
    std::lock_guard   lock{create_mutex};

    if(!utils::enable_dpi_awareness()) {
        set_error(LOTTIE_SPLASH_ERROR_DISPLAY_INIT_FAILED);
        return nullptr;
    }

    auto ctx = std::make_unique<lottie_splash_context>();

    const auto [monitor_width, monitor_height] = utils::primary_monitor_dims();
    constexpr float WINDOW_COMFORT_RATIO       = 0.5f;
    const float     window_size                = std::min(monitor_width, monitor_height) * WINDOW_COMFORT_RATIO;

    const int final_window_width  = window_width ? window_width : int(window_size);
    const int final_window_height = window_height ? window_height : int(window_size);

    ctx->window = std::make_unique<SplashWindow>(std::make_pair(final_window_width, final_window_height));

    if(!ctx->window->init(lottie_animation_buf, buf_size, utils::utf8_to_wide(utf8_window_title).c_str())) {
        set_error(convert_init_error(ctx->window->_last_error));
        return nullptr;
    }

    ctx->window_message_loop_thread_id = std::this_thread::get_id();
    set_error(LOTTIE_SPLASH_SUCCESS);
    return ctx.release();
}

LOTTIE_SPLASH_API lottie_splash_error lottie_splash_run_window(lottie_splash_context * ctx) {
    if(!ctx || !ctx->window || !ctx->window_message_loop_thread_id)
        return LOTTIE_SPLASH_ERROR_INVALID_ARGUMENT;

    if(*ctx->window_message_loop_thread_id != std::this_thread::get_id())
        return LOTTIE_SPLASH_ERROR_WINDOW_ALREADY_RUNNING;

    ctx->window->show();
    const bool user_closed_window = !ctx->window->run_message_loop();

    if(ctx->window->_last_error != SplashWindow::InitError::None)
        return LOTTIE_SPLASH_ERROR_RENDER_FAILED;

    ctx->window_message_loop_thread_id = std::nullopt;
    return user_closed_window ? LOTTIE_SPLASH_WINDOW_CLOSED_BY_USER : LOTTIE_SPLASH_SUCCESS;
}

LOTTIE_SPLASH_API lottie_splash_error lottie_splash_destroy(lottie_splash_context * ctx) {
    if(!ctx)
        return LOTTIE_SPLASH_ERROR_INVALID_ARGUMENT;

    delete ctx;
    return LOTTIE_SPLASH_SUCCESS;
}

LOTTIE_SPLASH_API lottie_splash_error lottie_splash_close_window(const lottie_splash_context * ctx) {
    if(!ctx->window_message_loop_thread_id.has_value())
        return LOTTIE_SPLASH_ERROR_INVALID_ARGUMENT;

    ctx->window->request_close();

    return ctx->window->wait_until_closed() ? LOTTIE_SPLASH_SUCCESS : LOTTIE_SPLASH_ERROR_WINDOW_CLOSE_FAILED;
}

LOTTIE_SPLASH_API lottie_splash_error lottie_splash_set_status_message(lottie_splash_context * ctx,
                                                                       const char8_t *         utf8_string) {
    if(!ctx || !ctx->window)
        return LOTTIE_SPLASH_ERROR_INVALID_ARGUMENT;

    if(!ctx->window->is_initialized())
        return LOTTIE_SPLASH_WINDOW_CLOSED_BY_USER;

    if(!utf8_string)
        return LOTTIE_SPLASH_ERROR_INVALID_ARGUMENT;

    ctx->window->set_status_message(utf8_string);
    return LOTTIE_SPLASH_SUCCESS;
}

LOTTIE_SPLASH_API lottie_splash_error lottie_splash_set_progress(lottie_splash_context * ctx,
                                                                 float                   normalized_progress_value) {
    if(normalized_progress_value < 0.0f || normalized_progress_value > 1.0f)
        return LOTTIE_SPLASH_ERROR_INVALID_ARGUMENT;

    if(!ctx || !ctx->window)
        return LOTTIE_SPLASH_ERROR_INVALID_ARGUMENT;

    if(!ctx->window->is_initialized())
        return LOTTIE_SPLASH_WINDOW_CLOSED_BY_USER;

    ctx->window->set_progress(normalized_progress_value);
    return LOTTIE_SPLASH_SUCCESS;
}
}