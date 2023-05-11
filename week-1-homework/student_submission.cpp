#include <iostream>
#include <algorithm>
#include <limits>

constexpr int BLOCK_SIZE = 7;
constexpr int UNIQUE_CHARACTERS = 256;
constexpr int ROUNDS = 9;
constexpr int ITERATIONS = 400000;

/*
 * This is the message given to you to encrypt to verify the correctness and speed of your approach. Unfortunately,
 * there are no state secrets to be found here. Those would have probably made a pretty penny on the black market.
 */
uint8_t message[BLOCK_SIZE][BLOCK_SIZE] = {
        {'T', 'H', 'I', 'S', ' ', 'I', 'S'},
        {' ', 'A', 'V', 'E', 'R', 'Y', ' '},
        {'S', 'E', 'C', 'R', 'E', 'T', ' '},
        {'M', 'E', 'S', 'S', 'A', 'G', 'E'},
        {'!', ' ', 'T', 'R', 'Y', ' ', 'T'},
        {'O', ' ', 'C', 'R', 'A', 'C', 'K'},
        {' ', 'I', 'T', '.', '.', '.', ' '}
};

/*
 * The set of all keys generated at runtime and the index of the current key.
 */
int currentKey = 0;
uint8_t allKeys[ROUNDS][BLOCK_SIZE][BLOCK_SIZE];

/*
 * Use this like a 2D-Matrix key[BLOCK_SIZE][BLOCK_SIZE];
 * The key is only handed to you when it's time to actually encrypt something.
 */
const uint8_t (*key)[BLOCK_SIZE];

const uint8_t polynomialCoefficients[BLOCK_SIZE][BLOCK_SIZE] = {
        { 3, 1, 6, 5, 9, 4, 3},
        { 9, 6, 3, 8, 5, 2, 1},
        { 1, 2, 3, 4, 5, 6, 7},
        { 9, 8, 7, 6, 5, 4, 7},
        { 3, 8, 6, 5, 9, 1, 9},
        { 3, 5, 2, 8, 7, 9, 2},
        { 8, 3, 4, 6, 5, 1, 1}
};

/*
 * Generate some keys that can be used for encryption based on the seed set from the input.
 */
inline void generate_keys() {
    // Fill the key block
    for(auto & allKey : allKeys) {
        for (auto & row : allKey) {
            for (unsigned char & column : row) {
                column = rand() % std::numeric_limits<uint8_t>::max();
            }
        }
    }
}

inline void readInput() {
    std::cout << "READY" << std::endl;
    unsigned int seed = 0;
    std::cin >> seed;

    // Set the pseudo random number generator seed used for generating encryption keys
    srand(seed);

    generate_keys();
}

inline void writeOutput() {
    // Output the current state of the message in hexadecimal.
    for (const auto & row : message) {
        std::cout << std::hex << (int) row[0] << (int) row[1] << (int) row[2]
                  << (int) row[3];
    }
    // This stops the timer.
    std::cout << std::endl << "DONE" << std::endl;
}

/*
 * This is a utility method. It determines the next key to use from the set of pre-generated keys. In a real
 * cryptographic system, the subsequent keys are generated from a key schedule from the original key. To keep the code
 * short, we have omitted this behavior.
 */
inline void set_next_key() {
    key = &allKeys[currentKey][0];
    currentKey = (currentKey + 1) % ROUNDS;
}

/**
 * This function takes the characters stored in the 7x7 message array and substitutes each character for the
 * corresponding replacement as specified in the originalCharacter and substitutedCharacter array.
 * This corresponds to step 2.1 in the VV-AES explanation.
 */

const uint8_t dict[UNIQUE_CHARACTERS] = {13, 103, 113, 180, 116, 255, 106, 225, 47, 125, 233, 78, 231, 43, 222, 93, 126, 112, 57, 174, 108, 75, 161, 84, 100, 36, 74, 80, 33, 77, 58, 154, 134, 12, 64, 51, 175, 102, 152, 186, 109, 142, 226, 189, 163, 166, 61, 234, 209, 190, 95, 117, 248, 131, 227, 42,85, 199, 210, 46, 133, 10, 39, 97, 177, 83, 38, 169, 68, 220, 139, 241, 119, 53, 90, 110, 63, 205, 79, 244, 239, 148, 67, 254, 167, 30, 178, 150, 212, 99, 204, 235, 31, 29, 172, 170, 171, 146, 81, 187, 5, 98, 70, 211, 96, 86, 121, 145, 8, 216, 149, 66, 162, 156,                                147, 215, 1, 16, 2, 183, 182, 45, 20, 101, 236, 207, 35, 115, 122, 197, 60, 173, 214, 136, 111, 138, 137, 118, 192, 213, 203, 34, 219, 19, 124, 48, 21, 132, 176, 6, 56, 18, 76, 88, 164, 0, 127, 114, 224, 184, 246, 201, 15, 202, 157, 218, 144, 238, 44, 105, 155,7, 206, 55, 188, 159, 191, 229, 228, 41, 52, 140, 128, 120, 9, 230, 62, 54, 252, 141, 91, 130, 195, 123, 245, 153, 32, 25, 232, 11, 107, 26, 59, 22, 73, 193, 37, 143, 49, 50, 87, 179, 27, 94, 72, 14, 242, 92, 185, 135, 165, 198, 253, 196, 221, 217, 24, 249, 237, 251, 23, 181, 250, 40, 69, 17, 28, 223, 240, 151, 200, 3, 243, 194, 168, 82, 89, 104, 158, 4, 71, 208, 129, 65, 247, 160};


