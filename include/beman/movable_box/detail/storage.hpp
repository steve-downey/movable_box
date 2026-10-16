// include/beman/movable_box/detail/storage.hpp                      -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_MOVABLE_BOX_DETAIL_STORAGE_HPP
#define BEMAN_MOVABLE_BOX_DETAIL_STORAGE_HPP

// The module interface unit gets these from `import std;`; textually including them again in its
// purview redefines the standard library entities (MSVC rejects it outright).
#if !defined(BEMAN_MOVABLE_BOX_INCLUDED_FROM_INTERFACE_UNIT)
    #include <concepts>
    #include <memory>
    #include <type_traits>
    #include <utility>
#endif

// The storage behind movable_box<T>, kept out of the public header so that header holds only what
// the proposed wording specifies; scripts/gen-wording.sh generates that wording from it.

namespace beman::movable_box::detail {

// Storage selection, per [movable.box.general] p3 (recommended practice) and design decision D8:
//
//   If copy_constructible<T> is modeled, movable_box<T> should store only a T if either T models
//   copyable, or is_nothrow_move_constructible_v<T> && is_nothrow_copy_constructible_v<T> is true.
//   Otherwise, movable_box<T> should store only a T if either T models movable or
//   is_nothrow_move_constructible_v<T> is true.
//
// When the condition holds, the synthesized assignment operators (see below) can never leave the
// box valueless, so no discriminant is required: the "flat" storage below stores exactly a T, with
// has_value() unconditionally true. Otherwise the "boxed" storage carries an explicit engaged flag.
template <class T>
inline constexpr bool use_flat_storage_v =
    std::copy_constructible<T>
        ? (std::copyable<T> || (std::is_nothrow_move_constructible_v<T> && std::is_nothrow_copy_constructible_v<T>))
        : (std::movable<T> || std::is_nothrow_move_constructible_v<T>);

// Storage that always contains a value: sizeof(flat_storage<T>) == sizeof(T), and has_value() is
// unconditionally true. Selected whenever the synthesized assignment operators cannot create an
// empty state (see use_flat_storage_v above), so the destroy-then-construct fallback used by
// operator= is statically guaranteed not to throw.
template <class T>
class flat_storage {
  public:
    constexpr flat_storage() noexcept(std::is_nothrow_default_constructible_v<T>)
        requires std::default_initializable<T>
        : value_() {}

    template <class... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr explicit flat_storage(std::in_place_t, Args&&... args) : value_(std::forward<Args>(args)...) {}

    // T is a plain member, not a union member, so the implicitly declared copy/move constructors,
    // assignment operators, and destructor are each exactly as trivial, noexcept, or deleted as T's.

    static constexpr bool has_value() noexcept { return true; }

    constexpr T*       get() noexcept { return std::addressof(value_); }
    constexpr const T* get() const noexcept { return std::addressof(value_); }

    // Preconditions: none. A value is always alive; the caller is responsible for immediately
    // reconstructing after destroy(), which this type's selection guarantees is non-throwing.
    constexpr void destroy() noexcept(std::is_nothrow_destructible_v<T>) { std::destroy_at(std::addressof(value_)); }

    template <class... Args>
    constexpr void construct(Args&&... args) {
        std::construct_at(std::addressof(value_), std::forward<Args>(args)...);
    }

  private:
    T value_;
};

// Storage with an explicit engaged flag: used when the synthesized assignment operators could
// leave the box valueless (a throwing destroy-then-construct fallback). The union member's
// lifetime is managed manually via std::construct_at / std::destroy_at.
template <class T>
class boxed_storage {
  public:
    constexpr boxed_storage() noexcept(std::is_nothrow_default_constructible_v<T>)
        requires std::default_initializable<T>
        : dummy_() {
        std::construct_at(std::addressof(value_));
        engaged_ = true;
    }

    template <class... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr explicit boxed_storage(std::in_place_t, Args&&... args) : dummy_() {
        std::construct_at(std::addressof(value_), std::forward<Args>(args)...);
        engaged_ = true;
    }

    // Trivial copy and move are noexcept, so a T reaches this storage with a trivial copy or move
    // constructor only when the other one can throw. The defaulted forms copy the union and flag
    // as they stand, which is correct whether or not a value is engaged.
    constexpr boxed_storage(const boxed_storage&)
        requires std::copy_constructible<T> && std::is_trivially_copy_constructible_v<T>
    = default;

    constexpr boxed_storage(const boxed_storage& other)
        requires std::copy_constructible<T> && (!std::is_trivially_copy_constructible_v<T>)
        : dummy_(), engaged_(other.engaged_) {
        if (engaged_) {
            std::construct_at(std::addressof(value_), *other.get());
        }
    }

    constexpr boxed_storage(const boxed_storage&)
        requires(!std::copy_constructible<T>)
    = delete;

    constexpr boxed_storage(boxed_storage&&)
        requires std::is_trivially_move_constructible_v<T>
    = default;

    constexpr boxed_storage(boxed_storage&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        requires(!std::is_trivially_move_constructible_v<T>)
        : dummy_(), engaged_(other.engaged_) {
        if (engaged_) {
            std::construct_at(std::addressof(value_), std::move(*other.get()));
        }
    }

    constexpr ~boxed_storage()
        requires std::is_trivially_destructible_v<T>
    = default;

    constexpr ~boxed_storage()
        requires(!std::is_trivially_destructible_v<T>)
    {
        destroy();
    }

    constexpr bool has_value() const noexcept { return engaged_; }

    constexpr T*       get() noexcept { return std::addressof(value_); }
    constexpr const T* get() const noexcept { return std::addressof(value_); }

    constexpr void destroy() noexcept(std::is_nothrow_destructible_v<T>) {
        if (engaged_) {
            std::destroy_at(std::addressof(value_));
            engaged_ = false;
        }
    }

    // Preconditions: !has_value(). Leaves has_value() false if construction exits via exception.
    template <class... Args>
    constexpr void construct(Args&&... args) {
        std::construct_at(std::addressof(value_), std::forward<Args>(args)...);
        engaged_ = true;
    }

  private:
    union {
        char dummy_;
        T    value_;
    };
    bool engaged_ = false;
};

template <class T>
using storage_for = std::conditional_t<use_flat_storage_v<T>, flat_storage<T>, boxed_storage<T> >;

// The synthesized assignment operators can be defaulted -- and are then trivial -- when their
// specified effects are indistinguishable from copying the object representation: the storage is
// flat (always engaged, no discriminant), T's corresponding assignment, construction, and
// destruction are all trivial (so both the copy-assign fast path and the destroy-then-construct
// fallback reduce to a memcpy), and the self-assignment check is unobservable. These are the
// conditions of [optional.assign]. So a trivially copyable, trivially assignable T yields a
// trivially copyable box -- and with it, a trivially copyable view.
template <class T>
inline constexpr bool use_trivial_copy_assign_v =
    use_flat_storage_v<T> && std::is_trivially_copy_constructible_v<T> && std::is_trivially_copy_assignable_v<T> &&
    std::is_trivially_destructible_v<T>;

template <class T>
inline constexpr bool use_trivial_move_assign_v =
    use_flat_storage_v<T> && std::is_trivially_move_constructible_v<T> && std::is_trivially_move_assignable_v<T> &&
    std::is_trivially_destructible_v<T>;

} // namespace beman::movable_box::detail

#endif // BEMAN_MOVABLE_BOX_DETAIL_STORAGE_HPP
