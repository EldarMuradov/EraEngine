#include "asset/io.h"

namespace era_engine
{
	EntireFile load_file(const fs::path& path)
	{
		FILE* file = fopen(path.string().c_str(), "rb");
		if (!file)
		{
			return {};
		}

		fseek(file, 0, SEEK_END);
		uint32 file_size = ftell(file);

		if (file_size == 0)
		{
			fclose(file);
			return {};
		}

		fseek(file, 0, SEEK_SET);
		uint8* buffer = (uint8*)malloc(file_size);
		fread(buffer, file_size, 1, file);
		fclose(file);

		return { buffer, file_size };
	}

	void free_file(const EntireFile& file)
	{
		free(file.content);
	}
}
