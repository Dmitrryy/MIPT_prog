#include <iostream>
#include <thread>
#include <string>
#include <functional>
#include <limits>
#include <map>
#include <vector>
#include <algorithm>
#include <numeric>

#include "ThreadPool.hpp"
#include "Timer.hpp"

#define NUM_SPLITS 8

// Simpson method
double Integrate(const std::function<double(double)> &func, double a, double b, double h)
{
    double I = 0;

    size_t N = (b - a) / h + 1;

    for (size_t i = 1; i < N - 1; ++i)
    {
        I += func(a + h * (i - 1)) + 4 * func(a + h * i) + func(a + h * (i + 1));
    }
    I *= h / 6.0;

    return I;
}

double threadTask(const std::function<double(double)> &func,
                double a, double b, double h, double eps,
                ThreadPool<double> &threadPool)
{
    auto I = Integrate(func, a, b, h);
    auto I2 = Integrate(func, a, b, h / 4.0);

    // Richardson method
    auto delta = (I - I2) / 6.0;
    auto res = I2;
    if (std::abs(delta) > eps)
    {
        // we divide the task into parts for each process and put it in a common queue
        //auto n = threadPool.numThreads();
        double offset = (b - a) / NUM_SPLITS;
        for (size_t i = 0; i < NUM_SPLITS; ++i)
        {
            double tmpA = a + offset * i;
            double tmpB = a + offset * (i + 1);
            threadPool.addTask(threadTask, func, tmpA, tmpB, h / 4.0, eps, std::ref(threadPool));
        }
        res = 0.0;
    }
    return res;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("invalid arguments command line:\n argc = %d\n", argc);
        printf("Required 2: [num threads] [accuracy]");
        printf("We provide:\n");
        for (int i = 0; i < argc; i++)
        {
            printf("[%d] %s\n", i, argv[i]);
        }
        exit(1);
    }
    auto &&numThreads = std::stoll(argv[1]);
    auto &&epsilon = std::stod(argv[2]);

    // initial conditions
    //=--------------------------------------
    // information about the function is not used anywhere else (versatility)
    auto&& Func = [](double x)
    {
        return std::cos(1 / x) / x;
    };

    double A = 0.0001;
    double B = 1.0; 
    //=--------------------------------------

    // Each thread adds the result to its own variable.
    // Then all the values are summed up (correctly due to the small number of threads)
    // FIXME: make variables cache friendly. 
    // Because of the proximity, processes are fighting for a cache line containing all the values of the results.
    std::vector<double> results(numThreads);
    std::function<void(size_t, double)> resultHolder = [&results](size_t threadID, double res) -> void {
        results[threadID] += res;
    };

    // all the work is done by the created threads. 
    // The main just launch them and waits for completion
    ThreadPool<double> threadPool(numThreads, resultHolder);

    double h = epsilon; //std::pow(epsilon, 1.0/1.0);
    double offset = (B - A) / NUM_SPLITS;
    Timer time;
    for (size_t i = 0; i < NUM_SPLITS; ++i)
    {
        double tmpA = A + offset * i;
        double tmpB = A + offset * (i + 1);
        threadPool.addTask(threadTask, Func, tmpA, tmpB, h, epsilon / NUM_SPLITS, std::ref(threadPool));
    }

    // Waiting for the completion of all tasks
    threadPool.waitAll();
    auto resTime = time.elapsed();

    // summarize the results of each stream and get the final result
    auto res = std::accumulate(results.begin(), results.end(), 0.0);

    std::cout << "time = " << resTime << std::endl;
    std::cout << "result = " << res << std::endl;
    std::cout << "num tasks = " << threadPool.lastTaskID() << std::endl;

    return 0;
}
