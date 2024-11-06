kind "StaticLib"
runtime "Release"

if _OPTIONS["with-opengl"] then
  links {
    "OpenGL32.lib"
  }
end

warnings "Off"

cppdialect "C++14"

externalincludedirs {
  "src/deps/thorvg/src/loaders/lottie/jerryscript/jerry-core/ecma/base",
  "src/deps/thorvg/src/loaders/lottie/jerryscript/jerry-core/include",
  "src/deps/thorvg/src/loaders/lottie/jerryscript/jerry-core/jmem",
  "src/deps/thorvg/src/loaders/lottie/jerryscript/jerry-core/jrt",
  "src/deps/thorvg/src/loaders/lottie/jerryscript/jerry-core/lit",
  "src/deps/config",
  "src/deps/thorvg/src/common",
  "src/deps/thorvg/src/loaders/lottie/jerryscript/jerry-core/ecma/operations",
  "src/deps/thorvg/src/loaders/lottie/jerryscript/jerry-core/parser/js",
  "src/deps/thorvg/src/loaders/lottie/jerryscript/jerry-core/jcontext",
  "src/deps/thorvg/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects",
  "src/deps/thorvg/src/loaders/lottie/jerryscript/jerry-core/parser/regexp",
  "src/deps/thorvg/inc",
  "src/deps/thorvg/src/renderer",
  "src/deps/thorvg/src/loaders/lottie/jerryscript/jerry-core/vm/",
  "src/deps/thorvg/src/loaders/lottie",
  "src/deps/thorvg/src/loaders/raw",
  "src/deps/thorvg/src/loaders/ttf",
  "src/deps/glad/include"
}

removefiles {
  "src/deps/thorvg/examples/**",
  "src/deps/thorvg/test/**",
  "src/deps/thorvg/tools/**",
  "src/deps/thorvg/src/bindings/**",
  "src/deps/thorvg/src/loaders/**",
  "src/deps/thorvg/src/savers/**",
  "src/deps/thorvg/src/renderer/wg_engine/**"
}

files {
  "src/deps/thorvg/src/loaders/lottie/**",
  "src/deps/thorvg/src/loaders/raw/**",
  "src/deps/thorvg/src/loaders/ttf/**",
}

if _OPTIONS["with-opengl"] then
  externalincludedirs {"src/deps/thorvg/src/renderer/gl_engine"}
  removefiles {"src/deps/thorvg/src/renderer/sw_engine/**"}
  files {"src/deps/glad/src/glad.c"}
else
  externalincludedirs {"src/deps/thorvg/src/renderer/sw_engine"}
  removefiles {"src/deps/thorvg/src/renderer/gl_engine/**"}
end
