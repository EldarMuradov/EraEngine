#include "pch.h"
#include "asset/io.h"

namespace era_engine
{
	NODISCARD entire_file loadFile(const fs::path& path)
	{
		FILE* f = fopen(path.string().c_str(), "rb");
		if (!f)
		{
			return {};
		}
		fseek(f, 0, SEEK_END);
		uint32 fileSize = ftell(f);
		if (fileSize == 0)
		{
			fclose(f);
			return {};
		}
		fseek(f, 0, SEEK_SET);

		uint8* buffer = (uint8*)malloc(fileSize);
		fread(buffer, fileSize, 1, f);
		fclose(f);

		return { buffer, fileSize };
	}

	void freeFile(entire_file file)
	{
		free(file.content);
	}
}
