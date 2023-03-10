#pragma once
#ifndef _DET_HPP_
#define _DET_HPP_

#include "../../md_complex/md_complex.hpp"
#include "./md_linear_algebra.hpp"

template <typename _T>
clongdouble MdLinearAlgebra::c_det(const MdStaticArray<_T> &__2darray) {
    auto [L, U, P, sign] = MdLinearAlgebra::lu_decompose<_T>(__2darray);
    const size_t n = __2darray.get_shape()[0];
    clongdouble det = 1;
    for (size_t index = 0; index < n; ++index) {
        det *= U.__array[index * n + index];
    }

    return sign * det;
}

template <typename _T>
clongdouble MdLinearAlgebra::c_det(
    const MdStaticArrayReference<_T> &__2darray) {
    return MdLinearAlgebra::det(MdStaticArray<_T>(
        *__2darray.__array_reference, __2darray.offset, __2darray.shp_offset));
}

#endif
