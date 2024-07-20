// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "pch.h"

#include "core/builder.h"
#include "core/project.h"

#include "editor/file_dialog.h"
#include "editor/system_calls.h"

namespace era_engine::build
{
	std::optional<std::string> ebuilder::selectBuildFolder()
	{
		const std::string result = directoryDialog();
		if (!result.empty())
		{
			eproject::exePath = result;

			return result;
		}
		else
		{
			return {};
		}
	}

	bool ebuilder::build(bool autoRun, bool tempFolder)
	{
		std::string destinationFolder;

		if (tempFolder)
		{
			destinationFolder = std::string(fs::current_path().string()) + std::string("Editor\\TempBuild\\");
			try
			{
				fs::remove_all(destinationFolder);
			}
			catch (fs::filesystem_error error)
			{
				LOG_ERROR("Building> Temporary build failed!");
				return false;
			}
		}
		else if (auto res = selectBuildFolder(); res.has_value())
			destinationFolder = res.value();
		else
			return false;

		std::string config = eproject::config == configuration_type_debug ? "Debug" : "Release";

		bool result = buildAtLocation(config, destinationFolder, autoRun);
		if (!result)
			fs::remove_all(destinationFolder);
		return result;
	}

	bool ebuilder::buildAtLocation(std::string_view configuration, std::string_view pbuildPath, bool autoRun)
	{
		std::string buildPath = pbuildPath.data();
		std::string executableName = eproject::exePath + ".exe";

		bool failed = false;

		LOG_MESSAGE("Preparing to build at location: \"", buildPath, "\"");

		fs::remove_all(buildPath);

		if (!fs::create_directory(buildPath))
			return false;

		LOG_MESSAGE("Build directory created");

		if (!fs::create_directory(buildPath + "\\Data\\"))
			return false;

		LOG_MESSAGE("Data directory created");

		if (!fs::create_directory(buildPath + "\\Data\\User\\"))
			return false;

		LOG_MESSAGE("Data\\User directory created");

		std::error_code err;

		fs::copy(eproject::enginePath + "\\runtime\\x64\\Release", buildPath + "\\Data\\User", fs::copy_options::recursive, err);
		eproject::exePath = buildPath + "\\Data\\User\\";

		if (err)
			return false;

		LOG_MESSAGE("Building .exe finished.");

		fs::copy(eproject::path + "\\Game.eproj", buildPath + "\\Data\\User\\Game.eproj", err);

		if (err)
			return false;

		fs::remove_all(buildPath + "\\Data\\User\\assets");
		fs::remove_all(buildPath + "\\Data\\User\\asset_cache");
		fs::remove_all(buildPath + "\\Data\\User\\resources");
		fs::remove_all(buildPath + "\\Data\\User\\shaders");
		
		fs::copy(eproject::path + "\\assets", buildPath + "\\Data\\User\\assets\\", fs::copy_options::recursive, err);

		if (!fs::exists(buildPath + "\\Data\\User\\assets\\"))
		{
			LOG_ERROR("Building> Failed to find Start Scene at expected path. Verify your Project Setings.");
			return false;
		}

		if (err)
			return false;

		LOG_MESSAGE("Data\\User\\assets\\ directory copied");

		fs::copy(eproject::path + "\\asset_cache", buildPath + "\\Data\\User\\asset_cache", fs::copy_options::recursive, err);

		if (err)
			return false;

		LOG_MESSAGE("Data\\User\\asset_cache\\ directory copied");

		fs::copy(eproject::path + "\\resources", buildPath + "\\Data\\User\\resources", fs::copy_options::recursive, err);

		if (err)
			return false;

		LOG_MESSAGE("Data\\User\\resources\\ directory copied");

		fs::copy(eproject::path + "\\shaders", buildPath + "\\Data\\User\\shaders", fs::copy_options::recursive, err);

		if (err)
			return false;

		LOG_MESSAGE("Data\\User\\shaders\\ directory copied");

		LOG_MESSAGE("Builder> builded successfuly.");

		if (autoRun)
		{
			fs::path exe_path = eproject::exePath + "EraRuntime.exe";
			os::system_calls::openFile(exe_path);
		}

		return !failed;
	}

}