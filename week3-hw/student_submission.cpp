//
// Created by Dennis-Florian Herr on 13/06/2022.
//

#include <string>
#include <deque>
#include <future>
#include <functional>
#include "Utility.h"

struct Problem {
    Sha1Hash sha1_hash;
    int problemNum;
};


class ProblemQueue {
public:
    void push(Problem problem){
        {
            std::lock_guard<std::mutex> lockGuard(queue_mutex);
            problemQueue.push_back(problem);
        }
        cv.notify_one();
    }

    Problem pop(){
        std::unique_lock<std::mutex> lock(queue_mutex);

        while(problemQueue.empty()){
            cv.wait(lock);
        }
        Problem p = problemQueue.front();
        problemQueue.pop_front();
        return p;
    }

    bool empty(){
        return problemQueue.empty();
    }
    bool stop = false;

private:
    std::deque<Problem> problemQueue;
    std::condition_variable cv;
    std::mutex queue_mutex;

};

ProblemQueue problemQueue;


// generate numProblems sha1 hashes with leadingZerosProblem leading zero bits
// This method is intentionally compute intense so you can already start working on solving
// problems while more problems are generated
inline void generateProblem(int seed){
    srand(seed);

    for(int i = 0; i < 10000; i++){
        std::string base = std::to_string(rand()) + std::to_string(rand());
        Sha1Hash hash = Utility::sha1(base);
        do {
            // we keep hashing ourself until we find the desired amount of leading zeros
            hash = Utility::sha1(hash);
        } while(Utility::count_leading_zero_bits(hash) < 9);
        problemQueue.push(Problem{hash, i});
    }
    for (int i = 0; i < 23; i++) {
        problemQueue.push(Problem{Sha1Hash(), -1});
    }
}

// This method repeatedly hashes itself until the required amount of leading zero bits is found
inline Sha1Hash findSolutionHash(Sha1Hash hash){
    do{
        hash = Utility::sha1(hash);
    } while(Utility::count_leading_zero_bits(hash) < 13);

    return hash;
}

int main() {
    Sha1Hash solutionHashes[10000];

    unsigned int seed = Utility::readInput();
    std::thread generateProblems = std::thread(generateProblem, seed);


    uint8_t num_threads = 23;
    std::thread threads[num_threads];

    // create threads to solve problems
    for (int i = 0; i < num_threads; i++) {
        threads[i] = std::thread([i, &solutionHashes]() {
            while(true) {
                Problem p = problemQueue.pop();
                if (p.problemNum == -1) {
                    break;
                }
                solutionHashes[p.problemNum] = findSolutionHash(p.sha1_hash);
            }
        });
    }


    // join threads
    for (int i = 0; i < num_threads; i++) {
        threads[i].join();
    }

    Sha1Hash solution;

    // this doesn't need parallelization. it's fast
    for(int i = 0; i < 10000; i++){
        solution = Utility::sha1(solution, solutionHashes[i]);
    }

    Utility::printHash(solution);
    printf("DONE\n");
    generateProblems.join();
    return 0;
}
