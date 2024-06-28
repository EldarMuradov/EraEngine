// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include <pch.h>
#include "navigation.h"
#include <core/memory.h>
#include <core/hash.h>

namespace era_engine::ai
{
    static bool isDestination(vec2 pos, nav_node dest) noexcept
    {
        return pos == dest.position;
    }

    static float calculateH(vec2 pos, nav_node dest) noexcept
    {
        return length(pos - dest.position);
    }

    // TODO: Obstacle checking
    static bool isValid(vec2 pos) noexcept
    {
        return true;
    }

    NODISCARD std::vector<nav_node> path_finder::a_star_makePath(std::unordered_map<vec2, nav_node>& map, nav_node& dest)
    {
        try
        {
            std::cout << "Found a path\n";
            int x = dest.position.x;
            int y = dest.position.y;
            std::stack<nav_node> path;
            std::vector<nav_node> usablePath;

            while (!(map[vec2(x, y)].parentPosition == vec2(x, y))
                && map[vec2(x, y)].position != NAV_INF_POS)
            {
                path.push(map[vec2(x, y)]);
                int tempX = map[vec2(x, y)].parentPosition.x;
                int tempY = map[vec2(x, y)].parentPosition.y;
                x = tempX;
                y = tempY;
            }
            path.push(map[vec2(x, y)]);

            while (!path.empty())
            {
                nav_node top = path.top();
                path.pop();
                usablePath.emplace_back(top);
            }

            return usablePath;
        }
        catch (const std::exception& e)
        {
            std::cout << e.what() << "\n";
            return {};
        }
    }

    NODISCARD std::vector<nav_node> a_star_navigation::navigate(nav_node& from, nav_node& to)
    {
        if (isValid(to.position) == false)
        {
            std::cout << "Destination is an obstacle\n";
            return {};
        }
        if (isDestination(from.position, to))
        {
            std::cout << "You are the destination\n";
            return {};
        }

        std::unordered_map<vec2, bool> closedList;
        closedList.reserve((NAV_X_MAX / NAV_X_STEP) * (NAV_Y_MAX / NAV_Y_STEP) + 1);

        std::unordered_map<vec2, nav_node> allMap;
        allMap.reserve((NAV_X_MAX / NAV_X_STEP) * (NAV_Y_MAX / NAV_Y_STEP) + 1);
        for (int x = -1; x < (NAV_X_MAX / NAV_X_STEP) + 1; x++)
        {
            for (int y = -1; y < (NAV_Y_MAX / NAV_Y_STEP) + 1; y++)
            {
                allMap[vec2(x, y)].fCost = FLT_MAX;
                allMap[vec2(x, y)].gCost = FLT_MAX;
                allMap[vec2(x, y)].hCost = FLT_MAX;
                allMap[vec2(x, y)].parentPosition = NAV_INF_POS;
                allMap[vec2(x, y)].position = vec2(x, y);

                closedList.emplace(vec2(x, y), false);
            }
        }

        int x = from.position.x;
        int y = from.position.y;
        allMap[vec2(x, y)].fCost = 0.0;
        allMap[vec2(x, y)].gCost = 0.0;
        allMap[vec2(x, y)].hCost = 0.0;
        allMap[vec2(x, y)].parentPosition = vec2(x, y);

        std::vector<nav_node> openList;
        openList.emplace_back(allMap[vec2(x, y)]);
        bool destinationFound = false;

        while (!openList.empty() && openList.size() < (NAV_X_MAX / NAV_X_STEP) * (NAV_Y_MAX / NAV_Y_STEP))
        {
            nav_node node;
            do
            {
                float temp = FLT_MAX;
                std::vector<nav_node>::iterator itNode;
                for (auto it = openList.begin(); it != openList.end(); it = next(it))
                {
                    nav_node n = *it;
                    if (n.fCost < temp)
                    {
                        temp = n.fCost;
                        itNode = it;
                    }
                }
                node = *itNode;
                openList.erase(itNode);
            } while (isValid(node.position) == false);

            x = node.position.x;
            y = node.position.y;
            closedList[vec2(x, y)] = true;

            for (int newX = -1; newX <= 1; newX++)
            {
                for (int newY = -1; newY <= 1; newY++)
                {
                    double gNew, hNew, fNew;
                    if (isValid(vec2(x + newX, y + newY)))
                    {
                        if (isDestination(vec2(x + newX, y + newY), to))
                        {
                            allMap[vec2(x + newX, y + newY)].parentPosition = vec2(x, y);
                            destinationFound = true;
                            return path_finder::a_star_makePath(allMap, to);;
                        }
                        else if (closedList[vec2(x + newX, y + newY)] == false)
                        {
                            gNew = node.gCost + 1.0;
                            hNew = calculateH(vec2(x + newX, y + newY), to);
                            fNew = gNew + hNew;

                            if (allMap[vec2(x + newX, y + newY)].fCost == FLT_MAX ||
                                allMap[vec2(x + newX, y + newY)].fCost > fNew)
                            {
                                allMap[vec2(x + newX, y + newY)].fCost = fNew;
                                allMap[vec2(x + newX, y + newY)].gCost = gNew;
                                allMap[vec2(x + newX, y + newY)].hCost = hNew;
                                allMap[vec2(x + newX, y + newY)].parentPosition = vec2(x, y);
                                openList.emplace_back(allMap[vec2(x + newX, y + newY)]);
                            }
                        }
                    }
                }
            }
        }

        if (destinationFound == false)
        {
            std::cout << "Destination not found\n";
            return {};
        }

        return {};
    }
}