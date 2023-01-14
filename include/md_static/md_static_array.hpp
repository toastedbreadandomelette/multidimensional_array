#pragma once

#ifndef _MD_STATIC_ARRAY_HPP_
#define _MD_STATIC_ARRAY_HPP_
#include <algorithm>
#include <iostream>
#include <thread>
#include <vector>

#define EN_IF(C)    std::enable_if<C>::type
#define IS_ARITH(E) std::is_arithmetic<E>::value

#define OP_INTERNAL_MACRO(func)                                      \
    if constexpr (sizeof(_T1) > sizeof(_T)) {                        \
        if constexpr ((std::is_floating_point<_T>::value ||          \
                       std::is_floating_point<_T1>::value) &&        \
                      sizeof(_T1) == sizeof(float)) {                \
            return func(__other, static_cast<float>(0));             \
        } else if constexpr ((std::is_floating_point<_T>::value ||   \
                              std::is_floating_point<_T1>::value) && \
                             sizeof(_T1) == sizeof(double)) {        \
            return func(__other, static_cast<double>(0));            \
        } else if constexpr ((std::is_floating_point<_T>::value ||   \
                              std::is_floating_point<_T1>::value) && \
                             sizeof(_T1) == sizeof(long double)) {   \
            return func(__other, static_cast<long double>(0));       \
        } else {                                                     \
            return func(__other, (_T1)0);                            \
        }                                                            \
    } else {                                                         \
        if constexpr ((std::is_floating_point<_T>::value ||          \
                       std::is_floating_point<_T1>::value) &&        \
                      sizeof(_T1) == sizeof(float)) {                \
            return func(__other, static_cast<double>(0));            \
        } else if constexpr ((std::is_floating_point<_T>::value ||   \
                              std::is_floating_point<_T1>::value) && \
                             sizeof(_T1) == sizeof(double)) {        \
            return func(__other, static_cast<double>(0));            \
        } else if constexpr ((std::is_floating_point<_T>::value ||   \
                              std::is_floating_point<_T1>::value) && \
                             sizeof(_T1) == sizeof(long double)) {   \
            return func(__other, static_cast<long double>(0));       \
        } else if constexpr ((std::is_floating_point<_T>::value ||   \
                              std::is_floating_point<_T1>::value) && \
                             sizeof(_T1) < 4) {                      \
            return func(__other, static_cast<float>(0));             \
        } else {                                                     \
            return func(__other, static_cast<_T>(0));                \
        }                                                            \
    }

static size_t s_threshold_size = 10000000;
static uint8_t s_thread_count = 16;

template <typename _T>
class MdStaticArray {
    void init_shape(const size_t *_shape, const size_t _shp_size) {
        if (shape) {
            free(shape);
            skip_vec = nullptr;
        }
        if (skip_vec) {
            free(skip_vec);
            skip_vec = nullptr;
        }
        shp_size = _shp_size;
        shape = static_cast<size_t *>(malloc(shp_size * sizeof(size_t)));
        skip_vec = static_cast<size_t *>(malloc(shp_size * sizeof(size_t)));
        shape[shp_size - 1] = _shape[shp_size - 1];
        skip_vec[shp_size - 1] = 1;
        for (int16_t i = shp_size - 2; i >= 0; --i) {
            shape[i] = _shape[i];
            skip_vec[i] = _shape[i + 1] * skip_vec[i + 1];
        }
    }

    void init_shape(const size_t size) {
        if (shape) {
            free(shape);
            skip_vec = nullptr;
        }
        if (skip_vec) {
            free(skip_vec);
            skip_vec = nullptr;
        }
        shape = static_cast<size_t *>(malloc(sizeof(size_t)));
        skip_vec = static_cast<size_t *>(malloc(sizeof(size_t)));
        shape[0] = size;
        skip_vec[0] = 1;
        shp_size = 1;
    }

    /**
     * @brief create a temporary reference for MdStaticArray::reference.
     * This will not accessible to user.
     */
    MdStaticArray(const MdStaticArray<_T> &__other, const size_t offset,
                  const uint16_t shp_offset)
        : shape(nullptr), skip_vec(nullptr) {
        __array = &__other.__array[offset];
        shape = &__other.shape[shp_offset];
        skip_vec = &__other.skip_vec[shp_offset];
        shp_size = __other.shp_size - shp_offset;
        __size = __other.__size;
        uint16_t index = 0;
        while (index < shp_offset) {
            __size /= __other.shape[index++];
        }
        dont_free = true;
    }

    bool dont_free = false;
    _T *__array;
    size_t *shape;
    size_t *skip_vec;
    size_t __size;
    uint16_t shp_size;

