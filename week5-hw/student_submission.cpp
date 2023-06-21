#include <queue>
#include <vector>
#include "Utility.h"

void simulate_waves(ProblemData &problemData, int timestep)
{
    auto &islandMap = problemData.islandMap;
    float(&secondLastWaveIntensity)[MAP_SIZE][MAP_SIZE] = *problemData.secondLastWaveIntensity;
    float(&lastWaveIntensity)[MAP_SIZE][MAP_SIZE] = *problemData.lastWaveIntensity;
    float(&currentWaveIntensity)[MAP_SIZE][MAP_SIZE] = *problemData.currentWaveIntensity;
    auto &portRoyal = problemData.portRoyal;

#pragma omp parallel for
    for (int x = 1; x < MAP_SIZE - 1; ++x)
    {
        for (int y = 1; y < MAP_SIZE - 1; ++y)
        {
            Position2D currentPosition(x, y);
            if (currentPosition.distanceTo(portRoyal) > TIME_STEPS - timestep + 1)
            {
                continue;
            }
            // Simulate some waves
            if (islandMap[x][y] >= LAND_THRESHOLD)
            {
                currentWaveIntensity[x][y] = 0.0f;
            }
            else {
                // The acceleration is the relative difference between the current point and the last.
                float acceleration = lastWaveIntensity[x][y - 1] + lastWaveIntensity[x - 1][y] + lastWaveIntensity[x + 1][y] + lastWaveIntensity[x][y + 1] - 4 * lastWaveIntensity[x][y];

                // The acceleration is multiplied with an attack value, specifying how fast the system can accelerate.
                acceleration *= ATTACK_FACTOR;

                // The last_velocity is calculated from the difference between the last intensity and the
                // second to last intensity
                float last_velocity = lastWaveIntensity[x][y] - secondLastWaveIntensity[x][y];

                // energy preserved takes into account that storms lose energy to their environments over time. The
                // ratio of energy preserved is higher on open water, lower close to the shore and 0 on land.
                float energyPreserved = std::clamp(
                        ENERGY_PRESERVATION_FACTOR * (LAND_THRESHOLD - 0.1f * islandMap[x][y]), 0.0f, 1.0f);

                currentWaveIntensity[x][y] =
                            std::clamp(lastWaveIntensity[x][y] + (last_velocity + acceleration) * energyPreserved, 0.0f, 1.0f);
            }

        }
    }
}

// Since all pirates like navigating by the stars, Captain Jack's favorite pathfinding algorithm is called A*.
// Unfortunately, sometimes you just have to make do with what you have. So here we use a search algorithm that searches
// the entire domain every time step and calculates all possible ship positions.
bool findPathWithExhaustiveSearch(ProblemData &problemData, int timestep)
{
    auto &start = problemData.shipOrigin;
    auto &portRoyal = problemData.portRoyal;
    auto &islandMap = problemData.islandMap;
    auto &currentWaveIntensity = *problemData.currentWaveIntensity;

    int numPossiblePositions = 0;

    bool(&currentShipPositions)[MAP_SIZE][MAP_SIZE] = *problemData.currentShipPositions;
    bool(&previousShipPositions)[MAP_SIZE][MAP_SIZE] = *problemData.previousShipPositions;

    // We could always have been at the start in the previous frame since we get to choose when we start our journey.
    previousShipPositions[start.x][start.y] = true;

    // Ensure that our new buffer is set to zero. We need to ensure this because we are reusing previously used buffers.
#pragma omp parallel for
    for (int x = 0; x < MAP_SIZE; ++x)
    {
        for (int y = 0; y < MAP_SIZE; ++y)
        {
            currentShipPositions[x][y] = false;
        }
    }

    bool flag = false;

// Do the actual path finding.
#pragma omp parallel for
    for (int x = 0; x < MAP_SIZE; ++x)
    {
        for (int y = 0; y < MAP_SIZE; ++y)
        {
            if (!previousShipPositions[x][y])
                continue;

            Position2D currentPosition(x, y);
            if (currentPosition.distanceTo(portRoyal) > TIME_STEPS - timestep + 96)
            {
                continue;
            }
            Position2D previousPosition(x, y);

            for (Position2D &neighbor : neighbours)
            {
                Position2D neighborPosition = previousPosition + neighbor;

                if (neighborPosition.x < 0 || neighborPosition.y < 0 || neighborPosition.x >= MAP_SIZE || neighborPosition.y >= MAP_SIZE)
                    continue;

                if (currentShipPositions[neighborPosition.x][neighborPosition.y])
                    continue;

                if (islandMap[neighborPosition.x][neighborPosition.y] >= LAND_THRESHOLD ||
                    currentWaveIntensity[neighborPosition.x][neighborPosition.y] >= SHIP_THRESHOLD)
                    continue;

                if (neighborPosition.distanceTo(portRoyal) > TIME_STEPS - timestep + 32)
                {
                    continue;
                }

                // If we reach Port Royal, we win.
                if (neighborPosition == portRoyal)
                    flag = true;

                currentShipPositions[neighborPosition.x][neighborPosition.y] = true;
            }
        }
    }

    return flag;
}

// Your main simulation routine.
int main(int argc, char *argv[])
{
    bool outputVisualization = false;
    bool constructPathForVisualization = false;
    int numProblems = 1;
    int option;

    // Not interesting for parallelization
    Utility::parse_input(outputVisualization, constructPathForVisualization, numProblems, option, argc, argv);

    // Fetch the seed from our container host used to generate the problem. This starts the timer.
    unsigned int seed = Utility::readInput();


    // Note that on the submission server, we are solving "numProblems" problems
    for (int problem = 0; problem < numProblems; ++problem)
    {
        auto *problemData = new ProblemData();

        // Receive the problem from the system.
        Utility::generateProblem((seed + problem * JUMP_SIZE) & INT_LIM, *problemData);
        int pathLength = -1;

        for (int t = 2; t < TIME_STEPS; t++)
        {
            // First simulate all cycles of the storm
            simulate_waves(*problemData, t);

            // Help captain Sparrow navigate the waves
            if (findPathWithExhaustiveSearch(*problemData, t))
            {
                // The length of the path is one shorter than the time step because the first frame is not part of the
                // pathfinding, and the second frame is always the start position.
                pathLength = t - 1;
            }

            if (pathLength != -1)
            {
                break;
            }

            // Rotates the buffers, recycling no longer needed data buffers for writing new data.
            problemData->flipSearchBuffers();
            problemData->flipWaveBuffers();
        }
        // Submit our solution back to the system.
        Utility::writeOutput(pathLength);

    }
    // This stops the timer by printing DONE.
    Utility::stopTimer();

    return 0;
}