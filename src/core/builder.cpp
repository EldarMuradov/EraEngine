#include <pch.h>
#include "core/builder.h"
#include <editor/file_dialog.h>
#include "log.h"
#include <core/project.h>

std::optional<std::string> ebuilder::selectBuildFolder()
{
	const std::string result = openFileDialog("Build location", "..");
	if (!result.empty())
	{
		eproject::exe_path = result;

		if (!std::filesystem::exists(result))
			return result;
		else
		{
			LOG_ERROR("Building> Folder already exists!");
			return {};
		}
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

	if(eproject::config == configuration::Debug)
		return buildAtLocation("Debug", destinationFolder, autoRun);
	else
		return buildAtLocation("Release", destinationFolder, autoRun);
}

bool ebuilder::buildAtLocation(const std::string& configuration, const std::string pbuildPath, bool autoRun)
{
	std::string buildPath(pbuildPath);
	std::string executableName = eproject::exe_path + ".exe";

	bool failed = false;

	LOG_MESSAGE("Preparing to build at location: \"", buildPath, "\"");

	std::filesystem::remove_all(buildPath);

	if (std::filesystem::create_directory(buildPath))
	{
		LOG_MESSAGE("Build directory created");

		if (std::filesystem::create_directory(buildPath + "Data\\"))
		{
			LOG_MESSAGE("Data directory created");
		}
	}

	return !failed;
}
