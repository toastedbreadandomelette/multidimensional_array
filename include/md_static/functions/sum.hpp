#pragma once
#ifndef _SUM_HPP_
#define _SUM_HPP_
#include "./md_static_array_utility.hpp"
#include "./reduce.hpp"

template <typename T>
MdStaticArray<T> MdArrayUtility::sum(const MdStaticArray<T> &values,
                                     const T init, const int axis) {
    return reduce(
        values,
        [](const T prev_value, const T current_value) {
            return prev_value + current_value;
        },
        init, axis);
}

template <typename T>
MdStaticArray<T> MdArrayUtility::sum(const MdStaticArrayReference<T> &values,
                                     const T init, const int axis) {
    return sum(MdStaticArray<T>(*values.__array_reference, values.offset,
                                values.shp_offset),
               init, axis);
}

#endif
