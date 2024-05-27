workspace "MKWheel"
	architecture "x64"
	configurations
	{
		"Debug",
		"Release"
	}
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Core"
	location "%{prj.name}"
	kind "ConsoleApp"
	language "C++"
	targetname "%{prj.name}"
	targetdir ("bin/".. outputdir)
	objdir ("%{prj.name}/int/".. outputdir)
	cppdialect "C++17"
	staticruntime "Off"

	files
	{
		"**.h",
		"**.cpp"
	}
	
	includedirs
	{
		"%{prj.name}/include",
		"%{prj.name}/src"
	}
	
	libdirs "%{prj.name}/lib"
	links { "setupapi", "hidapi" }
	
	
	filter "system:windows"
		systemversion "latest"
		defines { "WIN32" }
		
	filter "system:linux"
		systemversion "latest"
		defines { "LINUX" }

	filter "configurations:Debug"
		defines { "_DEBUG", "_CONSOLE" }
		symbols "On"
		links "ViGEmClient-d"
		
	filter "configurations:Release"
		defines { "NDEBUG", "NCONSOLE" }
		optimize "On"
		links "ViGEmClient"
