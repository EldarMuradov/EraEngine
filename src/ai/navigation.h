#pragma once
#include <core/math.h>
#include <stack>

#define NAV_X_MAX 1000
#define NAV_X_STEP 20
#define NAV_Y_MAX 1000
#define NAV_Y_STEP 20

#define NAV_INF_POS 0xffffffff

struct nav_node
{
	vec2 position;
	vec2 parentPosition;
	float gCost;
	float hCost;
	float fCost;
};

inline bool operator <(const nav_node& lhs, const nav_node& rhs)
{
	return lhs.fCost < rhs.fCost;
}

static bool isDestination(vec2 pos, nav_node dest) noexcept
{
	if (pos == dest.position)
		return true;
	return false;
}

static float calculateH(vec2 pos, nav_node dest) noexcept
{
	return length(pos - dest.position);
}

static bool isValid(vec2 pos) noexcept
{
	return true;
}

struct path_finder
{
    static std::vector<nav_node> makePath(std::array<std::array<nav_node, (NAV_Y_MAX / NAV_Y_STEP)>, (NAV_X_MAX / NAV_X_STEP)> map, nav_node& dest)
    {
        try
        {
            std::cout << "Found a path\n";
            int x = dest.position.x;
            int y = dest.position.y;
            std::stack<nav_node> path;
            std::vector<nav_node> usablePath;

            while (!(map[x][y].parentPosition == dest.position)
                && map[x][y].position != NAV_INF_POS)
            {
                path.push(map[x][y]);
                int tempX = map[x][y].parentPosition.x;
                int tempY = map[x][y].parentPosition.y;
                x = tempX;
                y = tempY;
            }
            path.push(map[x][y]);

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
        }
    }
};

struct a_star_navigation
{
    static std::vector<nav_node> navigate(nav_node& player, nav_node& dest)
    {
        std::vector<nav_node> empty;
        if (isValid(dest.position) == false)
        {
            std::cout << "Destination is an obstacle\n";
            return empty;
        }
        if (isDestination(player.position, dest))
        {
            std::cout << "You are the destination\n";
            return empty;
        }

        bool closedList[(NAV_X_MAX / NAV_X_STEP)][(NAV_Y_MAX / NAV_Y_STEP)];

        std::array<std::array<nav_node, (NAV_Y_MAX / NAV_Y_STEP)>, (NAV_X_MAX / NAV_X_STEP)> allMap;
        for (int x = 0; x < (NAV_X_MAX / NAV_X_STEP); x++)
        {
            for (int y = 0; y < (NAV_Y_MAX / NAV_Y_STEP); y++)
            {
                allMap[x][y].fCost = FLT_MAX;
                allMap[x][y].gCost = FLT_MAX;
                allMap[x][y].hCost = FLT_MAX;
                allMap[x][y].parentPosition = NAV_INF_POS;
                allMap[x][y].position = vec2(x, y);

                closedList[x][y] = false;
            }
        }

        int x = player.position.x;
        int y = player.position.y;
        allMap[x][y].fCost = 0.0;
        allMap[x][y].gCost = 0.0;
        allMap[x][y].hCost = 0.0;
        allMap[x][y].parentPosition = vec2(x, y);

        std::vector<nav_node> openList;
        openList.emplace_back(allMap[x][y]);
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
            closedList[x][y] = true;

            for (int newX = -1; newX <= 1; newX++)
            {
                for (int newY = -1; newY <= 1; newY++)
                {
                    double gNew, hNew, fNew;
                    if (isValid(vec2(x + newX, y + newY)))
                    {
                        if (isDestination(vec2(x + newX, y + newY), dest))
                        {
                            allMap[x + newX][y + newY].parentPosition = vec2(x, y);
                            destinationFound = true;
                            return path_finder::makePath(allMap, dest);
                        }
                        else if (closedList[x + newX][y + newY] == false)
                        {
                            gNew = node.gCost + 1.0;
                            hNew = calculateH(vec2(x + newX, y + newY), dest);
                            fNew = gNew + hNew;

                            if (allMap[x + newX][y + newY].fCost == FLT_MAX ||
                                allMap[x + newX][y + newY].fCost > fNew)
                            {
                                allMap[x + newX][y + newY].fCost = fNew;
                                allMap[x + newX][y + newY].gCost = gNew;
                                allMap[x + newX][y + newY].hCost = hNew;
                                allMap[x + newX][y + newY].parentPosition = vec2(x, y);
                                openList.emplace_back(allMap[x + newX][y + newY]);
                            }
                        }
                    }
                }
            }
        }
        if (destinationFound == false)
        {
            std::cout << "Destination not found\n";
            return empty;
        }
    }
};