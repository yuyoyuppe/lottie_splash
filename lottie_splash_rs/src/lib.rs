#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use num_derive::{FromPrimitive, ToPrimitive};
use num_traits::FromPrimitive;
use std::ffi::{c_char, c_float, c_void, CString};
use std::ptr::NonNull;
use thiserror::Error;

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, FromPrimitive, ToPrimitive)]
pub enum lottie_splash_error {
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
}

#[derive(Debug, Copy, Clone, FromPrimitive, ToPrimitive)]
#[repr(i32)]
enum FfiError {
    Success = 0,
    WindowClosedByUser,
    InvalidArgument,
    ThorVGInitFailed,
    WindowCreationFailed,
    WindowCloseFailed,
    OpenGLInitFailed,
    AnimationLoadFailed,
    WindowAlreadyRunning,
    FontLoadFailed,
    DisplayInitFailed,
    RenderFailed,
}

#[derive(Error, Debug)]
pub enum Error {
    #[error("Window was closed by user")]
    WindowClosedByUser,
    #[error("Invalid argument provided")]
    InvalidArgument,
    #[error("ThorVG initialization failed")]
    ThorVGInitFailed,
    #[error("Window creation failed")]
    WindowCreationFailed,
    #[error("Window close failed")]
    WindowCloseFailed,
    #[error("OpenGL initialization failed")]
    OpenGLInitFailed,
    #[error("Animation load failed")]
    AnimationLoadFailed,
    #[error("Window already running")]
    WindowAlreadyRunning,
    #[error("Font load failed")]
    FontLoadFailed,
    #[error("Display initialization failed")]
    DisplayInitFailed,
    #[error("Render failed")]
    RenderFailed,
    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),
    #[error("UTF-8 conversion error: {0}")]
    Utf8(#[from] std::string::FromUtf8Error),
    #[error("UTF-16 conversion error")]
    Utf16(#[from] std::string::FromUtf16Error),
    #[error("String contains null byte")]
    NulError(#[from] std::ffi::NulError),
}

#[repr(C)]
pub struct lottie_splash_context(c_void);

extern "C" {
    fn lottie_splash_create(
        lottie_animation_buf: *const c_char,
        buf_size: usize,
        utf8_window_title: *const c_char,
        window_width: u32,
        window_height: u32,
        out_error: *mut lottie_splash_error,
    ) -> *mut lottie_splash_context;

    fn lottie_splash_destroy(ctx: *mut lottie_splash_context) -> lottie_splash_error;

    fn lottie_splash_run_window(ctx: *mut lottie_splash_context) -> lottie_splash_error;

    fn lottie_splash_close_window(ctx: *const lottie_splash_context) -> lottie_splash_error;

    fn lottie_splash_set_status_message(
        ctx: *mut lottie_splash_context,
        utf8_string: *const c_char,
    ) -> lottie_splash_error;

    fn lottie_splash_set_progress(
        ctx: *mut lottie_splash_context,
        normalized_progress_value: c_float,
    ) -> lottie_splash_error;
}

impl From<lottie_splash_error> for Result<(), Error> {
    fn from(error: lottie_splash_error) -> Self {
        match FfiError::from_i32(error as i32).unwrap_or(FfiError::InvalidArgument) {
            FfiError::Success => Ok(()),
            FfiError::WindowClosedByUser => Err(Error::WindowClosedByUser),
            FfiError::InvalidArgument => Err(Error::InvalidArgument),
            FfiError::ThorVGInitFailed => Err(Error::ThorVGInitFailed),
            FfiError::WindowCreationFailed => Err(Error::WindowCreationFailed),
            FfiError::WindowCloseFailed => Err(Error::WindowCloseFailed),
            FfiError::OpenGLInitFailed => Err(Error::OpenGLInitFailed),
            FfiError::AnimationLoadFailed => Err(Error::AnimationLoadFailed),
            FfiError::WindowAlreadyRunning => Err(Error::WindowAlreadyRunning),
            FfiError::FontLoadFailed => Err(Error::FontLoadFailed),
            FfiError::DisplayInitFailed => Err(Error::DisplayInitFailed),
            FfiError::RenderFailed => Err(Error::RenderFailed),
        }
    }
}

impl From<lottie_splash_error> for Error {
    fn from(error: lottie_splash_error) -> Self {
        match FfiError::from_i32(error as i32).unwrap_or(FfiError::InvalidArgument) {
            FfiError::Success => Error::InvalidArgument, // Success shouldn't convert to an error
            FfiError::WindowClosedByUser => Error::WindowClosedByUser,
            FfiError::InvalidArgument => Error::InvalidArgument,
            FfiError::ThorVGInitFailed => Error::ThorVGInitFailed,
            FfiError::WindowCreationFailed => Error::WindowCreationFailed,
            FfiError::WindowCloseFailed => Error::WindowCloseFailed,
            FfiError::OpenGLInitFailed => Error::OpenGLInitFailed,
            FfiError::AnimationLoadFailed => Error::AnimationLoadFailed,
            FfiError::WindowAlreadyRunning => Error::WindowAlreadyRunning,
            FfiError::FontLoadFailed => Error::FontLoadFailed,
            FfiError::DisplayInitFailed => Error::DisplayInitFailed,
            FfiError::RenderFailed => Error::RenderFailed,
        }
    }
}

pub struct LottieSplash {
    ctx: NonNull<lottie_splash_context>,
}

// SAFETY: The underlying C++ implementation is thread-safe
unsafe impl Send for LottieSplash {}
// SAFETY: All methods take &self and the C++ side handles synchronization
unsafe impl Sync for LottieSplash {}

impl LottieSplash {
    pub fn new(
        animation_data: &[u8],
        window_title: &str,
        windows_width: u32,
        windows_height: u32,
    ) -> Result<Self, Error> {
        let mut error = lottie_splash_error::LOTTIE_SPLASH_SUCCESS;

        let window_title = CString::new(window_title)?;

        // SAFETY: We ensure the pointers are valid and the data outlives the call
        let ctx = unsafe {
            lottie_splash_create(
                animation_data.as_ptr() as *const c_char,
                animation_data.len(),
                window_title.as_ptr(),
                windows_width,
                windows_height,
                &mut error as *mut _,
            )
        };

        match NonNull::new(ctx) {
            Some(ctx) => Ok(Self { ctx }),
            None => Err(Error::from(error)),
        }
    }

    pub fn run_window(&self) -> Result<(), Error> {
        // SAFETY: ctx is guaranteed to be non-null by NonNull
        unsafe { lottie_splash_run_window(self.ctx.as_ptr()).into() }
    }

    pub fn set_status_message(&self, message: &str) -> Result<(), Error> {
        let message = CString::new(message)?;

        // SAFETY: ctx is guaranteed to be non-null by NonNull, and message is valid null-terminated UTF-8
        unsafe { lottie_splash_set_status_message(self.ctx.as_ptr(), message.as_ptr()).into() }
    }

    pub fn set_progress(&self, progress: f32) -> Result<(), Error> {
        // SAFETY: ctx is guaranteed to be non-null by NonNull
        unsafe { lottie_splash_set_progress(self.ctx.as_ptr(), progress).into() }
    }

    pub fn close_window(&self) -> Result<(), Error> {
        // SAFETY: ctx is guaranteed to be non-null by NonNull
        unsafe { lottie_splash_close_window(self.ctx.as_ptr()).into() }
    }
}

impl Drop for LottieSplash {
    fn drop(&mut self) {
        // SAFETY: ctx is guaranteed to be non-null by NonNull
        unsafe {
            lottie_splash_destroy(self.ctx.as_ptr());
        }
    }
}
