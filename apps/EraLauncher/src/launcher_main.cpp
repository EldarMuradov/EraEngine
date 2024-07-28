// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "launcher/hub_win_app.h"

int main(int argc, char** argv)
{
	using namespace era_engine::launcher;

	hub_win_app app{};
	app.init();

	return 0;
}