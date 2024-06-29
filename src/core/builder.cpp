// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include <pch.h>
#include "core/builder.h"
#include <editor/file_dialog.h>
#include <editor/system_calls.h>
#include <core/project.h>

namespace era_engine::build
{
	NODISCARD std::optional<std::string> ebuilder::selectBuildFolder()
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
			destinationFolder = std::string(std::filesystem::current_path().string()) + std::string("Editor\\TempBuild\\");
			try
			{
				std::filesystem::remove_all(destinationFolder);
			}
			catch (std::filesystem::filesystem_error error)
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
			std::filesystem::remove_all(destinationFolder);
		return result;
	}

	bool ebuilder::buildAtLocation(std::string_view configuration, std::string_view pbuildPath, bool autoRun)
	{
		std::string buildPath = pbuildPath.data();
		std::string executableName = eproject::exePath + ".exe";

		bool failed = false;

		LOG_MESSAGE("Preparing to build at location: \"", buildPath, "\"");

		std::filesystem::remove_all(buildPath);

		if (!std::filesystem::create_directory(buildPath))
			return false;

		LOG_MESSAGE("Build directory created");

		if (!std::filesystem::create_directory(buildPath + "\\Data\\"))
			return false;

		LOG_MESSAGE("Data directory created");

		if (!std::filesystem::create_directory(buildPath + "\\Data\\User\\"))
			return false;

		LOG_MESSAGE("Data\\User directory created");

		std::error_code err;

		std::filesystem::copy(eproject::enginePath + "\\runtime\\x64\\Release", buildPath + "\\Data\\User", std::filesystem::copy_options::recursive, err);
		eproject::exePath = buildPath + "\\Data\\User\\";

		if (err)
			return false;

		LOG_MESSAGE("Building .exe finished.");

		std::filesystem::copy(eproject::path + "\\Game.eproj", buildPath + "\\Data\\User\\Game.eproj", err);

		if (err)
			return false;

		std::filesystem::remove_all(buildPath + "\\Data\\User\\assets");
		std::filesystem::remove_all(buildPath + "\\Data\\User\\asset_cache");
		std::filesystem::remove_all(buildPath + "\\Data\\User\\resources");
		std::filesystem::remove_all(buildPath + "\\Data\\User\\shaders");

		std::filesystem::copy(eproject::path + "\\assets", buildPath + "\\Data\\User\\assets\\", std::filesystem::copy_options::recursive, err);

		if (!std::filesystem::exists(buildPath + "\\Data\\User\\assets\\"))
		{
			LOG_ERROR("Building> Failed to find Start Scene at expected path. Verify your Project Setings.");
			return false;
		}

		if (err)
			return false;

		LOG_MESSAGE("Data\\User\\assets\\ directory copied");

		std::filesystem::copy(eproject::path + "\\asset_cache", buildPath + "\\Data\\User\\asset_cache", std::filesystem::copy_options::recursive, err);

		if (err)
			return false;

		LOG_MESSAGE("Data\\User\\asset_cache\\ directory copied");

		std::filesystem::copy(eproject::path + "\\resources", buildPath + "\\Data\\User\\resources", std::filesystem::copy_options::recursive, err);

		if (err)
			return false;

		LOG_MESSAGE("Data\\User\\resources\\ directory copied");

		std::filesystem::copy(eproject::path + "\\shaders", buildPath + "\\Data\\User\\shaders", std::filesystem::copy_options::recursive, err);

		if (err)
			return false;

		LOG_MESSAGE("Data\\User\\shaders\\ directory copied");

		LOG_MESSAGE("Builder> builded successfuly.");

		if (autoRun)
		{
			std::filesystem::path exe_path = eproject::exePath + "EraRuntime.exe";
			os::system_calls::openFile(exe_path);
		}

		return !failed;
	}

}