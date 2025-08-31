project "Runtime"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"

    targetdir (outputdir .. "/bin")
    objdir (outputdir .. "/obj")

    files {
        "src/**.cpp",
        "shaders/**.vert",
        "shaders/**.frag"
    }

    filter "files:**.vert or **.frag"
        buildcommands {
            'glslc -fshader-stage=%{file.extension:sub(2)} "%{file.relpath}" -o "%{cfg.targetdir}/../shaders/%{file.name}.spv"'
        }
        buildoutputs { "%{cfg.targetdir}/../shaders/%{file.name}.spv" }
    filter {}

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
