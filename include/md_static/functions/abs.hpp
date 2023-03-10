#pragma once
#ifndef _ABS_HPP_
#define _ABS_HPP_
#include <cmath>

#include "./map.hpp"
#include "./md_static_array_utility.hpp"

template <typename _T>
MdStaticArray<_T> MdArrayUtility::abs(const MdStaticArray<_T> &__values) {
    return MdArrayUtility::map<_T>(
        __values, [](const _T &value) { return std::abs(value); });
}

template <typename _T>
MdStaticArray<_T> MdArrayUtility::abs(
    const MdStaticArrayReference<_T> &__values) {
    return MdArrayUtility::abs<_T>(MdStaticArray<_T>(
        *__values.__array_reference, __values.offset, __values.shp_offset));
}

#endif
