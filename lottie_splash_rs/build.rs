use anyhow::{bail, Context, Result};
use std::env;
use std::fs;
use std::path::Path;
use std::process::Command;

fn get_latest_git_tag() -> Result<String> {
    let output = Command::new("git")
        .args(["describe", "--tags", "--abbrev=0"])
        .output()
        .context("Failed to run git command")?;

    if !output.status.success() {
        bail!(
            "Git command failed: {}",
            String::from_utf8_lossy(&output.stderr)
        );
    }

    Ok(String::from_utf8(output.stdout)
        .context("Invalid UTF-8 in git tag")?
        .trim()
        .to_string())
}

fn get_library_name() -> &'static str {
    let arch = env::var("CARGO_CFG_TARGET_ARCH").expect("Failed to get target architecture");
    let is_debug = env::var("PROFILE")
        .expect("Failed to get build profile")
        .eq("debug");

    match (arch.as_str(), is_debug) {
        ("x86_64", true) => "lottie_splashd_x86_64.lib",
        ("x86_64", false) => "lottie_splash_x86_64.lib",
        ("aarch64", true) => "lottie_splashd_ARM64.lib",
        ("aarch64", false) => "lottie_splash_ARM64.lib",
        (arch, _) => panic!("Unsupported architecture: {}", arch),
    }
}

fn find_local_lib() -> Option<std::path::PathBuf> {
    if let Ok(path) = env::var("LOTTIE_SPLASH_LIB_PATH") {
        let path = Path::new(&path);
        if path.exists() {
            return Some(path.to_path_buf());
        }
        println!(
            "cargo:warning=LOTTIE_SPLASH_LIB_PATH set but file not found at: {}",
            path.display()
        );
    }

    let possible_paths = [format!("../build/bin/{}", get_library_name())];

    for path in possible_paths {
        let path = Path::new(&path);
        if path.exists() {
            return Some(path.to_path_buf());
        }
    }

    None
}

fn download_lib(url: &str, output_path: &Path) -> Result<()> {
    println!(
        "cargo:warning=Downloading {} from {}",
        get_library_name(),
        url
    );
    let response = reqwest::blocking::get(url)
        .with_context(|| format!("Failed to download library from {}", url))?;

    if !response.status().is_success() {
        bail!("Failed to download library: HTTP {}", response.status());
    }

    let content = response.bytes().context("Failed to read library content")?;

    fs::write(output_path, content)
        .with_context(|| format!("Failed to write library to {}", output_path.display()))?;

    Ok(())
}

fn main() -> Result<()> {
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-env-changed=LOTTIE_SPLASH_LIB_PATH");

    let out_dir = env::var("OUT_DIR").context("Failed to get OUT_DIR")?;
    let out_dir = Path::new(&out_dir);
    let lib_path = out_dir.join(get_library_name());

    if let Some(local_path) = find_local_lib() {
        println!(
            "cargo:warning=Using local library from: {}",
            local_path.display()
        );
        fs::copy(&local_path, &lib_path).with_context(|| {
            format!(
                "Failed to copy local library from {} to {}",
                local_path.display(),
                lib_path.display()
            )
        })?;
    } else {
        let tag = get_latest_git_tag()?;

        let url = format!(
            "https://github.com/yuyoyuppe/lottie_splash/releases/download/{}/{}",
            tag,
            get_library_name()
        );

        download_lib(&url, &lib_path)?;
    }

    println!("cargo:rustc-link-search=native={}", out_dir.display());
    println!(
        "cargo:rustc-link-lib=static={}",
        get_library_name()
            .strip_suffix(".lib")
            .expect("Library should have .lib suffix")
    );

    println!("cargo:rustc-link-lib=dylib=user32");
    println!("cargo:rustc-link-lib=dylib=gdi32");
    println!("cargo:rustc-link-lib=dylib=opengl32");
    println!("cargo:rustc-link-lib=dylib=dwmapi");
    println!("cargo:rustc-link-lib=dylib=Shcore");

    Ok(())
}
