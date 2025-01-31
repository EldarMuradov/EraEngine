// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "core/file_system.h"

namespace era_engine
{
	struct ObserveParams
	{
		fs::path directory;
		FileSystemObserver callback;
		bool watch_subdirectories;
	};

	static DWORD observe_directory_thread(void* in_params)
	{
		ObserveParams* params = (ObserveParams*)in_params;

		uint8 buffer[1024] = {};

		HANDLE directory_handle = CreateFileW(
			params->directory.c_str(),
			GENERIC_READ | FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL);

		if (directory_handle == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Monitor directory failed.\n";
			return 1;
		}

		OVERLAPPED overlapped;
		overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		ResetEvent(overlapped.hEvent);

		DWORD event_name =
			FILE_NOTIFY_CHANGE_LAST_WRITE |
			FILE_NOTIFY_CHANGE_FILE_NAME |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_SIZE;

		DWORD bytes_returned;

		fs::path last_changed_path = "";
		fs::file_time_type last_changed_path_timestamp;

		while (true)
		{
			DWORD result = ReadDirectoryChangesW(directory_handle,
				buffer, sizeof(buffer), params->watch_subdirectories,
				event_name,
				&bytes_returned, &overlapped, NULL);

			if (!result)
			{
				std::cerr << "Read directory changes failed\n";
				break;
			}

			WaitForSingleObject(overlapped.hEvent, INFINITE);

			DWORD dw;
			if (!GetOverlappedResult(directory_handle, &overlapped, &dw, FALSE) || dw == 0)
			{
				std::cerr << "Get overlapped result failed.\n";
				break;
			}

			FILE_NOTIFY_INFORMATION* filenotify;
			DWORD offset = 0;

			fs::path old_path;

			do
			{
				filenotify = (FILE_NOTIFY_INFORMATION*)(&buffer[offset]);

				FileSystemChange change = FileSystemChange::None;

				switch (filenotify->Action)
				{
				case FILE_ACTION_ADDED: { change = FileSystemChange::Add; } break;
				case FILE_ACTION_REMOVED: { change = FileSystemChange::Delete; } break;
				case FILE_ACTION_MODIFIED: { change = FileSystemChange::Modify; } break;
				case FILE_ACTION_RENAMED_OLD_NAME:
				{
					uint32 filename_length = filenotify->FileNameLength / sizeof(WCHAR);
					old_path = (params->directory / std::wstring(filenotify->FileName, filename_length)).lexically_normal();
				} break;
				case FILE_ACTION_RENAMED_NEW_NAME: { change = FileSystemChange::Rename; } break;
				}

				if (change != FileSystemChange::None)
				{
					uint32 filename_length = filenotify->FileNameLength / sizeof(WCHAR);
					fs::path path = (params->directory / std::wstring(filenotify->FileName, filename_length)).lexically_normal();

					if (change == FileSystemChange::Modify)
					{
						auto writeTime = fs::last_write_time(path);

						// The filesystem usually sends multiple notifications for changed files, since the file is first written, then metadata is changed etc.
						// This check prevents these notifications if they are too close together in time.
						// This is a pretty crude fix. In this setup files should not change at the same time, since we only ever track one file.
						if (path == last_changed_path
							&& std::chrono::duration_cast<std::chrono::milliseconds>(writeTime - last_changed_path_timestamp).count() < 200)
						{
							last_changed_path = path;
							last_changed_path_timestamp = writeTime;
							break;
						}

						last_changed_path = path;
						last_changed_path_timestamp = writeTime;
					}

					FileSystemEvent e;
					e.change = change;
					e.path = std::move(path);
					if (change == FileSystemChange::Rename)
					{
						e.old_path = std::move(old_path);
					}

					params->callback(e);
				}

				offset += filenotify->NextEntryOffset;

			} while (filenotify->NextEntryOffset != 0);

			if (!ResetEvent(overlapped.hEvent))
			{
				std::cerr << "Reset event failed.\n";
			}
		}

		CloseHandle(directory_handle);

		delete params;

		return 0;
	}

	bool observe_directory(const fs::path& directory, const FileSystemObserver& callback, bool watch_subdirectories)
	{
		ObserveParams* params = new ObserveParams;
		params->directory = directory;
		params->callback = callback;
		params->watch_subdirectories = watch_subdirectories;

		HANDLE handle = CreateThread(0, 0, observe_directory_thread, params, 0, 0);
		bool result = handle != INVALID_HANDLE_VALUE;
		CloseHandle(handle);

		return result;
	}

}