 public:
    class reference;
    template <typename _T1>
    friend class MdStaticArray<_T1>::reference;

    friend struct MdArrayUtility;
    friend struct MdLinearAlgebra;

    friend std::ostream &operator<<(
        std::ostream &op, const typename MdStaticArray<_T>::reference &ot);

    static void set_thread_count(const uint8_t value);

    static void set_threshold_size(const size_t size);

    void init_array(const size_t size) {
        if constexpr (std::is_fundamental<_T>::value) {
#ifdef _WIN32
            __array = static_cast<_T *>(_aligned_malloc(size * sizeof(_T), 64));
#else
            __array = static_cast<_T *>(aligned_alloc(64, size * sizeof(_T)));
#endif
        } else {
            __array = new _T[size];
        }
        __size = size;
    }

    explicit MdStaticArray(const size_t size)
        : shape(nullptr), skip_vec(nullptr) {
        init_array(size);
        init_shape(size);
    }

    MdStaticArray(const size_t size, const _T &value)
        : shape(nullptr), skip_vec(nullptr) {
        init_array(size);
        init_shape(size);

        if (size > s_threshold_size) {
#pragma omp parallel for
            for (size_t index = 0; index < size; ++index) {
                __array[index] = value;
            }
        } else {
            for (size_t index = 0; index < size; ++index) {
                __array[index] = value;
            }
        }
    }

    MdStaticArray(const std::vector<size_t> &_shape, const _T &value)
        : shape(nullptr), skip_vec(nullptr) {
        size_t overall_size = 1;
        for (auto &dim : _shape) {
            overall_size *= dim;
        }
        init_array(overall_size);
        init_shape(_shape.data(), _shape.size());

        if (__size > s_threshold_size) {
#pragma omp parallel for
            for (size_t index = 0; index < __size; ++index) {
                __array[index] = value;
            }
        } else {
            for (size_t index = 0; index < __size; ++index) {
                __array[index] = value;
            }
        }
    }

    MdStaticArray() : shape(nullptr), skip_vec(nullptr) { __array = nullptr; }

    explicit MdStaticArray(const std::vector<_T> &__other)
        : shape(nullptr), skip_vec(nullptr) {
        __size = __other.size();
        init_array(__size);
        init_shape(__size);

        if (__size > s_threshold_size) {
#pragma omp parallel for
            for (size_t index = 0; index < __size; ++index) {
                __array[index] = __other[index];
            }
        } else {
            for (size_t index = 0; index < __size; ++index) {
                __array[index] = __other[index];
            }
        }
    }

    MdStaticArray(const MdStaticArray &__other)
        : shape(nullptr), skip_vec(nullptr) {
        init_array(__other.get_size());
        const auto shp = __other.get_shape();
        init_shape(shp, __other.shp_size);

        if (__size > s_threshold_size) {
#pragma omp parallel for
            for (size_t index = 0; index < __size; ++index) {
                __array[index] = __other.__array[index];
            }
        } else {
            for (size_t index = 0; index < __size; ++index) {
                __array[index] = __other.__array[index];
            }
        }
    }

    MdStaticArray(MdStaticArray<_T> &&__other) {
        __array = __other.__array;
        __other.__array = nullptr;

        __size = __other.__size;
        shp_size = __other.shp_size;
        shape = __other.shape;
        __other.shape = nullptr;

        skip_vec = __other.skip_vec;
        __other.skip_vec = nullptr;

        dont_free = __other.dont_free;
    }

    MdStaticArray &operator=(MdStaticArray<_T> &&__other) {
        __array = __other.__array;
        __other.__array = nullptr;

        __size = __other.__size;
        shp_size = __other.shp_size;
        shape = __other.shape;
        __other.shape = nullptr;

        skip_vec = __other.skip_vec;
        __other.skip_vec = nullptr;

        dont_free = __other.dont_free;

        return *this;
    }

    /**
     * @brief Assigning reference to the newly created array
     */
    MdStaticArray<_T>(const reference &__other);

    /**
     * @brief Casting operator
     */
    inline _T operator()() const { return __array[0]; }

    /**
     * @brief Casting operator
     */
    inline operator _T() const {
        if (__size > 1) {
            throw std::runtime_error(
                "Value casted should be a single element, found with size: " +
                std::to_string(__size));
        }
        return __array[0];
    }

    /**
     * @brief Casting operator
     */
    template <typename _T1>
    operator MdStaticArray<_T1>() const {
        MdStaticArray<_T1> __result(__size);
        __result.init_shape(shape, shp_size);

#pragma omp parallel for
        for (size_t index = 0; index < __size; ++index) {
            __result.__array[index] = __array[index];
        }

        return __result;
    }

