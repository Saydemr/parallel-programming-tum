#include <iostream>
#include <vector>
#include <omp.h>
#include <algorithm>
#include "a-star-navigator.h"
#include "Utility.h"


inline void simulate_waves(ProblemData &problemData) {
    auto &islandMap = problemData.islandMap;
    float (&secondLastWaveIntensity)[MAP_SIZE][MAP_SIZE] = *problemData.secondLastWaveIntensity;
    float (&lastWaveIntensity)[MAP_SIZE][MAP_SIZE] = *problemData.lastWaveIntensity;
    float (&currentWaveIntensity)[MAP_SIZE][MAP_SIZE] = *problemData.currentWaveIntensity;

    #pragma omp parallel for
    for (int x = 1; x < MAP_SIZE - 1; ++x) {
        for (int y = 1; y < MAP_SIZE - 1; ++y) {
            if (islandMap[x][y] >= LAND_THRESHOLD) {
                currentWaveIntensity[x][y] = 0.0f;
            }
            else {
                float acceleration = lastWaveIntensity[x][y - 1]
                                     + lastWaveIntensity[x - 1][y]
                                     + lastWaveIntensity[x + 1][y]
                                     + lastWaveIntensity[x][y + 1]
                                     - 4 * lastWaveIntensity[x][y];

                // The acceleration is multiplied with an attack value, specifying how fast the system can accelerate.
                acceleration *= ATTACK_FACTOR;

                // The last_velocity is calculated from the difference between the last intensity and the
                // second to last intensity
                float last_velocity = lastWaveIntensity[x][y] - secondLastWaveIntensity[x][y];

                // energy preserved takes into account that storms lose energy to their environments over time. The
                // ratio of energy preserved is higher on open water, lower close to the shore and 0 on land.
                float energyPreserved = std::clamp(
                        ENERGY_PRESERVATION_FACTOR * (LAND_THRESHOLD - 0.1f * islandMap[x][y]), 0.0f, 1.0f);

                // The acceleration is the relative difference between the current point and the last.
                currentWaveIntensity[x][y] =
                        std::clamp(lastWaveIntensity[x][y] + (last_velocity + acceleration) * energyPreserved, 0.0f, 1.0f);

            }

        }
    }
}



inline bool findPathWithExhaustiveSearch(ProblemData &problemData) {
    auto &start = problemData.shipOrigin;
    auto &portRoyal = problemData.portRoyal;
    auto &islandMap = problemData.islandMap;
    auto &currentWaveIntensity = *problemData.currentWaveIntensity;

    int numPossiblePositions = 0;

    bool (&currentShipPositions)[MAP_SIZE][MAP_SIZE] = *problemData.currentShipPositions;
    bool (&previousShipPositions)[MAP_SIZE][MAP_SIZE] = *problemData.previousShipPositions;

    previousShipPositions[start.x][start.y] = true;

#pragma omp parallel for
    for (int x = 0; x < MAP_SIZE; ++x) {
        for (int y = 0; y < MAP_SIZE; ++y) {
            currentShipPositions[x][y] = false;
        }
    }

    bool flag = false;
#pragma omp parallel for
    for (int x = 0; x < MAP_SIZE; ++x) {
        for (int y = 0; y < MAP_SIZE; ++y) {
            if (!previousShipPositions[x][y]) {
                continue;
            }
            Position2D previousPosition(x, y);

            for (Position2D &neighbor: neighbours) {
                Position2D neighborPosition = previousPosition + neighbor;

                if (neighborPosition.x < 0 || neighborPosition.y < 0
                    || neighborPosition.x >= MAP_SIZE || neighborPosition.y >= MAP_SIZE) {
                    continue;
                }

                if (currentShipPositions[neighborPosition.x][neighborPosition.y]) {
                    continue;
                }

                if (islandMap[neighborPosition.x][neighborPosition.y] >= LAND_THRESHOLD ||
                    currentWaveIntensity[neighborPosition.x][neighborPosition.y] >= SHIP_THRESHOLD) {
                    continue;
                }

                if (neighborPosition == portRoyal) {
                    flag = true;
                }
                currentShipPositions[neighborPosition.x][neighborPosition.y] = true;
                numPossiblePositions++;
            }
        }
    }

    return flag;
}


int main() {
    unsigned int seed = 0;
    omp_set_num_threads(16);
    std::cout << "READY" << std::endl;
    std::cin >> seed;

    for (uint8_t problem = 0; problem < 4; ++problem) {
        auto *problemData = new ProblemData();

        Utility::generateProblem((seed + problem * JUMP_SIZE) & INT_LIM, *problemData);

        int pathLength = -1;
        std::vector<Position2D> path;


        for (int t = 2; t < TIME_STEPS; t++) {
            simulate_waves(*problemData);

            if (findPathWithExhaustiveSearch(*problemData)) {
                // The length of the path is one shorter than the time step because the first frame is not part of the
                // pathfinding, and the second frame is always the start position.
                pathLength = t - 1;
            }

            if (pathLength != -1) {
                break;
            }

            problemData->flipSearchBuffers();
            problemData->flipWaveBuffers();
        }

        if (pathLength == -1) {
            std::cout << "Path length: no solution." << std::endl;
        } else {
            std::cout << "Path length: " << pathLength << std::endl;
        }
    }

    std::cout << "DONE" << std::endl;

    return 0;
}