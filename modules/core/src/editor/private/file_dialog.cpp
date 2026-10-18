// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "editor/file_dialog.h"

#include <commdlg.h>

#include <shlobj_core.h>

namespace era_engine
{
    static void create_filter(char* filter, const std::string& file_description, const std::string& extension)
    {
        int offset = sprintf(filter, "%s (*.%s)", file_description.c_str(), extension.c_str());
        filter[offset] = 0;
        ++offset;
        offset += sprintf(filter + offset, "*.%s", extension.c_str());
    }

    static bool ends_with(std::string const& value, std::string const& ending)
    {
        if (ending.size() > value.size())
            return false;
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    std::string open_file_dialog(const std::string& file_description, const std::string& extension)
    {
        char filter[128];
        create_filter(filter, file_description, extension);

        OPENFILENAMEA ofn = {};
        char sz_file[MAX_PATH] = "";

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = GetActiveWindow();
        ofn.lpstrFile = sz_file;
        ofn.nMaxFile = sizeof(sz_file);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.lpstrInitialDir = ".";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE)
        {
            std::string result = ofn.lpstrFile;
            if (!ends_with(result, "." + extension))
            {
                result += "." + extension;
            }
            return result;
        }
        return std::string();
    }

    std::string save_file_dialog(const std::string& file_description, const std::string& extension)
    {
        char filter[128];
        create_filter(filter, file_description, extension);

        OPENFILENAMEA ofn = {};
        char sz_file[MAX_PATH] = "";

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = GetActiveWindow();
        ofn.lpstrFile = sz_file;
        ofn.nMaxFile = sizeof(sz_file);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.lpstrInitialDir = ".";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetSaveFileNameA(&ofn) == TRUE)
        {
            std::string result = ofn.lpstrFile;
            if (!ends_with(result, "." + extension))
            {
                result += "." + extension;
            }
            return result;
        }
        return std::string();
    }

#pragma warning(disable : 4244)

    std::string directory_dialog()
    {
        IFileDialog* p_file_open;

        // Create the FileOpenDialog object
        auto hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&p_file_open));

        p_file_open->SetOptions(FOS_PICKFOLDERS | FOS_NOCHANGEDIR | FOS_PATHMUSTEXIST);

        std::string result;

        if (SUCCEEDED(hr))
        {
            hr = p_file_open->Show(NULL);

            // Get the file name from the dialog box
            if (SUCCEEDED(hr))
            {
                IShellItem* p_item;
                hr = p_file_open->GetResult(&p_item);
                if (SUCCEEDED(hr))
                {
                    PWSTR psz_file_path;
                    hr = p_item->GetDisplayName(SIGDN_FILESYSPATH, &psz_file_path);

                    // Display the file name to the user
                    if (SUCCEEDED(hr))
                    {
                        std::wstring wresult(psz_file_path);
                        result = std::string(wresult.begin(), wresult.end());
                        CoTaskMemFree(psz_file_path);
                    }
                    p_item->Release();
                }
            }
            p_file_open->Release();
        }

        return result;
    }

}