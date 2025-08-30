workspace "DirkEngine"
    configurations { "Editor", "Debug", "Shipping" }
    startproject "Editor"
    
    workingdirectory = os.getenv("PWD")
    outputdir = workingdirectory .. "/build/%{prj.name}-%{cfg.buildcfg}-%{cfg.system}"

    VulkanSDK = os.getenv("VULKAN_SDK")

    IncludeDir = {}
    IncludeDir["glm"] = "Thirdparty/glm"
    IncludeDir["tinygltf"] = "Thirdparty/tinygltf/include"
    IncludeDir["vulkan"] = VulkanSDK .. "/include"

    Library = {}
    Library["vulkan"] = VulkanSDK .. "/lib/libvulkan.so"

    filter "configurations:Editor"
      defines { "WITH_EDITOR" }
      runtime "Debug"
      symbols "On"

   filter "configurations:Debug"
      defines { "DIRK_DEBUG_BUILD" }
      runtime "Debug"
      symbols "On"

   filter "configurations:Shipping"
      defines { "DIRK_SHIPPING_BUILD" }
      runtime "Release"
      optimize "On"
      symbols "Off"

    filter {}
    
include "Source"
