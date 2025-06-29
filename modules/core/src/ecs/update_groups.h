#pragma once

#include "ecs/system.h"

namespace era_engine::update_types
{
	static inline UpdateGroup INPUT = UpdateGroup("INPUT", UpdateType::NORMAL, true);

	static inline UpdateGroup BEGIN = UpdateGroup("BEGIN", UpdateType::NORMAL);

	static inline UpdateGroup GAMEPLAY_BEFORE_PHYSICS = UpdateGroup("GAMEPLAY_BEFORE_PHYSICS", UpdateType::FIXED);

	static inline UpdateGroup BEFORE_PHYSICS = UpdateGroup("BEFORE_PHYSICS", UpdateType::FIXED, true);
	static inline UpdateGroup PHYSICS = UpdateGroup("PHYSICS", UpdateType::FIXED, true);
	static inline UpdateGroup AFTER_PHYSICS = UpdateGroup("AFTER_PHYSICS", UpdateType::FIXED, true);

	static inline UpdateGroup GAMEPLAY_AFTER_PHYSICS = UpdateGroup("GAMEPLAY_AFTER_PHYSICS", UpdateType::FIXED);

	static inline UpdateGroup GAMEPLAY_NORMAL_MAIN_THREAD = UpdateGroup("GAMEPLAY_NORMAL", UpdateType::NORMAL, true);
	static inline UpdateGroup GAMEPLAY_NORMAL = UpdateGroup("GAMEPLAY_NORMAL", UpdateType::NORMAL);

	static inline UpdateGroup BEFORE_RENDER = UpdateGroup("BEFORE_RENDER", UpdateType::NORMAL, true);
	static inline UpdateGroup RENDER = UpdateGroup("RENDER", UpdateType::NORMAL, true);
	static inline UpdateGroup AFTER_RENDER = UpdateGroup("AFTER_RENDER", UpdateType::NORMAL, true);

	static inline UpdateGroup END = UpdateGroup("END", UpdateType::NORMAL);

	void register_default_order();
}