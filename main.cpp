#include <chrono>
#include <cmath>
#include <complex>
#include <iomanip>
#include <iostream>

#include "./include/data_table.hpp"
#include "./include/md_static/functions.hpp"
#include "./include/md_static/functions/md_static_array_utility.hpp"
#include "./include/md_static/linear_algebra.hpp"
#include "./include/md_static/md_static_array.hpp"

int main(int argc, const char **argv) {
    // auto c = MdArrayUtility::f_arctan(f);
    size_t sz = 128;
    MdStaticArray<double> c({3, sz, sz, sz}, 1), d({sz, sz}, 1);
    // auto c = MdArrayUtility::range<double>(0, 1, 0.000000005);
    auto start = std::chrono::system_clock::now();
    // auto c = MdUtility::dot<size_t, size_t, size_t>(f, g, 16);
    // auto csum = MdArrayUtility::cumulative_sum<double>(c);
    auto dd = MdLinearAlgebra::dot<double, double, double>(c[0], d, 16);
    // auto sum = MdArrayUtility::sum(c);
    // auto c = f + g;
    auto end = std::chrono::system_clock::now();

    std::cout << '\n';
    std::chrono::duration<double> time = end - start;

    std::cout << dd[0].get_size() << " " << dd[0][0].get_size() << '\n';

    std::cout << dd.get_size() << " Time: " << time.count() << "s" << std::endl;

    return 0;
}
