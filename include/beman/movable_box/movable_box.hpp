// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_MOVABLE_BOX_MOVABLE_BOX_HPP
#define BEMAN_MOVABLE_BOX_MOVABLE_BOX_HPP

#include <beman/movable_box/config.hpp>

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

#if BEMAN_MOVABLE_BOX_USE_MODULES() && !defined(BEMAN_MOVABLE_BOX_INCLUDED_FROM_INTERFACE_UNIT)

import beman.movable_box;

#else

namespace beman::movable_box {

namespace detail {

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

    constexpr flat_storage(const flat_storage&)
        requires std::copy_constructible<T> && std::is_trivially_copy_constructible_v<T>
    = default;

    constexpr flat_storage(const flat_storage& other)
        requires std::copy_constructible<T> && (!std::is_trivially_copy_constructible_v<T>)
        : value_(*other.get()) {}

    constexpr flat_storage(const flat_storage&)
        requires(!std::copy_constructible<T>)
    = delete;

    constexpr flat_storage(flat_storage&&) noexcept(std::is_nothrow_move_constructible_v<T>)
        requires std::is_trivially_move_constructible_v<T>
    = default;

    constexpr flat_storage(flat_storage&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        requires(!std::is_trivially_move_constructible_v<T>)
        : value_(std::move(*other.get())) {}

    // Declared only so movable_box's conditionally defaulted assignment operators (see
    // use_trivial_copy_assign_v below) have a non-deprecated member assignment to default to;
    // deleted when T is not assignable, in which case movable_box provides its own.
    constexpr flat_storage& operator=(const flat_storage&) = default;
    constexpr flat_storage& operator=(flat_storage&&)      = default;

    constexpr ~flat_storage()
        requires std::is_trivially_destructible_v<T>
    = default;

    constexpr ~flat_storage()
        requires(!std::is_trivially_destructible_v<T>)
    {
        std::destroy_at(std::addressof(value_));
    }

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

    constexpr boxed_storage(const boxed_storage& other)
        requires std::copy_constructible<T>
        : dummy_(), engaged_(other.engaged_) {
        if (engaged_) {
            std::construct_at(std::addressof(value_), *other.get());
        }
    }

    constexpr boxed_storage(const boxed_storage&)
        requires(!std::copy_constructible<T>)
    = delete;

    constexpr boxed_storage(boxed_storage&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
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

} // namespace detail

// [movable.box], class template movable_box
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
class movable_box {
  public:
    using value_type = T;

    // [movable.box.ctor], constructors

    // Effects: Equivalent to movable_box(in_place).
    constexpr movable_box() noexcept(std::is_nothrow_default_constructible_v<T>)
        requires std::default_initializable<T>
        : storage_() {}

    // Effects: Direct-non-list-initializes the contained value with t.
    constexpr explicit movable_box(const T& t)
        requires std::copy_constructible<T>
        : storage_(std::in_place, t) {}

    // Effects: Direct-non-list-initializes the contained value with std::move(t).
    constexpr explicit movable_box(T&& t) : storage_(std::in_place, std::move(t)) {}

    // Constraints: is_constructible_v<T, Args...> is true.
    // Effects: Direct-non-list-initializes the contained value with std::forward<Args>(args)...
    template <class... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr explicit movable_box(std::in_place_t, Args&&... args)
        : storage_(std::in_place, std::forward<Args>(args)...) {}

    // Effects: If rhs contains a value, direct-non-list-initializes the contained value with
    // *rhs; otherwise, *this contains no value.
    // Remarks: This constructor is trivial if is_trivially_copy_constructible_v<T> is true.
    // (Triviality is inherited from storage_, whose copy constructor is conditionally trivial.)
    constexpr movable_box(const movable_box&)
        requires std::copy_constructible<T>
    = default;

    // Effects: If rhs contains a value, direct-non-list-initializes the contained value with
    // std::move(*rhs); otherwise, *this contains no value. rhs.has_value() is unchanged.
    // Remarks: This constructor is trivial if is_trivially_move_constructible_v<T> is true.
    constexpr movable_box(movable_box&&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;

    // [movable.box.dtor], destructor
    // Effects: Destroys the contained value, if any.
    // Remarks: This destructor is trivial if is_trivially_destructible_v<T> is true.
    constexpr ~movable_box() = default;

    // [movable.box.assign], assignment

    // This operator is trivial (defaulted) when the specified effects reduce to copying the
    // object representation; see detail::use_trivial_copy_assign_v.
    constexpr movable_box& operator=(const movable_box&)
        requires std::copy_constructible<T> && detail::use_trivial_copy_assign_v<T>
    = default;

    // Remarks: The exception specification is equivalent to:
    // is_nothrow_copy_constructible_v<T> && (!copyable<T> || is_nothrow_copy_assignable_v<T>).
    constexpr movable_box& operator=(const movable_box& rhs) noexcept(std::is_nothrow_copy_constructible_v<T> &&
                                                                      (!std::copyable<T> ||
                                                                       std::is_nothrow_copy_assignable_v<T>))
        requires std::copy_constructible<T> && (!detail::use_trivial_copy_assign_v<T>)
    {
        // Effects: If this == addressof(rhs) is true, no effects.
        if (this == std::addressof(rhs)) {
            return *this;
        }
        if constexpr (detail::use_flat_storage_v<T>) {
            if constexpr (std::copyable<T>) {
                *storage_.get() = *rhs.storage_.get();
            } else {
                storage_.destroy();
                storage_.construct(*rhs.storage_.get());
            }
        } else {
            // Otherwise: the contained value of *this, if any, is destroyed; then, if rhs
            // contains a value, a contained value is direct-non-list-initialized with *rhs. If
            // the initialization exits via an exception, *this contains no value.
            static_assert(!std::copyable<T>);
            storage_.destroy();
            if (rhs.storage_.has_value()) {
                storage_.construct(*rhs.storage_.get());
            }
        }
        return *this;
    }

    // This operator is trivial (defaulted) when the specified effects reduce to copying the
    // object representation; see detail::use_trivial_copy_assign_v.
    constexpr movable_box& operator=(movable_box&&)
        requires detail::use_trivial_move_assign_v<T>
    = default;

    // Remarks: The exception specification is equivalent to:
    // is_nothrow_move_constructible_v<T> && (!movable<T> || is_nothrow_move_assignable_v<T>).
    constexpr movable_box& operator=(movable_box&& rhs) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                                                 (!std::movable<T> ||
                                                                  std::is_nothrow_move_assignable_v<T>))
        requires(!detail::use_trivial_move_assign_v<T>)
    {
        // Effects: If this == addressof(rhs) is true, no effects.
        if (this == std::addressof(rhs)) {
            return *this;
        }
        if constexpr (detail::use_flat_storage_v<T>) {
            if constexpr (std::movable<T>) {
                *storage_.get() = std::move(*rhs.storage_.get());
            } else {
                storage_.destroy();
                storage_.construct(std::move(*rhs.storage_.get()));
            }
        } else {
            if constexpr (std::movable<T>) {
                // If movable<T> is modeled: if *this and rhs each contain a value, the contained
                // value of *this is move-assigned from *rhs; otherwise, if rhs contains a value, a
                // contained value is direct-non-list-initialized with std::move(*rhs); otherwise,
                // the contained value of *this, if any, is destroyed. rhs.has_value() is unchanged.
                if (storage_.has_value() && rhs.storage_.has_value()) {
                    *storage_.get() = std::move(*rhs.storage_.get());
                } else if (rhs.storage_.has_value()) {
                    storage_.construct(std::move(*rhs.storage_.get()));
                } else {
                    storage_.destroy();
                }
            } else {
                // Otherwise: the contained value of *this, if any, is destroyed; then, if rhs
                // contains a value, a contained value is direct-non-list-initialized with
                // std::move(*rhs). If the initialization exits via an exception, *this contains no
                // value. rhs.has_value() is unchanged.
                storage_.destroy();
                if (rhs.storage_.has_value()) {
                    storage_.construct(std::move(*rhs.storage_.get()));
                }
            }
        }
        return *this;
    }

    // [movable.box.obs], observers

    // Returns: true if and only if *this contains a value.
    constexpr bool     has_value() const noexcept { return storage_.has_value(); }
    constexpr explicit operator bool() const noexcept { return storage_.has_value(); }

    // Preconditions: *this contains a value.
    // Returns: A reference to the contained value.
    constexpr T&       operator*() & noexcept { return *storage_.get(); }
    constexpr const T& operator*() const& noexcept { return *storage_.get(); }

    // Preconditions: *this contains a value.
    // Returns: std::move(the contained value).
    constexpr T&&       operator*() && noexcept { return std::move(*storage_.get()); }
    constexpr const T&& operator*() const&& noexcept { return std::move(*storage_.get()); }

    // Preconditions: *this contains a value.
    // Returns: addressof(the contained value).
    constexpr T*       operator->() noexcept { return storage_.get(); }
    constexpr const T* operator->() const noexcept { return storage_.get(); }

  private:
    detail::storage_for<T> storage_;
};

} // namespace beman::movable_box

#endif // BEMAN_MOVABLE_BOX_USE_MODULES() &&
       // !defined(BEMAN_MOVABLE_BOX_INCLUDED_FROM_INTERFACE_UNIT)

#endif // BEMAN_MOVABLE_BOX_MOVABLE_BOX_HPP
