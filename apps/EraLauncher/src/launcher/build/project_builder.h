#pragma once
#include <commdlg.h>

namespace era_engine::launcher
{
	struct hub_project;

	class builder
	{
	public:
		std::optional<std::string> selectBuildFolder();

		void build(hub_project& project, bool autoRun = false, bool tempFolder = false);
	};

	enum class explorer_flags
	{
		READONLY = 0x00000001,
		OVERWRITEPROMPT = 0x00000002,
		HIDEREADONLY = 0x00000004,
		NOCHANGEDIR = 0x00000008,
		SHOWHELP = 0x00000010,
		ENABLEHOOK = 0x00000020,
		ENABLETEMPLATE = 0x00000040,
		ENABLETEMPLATEHANDLE = 0x00000080,
		NOVALIDATE = 0x00000100,
		ALLOWMULTISELECT = 0x00000200,
		EXTENSIONDIFFERENT = 0x00000400,
		PATHMUSTEXIST = 0x00000800,
		FILEMUSTEXIST = 0x00001000,
		CREATEPROMPT = 0x00002000,
		SHAREAWARE = 0x00004000,
		NOREADONLYRETURN = 0x00008000,
		NOTESTFILECREATE = 0x00010000,
		NONETWORKBUTTON = 0x00020000,
		NOLONGNAMES = 0x00040000,	// Force no long names for 4.x modules
		EXPLORER = 0x00080000,	// New look commdlg
		NODEREFERENCELINKS = 0x00100000,
		LONGNAMES = 0x00200000,	// Force long names for 3.x modules
		ENABLEINCLUDENOTIFY = 0x00400000,	// Send include message to callback
		ENABLESIZING = 0x00800000,
		DONTADDTORECENT = 0x02000000,
		FORCESHOWHIDDEN = 0x10000000	// Show All files including System and hidden files
	};

	inline explorer_flags operator~ (explorer_flags a) { return (explorer_flags)~(int)a; }
	inline explorer_flags operator| (explorer_flags a, explorer_flags b) { return (explorer_flags)((int)a | (int)b); }
	inline explorer_flags operator& (explorer_flags a, explorer_flags b) { return (explorer_flags)((int)a & (int)b); }
	inline explorer_flags operator^ (explorer_flags a, explorer_flags b) { return (explorer_flags)((int)a ^ (int)b); }
	inline explorer_flags& operator|= (explorer_flags& a, explorer_flags b) { return (explorer_flags&)((int&)a |= (int)b); }
	inline explorer_flags& operator&= (explorer_flags& a, explorer_flags b) { return (explorer_flags&)((int&)a &= (int)b); }
	inline explorer_flags& operator^= (explorer_flags& a, explorer_flags b) { return (explorer_flags&)((int&)a ^= (int)b); }

	class file_dialog
	{
	public:
		file_dialog(std::function<int(tagOFNA*)> callback_, const std::string& dialogTitle_);

		void setInitialDirectory(const std::string& initialDirectory_);

		virtual void show(explorer_flags flags = explorer_flags::DONTADDTORECENT | explorer_flags::FILEMUSTEXIST | explorer_flags::HIDEREADONLY | explorer_flags::NOCHANGEDIR);

		bool hasSucceeded() const;

		std::string getSelectedFileName();

		std::string getSelectedFilePath();

		std::string getErrorInfo();

		bool isFileExisting() const;

	private:
		void handleError();

	protected:
		std::function<int(tagOFNA*)> callback;

		const std::string dialogTitle;

		std::string initialDirectory;
		std::string filter;
		std::string error;
		std::string filename;
		std::string filepath;

		bool succeeded;
	};

	class save_file_dialog : public file_dialog
	{
	public:
		save_file_dialog(const std::string& dialogTitle_);

		virtual void show(explorer_flags flags = explorer_flags::DONTADDTORECENT | explorer_flags::FILEMUSTEXIST | explorer_flags::HIDEREADONLY | explorer_flags::NOCHANGEDIR) override;

		void defineExtension(const std::string& label_, const std::string& extension_);

	private:
		void addExtensionToFilePathAndName();

	private:
		std::string extension;
	};

	class open_file_dialog : public file_dialog
	{
	public:
		open_file_dialog(const std::string& dialogTitle_);

		void addFileType(const std::string& label_, const std::string& filter_);
	};
}
