// This file is part of the `Selene` library.
// Copyright 2017 Michael Hofmann (https://github.com/kmhofmann).
// Distributed under MIT license. See accompanying LICENSE file in the top-level directory.

#ifndef SELENE_BASE_UTILS_HPP
#define SELENE_BASE_UTILS_HPP

/// @file

#include <selene/base/Assert.hpp>
#include <selene/base/Promote.hpp>
#include <selene/base/Round.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <type_traits>

namespace sln {

/** \brief Clamp a value between a minimum and a maximum value.
 *
 * If `value` compares less than `min`, returns `min`. Otherwise if `max` compares less than `value`, returns `max`.
 * Otherwise returns `value`.
 *
 * @tparam T The value type.
 * @tparam Compare The comparison function.
 * @param value The value to clamp.
 * @param min The minimum value/lower bound.
 * @param max The maximum value/upper bound.
 * @param comp The comparison function.
 * @return The clamped value.
 */
template <typename T, typename Compare>
inline constexpr const T& clamp(const T& value, const T& min, const T& max, Compare comp) noexcept
{
  SELENE_ASSERT(!comp(max, min));
  return comp(value, min) ? min : comp(max, value) ? max : value;
}

/** \brief Clamp a value between a minimum and a maximum value.
 *
 * If `value` compares less than `min`, returns `min`. Otherwise if `max` compares less than `value`, returns `max`.
 * Otherwise returns `value`. Uses `operator<()` to compare the values.
 *
 * Given usual semantics, the function returns max(a, min(b, x)), where x is the input value and a, b are minimum and
 * maximum, respectively.
 *
 * @tparam T The value type.
 * @param value The value to clamp.
 * @param min The minimum value/lower bound.
 * @param max The maximum value/upper bound.
 * @return The clamped value.
 */
template <typename T>
inline constexpr const T& clamp(const T& value, const T& min, const T& max) noexcept
{
  return sln::clamp(value, min, max, std::less<T>());
}

/** \brief Computes the value of `base` raised to the power of `exponent`, where `exponent` is integral and unsigned.
 *
 * @tparam T The type of the base.
 * @tparam U The type of the exponent. Needs to be integaral and unsigned
 * @param base The base.
 * @param exponent The exponent.
 * @return `base` raised to the power of `exponent`.
 */
template <typename T, typename U>
inline constexpr T power(T base, U exponent) noexcept
{
  static_assert(std::is_integral<U>::value, "Exponent needs to be integral");
  static_assert(std::is_unsigned<U>::value, "Exponent needs to be unsigned");
  return (exponent == 0) ? T{1} : base * power(base, U(exponent - 1));
}

/** Computes the next largest power of two given an unsigned integer value.
 *
 * @param x An unsigned integer value.
 * @return The next largest power of two.
 */
inline constexpr std::uint64_t next_power_of_two(std::uint64_t x)
{
  // from https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  return ++x;
}

/** \brief Returns a `std::array` filled with `N` equal values.
 *
 * @tparam T The value type of the array.
 * @tparam N The number of elements in the array.
 * @param value The value to fill the array with.
 * @return A `std::array<T, N>`, where each entry has value `value`.
 */
template <typename T, std::uint32_t N>
inline constexpr std::array<T, N> make_array_n_equal(T value)
{
  using result_t = ::std::array<T, N>;
  result_t arr = {{value}};
  const result_t& const_arr = arr;

  for (std::uint32_t i = 1; i < N; ++i)
  {
    // const_cast not needed anymore in C++17: https://stackoverflow.com/a/46779579/218634
    const_cast<typename result_t::reference>(const_arr[i]) = value;  // NOLINT
  }

  return arr;
}

/** \brief Returns a `std::array<T, N>` filled with values returned by the specified function `func` at `func(i)`.
 *
 * @tparam T The value type of the array.
 * @tparam N The number of elements in the array.
 * @tparam Func The type of the passed function.
 * @param func The function to fill the array with. Needs to accept one argument, the array index; e.g.
 * `T func(std::size_t i)`. The i-th value of the array will be set to the return value of `func(i)`.
 * @return A `std::array<T, N>` with the respective content.
 */
template <typename T, std::size_t N, typename Func>
inline constexpr auto make_array_from_function(Func func) noexcept
{
  using result_t = ::std::array<T, N>;
  result_t arr = {};
  const result_t& const_arr = arr;

  for (std::size_t i = 0; i < N; ++i)
  {
    // const_cast not needed anymore in C++17: https://stackoverflow.com/a/46779579/218634
    const_cast<typename result_t::reference>(const_arr[i]) = func(i);  // NOLINT
  }

  return arr;
}

// \cond INTERNAL

namespace detail {

template <typename PT, typename Coeff, PT shift>
inline constexpr PT rounded_linear_combination_coeff_func(std::size_t i)
{
  using UT = std::make_unsigned_t<PT>;
  return constexpr_round_half_up<PT>(Coeff::values[i] * PT(power(UT{2}, UT{shift})));
}

}  // namespace detail

// \endcond

/** \brief Computes an approximate linear combination of values with coefficients provided at compile time.
 *
 * The computation is performed using integer arithmetic. The result is required to be of integral type.
 *
 * @tparam T The result type. Needs to be integral.
 * @tparam N The number of input values/coefficients.
 * @tparam Coeff The type of the coefficient array. Needs to be a structure that contains a static variable `values`.
 * The type of `values` needs to support the subscript operator `operator[]`; e.g. it can be a `std::array<>`.
 * @tparam PromotedType A promoted type to hold the intermediate integer arithmetic computations. Defaults to
 * `sln::promote_t<T>`.
 * @tparam Array The type of the input array. Needs to support the subscript operator `operator[]`.
 * @param src The input array.
 * @return A linear combination of the input array with the provided coefficients.
 */
template <typename T, std::size_t N, typename Coeff, typename PromotedType = promote_t<T>, typename Array>
inline constexpr T approximate_linear_combination(const Array& src) noexcept
{
  static_assert(std::is_integral<T>::value, "Conversion type has to be integral");

  // Compute integer coefficients at compile time
  using PT = PromotedType;  // Larger type
  using UPT = std::make_unsigned_t<PT>;
  constexpr auto shift = UPT{(sizeof(PT) - sizeof(T)) * 8};
  constexpr auto half = PT(power(UPT{2}, shift) / PT{2});  // "0.5" for rounding

  using Func = decltype(detail::rounded_linear_combination_coeff_func<PT, Coeff, shift>);  // GCC 7.2.0 wants this...
  constexpr auto c = make_array_from_function<PT, N, Func>(
      detail::rounded_linear_combination_coeff_func<PT, Coeff, shift>);

  auto sum = PT{0};
  for (std::size_t i = 0; i < N; ++i)  // N is known at compile time
  {
    sum += static_cast<PT>(c[i] * src[i]);
  }

  return static_cast<T>((sum + half) >> shift);
}

/** \brief Computes a linear combination of values with coefficients provided at compile time.
 *
 * The result is required to be of floating point type.
 *
 * @tparam T The result type. Needs to be integral.
 * @tparam N The number of input values/coefficients.
 * @tparam Coeff The type of the coefficient array. Needs to be a structure that contains a static variable `values`.
 * The type of `values` needs to support the subscript operator `operator[]`; e.g. it can be a `std::array<>`.
 * @tparam Array The type of the input array. Needs to support the subscript operator `operator[]`.
 * @param src The input array.
 * @return A linear combination of the input array with the provided coefficients.
 */
template <typename T, std::size_t N, typename Coeff, typename Array>
inline constexpr T linear_combination(const Array& src) noexcept
{
  static_assert(std::is_floating_point<T>::value, "Conversion type has to be floating point");

  auto sum = T{0};
  for (std::size_t i = 0; i < N; ++i)  // N is known at compile time
  {
    sum += static_cast<T>(Coeff::values[i]) * static_cast<T>(src[i]);
  }

  return sum;
}

}  // namespace sln

#endif  // SELENE_BASE_UTILS_HPP
