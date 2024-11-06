#pragma once

#ifdef LOTTIE_SPLASH_EXPORTS
#define LOTTIE_SPLASH_API __declspec(dllexport)
#else
#define LOTTIE_SPLASH_API __declspec(dllimport)
#endif

typedef struct lottie_splash_context lottie_splash_context;

typedef enum lottie_splash_error {
    LOTTIE_SPLASH_SUCCESS = 0,
    LOTTIE_SPLASH_WINDOW_CLOSED_BY_USER,
    LOTTIE_SPLASH_ERROR_INVALID_ARGUMENT,
    LOTTIE_SPLASH_ERROR_THORVG_INIT_FAILED,
    LOTTIE_SPLASH_ERROR_WINDOW_CREATION_FAILED,
    LOTTIE_SPLASH_ERROR_WINDOW_CLOSE_FAILED,
    LOTTIE_SPLASH_ERROR_OPENGL_INIT_FAILED,
    LOTTIE_SPLASH_ERROR_ANIMATION_LOAD_FAILED,
    LOTTIE_SPLASH_ERROR_WINDOW_ALREADY_RUNNING,
    LOTTIE_SPLASH_ERROR_FONT_LOAD_FAILED,
    LOTTIE_SPLASH_ERROR_DISPLAY_INIT_FAILED,
    LOTTIE_SPLASH_ERROR_RENDER_FAILED,
} lottie_splash_error;

#ifdef __cplusplus
extern "C" {
#endif


/// <summary>
/// Creates a new lottie splash context. Doesn't open a windows yet.
/// </summary>
/// <param name="lottie_animation_buf">Raw Lottie json data</param>
/// <param name="buf_size">Lottie json data size in bytes</param>
/// <param name="utf8_window_title">Zero-terminated window title encoded in UTF-8</param>
/// <param name="window_width">Window width in pixels. You can pass 0 to make it 1/2 of the main monitor size. </param>
/// <param name="window_height">Window height in pixels</param>
/// <param name="out_error">A pointer to error code to indiciate the result</param>
/// <returns></returns>
LOTTIE_SPLASH_API lottie_splash_context * lottie_splash_create(const char *          lottie_animation_buf,
                                                               size_t                buf_size,
                                                               const char8_t *       utf8_window_title,
                                                               const unsigned        window_width,
                                                               const unsigned        window_height,
                                                               lottie_splash_error * out_error);

/// <summary>
/// Destroys the lottie splash context and releases all resources. It's the caller's responsibility to close the window before calling this function.
/// </summary>
/// <param name="ctx">Lottie context object obtained from lottie_splash_create.</param>
/// <returns></returns>
LOTTIE_SPLASH_API lottie_splash_error lottie_splash_destroy(lottie_splash_context * ctx);

/// <summary>
/// Opens the window and starts the rendering loop. This function blocks until the window is closed. This function must be called on the same thread as the one that called lottie_splash_create.
/// </summary>
/// <param name="ctx">Lottie context object obtained from lottie_splash_create.</param>
/// <returns>LOTTIE_SPLASH_WINDOW_CLOSED_BY_USER is returned if the user closed the window by pressing ALT+F4</returns>
LOTTIE_SPLASH_API lottie_splash_error lottie_splash_run_window(lottie_splash_context * ctx);

/// <summary>
/// Closes the window and returns the control back to the caller once it's fully closed.
/// </summary>
/// <param name="ctx">Lottie context object obtained from lottie_splash_create.</param>
/// <returns></returns>
LOTTIE_SPLASH_API lottie_splash_error lottie_splash_close_window(const lottie_splash_context * ctx);

/// <summary>
/// Sets the status message.
/// </summary>
/// <param name="ctx">Lottie context object obtained from lottie_splash_create.</param>
/// <param name="utf8_string">Zero-terminated message string encoded in UTF-8.</param>
/// <returns></returns>
LOTTIE_SPLASH_API lottie_splash_error lottie_splash_set_status_message(lottie_splash_context * ctx,
                                                                       const char8_t *         utf8_string);

/// <summary>
/// Sets the progress value for the progress bar.
/// </summary>
/// <param name="ctx">Lottie context object obtained from lottie_splash_create.</param>
/// <param name="normalized_progress_value">The value should be between 0.0 and 1.0.</param>
/// <returns></returns>
LOTTIE_SPLASH_API lottie_splash_error lottie_splash_set_progress(lottie_splash_context * ctx,
                                                                 float                   normalized_progress_value);

#ifdef __cplusplus
}
#endif
