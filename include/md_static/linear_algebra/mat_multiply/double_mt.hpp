#pragma once
#ifndef _MAT_MULTIPLY_DOUBLE_MT_HPP_
#define _MAT_MULTIPLY_DOUBLE_MT_HPP_

#include <immintrin.h>

#include <thread>
#include <vector>

#include "../../md_static_array/md_static_array.hpp"
#include "../../utility/alloc.hpp"
#include "../../utility/simd.hpp"

void mul_mt_internal(f64 *a, f64 *tb, f64 *c, i32 m, i32 n, i32 p,
                     i32 start_row, i32 end_row) {
    const i32 block_size = 128;
    const i32 remainder_cols = p & 3;
    const i32 remainder_rows = (end_row - start_row) & 3;
    const i32 remainder_vec = n & 15;
    // Compute block by block
    for (i32 iblock = start_row; iblock < end_row - remainder_rows;
         iblock += block_size) {
        // This will compute c tile by tile
        i32 ibound = iblock + block_size > end_row - remainder_rows
                         ? end_row - remainder_rows
                         : iblock + block_size;
        for (i32 jblock = 0; jblock < p - remainder_cols;
             jblock += block_size) {
            // now compute inner block_size x block_size matrices
            i32 jbound = jblock + block_size > p - remainder_cols
                             ? p - remainder_cols
                             : jblock + block_size;
            // Compute
            for (i32 i = iblock; i < ibound; i += 4) {
                // Process 4 values at a time
                for (i32 j = jblock; j < jbound; j += 4) {
                    // Accumulator:
                    // we compute 4x4 matrix at a time
                    f64x4 acc00 = F64x4::zero();
                    f64x4 acc01 = F64x4::zero();
                    f64x4 acc02 = F64x4::zero();
                    f64x4 acc03 = F64x4::zero();

                    f64x4 acc10 = F64x4::zero();
                    f64x4 acc11 = F64x4::zero();
                    f64x4 acc12 = F64x4::zero();
                    f64x4 acc13 = F64x4::zero();

                    f64x4 acc20 = F64x4::zero();
                    f64x4 acc21 = F64x4::zero();
                    f64x4 acc22 = F64x4::zero();
                    f64x4 acc23 = F64x4::zero();

                    f64x4 acc30 = F64x4::zero();
                    f64x4 acc31 = F64x4::zero();
                    f64x4 acc32 = F64x4::zero();
                    f64x4 acc33 = F64x4::zero();

                    // Loop over second axis of A and first axis of B
                    // Processing 4 values at a time, loop unrolled by 4,
                    // we get
                    for (i32 k = 0; k < n - remainder_vec; k += 16) {
                        auto avec = F64x4::fromptr(a + (i * n + k));

                        auto bvec00 = F64x4::fromptr(tb + (j * n + k));
                        auto bvec01 = F64x4::fromptr(tb + (j * n + k + 4));
                        auto bvec02 = F64x4::fromptr(tb + (j * n + k + 8));
                        auto bvec03 = F64x4::fromptr(tb + (j * n + k + 12));

                        auto bvec10 = F64x4::fromptr(tb + ((j + 1) * n + k));
                        auto bvec11 =
                            F64x4::fromptr(tb + ((j + 1) * n + k + 4));
                        auto bvec12 =
                            F64x4::fromptr(tb + ((j + 1) * n + k + 8));
                        auto bvec13 =
                            F64x4::fromptr(tb + ((j + 1) * n + k + 12));

                        auto bvec20 = F64x4::fromptr(tb + ((j + 2) * n + k));
                        auto bvec21 =
                            F64x4::fromptr(tb + ((j + 2) * n + k + 8));
                        auto bvec22 =
                            F64x4::fromptr(tb + ((j + 2) * n + k + 4));
                        auto bvec23 =
                            F64x4::fromptr(tb + ((j + 2) * n + k + 12));

                        auto bvec30 = F64x4::fromptr(tb + ((j + 3) * n + k));
                        auto bvec31 =
                            F64x4::fromptr(tb + ((j + 3) * n + k + 4));
                        auto bvec32 =
                            F64x4::fromptr(tb + ((j + 3) * n + k + 8));
                        auto bvec33 =
                            F64x4::fromptr(tb + ((j + 3) * n + k + 12));

                        acc00 = F64x4::fmadd(avec, bvec00, acc00);
                        acc01 = F64x4::fmadd(avec, bvec10, acc01);
                        acc02 = F64x4::fmadd(avec, bvec20, acc02);
                        acc03 = F64x4::fmadd(avec, bvec30, acc03);

                        avec = F64x4::fromptr(a + (i * n + k + 4));

                        acc00 = F64x4::fmadd(avec, bvec01, acc00);
                        acc01 = F64x4::fmadd(avec, bvec11, acc01);
                        acc02 = F64x4::fmadd(avec, bvec21, acc02);
                        acc03 = F64x4::fmadd(avec, bvec31, acc03);

                        avec = F64x4::fromptr(a + (i * n + k + 8));

                        acc00 = F64x4::fmadd(avec, bvec02, acc00);
                        acc01 = F64x4::fmadd(avec, bvec12, acc01);
                        acc02 = F64x4::fmadd(avec, bvec22, acc02);
                        acc03 = F64x4::fmadd(avec, bvec32, acc03);

                        avec = F64x4::fromptr(a + (i * n + k + 12));

                        acc00 = F64x4::fmadd(avec, bvec03, acc00);
                        acc01 = F64x4::fmadd(avec, bvec13, acc01);
                        acc02 = F64x4::fmadd(avec, bvec23, acc02);
                        acc03 = F64x4::fmadd(avec, bvec33, acc03);

                        /////////////////////////////////////////////////////////////////

                        avec = F64x4::fromptr(a + ((i + 1) * n + k + 12));

                        acc10 = F64x4::fmadd(avec, bvec03, acc10);
                        acc11 = F64x4::fmadd(avec, bvec13, acc11);
                        acc12 = F64x4::fmadd(avec, bvec23, acc12);
                        acc13 = F64x4::fmadd(avec, bvec33, acc13);

                        avec = F64x4::fromptr(a + ((i + 1) * n + k));

                        acc10 = F64x4::fmadd(avec, bvec00, acc10);
                        acc11 = F64x4::fmadd(avec, bvec10, acc11);
                        acc12 = F64x4::fmadd(avec, bvec20, acc12);
                        acc13 = F64x4::fmadd(avec, bvec30, acc13);

                        avec = F64x4::fromptr(a + ((i + 1) * n + k + 4));

                        acc10 = F64x4::fmadd(avec, bvec01, acc10);
                        acc11 = F64x4::fmadd(avec, bvec11, acc11);
                        acc12 = F64x4::fmadd(avec, bvec21, acc12);
                        acc13 = F64x4::fmadd(avec, bvec31, acc13);

                        avec = F64x4::fromptr(a + ((i + 1) * n + k + 8));

                        acc10 = F64x4::fmadd(avec, bvec02, acc10);
                        acc11 = F64x4::fmadd(avec, bvec12, acc11);
                        acc12 = F64x4::fmadd(avec, bvec22, acc12);
                        acc13 = F64x4::fmadd(avec, bvec32, acc13);

                        ///////////////////////////////////////////////////////////////////

                        avec = F64x4::fromptr(a + ((i + 2) * n + k + 8));

                        acc20 = F64x4::fmadd(avec, bvec02, acc20);
                        acc21 = F64x4::fmadd(avec, bvec12, acc21);
                        acc22 = F64x4::fmadd(avec, bvec22, acc22);
                        acc23 = F64x4::fmadd(avec, bvec32, acc23);

                        avec = F64x4::fromptr(a + ((i + 2) * n + k + 12));

                        acc20 = F64x4::fmadd(avec, bvec03, acc20);
                        acc21 = F64x4::fmadd(avec, bvec13, acc21);
                        acc22 = F64x4::fmadd(avec, bvec23, acc22);
                        acc23 = F64x4::fmadd(avec, bvec33, acc23);

                        avec = F64x4::fromptr(a + ((i + 2) * n + k));

                        acc20 = F64x4::fmadd(avec, bvec00, acc20);
                        acc21 = F64x4::fmadd(avec, bvec10, acc21);
                        acc22 = F64x4::fmadd(avec, bvec20, acc22);
                        acc23 = F64x4::fmadd(avec, bvec30, acc23);

                        avec = F64x4::fromptr(a + ((i + 2) * n + k + 4));

                        acc20 = F64x4::fmadd(avec, bvec01, acc20);
                        acc21 = F64x4::fmadd(avec, bvec11, acc21);
                        acc22 = F64x4::fmadd(avec, bvec21, acc22);
                        acc23 = F64x4::fmadd(avec, bvec31, acc23);

                        /////////////////////////////////////////////////////////////

                        avec = F64x4::fromptr(a + ((i + 3) * n + k + 4));

                        acc30 = F64x4::fmadd(avec, bvec01, acc30);
                        acc31 = F64x4::fmadd(avec, bvec11, acc31);
                        acc32 = F64x4::fmadd(avec, bvec21, acc32);
                        acc33 = F64x4::fmadd(avec, bvec31, acc33);

                        avec = F64x4::fromptr(a + ((i + 3) * n + k + 8));

                        acc30 = F64x4::fmadd(avec, bvec02, acc30);
                        acc31 = F64x4::fmadd(avec, bvec12, acc31);
                        acc32 = F64x4::fmadd(avec, bvec22, acc32);
                        acc33 = F64x4::fmadd(avec, bvec32, acc33);

                        avec = F64x4::fromptr(a + ((i + 3) * n + k + 12));

                        acc30 = F64x4::fmadd(avec, bvec03, acc30);
                        acc31 = F64x4::fmadd(avec, bvec13, acc31);
                        acc32 = F64x4::fmadd(avec, bvec23, acc32);
                        acc33 = F64x4::fmadd(avec, bvec33, acc33);

                        avec = F64x4::fromptr(a + ((i + 3) * n + k));

                        acc30 = F64x4::fmadd(avec, bvec00, acc30);
                        acc31 = F64x4::fmadd(avec, bvec10, acc31);
                        acc32 = F64x4::fmadd(avec, bvec20, acc32);
                        acc33 = F64x4::fmadd(avec, bvec30, acc33);
                    }

                    for (i32 k = n - remainder_vec; k < n; ++k) {
                        const auto tb0 = tb[j * n + k],
                                   tb1 = tb[(j + 1) * n + k],
                                   tb2 = tb[(j + 2) * n + k],
                                   tb3 = tb[(j + 3) * n + k];
                        const auto a0 = a[i * n + k], a1 = a[(i + 1) * n + k],
                                   a2 = a[(i + 2) * n + k],
                                   a3 = a[(i + 3) * n + k];

                        c[i * p + j] += a0 * tb0;
                        c[i * p + j + 1] += a0 * tb1;
                        c[i * p + j + 2] += a0 * tb2;
                        c[i * p + j + 3] += a0 * tb3;

                        c[(i + 1) * p + j] += a1 * tb0;
                        c[(i + 1) * p + j + 1] += a1 * tb1;
                        c[(i + 1) * p + j + 2] += a1 * tb2;
                        c[(i + 1) * p + j + 3] += a1 * tb3;

                        c[(i + 2) * p + j] += a3 * tb0;
                        c[(i + 2) * p + j + 1] += a3 * tb1;
                        c[(i + 2) * p + j + 2] += a3 * tb2;
                        c[(i + 2) * p + j + 3] += a3 * tb3;

                        c[(i + 3) * p + j] += a3 * tb0;
                        c[(i + 3) * p + j + 1] += a3 * tb1;
                        c[(i + 3) * p + j + 2] += a3 * tb2;
                        c[(i + 3) * p + j + 3] += a3 * tb3;
                    }

                    f64 ans[4] = {0, 0, 0, 0};
                    c[i * p + j] += F64x4::reduce_sum(acc00);
                    c[i * p + j + 1] += F64x4::reduce_sum(acc01);
                    c[i * p + j + 2] += F64x4::reduce_sum(acc02);
                    c[i * p + j + 3] += F64x4::reduce_sum(acc03);

                    F64x4::storeptr(ans, acc10);
                    c[(i + 1) * p + j] += ans[0] + ans[1] + ans[2] + ans[3];
                    F64x4::storeptr(ans, acc11);
                    c[(i + 1) * p + j + 1] += ans[0] + ans[1] + ans[2] + ans[3];
                    F64x4::storeptr(ans, acc12);
                    c[(i + 1) * p + j + 2] += ans[0] + ans[1] + ans[2] + ans[3];
                    F64x4::storeptr(ans, acc13);
                    c[(i + 1) * p + j + 3] += ans[0] + ans[1] + ans[2] + ans[3];

                    F64x4::storeptr(ans, acc20);
                    c[(i + 2) * p + j] += ans[0] + ans[1] + ans[2] + ans[3];
                    F64x4::storeptr(ans, acc21);
                    c[(i + 2) * p + j + 1] += ans[0] + ans[1] + ans[2] + ans[3];
                    F64x4::storeptr(ans, acc22);
                    c[(i + 2) * p + j + 2] += ans[0] + ans[1] + ans[2] + ans[3];
                    F64x4::storeptr(ans, acc23);
                    c[(i + 2) * p + j + 3] += ans[0] + ans[1] + ans[2] + ans[3];

                    F64x4::storeptr(ans, acc30);
                    c[(i + 3) * p + j] += ans[0] + ans[1] + ans[2] + ans[3];
                    F64x4::storeptr(ans, acc31);
                    c[(i + 3) * p + j + 1] += ans[0] + ans[1] + ans[2] + ans[3];
                    F64x4::storeptr(ans, acc32);
                    c[(i + 3) * p + j + 2] += ans[0] + ans[1] + ans[2] + ans[3];
                    F64x4::storeptr(ans, acc33);
                    c[(i + 3) * p + j + 3] += ans[0] + ans[1] + ans[2] + ans[3];
                }
                for (i32 j = p - remainder_cols; j < p; ++j) {
                    f64 ans0 = 0, ans1 = 0, ans2 = 0, ans3 = 0;
                    for (i32 k = 0; k < n; ++k) {
                        ans0 += a[i * n + k] * tb[j * n + k];
                        ans1 += a[(i + 1) * n + k] * tb[j * n + k];
                        ans2 += a[(i + 2) * n + k] * tb[j * n + k];
                        ans3 += a[(i + 3) * n + k] * tb[j * n + k];
                    }
                    c[i * p + j] = ans0;
                    c[(i + 1) * p + j] = ans1;
                    c[(i + 2) * p + j] = ans2;
                    c[(i + 3) * p + j] = ans3;
                }
            }
        }
    }
    for (i32 i = end_row - remainder_rows; i < end_row; ++i) {
        for (i32 j = 0; j < p; ++j) {
            f64 ans = 0;
            for (i32 k = 0; k < n; ++k) {
                ans += a[i * n + k] * tb[j * n + k];
            }
            c[i * p + j] = ans;
        }
    }
}

