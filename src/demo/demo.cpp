#include <lottie_splash.h>

#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdio>
#include <array>

#include <Windows.h>
#include <shellapi.h>

namespace {
bool read_file(const wchar_t * path, std::vector<char> & out_data) {
    std::FILE * file = _wfopen(path, L"rb");
    if(!file)
        return false;

    std::fseek(file, 0, SEEK_END);
    const long file_size = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);

    if(file_size <= 0) {
        std::fclose(file);
        return false;
    }

    out_data.resize(file_size);
    size_t read_size = std::fread(out_data.data(), 1, file_size, file);
    std::fclose(file);

    return read_size == static_cast<size_t>(file_size);
}

void print_error(const char * context, lottie_splash_error error) {
    const char * error_str = "Unknown error";
    switch(error) {
    case LOTTIE_SPLASH_SUCCESS:
        error_str = "Success";
        break;
    case LOTTIE_SPLASH_ERROR_INVALID_ARGUMENT:
        error_str = "Invalid argument";
        break;
    case LOTTIE_SPLASH_ERROR_THORVG_INIT_FAILED:
        error_str = "ThorVG init failed";
        break;
    case LOTTIE_SPLASH_ERROR_WINDOW_CREATION_FAILED:
        error_str = "Window creation failed";
        break;
    case LOTTIE_SPLASH_ERROR_OPENGL_INIT_FAILED:
        error_str = "OpenGL init failed";
        break;
    case LOTTIE_SPLASH_ERROR_ANIMATION_LOAD_FAILED:
        error_str = "Animation load failed";
        break;
    case LOTTIE_SPLASH_ERROR_WINDOW_ALREADY_RUNNING:
        error_str = "Window already running";
        break;
    }
    std::printf("%s: %s\n", context, error_str);
}

void run_demo_installation(lottie_splash_context * ctx) {
    std::array steps = {
      u8"Checking system requirements…",
      u8"Downloading dependencies…",
      u8"Installing core components…",
      u8"Configuring settings…",
      u8"Running post-install tasks…",
    };

    for(size_t i = 0; i < steps.size(); ++i) {
        lottie_splash_set_status_message(ctx, steps[i]);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        if(lottie_splash_set_progress(ctx, (1.f + i) / static_cast<float>(steps.size())) != LOTTIE_SPLASH_SUCCESS)
            return;
    }

    lottie_splash_set_status_message(ctx, u8"Installation complete!");
    lottie_splash_set_progress(ctx, 1.0f);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    lottie_splash_close_window(ctx);
}
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    int      argc;
    LPWSTR * argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if(argv == nullptr) {
        MessageBoxW(nullptr, L"Failed to parse command line arguments", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    if(argc != 2) {
        MessageBoxW(nullptr, L"Please provide a Lottie JSON file path as argument", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::vector<char> animation_data;
    if(!read_file(argv[1], animation_data)) {
        MessageBoxW(nullptr, L"Failed to read animation file", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    lottie_splash_error error = {};

    auto * ctx =
      lottie_splash_create(animation_data.data(), animation_data.size(), u8"Demo installation", 325, 328, &error);
    if(!ctx) {
        print_error("Failed to create splash window", error);
        return 1;
    }

    {
        std::jthread worker_thread(run_demo_installation, ctx);

        error = lottie_splash_run_window(ctx);
        if(error != LOTTIE_SPLASH_SUCCESS) {
            print_error("Window run failed", error);
            return 1;
        }
    }

    error = lottie_splash_destroy(ctx);
    if(error != LOTTIE_SPLASH_SUCCESS) {
        print_error("Cleanup failed", error);
        return 1;
    }

    return 0;
}