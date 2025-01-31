// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "asset/file_registry.h"
#include "asset/asset.h"

#include "core/file_system.h"
#include "core/yaml.h"
#include "core/string.h"
#include "core/log.h"
#include "core/sync.h"

namespace era_engine
{
	typedef std::unordered_map<fs::path, AssetHandle> path_to_handle;
	typedef std::unordered_map<AssetHandle, fs::path> handle_to_path;

	static path_to_handle pathToHandle;
	static handle_to_path handleToPath;

	static std::mutex fileRegistryMutex;
	static const fs::path registryPath = fs::path(get_asset_path(L"/resources/files.yaml")).lexically_normal();

	static path_to_handle loadRegistryFromDisk()
	{
		path_to_handle loadedRegistry;

		std::ifstream stream(registryPath);
		YAML::Node n = YAML::Load(stream);

		for (auto entryNode : n)
		{
			AssetHandle handle = 0;
			fs::path path;

			YAML_LOAD(entryNode, handle, "Handle");
			YAML_LOAD(entryNode, path, "Path");

			if (handle)
			{
				loadedRegistry[path] = handle;
			}
		}

		return loadedRegistry;
	}

	static void writeRegistryToDisk()
	{
		YAML::Node out;

		for (const auto& [path, handle] : pathToHandle)
		{
			YAML::Node n;
			n["Handle"] = handle;
			n["Path"] = path;
			out.push_back(n);
		}

		fs::create_directories(registryPath.parent_path());
		std::ofstream fout(registryPath);
		fout << out;
	}

	static void readDirectory(const fs::path& path, const path_to_handle& loadedRegistry)
	{
		for (const auto& dirEntry : fs::directory_iterator(path))
		{
			const auto& path = dirEntry.path();
			if (dirEntry.is_directory())
			{
				readDirectory(path, loadedRegistry);
			}
			else
			{
				auto it = loadedRegistry.find(path);

				// If already known, use the handle, otherwise generate one.
				AssetHandle handle = (it != loadedRegistry.end()) ? it->second : AssetHandle::generate();

				pathToHandle.insert({ path, handle });
				handleToPath.insert({ handle, path });
			}
		}
	}

	static void handleAssetChange(const FileSystemEvent& e)
	{
		if (!fs::is_directory(e.path) && e.path != registryPath)
		{
			{
				Lock lock{ fileRegistryMutex };
				switch (e.change)
				{
				case FileSystemChange::Add:
				{
					LOG_MESSAGE("Asset '%ws' added", e.path.c_str());

					ASSERT(pathToHandle.find(e.path) == pathToHandle.end());

					AssetHandle handle = AssetHandle::generate();
					pathToHandle.insert({ e.path, handle });
					handleToPath.insert({ handle, e.path });
				} break;

				case FileSystemChange::Delete:
				{
					LOG_MESSAGE("Asset '%ws' deleted", e.path.c_str());

					auto it = pathToHandle.find(e.path);

					ASSERT(it != pathToHandle.end());

					AssetHandle handle = it->second;
					pathToHandle.erase(it);
					handleToPath.erase(handle);
				} break;

				case FileSystemChange::Modify:
				{
					LOG_MESSAGE("Asset '%ws' modified", e.path.c_str());
				} break;

				case FileSystemChange::Rename:
				{
					LOG_MESSAGE("Asset renamed from '%ws' to '%ws'", e.old_path.c_str(), e.path.c_str());

					auto oldIt = pathToHandle.find(e.old_path);

					ASSERT(oldIt != pathToHandle.end()); // Old path exists.
					ASSERT(pathToHandle.find(e.path) == pathToHandle.end()); // New path does not exist.

					AssetHandle handle = oldIt->second;
					pathToHandle.erase(oldIt);
					pathToHandle.insert({ e.path, handle });
					handleToPath[handle] = e.path; // Replace.
				} break;
				}
			}

			// During runtime the registry is only written to in this function, so no need to protect the read with mutex.
			LOG_MESSAGE("Rewriting file registry");
			writeRegistryToDisk();
		}
	}

	NODISCARD AssetHandle getAssetHandleFromPath(const fs::path& path)
	{
		const std::lock_guard<std::mutex> lock(fileRegistryMutex);

		auto it = pathToHandle.find(path);
		if (it == pathToHandle.end())
		{
			return {};
		}
		return it->second;
	}

	NODISCARD fs::path getPathFromAssetHandle(AssetHandle handle)
	{
		const std::lock_guard<std::mutex> lock(fileRegistryMutex);

		auto it = handleToPath.find(handle);
		if (it == handleToPath.end())
		{
			return {};
		}
		return it->second;
	}

	void initializeFileRegistry()
	{
		auto loadedRegistry = loadRegistryFromDisk();
		readDirectory(get_asset_path(L"/resources/assets"), loadedRegistry);
		writeRegistryToDisk();

		observe_directory(get_asset_path(L"/resources/assets"), handleAssetChange);
	}

}