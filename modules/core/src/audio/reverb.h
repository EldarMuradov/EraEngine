// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

namespace era_engine
{
    enum ReverbPreset
    {
        REVERB_NONE = -1,

        REVERB_PRESET_DEFAULT,
        REVERB_PRESET_GENERIC,
        REVERB_PRESET_PADDED_CELL,
        REVERB_PRESET_ROOM,
        REVERB_PRESET_BATHROOM,
        REVERB_PRESET_LIVINGROOM,
        REVERB_PRESET_STONE_ROOM,
        REVERB_PRESET_AUDITORIUM,
        REVERB_PRESET_CONCERT_HALL,
        REVERB_PRESET_CAVE,
        REVERB_PRESET_ARENA,
        REVERB_PRESET_HANGAR,
        REVERB_PRESET_CARPETED_HALLWAY,
        REVERB_PRESET_HALLWAY,
        REVERB_PRESET_STONE_CORRIDOR,
        REVERB_PRESET_ALLEY,
        REVERB_PRESET_FOREST,
        REVERB_PRESET_CITY,
        REVERB_PRESET_MOUNTAINS,
        REVERB_PRESET_QUARRY,
        REVERB_PRESET_PLAIN,
        REVERB_PRESET_PARKING_LOT,
        REVERB_PRESET_SEWER_PIPE,
        REVERB_PRESET_UNDER_WATER,
        REVERB_PRESET_SMALL_ROOM,
        REVERB_PRESET_MEDIUM_ROOM,
        REVERB_PRESET_LARGE_ROOM,
        REVERB_PRESET_MEDIUM_HALL,
        REVERB_PRESET_LARGE_HALL,
        REVERB_PRESET_PLATE,

        REVERB_PRESET_COUNT,
    };

    static const char* reverb_preset_names[] =
        {
            "Default",
            "Generic",
            "Padded cell",
            "Room",
            "Bathroom",
            "Livingroom",
            "Stone room",
            "Auditorium",
            "Concert hall",
            "Cave",
            "Arena",
            "Hangar",
            "Carpeted hallway",
            "Hallway",
            "Stone corridor",
            "Alley",
            "Forest",
            "City",
            "Mountains",
            "Quarry",
            "Plain",
            "Parking lot",
            "Sewer pipe",
            "Under water",
            "Small room",
            "Medium room",
            "Large room",
            "Medium hall",
            "Large hall",
            "Plate",
    };
}