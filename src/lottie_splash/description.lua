kind "StaticLib"
links {"deps", "dwmapi.lib"}
dependson {"deps"}
runtime "Release"

externalincludedirs {
  "src/deps/thorvg/inc",
  "src/deps/config/",
  "src/deps/glad/include",
}

linkoptions {
  "/IGNORE:4006" -- Ignores warnings such as: dwmapi.lib: __NULL_IMPORT_DESCRIPTOR already defined in deps.lib(OPENGL32.dll); second definition ignored
}

filter "configurations:Debug"
  links {"bin\\depsd"}
  targetsuffix "d_%{cfg.architecture}"
filter "configurations:Release"
  targetsuffix "_%{cfg.architecture}"
filter {}