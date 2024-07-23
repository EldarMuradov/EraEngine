project "yaml-cpp"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "Off"
	
    flags {
		"MultiProcessorCompile"
	}

	targetdir ("../../bin/" .. outputdir)
	objdir ("../../bin_int/" .. outputdir .. "/%{prj.name}")

	
    files {
        "src/**",
    }

    sysincludedirs {
        
    }

    includedirs {
        "include",
    }
	

    filter "system:windows"
        systemversion "latest"

    filter  "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter  "configurations:Release"
        runtime "Release"
        optimize "on"

