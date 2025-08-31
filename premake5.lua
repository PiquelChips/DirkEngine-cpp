workspace "DirkEngine"
    configurations { "Editor", "Debug", "Shipping" }
    startproject "Editor"
    
    workingdirectory = os.getenv("PWD")
    outputdir = workingdirectory .. "/build/%{prj.name}-%{cfg.buildcfg}-%{cfg.system}"

    VulkanSDK = os.getenv("VULKAN_SDK")

    IncludeDir = {}
    IncludeDir["glm"] = workingdirectory .. "/Thirdparty/glm"
    IncludeDir["tinygltf"] = workingdirectory .. "/Thirdparty/tinygltf/include"
    IncludeDir["vulkan"] = VulkanSDK .. "/include"

    LibraryDir = {}
    LibraryDir["vulkan"] = VulkanSDK .. "/lib"

    Library = {}
    Library["vulkan"] = "vulkan"

    -- TODO: find better solution
    defines { "LOG_PATH", "RESOURCE_PATH" }

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
