#[cfg(test)]
mod tests {
    use lottie_splash_rs::*;
    use std::thread::{self, ScopedJoinHandle};
    use std::time::Duration;

    fn get_test_animation() -> Vec<u8> {
        let bytes =
            include_bytes!("../../src/deps/thorvg/examples/resources/lottie/cat_loader.json");
        bytes.to_vec()
    }

    #[test]
    fn test_basic_creation() -> Result<(), Error> {
        let animation_data = get_test_animation();
        let _splash = LottieSplash::new(&animation_data, "Test Window")?;
        Ok(())
    }

    #[test]
    fn test_invalid_animation() {
        let invalid_data = b"not a json";
        let result = LottieSplash::new(invalid_data, "Test Window");
        assert!(matches!(result, Err(Error::AnimationLoadFailed)));
    }

    #[test]
    fn test_empty_title() {
        let animation_data = get_test_animation();
        let result = LottieSplash::new(&animation_data, "");
        assert!(result.is_ok());
    }

    #[test]
    fn test_progress_bounds() -> Result<(), Error> {
        let animation_data = get_test_animation();
        let splash = LottieSplash::new(&animation_data, "Test Window")?;

        // Valid bounds
        assert!(splash.set_progress(0.0).is_ok());
        assert!(splash.set_progress(0.5).is_ok());
        assert!(splash.set_progress(1.0).is_ok());

        // Invalid bounds
        assert!(matches!(
            splash.set_progress(-0.1),
            Err(Error::InvalidArgument)
        ));
        assert!(matches!(
            splash.set_progress(1.1),
            Err(Error::InvalidArgument)
        ));

        Ok(())
    }

    #[test]
    fn test_status_message() -> Result<(), Error> {
        let animation_data = get_test_animation();
        let splash = LottieSplash::new(&animation_data, "Test Window")?;

        assert!(splash.set_status_message("Testing...").is_ok());
        assert!(splash.set_status_message("").is_ok()); // Empty message should be valid

        assert!(splash.set_status_message("Hello 世界!").is_ok());

        Ok(())
    }

    #[test]
    fn test_window_lifecycle() -> Result<(), Error> {
        let animation_data = get_test_animation();
        let splash = LottieSplash::new(&animation_data, "Test Window")?;

        thread::scope(|scope| {
            let handle: ScopedJoinHandle<Result<(), Error>> = scope.spawn(|| {
                for i in 0..=5 {
                    splash.set_progress(i as f32 / 5.0)?;
                    thread::sleep(Duration::from_millis(200));
                }
                splash.close_window()
            });

            // Run window in the main thread
            splash.run_window()?;

            // Wait for the update thread to finish
            handle.join().unwrap()
        })?;

        Ok(())
    }

    #[test]
    fn test_status_message_updates() {
        let splash = LottieSplash::new(&get_test_animation(), "Status Message Test").unwrap();

        thread::scope(|scope| {
            let handle = scope.spawn(|| {
                // Array of messages to display
                let messages = [
                    "Initializing...",
                    "Loading assets...",
                    "Preparing resources...",
                    "Almost there...",
                    "Finishing up...",
                    "Complete!",
                ];

                for (i, &msg) in messages.iter().enumerate() {
                    splash.set_status_message(msg)?;
                    splash.set_progress(i as f32 / (messages.len() - 1) as f32)?;
                    thread::sleep(Duration::from_millis(300));
                }

                // Keep the final message visible for a moment
                thread::sleep(Duration::from_millis(800));
                splash.close_window()
            });

            // Run window in the main thread
            splash.run_window()?;

            // Wait for the update thread to finish
            handle.join().unwrap()
        })
        .unwrap();
    }

    #[test]
    fn test_multiple_windows() {
        let handle1 = thread::spawn(|| {
            let splash = LottieSplash::new(&get_test_animation(), "Window 1").unwrap();

            // Run window for a short time
            thread::sleep(Duration::from_millis(500));
            splash.close_window().unwrap();

            splash.run_window()
        });

        thread::sleep(Duration::from_millis(100));

        let handle2 = thread::spawn(|| {
            let splash = LottieSplash::new(&get_test_animation(), "Window 2").unwrap();

            // Run window for a short time
            thread::sleep(Duration::from_millis(500));
            splash.close_window().unwrap();

            splash.run_window()
        });

        handle1.join().unwrap().unwrap();
        handle2.join().unwrap().unwrap();
    }

    #[test]
    fn test_resource_cleanup() {
        use std::mem::drop;

        let animation_data = get_test_animation();

        // Create and immediately drop
        let splash = LottieSplash::new(&animation_data, "Test Window").unwrap();
        drop(splash);

        // Should be able to create a new one
        let result = LottieSplash::new(&animation_data, "Test Window");
        assert!(result.is_ok());
    }
}
