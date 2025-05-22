#include "../world.hpp"
#include "../tile_data.hpp"
#include "debug/debug.hpp"

Path World::ComputePath(glm::ivec2 origin, glm::ivec2 goal, ComputePathParams params)
{
    // A* pathfinding based on https://en.wikipedia.org/wiki/A*_search_algorithm 
    // However, since the map is effectively infinite we simulataneously pathfind from the origin to the goal and vice versa, using the distance between the two heads as the heuristic.

    params.maxIterations = 100;

    struct PathfindingContext {
        std::unordered_map<glm::ivec2, float> nodeCosts; // key is node pos, value is cost of best currently known path to node (at index) from origin
        //std::unordered_map < glm::ivec2, float > nodeToGoalCosts; // key is node pos, value is cost of best known path from origin to goal that goes through that node

        std::unordered_map<glm::ivec2, glm::ivec2> cameFrom;

        // Potential places to check next. Sorted from greatest to lowest of nodeCosts[node] + heuristic[node].
        std::vector<glm::ivec2> openSet;

        glm::ivec2 goal;
    };

    PathfindingContext forward;
    forward.nodeCosts[origin] = 0; // free to go to origin since you started there
    forward.openSet.push_back(origin);
    forward.goal = goal;
    PathfindingContext backward;
    backward.nodeCosts[goal] = 0;
    backward.openSet.push_back(goal);
    backward.goal = origin;

    enum PathResult {
        Fail,
        Succeed,
        Unfinished
    };

    auto heuristic = [](glm::ivec2 node, glm::ivec2 goal) {
        // bruh glm::length doesn't work for integral types
        return 100 * std::sqrtf((goal.x - node.x) * (goal.x - node.x) + (goal.y - node.y) * (goal.y - node.y));
        };

    auto AddNodeNeighborsToQueue = [this, &heuristic](PathfindingContext& context, PathfindingContext& otherContext, glm::ivec2 node) mutable {
        constexpr std::array<glm::ivec2, 4> neighbors = { glm::ivec2{ -1, 0 },{ 1, 0 },{ 0, 1 },{ 0, -1 } };
        for (const auto& neighborOffset : neighbors) {
            auto neighbor = node + neighborOffset;
            auto tileMoveCost = GetTileData(GetTile(neighbor.x, neighbor.y).layers[TileLayer::Floor]).moveCost;
            auto furniture = GetTile(neighbor.x, neighbor.y).layers[TileLayer::Furniture];
            auto furnitureMoveCost = furniture < 0 ? 0 : GetFurnitureData(furniture).moveCostModifier;
            if (tileMoveCost < 0 || furnitureMoveCost < 0) { /*DebugPlacePointOnPosition(glm::dvec3(neighbor.x + 0.0, 3.0, neighbor.y + 0.0), {1, 0, 0, 1});*/ continue; }
            //DebugPlacePointOnPosition(glm::dvec3(neighbor.x + 0.5, 3.0, neighbor.y + 0.5), { 1, 1, 1, 0.5 });
            auto cost = context.nodeCosts[node] + tileMoveCost + furnitureMoveCost;
            if (!context.nodeCosts.contains(neighbor) || cost < context.nodeCosts[neighbor]) { // then we found a better way to get to this node

                context.nodeCosts[neighbor] = cost;
                context.cameFrom[neighbor] = node;

                // TODO: better time-complexity way to keep openSet sorted???
                // if this neighbor is in openSet, remove it because its cost changed and we gotta reinsert it in the right place.
                auto it = std::find(context.openSet.begin(), context.openSet.end(), neighbor);
                if (it != context.openSet.end())
                    context.openSet.erase(it);

                // regardless of whether it was in the set previously, we now need to insert it at the right place.
                auto costToEnd = cost + heuristic(neighbor, context.goal);
                if (context.openSet.size() == 0) {
                    context.openSet.push_back(neighbor);
                    //DebugPlacePointOnPosition(glm::dvec3(node.x, 3.0, node.y), {1, 0, 1, 1});
                    //TestBillboardUi(glm::dvec3(node.x - 0.5, 3.0, node.y - 0.5), std::to_string((int)(cost)));
                }
                else {
                    for (auto it = context.openSet.begin(); it != context.openSet.end(); it++) {
                        if (context.nodeCosts[*it] + heuristic(*it, context.goal) <= costToEnd) {
                            context.openSet.insert(it, neighbor);
                            goto done;
                        }
                    }
                    context.openSet.push_back(neighbor);
                done:;
                DebugPlacePointOnPosition(glm::dvec3(neighbor.x, 3.0, neighbor.y), { 1, 0, 1, 1 });
                }
            }

        }
        };

    auto StepPath = [&AddNodeNeighborsToQueue, &heuristic](PathfindingContext& context, PathfindingContext& otherContext) -> PathResult {
        if (context.openSet.empty()) return Fail;
        auto node = context.openSet.back();

        context.openSet.pop_back();

        //otherContext.goal = node;
        int c = std::numeric_limits<int>::max();
        auto bestit = context.openSet.begin();
        for (auto it = context.openSet.begin(); it != context.openSet.end(); it++) {
            int betterc = context.nodeCosts[*it] + heuristic(*it, context.goal);
            if (betterc < c) {
                bestit = it;
                c = betterc;
            }
        }
        node = *bestit;
        context.openSet.erase(bestit);

        //DebugLogInfo("Trying ", node, " set is ", openSet.size());



        if (node == context.goal) {
            // WE WIN
            return Succeed;


            //Assert(p.wayPoints[0] == origin);
            //Assert(p.wayPoints.back() == goal);

            //for (auto& node : p.wayPoints) {
            //    //TestBillboardUi(glm::dvec3(node.x - 0.5, 3.0, node.y - 0.5), std::to_string((int)(nodeCosts[node])) + std::string(" ") + std::to_string(node.x - 0.5) + std::string(", ") + std::to_string(node.y - 0.5));
            //    TestBillboardUi(glm::dvec3(node.x + 0.5, 3.0, node.y + 0.5), GetTileData(GetTile(node.x, node.y).layers[TileLayer::Floor]).displayName);
            //}
        }

        AddNodeNeighborsToQueue(context, otherContext, node);
        return Unfinished;
        };

    int i = 0;
    while (i++ < params.maxIterations && forward.openSet.size() > 0 && backward.openSet.size() > 0) {
        auto fResult = StepPath(forward, backward);
        auto bResult = Unfinished; // won't matter
        if (fResult == Unfinished)
            bResult = StepPath(backward, forward);

        if (fResult == Fail || bResult == Fail) {
            break;
        }
        else if (fResult == Succeed) {
            //assert(forward.goal == backward.goal);

            Path p;

            // add the forward half
            glm::ivec2 current = forward.goal;
            while (true) {
                p.wayPoints.push_back(current);
                if (forward.cameFrom.contains(current)) {
                    assert(current != forward.cameFrom[current]);
                    current = forward.cameFrom[current];
                }
                else {
                    break;
                }
            }
            std::reverse(p.wayPoints.begin(), p.wayPoints.end());

            // add the backward half
            //current = forward.goal;
            //while (true) {
                //if (backward.cameFrom.contains(current)) {
                    //current = backward.cameFrom[current];
                    //p.wayPoints.push_back(current);
                //}
                //else {
                    //break;
                //}
            //}

            //return p;
        }
    }

    DebugLogInfo("Failed with ", i, " iterations");

    // return empty path if we ran out of iterations or we confirmed that no valid path exists.
    return Path();
    //Path combined = forward;
    //combined.wayPoints.insert(forward.wayPoints.end(), backward.wayPoints.rbegin(), backward.wayPoints.rend());
    //return combined;
}