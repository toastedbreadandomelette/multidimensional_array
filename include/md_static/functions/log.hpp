#pragma once
#ifndef _LOG_HPP_
#define _LOG_HPP_
#include <cmath>

#include "./map.hpp"
#include "./md_static_array_utility.hpp"

template <typename T>
Array<T> MdArrayUtility::log(const Array<T> &values) {
    return MdArrayUtility::map<T>(values,
                                  [](const T &value) { return ::log(value); });
}

template <typename T>
Array<T> MdArrayUtility::log(const Reference<T> &values) {
    return MdArrayUtility::log<T>(Array<T>(
        *values.__array_reference, values.offset, values.shp_offset));
}

#endif
