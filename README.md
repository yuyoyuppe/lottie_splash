# Lottie Splash

A lightweight library for displaying animated splash screens using Lottie animations on Windows. Built with [ThorVG](https://github.com/thorvg/thorvg) for high-performance vector graphics rendering.

## Features

- Lottie animation playback
- Progress bar support
- Status message updates
- Modern Windows UI with transparency and rounded corners
- Thread-safe API
- Idiomatic Rust bindings

## TODO
- Allow configuring progress bar, logo and text styles.

## Usage (Rust)

```rust
use lottie_splash_rs::{LottieSplash, Error};
use std::thread;
use std::time::Duration;

fn main() -> Result<(), Error> {
    // Load your Lottie animation JSON
    let animation_data = std::fs::read("splash_animation.json")?;

    // Create the splash window
    let splash = LottieSplash::new(&animation_data, "Installing...", 0, 0)?;

    // Run updates in a separate thread
    thread::scope(|scope| {
        let handle = scope.spawn(|| {
            for i in 0..=10 {
                splash.set_progress(i as f32 / 10.0)?;
                splash.set_status_message(&format!("Step {} of 10", i))?;
                thread::sleep(Duration::from_millis(500));
            }

            splash.close_window()
        });

        // Blocks until closed
        splash.run_window()?;

        // Wait for the update thread
        handle.join().unwrap()
    })?;

    Ok(())
}
```

## Building

### Prerequisites

- Windows 10+
- Visual Studio 2022+
- Rust 1.70.0+

### Steps

1. Clone the repository
2. Run `premake5 vs2022 --arch=x64` and build Release
3. Make sure to run Rust tests with a single thread to avoid crashes: `cargo test -- --test-threads=1`

## License

MIT

