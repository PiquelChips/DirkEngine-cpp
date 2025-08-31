project "Editor"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"

    targetdir (outputdir .. "/bin")
    objdir (outputdir .. "/obj")

    files { "src/**.cpp" }

    includedirs { "include", }

    libdirs {
        "%{LibraryDir.vulkan}",
        "%{LibraryDir.glfw}"
    }

    links {
        "Runtime",
        "%{Library.vulkan}",
        "%{Library.glfw}"
    }