/**
 * @brief Multiply two matrices A and transposed B matrix
 * This is a single-thread operation.
 * @param a matrix A
 * @param tb Transposed matrix B
 * @param m first axis of A
 * @param n second axis of A | first axis of B
 * @param p second axis of B
 */
void mul_mt(f64 *a, f64 *tb, f64 *c, i32 m, i32 n, i32 p) {
    i32 rem = (m * p) & 3;

    // Initialize vector to zero
    for (usize index = 0; index < m * p - rem; index += 4) {
        F64x4::storeptr(c + index, F64x4::zero());
    }

    // Set remainder values to zero as well
    for (usize index = m * p - rem; index < m * p; ++index) {
        c[index] = 0;
    }

    std::vector<std::thread> threads;

    i32 clamped_thread_count = std::min((i32)::s_thread_count, m / 128);

    i32 total_rows_per_thread = m / clamped_thread_count;

    for (usize index = 0; index < clamped_thread_count - 1; ++index) {
        threads.emplace_back(std::thread(mul_mt_internal, a, tb, c, m, n, p,
                                         index * total_rows_per_thread,
                                         (index + 1) * total_rows_per_thread));
    }

    threads.emplace_back(
        std::thread(mul_mt_internal, a, tb, c, m, n, p,
                    (clamped_thread_count - 1) * total_rows_per_thread, m));

    for (auto &thread : threads) {
        thread.join();
    }
}

#endif
