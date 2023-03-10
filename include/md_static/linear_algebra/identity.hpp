#pragma once
#ifndef _IDENTITY_HPP_
#define _IDENTITY_HPP_
#include "./md_linear_algebra.hpp"

template <typename _T>
MdStaticArray<_T> MdLinearAlgebra::identity(const size_t n) {
    MdStaticArray<_T> result({n, n}, 0);
#pragma omp parallel for
    for (size_t index = 0; index < result.get_size(); index += (n + 1)) {
        result.__array[index] = 1;
    }

    return result;
}

template <typename _T>
bool MdLinearAlgebra::is_identity(const MdStaticArray<_T> &__other) {
    if (__other.shp_size != 2 || __other.shape[0] != __other.shape[1]) {
        return false;
    }
    size_t _n = __other.shape[0];

    for (size_t index = 0; index < __other.get_size(); ++index) {
        if (__other.__array[index] > 1 || __other.__array[index] < 0) {
            return false;
        }
        if (__other.__array[index] && (index % (_n + 1) != 0)) {
            return false;
        }
        if (!__other.__array[index] && (index % (_n + 1) == 0)) {
            return false;
        }
    }

    return true;
}

template <typename _T>
bool MdLinearAlgebra::is_identity(const MdStaticArrayReference<_T> &__other) {
    return MdLinearAlgebra::is_identity(MdStaticArray<_T>(
        *__other.array_reference, __other.offset, __other.shp_offset));
}

#endif
