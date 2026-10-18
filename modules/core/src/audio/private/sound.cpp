// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "audio/sound.h"
#include "audio/audio.h"
#include "audio/sound_management.h"

#include "asset/file_registry.h"

#include "core/log.h"

#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'

namespace era_engine
{
    static HANDLE open_file(const fs::path& path);
    static void close_file(HANDLE& file_handle);
    static bool get_wfx(HANDLE file_handle, const fs::path& path, WAVEFORMATEXTENSIBLE& wfx);
    static bool find_chunk(HANDLE file_handle, uint32 fourcc, uint32& chunk_size, uint32& chunk_data_position);
    static bool read_chunk_data(HANDLE file_handle, void* buffer, uint32 buffersize, uint32 bufferoffset);

    static std::unordered_map<uint64, ref<AudioSound>> file_sounds;
    static std::unordered_map<uint64, ref<AudioSound>> synth_sounds;

    static bool check_for_existing_file_sound(SoundID id)
    {
        auto it = file_sounds.find(id.hash);
        return (it != file_sounds.end()) && it->second != 0;
    }

    static bool check_for_existing_synth_sound(SoundID id)
    {
        auto it = synth_sounds.find(id.hash);
        return (it != synth_sounds.end()) && it->second != 0;
    }

    void register_sound(SoundID id, const ref<AudioSound>& sound)
    {
        if (!sound->is_synth)
        {
            file_sounds[id.hash] = sound;
        }
        else
        {
            synth_sounds[id.hash] = sound;
        }
    }

    bool load_file_sound(SoundID id)
    {
        if (check_for_existing_file_sound(id))
            return true;
        else
        {
            const SoundSpec& spec = get_sound_spec(id);

            fs::path path = get_path_from_asset_handle(spec.asset);
            if (!path.empty())
            {
                HANDLE file_handle = open_file(path);
                if (file_handle != INVALID_HANDLE_VALUE)
                {
                    WAVEFORMATEXTENSIBLE wfx;
                    uint32 chunk_size, chunk_position;

                    bool success = false;
                    BYTE* data_buffer = 0;

                    // Find and retrieve format chunk
                    if (get_wfx(file_handle, path, wfx))
                    {
                        // Find data chunk
                        if (find_chunk(file_handle, fourccDATA, chunk_size, chunk_position))
                        {
                            success = true;

                            if (!spec.stream)
                            {
                                data_buffer = new BYTE[chunk_size];
                                success = read_chunk_data(file_handle, data_buffer, chunk_size, chunk_position);
                                if (!success)
                                {
                                    delete[] data_buffer;
                                }

                                close_file(file_handle);
                            }
                        }
                    }
                    if (success)
                    {
                        ref<AudioSound> sound = make_ref<AudioSound>();
                        sound->id = id;
                        sound->path = path;
                        sound->stream = spec.stream;
                        sound->file_handle = file_handle;
                        sound->wfx = wfx;
                        sound->chunk_size = chunk_size;
                        sound->chunk_position = chunk_position;
                        sound->data_buffer = data_buffer;
                        sound->is_synth = false;
                        sound->type = spec.type;

                        register_sound(id, sound);
                    }
                    else
                    {
                        close_file(file_handle);
                    }

                    return success;
                }
            }

            return false;
        }
    }

    void unload_sound(SoundID id)
    {
        file_sounds.erase(id.hash);
        synth_sounds.erase(id.hash);
    }

    void unload_all_sounds()
    {
        file_sounds.clear();
        synth_sounds.clear();
    }

    NODISCARD ref<AudioSound> get_sound(SoundID id)
    {
        auto it = file_sounds.find(id.hash);
        ref<AudioSound> result = (it != file_sounds.end()) ? it->second : 0;
        if (result)
            return result;

        it = synth_sounds.find(id.hash);
        result = (it != synth_sounds.end()) ? it->second : 0;
        return result;
    }

