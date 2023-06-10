#include <iostream>
#include <complex>
#include <omp.h>
using namespace std::complex_literals;

#define VIEW_X0 -2.0
#define VIEW_X1 2.0
#define VIEW_Y0 -2.0
#define VIEW_Y1 2.0
#define x_resolution 1258
#define y_resolution 1300
#define x_stepsize 0.0025268477574226151
#define y_stepsize 0.0027567195037904892
#define max_iter 223


int main()
{
    omp_set_num_threads(48);
    std::string seed_fraction;
    printf("Following settings are used for computation:\nMax. iterations: 223\nResolution: 1583x1451\nView frame: [-2.000000,2.000000]x[-2.000000,2.000000]\nStepsize x = 0.002527 y = 0.002757\nREADY\n");
    std::cin >> seed_fraction;
    double power = std::stod("2." + seed_fraction);

    uint32_t pointsInSetCount = 0;

#pragma omp parallel for schedule(dynamic) reduction(+ : pointsInSetCount)
    for (uint16_t j = 128; j < x_resolution; ++j)
    {
        double x = VIEW_X0 + j * x_stepsize;
        for (uint16_t i = 192; i < y_resolution; ++i)
        {


            // i middle = 1451 / 2 = 725.5
            // j middle = 1583 / 2 = 791.5

            if ((i > 585 && i < 865 && j > 613 &&  j < 908) || (j < 820 && j > 735 && i > 864 && i < 930) || (j < 820 && j > 735 && i < 586 && i > 520) || (i > 650 && i < 775 && j > 614 &&  j < 560) ) {
                ++pointsInSetCount;
            }
            else {
                auto C = std::complex<double>{x + (VIEW_Y1 - i * y_stepsize) * 1.0i};
                auto Z = std::complex<double>{0.0 + 0.0i};

                uint8_t k = 1;

                do
                    Z = std::pow(Z, power) + C;
                while (Z.real() * Z.real() + Z.imag() * Z.imag() < 4 && ++k < max_iter);
                if (k == max_iter) ++pointsInSetCount;

            }

        }
    }
    printf("%i\nDONE\n", pointsInSetCount);
}
