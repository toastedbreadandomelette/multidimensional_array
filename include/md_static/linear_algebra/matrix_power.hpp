#pragma once
#ifndef _MATRIX_POWER_HPP_
#define _MATRIX_POWER_HPP_

#include "./identity.hpp"
#include "./mat_multiply.hpp"
#include "./md_linear_algebra.hpp"

template <typename T, class T1>
MdStaticArray<T> MdLinearAlgebra::matrix_power(const MdStaticArray<T> &__matrix,
                                               size_t power) {
    if (__matrix.get_shape_size() != 2) {
        throw std::runtime_error(
            "Given input should be of dimension 2. Found dimension " +
            std::to_string(__matrix.get_shape_size()) + ".");
    }

    if (__matrix.get_shape()[0] != __matrix.get_shape()[1]) {
        throw std::runtime_error(
            "Given input matrix should be square. Found dimension (" +
            std::to_string(__matrix.get_shape()[0]) + ", " +
            std::to_string(__matrix.get_shape()[1]) + ").");
    }

    MdStaticArray<T> result =
                         MdLinearAlgebra::identity<T>(__matrix.get_shape()[0]),
                     __ocp = __matrix;

    while (power > 0) {
        if (power & 1) {
            result = MdLinearAlgebra::mat_multiply<T, T, T>(result, __ocp);
        }
        __ocp = MdLinearAlgebra::mat_multiply<T, T, T>(__ocp, __ocp);
        power >>= 1;
    }

    return result;
}

template <typename T, class T1>
MdStaticArray<T> MdLinearAlgebra::matrix_power(
    const MdStaticArrayReference<T> &__matrix, size_t power) {
    return MdLinearAlgebra::matrix_power<T>(
        MdStaticArray<T>(*__matrix.__array_reference, __matrix.offset,
                         __matrix.shp_offset),
        power);
}

#endif
