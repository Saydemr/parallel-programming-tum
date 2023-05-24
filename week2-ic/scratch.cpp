#include "thread"
#include <chrono>
#include <iostream>

void kernel(int* args) {
    ++(*args);
}

void oops_kernel(int* args) {
    int argument = *args;
    std::cout << "Hello" << *args << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    int another = *args;

    /* to get sigsegv
    *args = 492348;
    *args = 41;
    *args = 4921233;
    *args = 492123348;
    *args = 492;
    *args = 999999999;
    *args = 444492348;
     */

    std::cout << argument << " " << another << "\n";

}


void a_kernel(int* args) {
    int argument = *args;
}


void wicked_kernel(int *args) {
    (*args) = 0;
}

const int num_threads = 10;
std::thread threads[num_threads];

void spawn_threads()
{
    int ids[num_threads];
    for (int i = 0; i < num_threads; ++i) {
        ids[i] = i;
        threads[i] = std::thread(oops_kernel, &ids[i]);
    }
}


int main() {
    int argument = 0;
    std::thread thread(kernel, &argument);

    thread.join();



     /*
    {
        // Pitfalls example one
        std::thread threads[num_threads];
        for (int i = 0; i < num_threads; ++i) {
            threads[i] = std::thread(oops_kernel, &i);
             // passing a reference of the counter
             //   When the main thread exits this loop, i will
             //   be out of scope and other threads that are
             //   accessing i will produce invalid memory access
             // also value of i will be non-deterministic. Threads,
             // doing work wrt to this value may collide.

            // if wicked kernel, we may not exit the loop

        }
        for (int j = 0; j < num_threads; ++j) {
            threads[j].join();
        }
    }
     */

    /*
    {
        // Pitfalls example two, works fine
        int ids[num_threads];
        std::thread threads[num_threads];
        for (int i = 0; i < num_threads; ++i) {
            // assign each location its index
            ids[i] = i;
            // then give address of index to the kernel. When accessed,
            // it will not constitute any problem because index is not
            // updated from any other thread
            threads[i] = std::thread(oops_kernel, &ids[i]);

        }
        for (int j = 0; j < num_threads; ++j) {
            threads[j].join();
        }
    }
     */

    // /*

    {
        // ids are specific to spawn threads function
        // if you write sigsegv
        spawn_threads();
        for (int i = 0; i < num_threads; ++i)
        {
            threads[i].join();
        }

    }

    // */
    return 0;
}