#include <iostream>
#include <mpi.h>

#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <random>
#include <chrono>

#define TEST_PIX_NUM 100
#define PROC_NUM 3
#define MAP_WIDTH 1024
#define MAP_HEIGHT (MAP_WIDTH * PROC_NUM)
#define ITER_NUM 100

#define MAX_WAVE_HEIGHT 4.0f

/**
 *
 */
inline void swap_ptr(float*& ptr1, float*& ptr2){
    float *tmp = ptr1;
    ptr1 = ptr2;
    ptr2 = tmp;
}



inline static unsigned int readInput()
{
    std::cout << "READY" << std::endl;
    unsigned int seed = 0;
    std::cin >> seed;

    std::cerr << "Using seed " << seed << std::endl;
    if (seed == 0)
    {
        std::cerr << "Warning: default value 0 used as seed." << std::endl;
    }

    return seed;
}

/*
 * Generates random tests.
 */
inline static void
generate_test(unsigned int seed, float *wave_map)
{
    std::minstd_rand0 generator(seed); // linear congruential random number generator.

    for (size_t i = 0; i < MAP_HEIGHT * MAP_WIDTH; i++)
    {
        wave_map[i] = static_cast<float>(generator()) * MAX_WAVE_HEIGHT / static_cast<float>(generator.max());
    }
}

/*
 * This function outputs the result.
 */
inline static void outputResult(float *highest_result)
{
    // print highest wave height at each iteration
    std::cout << highest_result[ITER_NUM-1] << "\n";

    std::cout << "DONE" << std::endl;
}



// uncomment this line to print used time
// comment this line before submission
#define PRINT_TIME 1

/**
 * This is a function that updates a block of the wave map specified by the starting and ending rows and columns
 * You don't need to modify this function. But it might be helpful to understand the update process.
 */
inline float update_block(int st_row, int end_row, int st_col, int end_col, float *old_wave_map, float *new_wave_map)
{
    float highest = 0.0f;
    for (uint16_t row = st_row; row < end_row; row++)
    {
        for (uint16_t col = st_col; col < end_col; col++)
        {
            // wrap the map (for example, the left column of the leftmost column is the rightmost column)
            uint16_t upper_row = (row - 1 + MAP_HEIGHT) % MAP_HEIGHT;
            uint16_t lower_row = (row + 1 + MAP_HEIGHT) % MAP_HEIGHT;
            uint16_t left_col = (col - 1 + MAP_WIDTH) % MAP_WIDTH;
            uint16_t right_col = (col + 1 + MAP_WIDTH) % MAP_WIDTH;

            // average the 4 neighbors and assign to the current position
            new_wave_map[row * MAP_WIDTH + col] = 0.2 * (old_wave_map[upper_row * MAP_WIDTH + col] + old_wave_map[lower_row * MAP_WIDTH + col] + old_wave_map[row * MAP_WIDTH + left_col] + old_wave_map[row * MAP_WIDTH + right_col] + old_wave_map[row * MAP_WIDTH + col]);
            if (new_wave_map[row * MAP_WIDTH + col] > highest){
                highest = new_wave_map[row * MAP_WIDTH + col];
            }
        }
    }
    return highest;
}


int main(int argc, char **argv)
{

    MPI_Init(&argc, &argv);
    // define the rank and size variable
    int rank, size;


    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_size(MPI_COMM_WORLD, &size);

    float *old_wave_map = new float [MAP_HEIGHT * MAP_WIDTH];
    float *new_wave_map = new float [MAP_HEIGHT * MAP_WIDTH];
    float *highest_result = new float[ITER_NUM];

    if (rank == 0){
        unsigned int seed = readInput();
        generate_test(seed, old_wave_map);
    }
    // pass the wave map to all other processes
    MPI_Bcast(old_wave_map, MAP_HEIGHT * MAP_WIDTH, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // the number of rows calculated by a process
    uint16_t proc_rows = MAP_HEIGHT / size;

    // the starting row in this process
    uint16_t st_row = rank * proc_rows;

    // the end row(not included) in this process
    uint16_t end_row = (rank + 1) * proc_rows;

    for (uint8_t i = 0; i < ITER_NUM; i++)
    {
        // communicate the ghost layers before update
        if (rank == 0)
        {
            // send lowest row in block to process with rank 1
            MPI_Send(old_wave_map + (end_row - 1) * MAP_WIDTH, MAP_WIDTH, MPI_FLOAT, 1, 0, MPI_COMM_WORLD);

            // receive upper ghost layer from process with rank size-1
            MPI_Recv(old_wave_map + (MAP_HEIGHT - 1) * MAP_WIDTH, MAP_WIDTH, MPI_FLOAT, size - 1, 0, MPI_COMM_WORLD, nullptr);

            // send uppermost row in block to process with rank size-1
            MPI_Send(old_wave_map + st_row * MAP_WIDTH, MAP_WIDTH, MPI_FLOAT, size - 1, 1, MPI_COMM_WORLD);

            // receive lower ghost layer from process with rank 1
            MPI_Recv(old_wave_map + end_row * MAP_WIDTH, MAP_WIDTH, MPI_FLOAT, 1, 1, MPI_COMM_WORLD, nullptr);
        }
        else
        {

            MPI_Recv(old_wave_map + (st_row - 1) * MAP_WIDTH, MAP_WIDTH, MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD, nullptr);


            MPI_Send(old_wave_map + (end_row-1) * MAP_WIDTH, MAP_WIDTH, MPI_FLOAT, (rank+1)%size, 0, MPI_COMM_WORLD);


            MPI_Recv(old_wave_map + (end_row % MAP_WIDTH) * MAP_WIDTH, MAP_WIDTH, MPI_FLOAT, (rank+1)%size, 1, MPI_COMM_WORLD, nullptr);


            MPI_Send(old_wave_map + st_row * MAP_WIDTH, MAP_WIDTH, MPI_FLOAT, rank-1, 1, MPI_COMM_WORLD);
        }
        highest_result[i] = update_block(st_row, end_row, 0, MAP_WIDTH, old_wave_map, new_wave_map);
        swap_ptr(old_wave_map, new_wave_map);

        float highest = 0;
        MPI_Reduce(highest_result + i, &highest, 1, MPI_FLOAT, MPI_MAX, 0, MPI_COMM_WORLD);

        if (rank == 0)
        {
            highest_result[i] = highest;
        }
    }

    if (rank == 0)
    {
        outputResult(highest_result);
    }

    MPI_Finalize();

    return 0;
}
