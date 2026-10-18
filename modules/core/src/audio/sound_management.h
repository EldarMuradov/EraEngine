// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

namespace era_engine
{
    struct SoundSpec;
    struct SoundID;

    void load_sound_registry();
    void draw_sound_editor(bool& open);

    NODISCARD const SoundSpec& get_sound_spec(const SoundID& id);
}