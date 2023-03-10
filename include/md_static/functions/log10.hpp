#pragma once
#ifndef _LOG10_HPP_
#define _LOG10_HPP_
#include <cmath>

#include "./map.hpp"
#include "./md_static_array_utility.hpp"

template <typename _T>
MdStaticArray<_T> MdArrayUtility::log10(const MdStaticArray<_T> &__values) {
    return MdArrayUtility::map<_T>(
        __values, [](const _T &__value) { return ::log10(__value); });
}

template <typename _T>
MdStaticArray<_T> MdArrayUtility::log10(
    const MdStaticArrayReference<_T> &__values) {
    return MdArrayUtility::log10<_T>(MdStaticArray<_T>(
        *__values.__array_reference, __values.offset, __values.shp_offset));
}

#endif