inline void substitute_bytes() {
    for (auto & row : message)
        for (auto & column : row)
            column = dict[column];
}


/*
 * This function shifts each row by the number of places it is meant to be shifted according to the AES specification.
 * Row zero is shifted by zero places. Row one by one, etc.
 * This corresponds to step 2.2 in the VV-AES explanation.
 */
inline void shift_rows() {
    // Shift each row, where the row index corresponds to how many columns the data is shifted.
    std::rotate(message[1],message[1] + 1, message[1] + BLOCK_SIZE);
    std::rotate(message[2],message[2] + 2, message[2] + BLOCK_SIZE);
    std::rotate(message[3],message[3] + 3, message[3] + BLOCK_SIZE);
    std::rotate(message[4],message[4] + 4, message[4] + BLOCK_SIZE);
    std::rotate(message[5],message[5] + 5, message[5] + BLOCK_SIZE);
    std::rotate(message[6],message[6] + 6, message[6] + BLOCK_SIZE);
}

inline constexpr uint8_t power(uint8_t x, uint8_t y)
{
    uint8_t temp = 0;
    if(y < 1)
        return 1;
    temp = power(x, y/2);
    if (y%2)
        return x*temp*temp;
    else
        return temp*temp;
}

/*
 * This function evaluates four different polynomials, one for each row in the column.
 * Each polynomial evaluated is of the form
 * m'[row, column] = c[r][3] m[3][column]^4 + c[r][2] m[2][column]^3 + c[r][1] m[1][column]^2 + c[r][0]m[0][column]^1
 * where m' is the new message value, c[r] is an array of polynomial coefficients for the current result row (each
 * result row gets a different polynomial), and m is the current message value.
 *
 */
uint8_t powers[256][BLOCK_SIZE+1];

inline void mix_columns() {
    for (uint8_t column = 0; column < BLOCK_SIZE; ++column) {
        for (uint8_t row = 0; row < BLOCK_SIZE; ++row) {
            uint8_t result = 0;
            for (uint8_t degree = 0; degree < BLOCK_SIZE; ++degree)
                result += polynomialCoefficients[row][degree] * powers[message[degree][column]][1+degree];
            message[row][column] = result;
        }
    }
}

/*
 * Add the current key to the message using the XOR operation.
 */
inline void add_key() {
    for (uint8_t row = 0; row < BLOCK_SIZE; ++row) {
        for (uint8_t column = 0; column < BLOCK_SIZE; ++column) {
            message[row][column] ^= key[row][column];
        }
    }
}

/*
 * Your main encryption routine.
 */
int main() {
    // Receive the problem from the system.
    readInput();

    for(uint16_t i = 0; i < 256; ++i) {
        for (uint8_t j = 0; j < BLOCK_SIZE+1; ++j) {
            powers[i][j] = power(i, j);
        }
    }

    // For extra security (and because Vars wasn't able to find enough test messages to keep you occupied) each message
    // is put through VV-AES lots of times. If we can't stop the adverse Masters from decrypting our highly secure
    // encryption scheme, we can at least slow them down.
    for (uint32_t i = 0; i < ITERATIONS; ++i) {
        // For each message, we use a predetermined key (e.g. the password). In our case, its just pseudo random.
        set_next_key();

        // First, we add the key to the message once:
        add_key();

        // We do 9+1 rounds for 128 bit keys
        for (uint8_t round = 0; round < ROUNDS; ++round) {
            //In each round, we use a different key derived from the original (refer to the key schedule).

            set_next_key();

            // These are the four steps described in the slides.
            substitute_bytes();
            shift_rows();
            mix_columns();
            add_key();

        }

        // Final round
        substitute_bytes();
        shift_rows();
        add_key();
    }

    // Submit our solution back to the system.
    writeOutput();
    return 0;
}