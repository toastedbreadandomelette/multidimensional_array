#pragma once
#ifndef _EXP2_HPP_
#define _EXP2_HPP_
#include <cmath>

#include "./map.hpp"
#include "./md_static_array_utility.hpp"

template <typename T>
Array<double> MdArrayUtility::exp2(const Array<T> &values) {
    return MdArrayUtility::map<T>(
        values, [](const T &value) { return ::pow(2, value); });
}

template <typename T>
Array<double> MdArrayUtility::exp2(
    const Reference<T> &values) {
    return MdArrayUtility::exp2<T>(Array<T>(
        *values.__array_reference, values.offset, values.shp_offset));
}

#endif
