#pragma once

#include <rttr/type>
#include <rttr/registration_friend>

#ifndef ERA_REFLECT
#define ERA_REFLECT \
					RTTR_ENABLE() \
					RTTR_REGISTRATION_FRIEND
#endif //!ERA_REFLECT

#ifndef ERA_VIRTUAL_REFLECT
#define ERA_VIRTUAL_REFLECT(BaseClass) \
					RTTR_ENABLE(BaseClass) \
					RTTR_REGISTRATION_FRIEND
#endif //!ERA_VIRTUAL_REFLECT