// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "scene.h"

uint64 serializeEntityToMemory(eentity entity, void* memory, uint64 maxSize);
bool deserializeEntityFromMemory(eentity entity, void* memory, uint64 size);