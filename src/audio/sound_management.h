// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "audio/sound.h"

namespace era_engine
{
	void loadSoundRegistry();
	void drawSoundEditor(bool& open);

	NODISCARD const sound_spec& getSoundSpec(const sound_id& id);
}