    /**
     * @brief Assignment operator (direct vector assignment)
     */
    MdStaticArray &operator=(const std::vector<_T> __other) {
        skip_vec = nullptr;
        shape = nullptr;
        __size = __other.size();
        init_array(__size);
        init_shape(__size);

#pragma omp parallel for
        for (size_t index = 0; index < __size; ++index) {
            __array[index] = __other[index];
        }
        return *this;
    }

    /**
     * @brief Assignment operator
     */
    MdStaticArray &operator=(const MdStaticArray &__other) {
        skip_vec = nullptr;
        shape = nullptr;
        __size = __other.get_size();
        init_array(__size);
        const auto shp = __other.get_shape();
        init_shape(shp, __other.shp_size);

#pragma omp parallel for
        for (size_t index = 0; index < __other.get_size(); ++index) {
            __array[index] = __other.__array[index];
        }
        return *this;
    }

    /**
     * @brief Assigning reference to the newly created array
     */
    MdStaticArray<_T> &operator=(const reference &__other);

    /**
     * @brief Destructor for array
     */
    ~MdStaticArray() {
        if (!dont_free) {
            if (__array != nullptr) {
                if constexpr (std::is_fundamental<_T>::value) {
#ifdef _WIN32
                    _aligned_free(__array);
#else
                    free(__array);
#endif
                } else {
                    delete[] __array;
                }
            }

            if (shape != nullptr) {
                free(shape);
            }
            if (skip_vec != nullptr) {
                free(skip_vec);
            }
        }
    }

    /**
     * @brief Get shape of the object
     */
    inline size_t *get_shape() const { return shape; }

    /**
     * @brief Get shape length
     * @param void
     * @returns uint16_t shape of the array
     */
    inline uint16_t get_shape_size() const { return shp_size; }

