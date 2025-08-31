project "Runtime"
    location "build"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"

    targetdir (outputdir .. "/bin")
    objdir (outputdir .. "/obj")

    files { "src/*.cpp" }

    includedirs {
        "include",
        "%{IncludeDir.glm}",
        "%{IncludeDir.tinygltf}",
        "%{IncludeDir.vulkan}"
    }

    links { "%{Library.vulkan}" }
    libdirs { "%{LibraryDir.vulkan}" }
