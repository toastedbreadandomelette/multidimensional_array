#pragma once
#ifndef _DIFF_HPP_
#define _DIFF_HPP_

#include "./md_static_array_utility.hpp"

template <typename T>
MdStaticArray<T> MdArrayUtility::diff(const MdStaticArray<T> &ndarray,
                                      const size_t axis,
                                      const size_t thread_count) {
    if (axis == -1) {
        MdStaticArray<T> result(ndarray.get_size() - 1);
#pragma omp parallel for
        for (size_t index = 0; index < ndarray.get_size() - 1; ++index) {
            result.__array[index] =
                ndarray.__array[index + 1] - ndarray.__array[index];
        }

        return result;
    }

    if (axis >= ndarray.get_shape_size()) {
        throw std::runtime_error("Unknown axis " + std::to_string(axis) +
                                 " requested for diff.");
    }

    std::vector<size_t> overall_shape;

    for (size_t index = 0; index < ndarray.get_shape_size(); ++index) {
        overall_shape.emplace_back(ndarray.get_shape()[index] -
                                   (index == axis));
    }

    MdStaticArray<T> result(overall_shape, 0);

    const size_t diff_index = ndarray.skip_vec[axis];

    const size_t loop_times =
        (axis - 1 <= ndarray.get_shape_size() ? ndarray.skip_vec[axis - 1]
                                              : ndarray.get_size());

    auto __perform_diff_parallel = [&ndarray, &result, diff_index, loop_times](
                                       const size_t thread_number,
                                       const size_t total_threads) {
        const size_t start_value = loop_times * thread_number;
        size_t breaker = thread_number * diff_index;
        for (size_t index = start_value; index < ndarray.get_size();
             index += (total_threads * loop_times)) {
            for (size_t d_index = index;
                 d_index < index + loop_times - diff_index; ++d_index) {
                result.__array[d_index - breaker] =
                    ndarray.__array[d_index + diff_index] -
                    ndarray.__array[d_index];
            }
            breaker += (total_threads * diff_index);
        }
    };

    std::vector<std::thread> thread_pool;

    for (size_t index = 0; index < thread_count; ++index) {
        thread_pool.emplace_back(
            std::thread(__perform_diff_parallel, index, thread_count));
    }

    for (auto &thread : thread_pool) {
        thread.join();
    }

    return result;
}

template <typename T>
MdStaticArray<T> MdArrayUtility::diff(const MdStaticArrayReference<T> &ndarray,
                                      const size_t axis,
                                      const size_t thread_count) {
    return diff<T>(MdStaticArray<T>(*ndarray.__array_reference, ndarray.offset,
                                    ndarray.shp_offset),
                   axis, thread_count);
}

#endif