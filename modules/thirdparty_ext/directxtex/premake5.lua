project "DirectXTex"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "Off"
	
    flags {
		"MultiProcessorCompile"
	}

    openmp "On"

	targetdir ("../../bin/" .. outputdir)
	objdir ("../../bin_int/" .. outputdir .. "/%{prj.name}")

	defines {
        "WIN32_LEAN_AND_MEAN",
        "UNICODE",
        "_UNICODE",
        "_SCL_SECURE_NO_WARNINGS",
        "_CRT_SECURE_NO_WARNINGS",
    }

    files {
        "DirectXTex/BC.h",
        "DirectXTex/DDS.h",
        "DirectXTex/DirectXTexP.h",
        "DirectXTex/filters.h",
        "DirectXTex/scoped.h",
        "DirectXTex/BC.cpp",
        "DirectXTex/BC4BC5.cpp",
        "DirectXTex/BC6HBC7.cpp",
        "DirectXTex/DirectXTexCompress.cpp",
        "DirectXTex/DirectXTexConvert.cpp",
        "DirectXTex/DirectXTexDDS.cpp",
        "DirectXTex/DirectXTexHDR.cpp",
        "DirectXTex/DirectXTexImage.cpp",
        "DirectXTex/DirectXTexMipmaps.cpp",
        "DirectXTex/DirectXTexMisc.cpp",
        "DirectXTex/DirectXTexNormalMaps.cpp",
        "DirectXTex/DirectXTexPMAlpha.cpp",
        "DirectXTex/DirectXTexResize.cpp",
        "DirectXTex/DirectXTexTGA.cpp",
        "DirectXTex/DirectXTexUtil.cpp",
        "DirectXTex/DirectXTexFlipRotate.cpp",
        "DirectXTex/DirectXTexWIC.cpp",
        "DirectXTex/d3dx12.h",
        "DirectXTex/DirectXTexD3D12.cpp",
    }

    filter "system:windows"
        systemversion "latest"

    filter  "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter  "configurations:Release"
        runtime "Release"
        optimize "on"

