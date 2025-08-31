workspace "DirkEngine"
    configurations { "Editor", "Debug", "Shipping" }
    startproject "Editor"
    
    workingdirectory = os.getenv("PWD")
    outputdir = workingdirectory .. "/build/%{prj.name}-%{cfg.buildcfg}-%{cfg.system}"

    VulkanSDK = os.getenv("VULKAN_SDK")
    GLFW = os.getenv("GLFW")

    IncludeDir = {}
    IncludeDir["glm"] = workingdirectory .. "/Engine/Thirdparty/glm"
    IncludeDir["tinygltf"] = workingdirectory .. "/Engine/Thirdparty"
    IncludeDir["vulkan"] = VulkanSDK .. "/include"
    IncludeDir["glfw"] = GLFW .. "/include"

    LibraryDir = {}
    LibraryDir["vulkan"] = VulkanSDK .. "/lib"
    LibraryDir["glfw"] = GLFW .. "/lib"

    Library = {}
    Library["vulkan"] = "vulkan"
    Library["glfw"] = "glfw"

    -- TODO: find better solution
    defines {
        "GLFW_INCLUDE_NONE=1",
        "GLFW_INCLUDE_VULKAN=1",

        "LOG_PATH=\"./\"",
        "RESOURCE_PATH=\"./\"",
        "SHADER_PATH=\"./\""
    }

    filter "configurations:Editor"
      defines { "WITH_EDITOR" }
      runtime "Debug"
      symbols "On"

   filter "configurations:Debug"
      defines { "DEBUG_BUILD" }
      runtime "Debug"
      symbols "On"

   filter "configurations:Shipping"
      defines { "SHIPPING_BUILD" }
      runtime "Release"
      optimize "On"
      symbols "Off"

    filter {}
    
include "Engine"
