#pragma once
#ifndef _RANGE_HPP_
#define _RANGE_HPP_

#include "./md_static_array_utility.hpp"

// To do: improve and update since it does not work for all cases
template <typename T, typename Ts, class T1>
Array<T> Utils::range(const T start, const T end, const Ts spacing) {
    usize size = 0;
    usize start_value = 0;
    Ts increment = 1;
    if (end < start && spacing > 0) {
        throw std::runtime_error(
            "Spacing given should be negative for ranges: [end (" +
            std::to_string(end) + ") < start (" + std::to_string(start) + ")]");
    }
    if (end == -1 && spacing == 0) {
        size = start;
        increment = 1;
    } else if (spacing == 0 && end > start) {
        const auto value = end - start;
        size = static_cast<usize>(
            ::ceil(std::max(value, static_cast<decltype(value)>(0))));
        start_value = start;
        increment = 1;
    } else {
        const double value = ::abs((end - start) / (spacing * 1.0));
        start_value = start;
        size = static_cast<usize>(::ceil(std::max(value, static_cast<f64>(0))));
        increment = spacing;
    }

    Array<T> result(size);

    const auto range_internal_ = [&result](const usize start, const T init,
                                           const usize end, const T increment) {
        T value = init;
        for (usize index = start; index < end; ++index, value += increment) {
            result.array_[index] = value;
        }
    };

    if (::s_thread_count == 1 || size < ::s_threshold_size) {
        range_internal_(0, start_value, size, increment);
        return result;
    }

    const usize block = size / s_thread_count;
    const Ts b_increment = increment * block;

    std::vector<std::thread> thread_pool;
    T b_start = start_value;
    for (usize thread_i = 0; thread_i < s_thread_count - 1;
         ++thread_i, b_start += b_increment) {
        thread_pool.emplace_back(range_internal_, block * thread_i, b_start,
                                 block * (thread_i + 1), increment);
    }

    thread_pool.emplace_back(range_internal_, block * (s_thread_count - 1),
                             b_start, size, increment);

    for (auto &thread : thread_pool) {
        thread.join();
    }

    return result;
}

#endif