    AudioSound::~AudioSound()
    {
        close_file(file_handle);
        if (data_buffer)
        {
            delete[] data_buffer;
        }
    }

    static HANDLE open_file(const fs::path& path)
    {
        HANDLE file_handle = CreateFileW(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0, // FILE_FLAG_NO_BUFFERING,
            NULL);

        if (file_handle == INVALID_HANDLE_VALUE)
        {
            LOG_ERROR("Could not open file '%ws'", path.c_str());
        }

        return file_handle;
    }

    static void close_file(HANDLE& file_handle)
    {
        if (file_handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(file_handle);
            file_handle = INVALID_HANDLE_VALUE;
        }
    }

    static bool find_chunk(HANDLE file_handle, uint32 fourcc, uint32& chunk_size, uint32& chunk_data_position)
    {
        if (SetFilePointer(file_handle, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            LOG_ERROR("Could not set file pointer");
            return false;
        }

        DWORD chunk_type;
        DWORD chunk_data_size;
        DWORD riff_data_size = 0;
        DWORD file_type;
        DWORD bytes_read = 0;
        DWORD offset = 0;

        HRESULT hr = S_OK;
        while (hr == S_OK)
        {
            DWORD dw_read;
            if (ReadFile(file_handle, &chunk_type, sizeof(DWORD), &dw_read, NULL) == 0)
            {
                LOG_ERROR("Could not read chunk type");
                return false;
            }

            if (ReadFile(file_handle, &chunk_data_size, sizeof(DWORD), &dw_read, NULL) == 0)
            {
                LOG_ERROR("Could not read chunk data size");
                return false;
            }

            if (chunk_type == fourccRIFF)
            {
                riff_data_size = chunk_data_size;
                chunk_data_size = 4;
                if (ReadFile(file_handle, &file_type, sizeof(DWORD), &dw_read, NULL) == 0)
                {
                    LOG_ERROR("Could not read file type");
                    return false;
                }
            }
            else
            {
                if (SetFilePointer(file_handle, chunk_data_size, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
                {
                    LOG_ERROR("Could not set file pointer");
                    return false;
                }
            }

            offset += sizeof(DWORD) * 2;

            if (chunk_type == fourcc)
            {
                chunk_size = chunk_data_size;
                chunk_data_position = offset;
                return true;
            }

            offset += chunk_data_size;

            if (bytes_read >= riff_data_size)
                return false;
        }

        return true;
    }

    static bool read_chunk_data(HANDLE file_handle, void* buffer, uint32 buffersize, uint32 bufferoffset)
    {
        if (SetFilePointer(file_handle, bufferoffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            LOG_ERROR("Could not set file pointer");
            return false;
        }

        DWORD dw_read;
        if (ReadFile(file_handle, buffer, buffersize, &dw_read, NULL) == 0)
        {
            LOG_ERROR("Could not read chunk");
            return false;
        }

        return true;
    }

    static bool get_wfx(HANDLE file_handle, const fs::path& path, WAVEFORMATEXTENSIBLE& wfx)
    {
        uint32 chunk_size, chunk_position;

        // Check the file type, should be fourccWAVE or 'XWMA'.
        if (!find_chunk(file_handle, fourccRIFF, chunk_size, chunk_position))
            return false;

        DWORD filetype;
        if (!read_chunk_data(file_handle, &filetype, sizeof(DWORD), chunk_position))
            return false;

        if (filetype != fourccWAVE)
        {
            LOG_ERROR("File type of file '%ws' must be WAVE", path.c_str());
            return false;
        }

        if (!find_chunk(file_handle, fourccFMT, chunk_size, chunk_position))
            return false;
        if (!read_chunk_data(file_handle, &wfx, chunk_size, chunk_position))
            return false;

        return true;
    }

    NODISCARD bool is_sound_extension(const fs::path& extension)
    {
        return extension == ".wav";
    }

    NODISCARD bool is_sound_extension(const std::string& extension)
    {
        return extension == ".wav";
    }
}