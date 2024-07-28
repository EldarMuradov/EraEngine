#include "launcher/build/project_builder.h"
#include "launcher/core/hub_project.h"

#include <cderr.h>

namespace era_engine::launcher
{
	std::optional<std::string> builder::selectBuildFolder()
	{
		save_file_dialog dialog("Project location");
		dialog.defineExtension("Project", "..");
		dialog.show();
		if (dialog.hasSucceeded())
		{
			std::string result = dialog.getSelectedFilePath();
			result = std::string(result.data(), result.data() + result.size() - std::string("..").size()) + "\\";

			if (!std::filesystem::exists(result))
				return result;
			else
			{
				//LOG_ERR("[Building] Folder already exists!");
				return {};
			}
		}
		else
		{
			return {};
		}
	}

	void builder::build(hub_project& project, bool autoRun, bool tempFolder)
	{
		std::string destinationFolder;

		if (tempFolder)
		{
			destinationFolder = std::string(std::filesystem::current_path().string()) + "\\Data\\Temp\\ProjectTemplate\\";
			try
			{
				std::filesystem::remove_all(destinationFolder);
			}
			catch (std::filesystem::filesystem_error error)
			{
				//LOG_ERR("[Building] Temporary build failed!");
				return;
			}
		}
		else if (auto res = selectBuildFolder(); res.has_value())
			destinationFolder = res.value();
		else
			return;

		std::string buildPath(destinationFolder);

		bool failed = false;

		project.path = buildPath;

		//LOG_INFO("Preparing to build at location: \"" + buildPath + "\"");

		std::filesystem::remove_all(buildPath);
		std::error_code err;

		try
		{
			std::filesystem::copy("Data\\ProjectTemplate", buildPath, std::filesystem::copy_options::recursive, err);
		}
		catch (...)
		{
			//LOG_ERR("[Building] " + err.message());
		}

		//LOG_INFO("[Building] Builded successfuly!");
	}

	file_dialog::file_dialog(std::function<int(tagOFNA*)> callback_, const std::string& dialogTitle_)
		: callback(callback_),
		dialogTitle(dialogTitle_),
		initialDirectory("")
	{
	}

	void file_dialog::setInitialDirectory(const std::string& initialDirectory_)
	{
		initialDirectory = initialDirectory_;
	}

	void file_dialog::show(explorer_flags flags)
	{
		if (!initialDirectory.empty())
			filepath = initialDirectory;

		filepath.resize(MAX_PATH);

		OPENFILENAMEA ofn;
		ZeroMemory(&ofn, sizeof(ofn));

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL;
		ofn.lpstrFilter = filter.c_str();
		ofn.lpstrFile = filepath.data();
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrTitle = dialogTitle.c_str();

		if (!initialDirectory.empty())
			ofn.lpstrInitialDir = initialDirectory.c_str();

		ofn.Flags = static_cast<DWORD>(flags);

		succeeded = callback(&ofn);

		if (!succeeded)
			handleError();

		filename.clear();

		for (auto it = filepath.rbegin(); it != filepath.rend() && *it != '\\' && *it != '/'; ++it)
			filename += *it;

		std::reverse(filename.begin(), filename.end());
	}

	bool file_dialog::hasSucceeded() const
	{
		return succeeded;
	}

	std::string file_dialog::getSelectedFileName()
	{
		return filename;
	}

	std::string file_dialog::getSelectedFilePath()
	{
		return filepath;
	}

	std::string file_dialog::getErrorInfo()
	{
		return error;
	}

	bool file_dialog::isFileExisting() const
	{
		return std::filesystem::exists(filepath);
	}

	void file_dialog::handleError()
	{
		switch (CommDlgExtendedError())
		{
		case CDERR_DIALOGFAILURE:	error = "CDERR_DIALOGFAILURE";   break;
		case CDERR_FINDRESFAILURE:	error = "CDERR_FINDRESFAILURE";  break;
		case CDERR_INITIALIZATION:	error = "CDERR_INITIALIZATION";  break;
		case CDERR_LOADRESFAILURE:	error = "CDERR_LOADRESFAILURE";  break;
		case CDERR_LOADSTRFAILURE:	error = "CDERR_LOADSTRFAILURE";  break;
		case CDERR_LOCKRESFAILURE:	error = "CDERR_LOCKRESFAILURE";  break;
		case CDERR_MEMALLOCFAILURE: error = "CDERR_MEMALLOCFAILURE"; break;
		case CDERR_MEMLOCKFAILURE:	error = "CDERR_MEMLOCKFAILURE";  break;
		case CDERR_NOHINSTANCE:		error = "CDERR_NOHINSTANCE";     break;
		case CDERR_NOHOOK:			error = "CDERR_NOHOOK";          break;
		case CDERR_NOTEMPLATE:		error = "CDERR_NOTEMPLATE";      break;
		case CDERR_STRUCTSIZE:		error = "CDERR_STRUCTSIZE";      break;
		case FNERR_BUFFERTOOSMALL:	error = "FNERR_BUFFERTOOSMALL";  break;
		case FNERR_INVALIDFILENAME: error = "FNERR_INVALIDFILENAME"; break;
		case FNERR_SUBCLASSFAILURE: error = "FNERR_SUBCLASSFAILURE"; break;
		default:					error = "You cancelled.";
		}
	}

	save_file_dialog::save_file_dialog(const std::string& dialogTitle_)
		: file_dialog(GetSaveFileNameA, dialogTitle_)
	{
	}

	void save_file_dialog::show(explorer_flags flags)
	{
		file_dialog::show(flags);

		if (succeeded)
			addExtensionToFilePathAndName();
	}

	void save_file_dialog::defineExtension(const std::string& label_, const std::string& extension_)
	{
		extension = extension_;
		filter = label_ + '\0' + '*' + extension + '\0';
	}

	void save_file_dialog::addExtensionToFilePathAndName()
	{
		if (filename.size() >= extension.size())
		{
			std::string fileEnd(filename.data() + filename.size() - extension.size(), filename.data() + filename.size());

			if (fileEnd != extension)
			{
				filepath += extension;
				filename += extension;
			}
		}
		else
		{
			filepath += extension;
			filename += extension;
		}
	}

	open_file_dialog::open_file_dialog(const std::string& dialogTitle_)
		: file_dialog(GetOpenFileNameA, dialogTitle_)
	{
	}

	void open_file_dialog::addFileType(const std::string& label_, const std::string& filter_)
	{
		filter += label_ + '\0' + filter_ + '\0';
	}

}