#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#include <algorithm>
#include <numeric>
#include <vector>
#include <iostream>
  
int main(int argc, char* argv[])
{
    char* cur_end = NULL;
    int num = (int) strtol(argv[1], &cur_end, 10);
    if (errno || *cur_end != 0 || num <= 0) {
        printf("incorrect number of threads: %s, (interpret as: %d(int))\n", argv[1], num);
        exit(2);
    }

    double start_time = omp_get_wtime();

    size_t i = 0;
    double result = 0.0;
    #pragma omp parallel for ordered shared(result) schedule(static, 1)
    for (i = 1; i <= num; ++i) {
        #pragma omp ordered
        {
            result += 1.0;
            printf("Thread num: %d, value: %f\n", omp_get_thread_num(), result);
        }
    }

    double end_time = omp_get_wtime();

    std::cout << "Result: " << result << std::endl;
    std::cout << "Time: " << end_time - start_time << std::endl;

    return 0;
}