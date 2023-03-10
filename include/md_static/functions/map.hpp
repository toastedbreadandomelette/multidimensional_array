#pragma once
#ifndef _MAP_HPP_
#define _MAP_HPP_
#include "./md_static_array_utility.hpp"

template <typename _T>
MdStaticArray<_T> MdArrayUtility::map(
    const MdStaticArray<_T> &__values,
    const std::function<_T(const _T &)> &function_exec) {
    const size_t size = __values.get_size();

    std::vector<size_t> shp;
    for (size_t index = 0; index < __values.get_shape_size(); ++index) {
        shp.emplace_back(__values.shape[index]);
    }

    MdStaticArray<_T> result(shp, 0);

    const uint8_t thread_count = ::s_thread_count;
    const size_t threshold_size = ::s_threshold_size;

    if (thread_count == 1 || size <= threshold_size) {
#pragma omp parallel for
        for (size_t index = 0; index < size; ++index) {
            result.__array[index] = function_exec(__values.__array[index]);
        }
    } else {
        std::vector<std::thread> st;
        st.reserve(thread_count);
        auto _add_int = [&result, &__values, &function_exec](const size_t start,
                                                             const size_t end) {
            for (size_t index = start; index < end; ++index) {
                result.__array[index] = function_exec(__values.__array[index]);
            }
        };

        const size_t block = size / thread_count;
        const uint8_t thread_but_one = thread_count - 1;
        for (int i = 0; i < thread_but_one; ++i) {
            st.emplace_back(std::thread(_add_int, block * i, block * (i + 1)));
        }

        st.emplace_back(std::thread(_add_int, block * thread_but_one, size));

        for (auto &th : st) {
            th.join();
        }
    }
    return result;
}

template <typename _T>
MdStaticArray<_T> MdArrayUtility::map(
    const MdStaticArrayReference<_T> &__values,
    const std::function<_T(const _T &)> &function_exec) {
    return map<_T>(MdStaticArray(*__values.__array_reference, __values.offset,
                                 __values.shp_offset),
                   function_exec);
}

#endif