    /**
     * @brief Check whether arrays have same structure
     * @param __other array to compare
     * @return true if they have same structure, else false
     */
    template <typename _T1>
    bool is_same_shape(const MdStaticArray<_T1> &__other) const {
        if (shp_size != __other.shp_size) {
            return false;
        }

        if (get_size() != __other.get_size()) {
            return false;
        }

        for (size_t index = 0; index < shp_size; ++index) {
            if (shape[index] != __other.shape[index]) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Add function, currently using threads
     * @param __other other array (might be of different type)
     * @returns new array of current type
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __add_internal(const MdStaticArray<_T1> &__other,
                                      const _T2) const;

    /**
     * @brief Add function, currently using threads
     * @param __other other integer (might be of different type)
     * @returns new array of current type
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __add_iinternal(const _T1 &__other, const _T2) const;

    /**
     * @brief Subtract function, currently using threads
     * @param __other other array (might be of different type)
     * @returns new array of current type
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __sub_internal(const MdStaticArray<_T1> &,
                                      const _T2) const;

    /**
     * @brief Subtract function, currently using threads
     * @param __other other integer (might be of different type)
     * @returns new array of current type
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __sub_iinternal(const _T1 &, const _T2) const;

    /**
     * @brief Subtract function, currently using threads
     * @param __other other integer (might be of different type)
     * @returns new array of current type
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __sub_iointernal(const _T1 &, const _T2) const;

    /**
     * @brief Multiplication function, currently using threads
     * @param __other other array (might be of different type)
     * @returns new array of current type
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __mul_internal(const MdStaticArray<_T1> &,
                                      const _T2) const;

    /**
     * @brief Multiplication function, currently using threads
     * @param __other other integer (might be of different type)
     * @returns new array of current type
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __mul_iinternal(const _T1 &__other, const _T2) const;

    /**
     * @brief Division function, currently using threads
     * @param __other other array (might be of different type)
     * @returns new array of current type
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __div_internal(const MdStaticArray<_T1> &,
                                      const _T2) const;

    /**
     * @brief Division function, currently using threads
     * @param __other other integer (might be of different type)
     * @returns new array of current type
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __div_iinternal(const _T1 &__other, const _T2) const;

    /**
     * @brief Modulo function, currently using threads
     * @param __other other integer (might be of different type)
     * @returns new array of current type
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __div_iointernal(const _T1 &__other, const _T2) const;

    /**
     * @brief Modulo function, currently using threads
     * @param __other other array (might be of different type)
     * @returns new array of current type
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __mod_internal(const MdStaticArray<_T1> &,
                                      const _T2) const;

    /**
     * @brief Bitwise AND function between two arrays
     * @param other other array to perform bitwise and
     * @returns new array
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __and_bit_internal(const MdStaticArray<_T1> &__other,
                                          const _T2) const;

    /**
     * @brief Bitwise AND function between two arrays
     * @param other other array to perform bitwise and
     * @returns new array
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __and_bit_iinternal(const _T1 &__other, const _T2) const;

    /**
     * @brief Bitwise OR function between two arrays
     * @param other other array to perform bitwise and
     * @returns new array
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __or_bit_internal(const MdStaticArray<_T1> &__other,
                                         const _T2) const;

    /**
     * @brief Bitwise OR function between two arrays
     * @param other other array to perform bitwise and
     * @returns new array
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __or_bit_iinternal(const _T1 &__other, const _T2) const;

    /**
     * @brief Bitwise XOR function between two arrays
     * @param other other array to perform bitwise and
     * @returns new array
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __xor_bit_internal(const MdStaticArray<_T1> &__other,
                                          const _T2) const;

    /**
     * @brief Bitwise XOR function between two arrays
     * @param other other array to perform bitwise and
     * @returns new array
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __xor_bit_iinternal(const _T1 &__other, const _T2) const;

    /**
     * @brief Bitwise left shift function between two arrays
     * @param other other array to perform bitwise and
     * @returns new array
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __lshft_bit_internal(const MdStaticArray<_T1> &__other,
                                            const _T2) const;

    /**
     * @brief Bitwise left shift function between two arrays
     * @param other other array to perform bitwise and
     * @returns new array
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __lshft_bit_iinternal(const _T1 &__other,
                                             const _T2) const;

    /**
     * @brief Bitwise left shift function between two arrays, but here
     * __other is shifted using data in array
     * @param other other array to perform bitwise and
     * @returns new array
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __lshft_bit_iointernal(const _T1 &__other,
                                              const _T2) const;

    /**
     * @brief Bitwise right shift function between two arrays
     * @param other other array to perform bitwise and
     * @returns new array
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __rshft_bit_internal(const MdStaticArray<_T1> &__other,
                                            const _T2) const;

    /**
     * @brief Bitwise right shift function between two arrays
     * @param other other array to perform bitwise and
     * @returns new array
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __rshft_bit_iinternal(const _T1 &__other,
                                             const _T2) const;

    /**
     * @brief Bitwise right shift function between two arrays, but here
     * __other is shifted using data in array
     * @param other other array to perform bitwise and
     * @returns new array
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __rshft_bit_iointernal(const _T1 &__other,
                                              const _T2) const;

    /**
     * @brief Division function, currently using threads
     * @param __other other integer (might be of different type)
     * @returns new array of current type
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __mod_iinternal(const _T1 &__other, const _T2) const;

    /**
     * @brief Modulo function, currently using threads, but the operand is to be
     * modded
     * @param __other other integer (might be of different type)
     * @returns new array of current type
     */
    template <typename _T1, typename _T2>
    MdStaticArray<_T2> __mod_iointernal(const _T1 &__other, const _T2) const;

    /**
     * @brief Add to self, using multi-threading
     * @param __other other vector to add
     * @returns new array
     */
    template <typename _T1>
    void __add_self_internal(const MdStaticArray<_T1> &__other);

    /**
     * @brief Subtract to self, using multi-threading
     * @param __other other vector to add
     * @returns new array
     */
    template <typename _T1>
    void __sub_self_internal(const MdStaticArray<_T1> &__other);

    /**
     * @brief Multiply to self, using multi-threading
     * @param __other other vector to add
     * @returns new array
     */
    template <typename _T1>
    void __mul_self_internal(const MdStaticArray<_T1> &__other);

    /**
     * @brief Divide to self, using multi-threading
     * @param __other other vector to add
     * @returns new array
     */
    template <typename _T1>
    void __div_self_internal(const MdStaticArray<_T1> &__other);

    /**
     * @brief Modulus to self, using multi-threading
     * @param __other other vector to add
     * @returns new array
     */
    template <typename _T1>
    void __mod_self_internal(const MdStaticArray<_T1> &__other);

    /**
     * @brief Add to self, using multi-threading
     * @param __other other vector to add
     * @returns new array
     */
    template <typename _T1>
    void __add_self_iinternal(const _T1 &__other);

    /**
     * @brief Subtract to self, using multi-threading
     * @param __other other vector to add
     * @returns new array
     */
    template <typename _T1>
    void __sub_self_iinternal(const _T1 &__other);

    /**
     * @brief Multiply to self, using multi-threading
     * @param __other other vector to add
     * @returns new array
     */
    template <typename _T1>
    void __mul_self_iinternal(const _T1 &__other);

    /**
     * @brief Divide to self, using multi-threading
     * @param __other other vector to add
     * @returns new array
     */
    template <typename _T1>
    void __div_self_iinternal(const _T1 &__other);

    /**
     * @brief Modulo to self, using multi-threading
     * @param __other other vector to add
     * @returns new array
     */
    template <typename _T1>
    void __mod_self_iinternal(const _T1 &__other);

    /**
     * @brief Bitwise and to self, using multi-threading
     * @param __other other vector to add
     * @returns new array
     */
    template <typename _T1>
    void __and_bit_self_internal(const MdStaticArray<_T1> &__other);

    /**
     * @brief Bitwise and to self, using multi-threading
     * @param __other other integer to perform bitwise AND
     * @returns new array
     */
    template <typename _T1>
    void __and_bit_self_iinternal(const _T1 &__other);

    /**
     * @brief Bitwise OR to self, using multi-threading
     * @param __other other vector to perform bitwise OR
     * @returns new array
     */
    template <typename _T1>
    void __or_bit_self_internal(const MdStaticArray<_T1> &__other);

    /**
     * @brief Bitwise OR to self, using multi-threading
     * @param __other other vector to perform bitwise OR
     * @returns new array
     */
    template <typename _T1>
    void __or_bit_self_iinternal(const _T1 &__other);

    /**
     * @brief Bitwise XOR to self, using multi-threading
     * @param __other other vector to perform bitwise XOR
     * @returns new array
     */
    template <typename _T1>
    void __xor_bit_self_internal(const MdStaticArray<_T1> &__other);

    /**
     * @brief Bitwise XOR to self, using multi-threading
     * @param __other other vector to perform bitwise XOR
     * @returns new array
     */
    template <typename _T1>
    void __xor_bit_self_iinternal(const _T1 &__other);

    /**
     * @brief Left shift to self, using multi-threading
     * @param __other other vector to perform bitwise XOR
     * @returns new array
     */
    template <typename _T1>
    void __lshft_bit_self_internal(const MdStaticArray<_T1> &__other);

    /**
     * @brief Left shift to self, using multi-threading
     * @param __other other vector to perform bitwise XOR
     * @returns new array
     */
    template <typename _T1>
    void __lshft_bit_self_iinternal(const _T1 &__other);

    /**
     * @brief Right shift to self, using multi-threading
     * @param __other other vector to perform bitwise XOR
     * @returns new array
     */
    template <typename _T1>
    void __rshft_bit_self_internal(const MdStaticArray<_T1> &__other);

    /**
     * @brief Right shift to self, using multi-threading
     * @param __other other vector to perform bitwise XOR
     * @returns new array
     */
    template <typename _T1>
    void __rshft_bit_self_iinternal(const _T1 &__other);

    template <typename _T1>
    MdStaticArray<bool> __comp_eq_internal(
        const MdStaticArray<_T1> &__other) const;

    template <typename _T1>
    MdStaticArray<bool> __comp_g_internal(
        const MdStaticArray<_T1> &__other) const;

    template <typename _T1>
    MdStaticArray<bool> __comp_geq_internal(
        const MdStaticArray<_T1> &__other) const;

    template <typename _T1>
    MdStaticArray<bool> __comp_l_internal(
        const MdStaticArray<_T1> &__other) const;

    template <typename _T1>
    MdStaticArray<bool> __comp_leq_internal(
        const MdStaticArray<_T1> &__other) const;

    template <typename _T1>
    MdStaticArray<bool> __comp_neq_internal(
        const MdStaticArray<_T1> &__other) const;

    template <typename _T1>
    MdStaticArray<bool> __comp_eq_iinternal(const _T1 &__other) const;

    template <typename _T1>
    MdStaticArray<bool> __comp_g_iinternal(const _T1 &__other) const;

    template <typename _T1>
    MdStaticArray<bool> __comp_geq_iinternal(const _T1 &__other) const;

    template <typename _T1>
    MdStaticArray<bool> __comp_l_iinternal(const _T1 &__other) const;

    template <typename _T1>
    MdStaticArray<bool> __comp_leq_iinternal(const _T1 &__other) const;

    template <typename _T1>
    MdStaticArray<bool> __comp_neq_iinternal(const _T1 &__other) const;

    template <typename _T1>
    inline auto operator+(const _T1 &__other) const {
        OP_INTERNAL_MACRO(__add_iinternal)
    }

    template <typename _T1>
    inline auto operator+(const MdStaticArray<_T1> &__other) const {
        OP_INTERNAL_MACRO(__add_internal)
    }

    template <typename _T1>
    inline auto operator-(const _T1 &__other) const {
        OP_INTERNAL_MACRO(__sub_iinternal)
    }

    template <typename _T1>
    inline auto operator-(const MdStaticArray<_T1> &__other) const {
        OP_INTERNAL_MACRO(__sub_internal)
    }

    template <typename _T1>
    inline auto operator*(const MdStaticArray<_T1> &__other) const {
        OP_INTERNAL_MACRO(__mul_internal)
    }

    template <typename _T1>
    inline auto operator*(const _T1 &__other) const {
        OP_INTERNAL_MACRO(__mul_iinternal)
    }

    template <typename _T1>
    inline auto operator/(const _T1 &__other) const {
        OP_INTERNAL_MACRO(__div_iinternal)
    }

    template <typename _T1>
    inline auto operator/(const MdStaticArray<_T1> &__other) const {
        OP_INTERNAL_MACRO(__div_internal)
    }

    template <typename _T1>
    inline auto operator%(const _T1 &__other) const {
        OP_INTERNAL_MACRO(__mod_iinternal)
    }

    template <typename _T1>
    inline auto operator%(const MdStaticArray<_T1> &__other) const {
        OP_INTERNAL_MACRO(__mod_internal)
    }

    template <typename _T1>
    inline auto operator&(const MdStaticArray<_T1> &__other) const {
        OP_INTERNAL_MACRO(__and_bit_internal)
    }

    template <typename _T1>
    inline auto operator&(const _T1 &__other) const {
        OP_INTERNAL_MACRO(__and_bit_iinternal)
    }

    template <typename _T1>
    inline auto operator|(const MdStaticArray<_T1> &__other) const {
        OP_INTERNAL_MACRO(__or_bit_internal)
    }

    template <typename _T1>
    inline auto operator|(const _T1 &__other) const {
        OP_INTERNAL_MACRO(__or_bit_iinternal)
    }

    template <typename _T1>
    inline auto operator^(const MdStaticArray<_T1> &__other) const {
        OP_INTERNAL_MACRO(__xor_bit_internal)
    }

    template <typename _T1>
    inline auto operator^(const _T1 &__other) const {
        OP_INTERNAL_MACRO(__xor_bit_iinternal)
    }

    template <typename _T1>
    inline auto operator<<(const MdStaticArray<_T1> &__other) const {
        OP_INTERNAL_MACRO(__lshft_bit_internal)
    }

    template <typename _T1>
    inline auto operator<<(const _T1 &__other) const {
        OP_INTERNAL_MACRO(__lshft_bit_iinternal)
    }

    template <typename _T1>
    inline auto operator>>(const MdStaticArray<_T1> &__other) const {
        OP_INTERNAL_MACRO(__rshft_bit_internal)
    }

    template <typename _T1>
    inline auto operator>>(const _T1 &__other) const {
        OP_INTERNAL_MACRO(__rshft_bit_iinternal)
    }

    template <typename _T1>
    inline MdStaticArray &operator+=(const MdStaticArray<_T1> &__other) {
        __add_self_internal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray &operator-=(const MdStaticArray<_T1> &__other) {
        __sub_self_internal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray &operator*=(const MdStaticArray<_T1> &__other) {
        __mul_self_internal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray &operator/=(const MdStaticArray<_T1> &__other) {
        __div_self_internal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray &operator%=(const MdStaticArray<_T1> &__other) {
        __mod_self_internal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray &operator+=(const _T1 &__other) {
        __add_self_iinternal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray &operator-=(const _T1 &__other) {
        __sub_self_ionternal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray &operator*=(const _T1 &__other) {
        __mul_self_iinternal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray &operator/=(const _T1 &__other) {
        __div_self_iinternal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray &operator%=(const _T1 &__other) {
        __mod_self_iinternal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray &operator<<=(const MdStaticArray<_T1> &__other) {
        __lshft_bit_self_internal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray &operator<<=(const _T1 &__other) {
        __lshft_bit_self_iinternal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray &operator>>=(const MdStaticArray<_T1> &__other) {
        __rshft_bit_self_internal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray &operator>>=(const _T1 &__other) {
        __rshft_bit_self_iinternal(__other);
        return *this;
    }

    template <typename _T1>
    inline MdStaticArray<bool> operator==(const MdStaticArray<_T1> &__other) {
        return __comp_eq_internal(__other);
    }

    template <typename _T1>
    inline MdStaticArray<bool> operator>(const MdStaticArray<_T1> &__other) {
        return __comp_g_internal(__other);
    }

    template <typename _T1>
    inline MdStaticArray<bool> operator>=(const MdStaticArray<_T1> &__other) {
        return __comp_geq_internal(__other);
    }

    template <typename _T1>
    inline MdStaticArray<bool> operator<(const MdStaticArray<_T1> &__other) {
        return __comp_l_internal(__other);
    }

    template <typename _T1>
    inline MdStaticArray<bool> operator<=(const MdStaticArray<_T1> &__other) {
        return __comp_leq_internal(__other);
    }

    template <typename _T1>
    inline MdStaticArray<bool> operator!=(const MdStaticArray<_T1> &__other) {
        return __comp_neq_internal(__other);
    }

    template <typename _T1>
    inline MdStaticArray<bool> operator==(const _T1 &__other) {
        return __comp_eq_iinternal(__other);
    }

    template <typename _T1>
    inline MdStaticArray<bool> operator>(const _T1 &__other) {
        return __comp_g_iinternal(__other);
    }

    template <typename _T1>
    inline MdStaticArray<bool> operator>=(const _T1 &__other) {
        return __comp_geq_iinternal(__other);
    }

    template <typename _T1>
    inline MdStaticArray<bool> operator<(const _T1 &__other) {
        return __comp_l_iinternal(__other);
    }

    template <typename _T1>
    inline MdStaticArray<bool> operator<=(const _T1 &__other) {
        return __comp_leq_iinternal(__other);
    }

    template <typename _T1>
    inline MdStaticArray<bool> operator!=(const _T1 &__other) {
        return __comp_neq_iinternal(__other);
    }

    // To do: create a reference for multi-dimensional arrays.
    inline reference operator[](const size_t index) const {
        if (index >= shape[0]) {
            throw std::runtime_error(
                "Index out of bounds while accessing index " +
                std::to_string(index) + " ( >= " + std::to_string(shape[0]) +
                ")");
        }
        return reference(*this, index * skip_vec[0]);
    }

    inline size_t get_size() const { return __size; }
};

#include "./md_static_array_op.hpp"

#define OP_INTERNAL_MACRO_EXT(func)                                  \
    if constexpr (sizeof(_T1) > sizeof(_T2)) {                       \
        if constexpr ((std::is_floating_point<_T2>::value ||         \
                       std::is_floating_point<_T1>::value) &&        \
                      sizeof(_T1) == sizeof(float)) {                \
            return first.func(__other, static_cast<float>(0));       \
        } else if constexpr ((std::is_floating_point<_T2>::value ||  \
                              std::is_floating_point<_T1>::value) && \
                             sizeof(_T1) == sizeof(double)) {        \
            return first.func(__other, static_cast<double>(0));      \
        } else if constexpr ((std::is_floating_point<_T2>::value ||  \
                              std::is_floating_point<_T1>::value) && \
                             sizeof(_T1) == sizeof(long double)) {   \
            return first.func(__other, static_cast<long double>(0)); \
        } else {                                                     \
            return first.func(__other, (_T1)0);                      \
        }                                                            \
    } else {                                                         \
        if constexpr ((std::is_floating_point<_T2>::value ||         \
                       std::is_floating_point<_T1>::value) &&        \
                      sizeof(_T1) == sizeof(float)) {                \
            return first.func(__other, static_cast<double>(0));      \
        } else if constexpr ((std::is_floating_point<_T2>::value ||  \
                              std::is_floating_point<_T1>::value) && \
                             sizeof(_T1) == sizeof(double)) {        \
            return first.func(__other, static_cast<double>(0));      \
        } else if constexpr ((std::is_floating_point<_T2>::value ||  \
                              std::is_floating_point<_T1>::value) && \
                             sizeof(_T1) == sizeof(long double)) {   \
            return first.func(__other, static_cast<long double>(0)); \
        } else if constexpr ((std::is_floating_point<_T2>::value ||  \
                              std::is_floating_point<_T1>::value) && \
                             sizeof(_T1) < sizeof(float)) {          \
            return first.func(__other, static_cast<float>(0));       \
        } else {                                                     \
            return first.func(__other, static_cast<_T2>(0));         \
        }                                                            \
    }

template <typename _T>
void MdStaticArray<_T>::set_thread_count(const uint8_t value) {
    s_thread_count = value;
}

template <typename _T>
void MdStaticArray<_T>::set_threshold_size(const size_t value) {
    s_threshold_size = value;
}

template <typename _T1, typename _T2, class = typename EN_IF(IS_ARITH(_T1))>
inline auto operator+(const _T1 &__other, const MdStaticArray<_T2> &first) {
    return first + __other;
}

template <typename _T1, typename _T2, class = typename EN_IF(IS_ARITH(_T1))>
inline auto operator-(const _T1 &__other, const MdStaticArray<_T2> &first) {
    OP_INTERNAL_MACRO_EXT(__sub_iointernal)
}

template <typename _T1, typename _T2, class = typename EN_IF(IS_ARITH(_T1))>
inline auto operator*(const _T1 &__other, const MdStaticArray<_T2> &first) {
    return first * __other;
}

template <typename _T1, typename _T2, class = typename EN_IF(IS_ARITH(_T1))>
inline auto operator/(const _T1 &__other, const MdStaticArray<_T2> &first) {
    OP_INTERNAL_MACRO_EXT(__div_iointernal)
}

template <typename _T1, typename _T2, class = typename EN_IF(IS_ARITH(_T1))>
inline auto operator%(const _T1 &__other, const MdStaticArray<_T2> &first) {
    OP_INTERNAL_MACRO_EXT(__mod_iointernal)
}

template <typename _T1, typename _T2>
inline auto operator==(const _T1 &__other, const MdStaticArray<_T2> &first) {
    return first.__comp_eq_iinternal(__other);
}

template <typename _T1, typename _T2>
inline auto operator!=(const _T1 &__other, const MdStaticArray<_T2> &first) {
    return first.__comp_neq_iinternal(__other);
}

template <typename _T1, typename _T2>
inline auto operator>(const _T1 &__other, const MdStaticArray<_T2> &first) {
    return first.__comp_g_iinternal(__other);
}

template <typename _T1, typename _T2>
inline auto operator<(const _T1 &__other, const MdStaticArray<_T2> &first) {
    return first.__comp_l_iinternal(__other);
}

template <typename _T1, typename _T2>
inline auto operator<=(const _T1 &__other, const MdStaticArray<_T2> &first) {
    return first.__comp_leq_iinternal(__other);
}

template <typename _T1, typename _T2>
inline auto operator>=(const _T1 &__other, const MdStaticArray<_T2> &first) {
    return first.__comp_geq_iinternal(__other);
}

template <typename _T1, typename _T2, class = typename EN_IF(IS_ARITH(_T1))>
inline auto operator&(const _T1 &__other, const MdStaticArray<_T2> &first) {
    OP_INTERNAL_MACRO_EXT(__and_bit_iinternal)
}

template <typename _T1, typename _T2, class = typename EN_IF(IS_ARITH(_T1))>
inline auto operator|(const _T1 &__other, const MdStaticArray<_T2> &first) {
    OP_INTERNAL_MACRO_EXT(__or_bit_iinternal)
}

template <typename _T1, typename _T2, class = typename EN_IF(IS_ARITH(_T1))>
inline auto operator^(const _T1 &__other, const MdStaticArray<_T2> &first) {
    OP_INTERNAL_MACRO_EXT(__xor_bit_iinternal)
}

template <typename _T1, typename _T2, class = typename EN_IF(IS_ARITH(_T1))>
inline auto operator<<(const _T1 &__other, const MdStaticArray<_T2> &first) {
    OP_INTERNAL_MACRO_EXT(__lshft_bit_iointernal)
}

template <typename _T1, typename _T2, class = typename EN_IF(IS_ARITH(_T1))>
inline auto operator>>(const _T1 &__other, const MdStaticArray<_T2> &first) {
    OP_INTERNAL_MACRO_EXT(__rshft_bit_iointernal)
}

#include "./md_static_reference.hpp"

template <typename _T>
MdStaticArray<_T> &MdStaticArray<_T>::operator=(const reference &__other) {
    init_array(__other.size);
    init_shape(&__other.__array_reference->shape[__other.shp_offset],
               __other.__array_reference->shp_size - __other.shp_offset);

    if (__size > s_threshold_size) {
#pragma omp parallel for
        for (size_t index = 0; index < __size; ++index) {
            __array[index] =
                __other.__array_reference->__array[__other.offset + index];
        }
    } else {
        for (size_t index = 0; index < __size; ++index) {
            __array[index] =
                __other.__array_reference->__array[__other.offset + index];
        }
    }

    return *this;
}

template <typename _T>
MdStaticArray<_T>::MdStaticArray(const reference &__other)
    : shape(nullptr), skip_vec(nullptr) {
    __size = __other.size;
    init_array(__size);
    init_shape(&__other.__array_reference->shape[__other.shp_offset],
               __other.__array_reference->shp_size - __other.shp_offset);

    if (__size > s_threshold_size) {
#pragma omp parallel for
        for (size_t index = 0; index < __size; ++index) {
            __array[index] =
                __other.__array_reference->__array[__other.offset + index];
        }
    } else {
        for (size_t index = 0; index < __size; ++index) {
            __array[index] =
                __other.__array_reference->__array[__other.offset + index];
        }
    }
}

#undef EN_IF
#undef IS_ARITH
#undef OP_INTERNAL_MACRO
#undef OP_INTERNAL_MACRO_EXT

#endif
