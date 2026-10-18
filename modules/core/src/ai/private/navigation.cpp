// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "ai/navigation.h"

#include "core/memory.h"
#include "core/hash.h"

namespace era_engine::ai
{
    static bool is_destination(const vec2& pos, const NavNode& dest)
    {
        return pos == dest.position;
    }

    static float calculate_h(const vec2& pos, const NavNode& dest)
    {
        return length(pos - dest.position);
    }

    // TODO: Obstacle checking
    static bool is_valid(const vec2& pos)
    {
        return true;
    }

    NODISCARD std::vector<NavNode> PathFinder::a_star_make_path(std::unordered_map<vec2, NavNode>& map, NavNode& dest)
    {
        try
        {
            std::cout << "Found a path\n";
            int x = dest.position.x;
            int y = dest.position.y;
            std::stack<NavNode> path;
            std::vector<NavNode> usable_path;

            while (!(map[vec2(x, y)].parent_position == vec2(x, y)) && map[vec2(x, y)].position != NAV_INF_POS)
            {
                path.push(map[vec2(x, y)]);
                int temp_x = map[vec2(x, y)].parent_position.x;
                int temp_y = map[vec2(x, y)].parent_position.y;
                x = temp_x;
                y = temp_y;
            }
            path.push(map[vec2(x, y)]);

            while (!path.empty())
            {
                NavNode top = path.top();
                path.pop();
                usable_path.emplace_back(top);
            }

            return usable_path;
        }
        catch (const std::exception& e)
        {
            std::cout << e.what() << "\n";
            return {};
        }
    }

    NODISCARD std::vector<NavNode> AStarNavigation::navigate(NavNode& from, NavNode& to)
    {
        if (is_valid(to.position) == false)
        {
            std::cout << "Destination is an obstacle\n";
            return {};
        }
        if (is_destination(from.position, to))
        {
            std::cout << "You are the destination\n";
            return {};
        }

        std::unordered_map<vec2, bool> closed_list;
        closed_list.reserve((NAV_X_MAX / NAV_X_STEP) * (NAV_Y_MAX / NAV_Y_STEP) + 1);

        std::unordered_map<vec2, NavNode> all_map;
        all_map.reserve((NAV_X_MAX / NAV_X_STEP) * (NAV_Y_MAX / NAV_Y_STEP) + 1);
        for (int x = -1; x < (NAV_X_MAX / NAV_X_STEP) + 1; x++)
        {
            for (int y = -1; y < (NAV_Y_MAX / NAV_Y_STEP) + 1; y++)
            {
                all_map[vec2(x, y)].f_cost = FLT_MAX;
                all_map[vec2(x, y)].g_cost = FLT_MAX;
                all_map[vec2(x, y)].h_cost = FLT_MAX;
                all_map[vec2(x, y)].parent_position = NAV_INF_POS;
                all_map[vec2(x, y)].position = vec2(x, y);

                closed_list.emplace(vec2(x, y), false);
            }
        }

        int x = from.position.x;
        int y = from.position.y;
        all_map[vec2(x, y)].f_cost = 0.0;
        all_map[vec2(x, y)].g_cost = 0.0;
        all_map[vec2(x, y)].h_cost = 0.0;
        all_map[vec2(x, y)].parent_position = vec2(x, y);

        std::vector<NavNode> open_list;
        open_list.emplace_back(all_map[vec2(x, y)]);
        bool destination_found = false;

        while (!open_list.empty() && open_list.size() < (NAV_X_MAX / NAV_X_STEP) * (NAV_Y_MAX / NAV_Y_STEP))
        {
            NavNode node;
            do
            {
                float temp = FLT_MAX;
                std::vector<NavNode>::iterator it_node;
                for (auto it = open_list.begin(); it != open_list.end(); it = next(it))
                {
                    NavNode n = *it;
                    if (n.f_cost < temp)
                    {
                        temp = n.f_cost;
                        it_node = it;
                    }
                }
                node = *it_node;
                open_list.erase(it_node);
            } while (is_valid(node.position) == false);

            x = node.position.x;
            y = node.position.y;
            closed_list[vec2(x, y)] = true;

            for (int new_x = -1; new_x <= 1; new_x++)
            {
                for (int new_y = -1; new_y <= 1; new_y++)
                {
                    double g_new, h_new, f_new;
                    if (is_valid(vec2(x + new_x, y + new_y)))
                    {
                        if (is_destination(vec2(x + new_x, y + new_y), to))
                        {
                            all_map[vec2(x + new_x, y + new_y)].parent_position = vec2(x, y);
                            destination_found = true;
                            return PathFinder::a_star_make_path(all_map, to);
                            ;
                        }
                        else if (closed_list[vec2(x + new_x, y + new_y)] == false)
                        {
                            g_new = node.g_cost + 1.0;
                            h_new = calculate_h(vec2(x + new_x, y + new_y), to);
                            f_new = g_new + h_new;

                            if (all_map[vec2(x + new_x, y + new_y)].f_cost == FLT_MAX ||
                                all_map[vec2(x + new_x, y + new_y)].f_cost > f_new)
                            {
                                all_map[vec2(x + new_x, y + new_y)].f_cost = f_new;
                                all_map[vec2(x + new_x, y + new_y)].g_cost = g_new;
                                all_map[vec2(x + new_x, y + new_y)].h_cost = h_new;
                                all_map[vec2(x + new_x, y + new_y)].parent_position = vec2(x, y);
                                open_list.emplace_back(all_map[vec2(x + new_x, y + new_y)]);
                            }
                        }
                    }
                }
            }
        }

        if (destination_found == false)
        {
            std::cout << "Destination not found\n";
            return {};
        }

        return {};
    }
}