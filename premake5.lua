ps = require "deps.premake_scaffold"
workspace "lottie_splash"

newoption {
  trigger = "with-opengl",
  description = "Use thorvg openGL backend instead of software"
}

defines {
  "NOMINMAX",
  "WIN32_LEAN_AND_MEAN",
  "TVG_STATIC",
  "THORVG_TTF_LOADER_SUPPORT",
  "LOTTIE_SPLASH_EXPORTS"
}

if _OPTIONS["with-opengl"] then
  defines {
    "THORVG_GL_RASTER_SUPPORT=1",
    "__gl_h_" -- prevent system gl.h header from inclusion. we use glad loader instead.
  }
else
  defines {
    "THORVG_SW_RASTER_SUPPORT=1"
  }
end

startproject "demo"

filter "configurations:Release"
    optimize "Size"
    omitframepointer "On"
    flags {
      "NoBufferSecurityCheck",
      "NoIncrementalLink",
      "NoRuntimeChecks",
      "NoImplicitLink"
    }
    exceptionhandling "Off"
filter {}

staticruntime "on"

if _OPTIONS["with-opengl"] then
  premake.error(
    "ThorVG's OpenGL rendering backend is currently buggy, see e.g. cat_loader.json. We also do not support window transparency for it yet."
  )
end

ps.generate({})
