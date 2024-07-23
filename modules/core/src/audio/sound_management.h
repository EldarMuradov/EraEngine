// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

namespace era_engine
{
	struct sound_spec;
	struct sound_id;

	void loadSoundRegistry();
	void drawSoundEditor(bool& open);

	NODISCARD const sound_spec& getSoundSpec(const sound_id& id);
}