project "Runtime"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"

    targetdir (outputdir .. "/bin")
    objdir (outputdir .. "/obj")

    files { "src/**.cpp" }

    includedirs {
        "include",
        "%{IncludeDir.glm}",
        "%{IncludeDir.tinygltf}",
        "%{IncludeDir.glfw}",
        "%{IncludeDir.vulkan}"
    }

    libdirs {
        "%{LibraryDir.vulkan}",
        "%{LibraryDir.glfw}"
    }

    links {
        "%{Library.vulkan}",
        "%{Library.glfw}"
    }
