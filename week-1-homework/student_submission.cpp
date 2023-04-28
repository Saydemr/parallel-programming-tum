#include <iostream>

#include "vv-aes.h"
#include <chrono>
#include <algorithm>


/**
 * This function takes the characters stored in the 7x7 message array and substitutes each character for the
 * corresponding replacement as specified in the originalCharacter and substitutedCharacter array.
 * This corresponds to step 2.1 in the VV-AES explanation.
 */

uint8_t dict[UNIQUE_CHARACTERS] = {13, 103, 113, 180, 116, 255, 106, 225, 47, 125, 233, 78, 231, 43, 222, 93, 126, 112, 57, 174, 108, 75, 161, 84, 100, 36, 74, 80, 33, 77, 58, 154, 134, 12, 64, 51, 175, 102, 152, 186, 109, 142, 226, 189, 163, 166, 61, 234, 209, 190, 95, 117, 248, 131, 227, 42,85, 199, 210, 46, 133, 10, 39, 97, 177, 83, 38, 169, 68, 220, 139, 241, 119, 53, 90, 110, 63, 205, 79, 244, 239, 148, 67, 254, 167, 30, 178, 150, 212, 99, 204, 235, 31, 29, 172, 170, 171, 146, 81, 187, 5, 98, 70, 211, 96, 86, 121, 145, 8, 216, 149, 66, 162, 156,                                147, 215, 1, 16, 2, 183, 182, 45, 20, 101, 236, 207, 35, 115, 122, 197, 60, 173, 214, 136, 111, 138, 137, 118, 192, 213, 203, 34, 219, 19, 124, 48, 21, 132, 176, 6, 56, 18, 76, 88, 164, 0, 127, 114, 224, 184, 246, 201, 15, 202, 157, 218, 144, 238, 44, 105, 155,7, 206, 55, 188, 159, 191, 229, 228, 41, 52, 140, 128, 120, 9, 230, 62, 54, 252, 141, 91, 130, 195, 123, 245, 153, 32, 25, 232, 11, 107, 26, 59, 22, 73, 193, 37, 143, 49, 50, 87, 179, 27, 94, 72, 14, 242, 92, 185, 135, 165, 198, 253, 196, 221, 217, 24, 249, 237, 251, 23, 181, 250, 40, 69, 17, 28, 223, 240, 151, 200, 3, 243, 194, 168, 82, 89, 104, 158, 4, 71, 208, 129, 65, 247, 160};


void substitute_bytes() {
    for (auto & row : message) {
        for (auto & column : row) {
            column = dict[column];
        }
    }
}


/*
 * This function shifts each row by the number of places it is meant to be shifted according to the AES specification.
 * Row zero is shifted by zero places. Row one by one, etc.
 * This corresponds to step 2.2 in the VV-AES explanation.
 */
void shift_rows() {
    // Shift each row, where the row index corresponds to how many columns the data is shifted.
    for (int row = 1; row < BLOCK_SIZE; ++row) {
        std::rotate(message[row],message[row] + row, message[row] + BLOCK_SIZE);
    }
}

int power(int x, unsigned int y)
{
    int temp;
    if(y == 0)
        return 1;
    temp = power(x, y/2);
    if (y%2 == 0) {
        return temp*temp;
    }
    else
        return x*temp*temp;
}

/*
 * This function evaluates four different polynomials, one for each row in the column.
 * Each polynomial evaluated is of the form
 * m'[row, column] = c[r][3] m[3][column]^4 + c[r][2] m[2][column]^3 + c[r][1] m[1][column]^2 + c[r][0]m[0][column]^1
 * where m' is the new message value, c[r] is an array of polynomial coefficients for the current result row (each
 * result row gets a different polynomial), and m is the current message value.
 *
 */
void multiply_with_polynomial(int column) {
    for (int row = 0; row < BLOCK_SIZE; ++row) {
        auto result = 0;
        for (int degree = 0; degree < BLOCK_SIZE; degree++) {
            result += polynomialCoefficients[row][degree] * power(message[degree][column], degree + 1);
        }
        message[row][column] = result % 256;
    }
}

void mix_columns() {
    for (int column = 0; column < BLOCK_SIZE; ++column) {
        multiply_with_polynomial(column);
    }
}

/*
 * Add the current key to the message using the XOR operation.
 */
void add_key() {
    for (auto row = 0; row < BLOCK_SIZE; ++row) {
        for (auto column = 0; column < BLOCK_SIZE; ++column) {
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

    // For extra security (and because Vars wasn't able to find enough test messages to keep you occupied) each message
    // is put through VV-AES lots of times. If we can't stop the adverse Masters from decrypting our highly secure
    // encryption scheme, we can at least slow them down.
    for (int i = 0; i < ITERATIONS; ++i) {
        // For each message, we use a predetermined key (e.g. the password). In our case, its just pseudo random.
        set_next_key();

        // First, we add the key to the message once:
        add_key();

        // We do 9+1 rounds for 128 bit keys
        for (int round = 0; round < ROUNDS; ++round) {
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