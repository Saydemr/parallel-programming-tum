#include <string>
#include <deque>
#include <future>
#include <functional>
#include <iostream>
#include <cstring>
#include <openssl/sha.h>


#define SHA1_BYTES 20

struct Sha1Hash {
    unsigned char data[SHA1_BYTES];
};

class MyUtility {
public:
    static Sha1Hash sha1(std::string& input);
    static Sha1Hash sha1(Sha1Hash& input);
    static Sha1Hash sha1(Sha1Hash& hash1, Sha1Hash& hash2);
    static Sha1Hash sha1(const unsigned char* input, size_t input_length);
};

// generate a sha1 hash from a std::string
inline Sha1Hash MyUtility::sha1(std::string& input){
    return sha1(reinterpret_cast<const unsigned char*>(input.data()), input.length());
}

// generate a new Sha1Hash from a Sha1Hash
inline Sha1Hash MyUtility::sha1(Sha1Hash& input){
    return sha1(input.data, SHA1_BYTES);
}

// generate a new Sha1 hash by concatenating 2 hashes
inline Sha1Hash MyUtility::sha1(Sha1Hash& hash1, Sha1Hash& hash2){

    unsigned char* combinedInput = static_cast<unsigned char*>(malloc(SHA1_BYTES * 2));
    memcpy(combinedInput, hash1.data, SHA1_BYTES);
    memcpy(&combinedInput[20], hash2.data, SHA1_BYTES);
    Sha1Hash result = sha1(combinedInput, SHA1_BYTES*2);
    return result;
}

// Wrapper method for creating a Sha1Hash struct using the openssl SHA1 method.
inline Sha1Hash MyUtility::sha1(const unsigned char* input, size_t input_length){
    Sha1Hash output;

    // call openssl sha1 method. this requires compilation with -lcrypto
    SHA1(input, input_length, output.data);

    return output;
}

inline uint8_t count_leading_zero_bits(Sha1Hash& hash){

    unsigned int bits;
    uint8_t total_leading_zero_bits = 0;

#pragma GCC unroll 4
    for(uint8_t i = 0; i < SHA1_BYTES; i+=4){
        // adjust bit order due to little endianness
        bits = (static_cast<unsigned int>(hash.data[i]) << 24) |
               (static_cast<unsigned int>(hash.data[i+1]) << 16);
        if(bits == 0){
            total_leading_zero_bits += 32;
        }else{
            // __builtin_clz is undefined for 0
            return total_leading_zero_bits + __builtin_clz(bits);
        }
    }

    return total_leading_zero_bits;
}

inline unsigned int readInput() {
    unsigned int seed = 0;
    std::cout << "READY" << std::endl;
    std::cin >> seed;

    return seed;
}

inline void printHash(Sha1Hash& hash) {

    for (uint8_t i = 0; i < SHA1_BYTES; ++i) {
        printf("%02x", hash.data[i]);
    }
    printf("\nDONE\n");
}


struct Problem {
    Sha1Hash sha1_hash;
    int16_t problemNum;
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

    for(int16_t i = 0; i < 10000; ++i){
        std::string base = std::to_string(rand()) + std::to_string(rand());
        Sha1Hash hash = MyUtility::sha1(base);
        do {
            hash = MyUtility::sha1(hash);
        } while(count_leading_zero_bits(hash) < 9);
        problemQueue.push(Problem{hash, i});
    }


    for (uint8_t i = 0; i < 25; ++i) {
        problemQueue.push(Problem{Sha1Hash(), -1});
    }
}

// This method repeatedly hashes itself until the required amount of leading zero bits is found
inline Sha1Hash findSolutionHash(Sha1Hash hash){
    do{
        hash = MyUtility::sha1(hash);
    } while(count_leading_zero_bits(hash) < 13);

    return hash;
}

int main() {
    Sha1Hash solutionHashes[10000];
    uint8_t num_threads = 25;
    std::thread threads[num_threads];


    unsigned int seed = readInput();
    std::thread generateProblems = std::thread(generateProblem, seed);


    // create threads to solve problems
    for (uint8_t i = 0; i < num_threads; ++i) {
        threads[i] = std::thread([&]() {
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
    for (uint8_t i = 0; i < num_threads; i++) {
        threads[i].join();
    }

    Sha1Hash solution;


    for(uint16_t i = 0; i < 10000; ++i){
        solution = MyUtility::sha1(solution, solutionHashes[i]);
    }

    printHash(solution);
    generateProblems.join();
    return 0;
}