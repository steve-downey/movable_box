// include/beman/movable_box/movable_box.hpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_MOVABLE_BOX_MOVABLE_BOX_HPP
#define BEMAN_MOVABLE_BOX_MOVABLE_BOX_HPP

#include <beman/movable_box/config.hpp>

// The module interface unit gets these from `import std;`; textually including them again in its
// purview redefines the standard library entities (MSVC rejects it outright).
#if !defined(BEMAN_MOVABLE_BOX_INCLUDED_FROM_INTERFACE_UNIT)
    #include <concepts>
    #include <memory>
    #include <type_traits>
    #include <utility>
#endif

#if BEMAN_MOVABLE_BOX_USE_MODULES() && !defined(BEMAN_MOVABLE_BOX_INCLUDED_FROM_INTERFACE_UNIT)

import beman.movable_box;

#else

    #include <beman/movable_box/detail/storage.hpp>

// This header is also the source of the proposed wording for [movable.box]: scripts/gen-wording.sh
// runs beman.specgen over it. The class body is the synopsis, the `//!` docblocks are the wording,
// and the `\rSec3` markers below place each item in its subclause. Edit the wording here, never in
// the generated fragments under papers/wording/. The `///` comments are API documentation for
// Doxygen and MrDocs only; specgen never promotes them into wording.
//
// Each special member is a pair of declarations: one defaulted, and so trivial, where T's
// corresponding member is trivial, and one user-provided everywhere else. The defaulted twin is
// marked `\merge`, and the wording describes the pair as the one declaration the paper proposes.

namespace beman::movable_box {

/// \brief A `move_constructible` object, made assignable.
///
/// When `T` is not itself copyable or movable, copy and move assignment are synthesized by destroying
/// the contained value and constructing a new one in its place, so `movable_box<T>` models `movable`
/// for every `T` it accepts. This is the standard library's exposition-only *movable-box*, made
/// available to users.
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
class movable_box {
  public:
    /// The type of the contained value.
    using value_type = T;

    // \ref{movable.box.ctor}, constructors
    constexpr movable_box() noexcept(std::is_nothrow_default_constructible_v<T>)
        requires std::default_initializable<T>;
    constexpr explicit movable_box(const T& t)
        requires std::copy_constructible<T>;
    constexpr explicit movable_box(T&& t);
    template <class... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr explicit movable_box(std::in_place_t, Args&&... args);

    constexpr movable_box(const movable_box& rhs)
        requires std::copy_constructible<T> && (!std::is_trivially_copy_constructible_v<T>);
    //! \merge
    constexpr movable_box(const movable_box&)
        requires std::copy_constructible<T> && std::is_trivially_copy_constructible_v<T>
    = default;
    constexpr movable_box(movable_box&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>)
        requires(!std::is_trivially_move_constructible_v<T>);
    //! \merge
    constexpr movable_box(movable_box&&)
        requires std::is_trivially_move_constructible_v<T>
    = default;

    // \ref{movable.box.dtor}, destructor
    constexpr ~movable_box()
        requires(!std::is_trivially_destructible_v<T>);
    //! \merge
    constexpr ~movable_box()
        requires std::is_trivially_destructible_v<T>
    = default;

    // \ref{movable.box.assign}, assignment
    constexpr movable_box& operator=(const movable_box& rhs) noexcept(std::is_nothrow_copy_constructible_v<T> &&
                                                                      (!std::copyable<T> ||
                                                                       std::is_nothrow_copy_assignable_v<T>))
        requires std::copy_constructible<T> && (!detail::use_trivial_copy_assign_v<T>);
    //! \merge
    constexpr movable_box& operator=(const movable_box&)
        requires std::copy_constructible<T> && detail::use_trivial_copy_assign_v<T>
    = default;
    constexpr movable_box& operator=(movable_box&& rhs) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                                                 (!std::movable<T> ||
                                                                  std::is_nothrow_move_assignable_v<T>))
        requires(!detail::use_trivial_move_assign_v<T>);
    //! \merge
    constexpr movable_box& operator=(movable_box&&)
        requires detail::use_trivial_move_assign_v<T>
    = default;

    // \ref{movable.box.obs}, observers
    constexpr bool     has_value() const noexcept;
    constexpr explicit operator bool() const noexcept;

    constexpr T&        operator*() & noexcept;
    constexpr const T&  operator*() const& noexcept;
    constexpr T&&       operator*() && noexcept;
    constexpr const T&& operator*() const&& noexcept;

    constexpr T*       operator->() noexcept;
    constexpr const T* operator->() const noexcept;

  private:
    //! \omit
    detail::storage_for<T> storage_;
};

// \rSec3[movable.box.ctor]{Constructors}

//! \constraints-in-decl
//! \effects Equivalent to `movable_box(in_place)`.
template <class T>
    requires std::move_constructible<T> &&
             std::is_object_v<T>
             constexpr movable_box<T>::movable_box() noexcept(std::is_nothrow_default_constructible_v<T>)
                 requires std::default_initializable<T>
    : storage_() {}

//! \constraints-in-decl
//! \effects Direct-non-list-initializes the contained value with `t`.
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
                                       constexpr movable_box<T>::movable_box(const T& t)
                 requires std::copy_constructible<T>
    : storage_(std::in_place, t) {}

//! \effects Direct-non-list-initializes the contained value with
//! `std::move(t)`.
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
constexpr movable_box<T>::movable_box(T&& t) : storage_(std::in_place, std::move(t)) {}

//! \effects Direct-non-list-initializes the contained value with
//! `std::forward<Args>(args)...`.
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
                                       template <class... Args>
                 requires std::is_constructible_v<T, Args...>
constexpr movable_box<T>::movable_box(std::in_place_t, Args&&... args)
    : storage_(std::in_place, std::forward<Args>(args)...) {}

//! \constraints-in-decl
//! \effects If `rhs` contains a value, direct-non-list-initializes the
//! contained value with `*rhs`; otherwise, `*this` contains no value.
//! \remarks This constructor is trivial if
//! `is_trivially_copy_constructible_v<T>` is `true`.
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
                                       constexpr movable_box<T>::movable_box(const movable_box& rhs)
                 requires std::copy_constructible<T> && (!std::is_trivially_copy_constructible_v<T>)
    : storage_(rhs.storage_) {}

//! \effects If `rhs` contains a value, direct-non-list-initializes the
//! contained value with `std::move(*rhs)`; otherwise, `*this` contains no
//! value. `rhs.has_value()` is unchanged.
//! \remarks This constructor is trivial if
//! `is_trivially_move_constructible_v<T>` is `true`.
template <class T>
    requires std::move_constructible<T> &&
             std::is_object_v<T>
             constexpr movable_box<T>::movable_box(movable_box&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>)
                 requires(!std::is_trivially_move_constructible_v<T>)
    : storage_(std::move(rhs.storage_)) {}

// \rSec3[movable.box.dtor]{Destructor}

//! \effects Destroys the contained value, if any.
//! \remarks This destructor is trivial if `is_trivially_destructible_v<T>`
//! is `true`.
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
                                       constexpr movable_box<T>::~movable_box()
                 requires(!std::is_trivially_destructible_v<T>)
{}

// \rSec3[movable.box.assign]{Assignment}

//! \seebelow noexcept
//! \constraints-in-decl
//! \effects If `this == addressof(rhs)` is `true`, no effects. Otherwise:
//! \item If `copyable<T>` is modeled: if `*this` and `rhs` each contain a
//! value, the contained value of `*this` is copy-assigned from `*rhs`;
//! otherwise, if `rhs` contains a value, a contained value is
//! direct-non-list-initialized with `*rhs`; otherwise, the contained value
//! of `*this`, if any, is destroyed.
//! \item Otherwise: the contained value of `*this`, if any, is destroyed;
//! then, if `rhs` contains a value, a contained value is
//! direct-non-list-initialized with `*rhs`. If the initialization exits via
//! an exception, `*this` contains no value.
//! \returns `*this`.
//! \remarks The exception specification is equivalent to
//! `is_nothrow_copy_constructible_v<T>` and, if `copyable<T>` is modeled,
//! `is_nothrow_copy_assignable_v<T>`.
//!
//! This operator is trivial if `is_trivially_copy_constructible_v<T>`,
//! `is_trivially_copy_assignable_v<T>`, and `is_trivially_destructible_v<T>`
//! are all `true`.
template <class T>
    requires std::move_constructible<T> &&
             std::is_object_v<T>
             constexpr movable_box<T>& movable_box<T>::operator=(const movable_box& rhs) noexcept(
                 std::is_nothrow_copy_constructible_v<T> &&
                 (!std::copyable<T> || std::is_nothrow_copy_assignable_v<T>))
                 requires std::copy_constructible<T> && (!detail::use_trivial_copy_assign_v<T>)
{
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
        // Boxed storage is selected for a copy_constructible T only when T is not copyable.
        static_assert(!std::copyable<T>);
        storage_.destroy();
        if (rhs.storage_.has_value()) {
            storage_.construct(*rhs.storage_.get());
        }
    }
    return *this;
}

//! \seebelow noexcept
//! \effects If `this == addressof(rhs)` is `true`, no effects. Otherwise:
//! \item If `movable<T>` is modeled: if `*this` and `rhs` each contain a
//! value, the contained value of `*this` is move-assigned from `*rhs`;
//! otherwise, if `rhs` contains a value, a contained value is
//! direct-non-list-initialized with `std::move(*rhs)`; otherwise, the
//! contained value of `*this`, if any, is destroyed. `rhs.has_value()` is
//! unchanged.
//! \item Otherwise: the contained value of `*this`, if any, is destroyed;
//! then, if `rhs` contains a value, a contained value is
//! direct-non-list-initialized with `std::move(*rhs)`. If the initialization
//! exits via an exception, `*this` contains no value. `rhs.has_value()` is
//! unchanged.
//! \returns `*this`.
//! \remarks The exception specification is equivalent to
//! `is_nothrow_move_constructible_v<T>` and, if `movable<T>` is modeled,
//! `is_nothrow_move_assignable_v<T>`.
//!
//! This operator is trivial if `is_trivially_move_constructible_v<T>`,
//! `is_trivially_move_assignable_v<T>`, and `is_trivially_destructible_v<T>`
//! are all `true`.
template <class T>
    requires std::move_constructible<T> &&
             std::is_object_v<T>
             constexpr movable_box<T>& movable_box<T>::operator=(movable_box&& rhs) noexcept(
                 std::is_nothrow_move_constructible_v<T> && (!std::movable<T> || std::is_nothrow_move_assignable_v<T>))
                 requires(!detail::use_trivial_move_assign_v<T>)
{
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
    } else if constexpr (std::movable<T>) {
        if (storage_.has_value() && rhs.storage_.has_value()) {
            *storage_.get() = std::move(*rhs.storage_.get());
        } else if (rhs.storage_.has_value()) {
            storage_.construct(std::move(*rhs.storage_.get()));
        } else {
            storage_.destroy();
        }
    } else {
        storage_.destroy();
        if (rhs.storage_.has_value()) {
            storage_.construct(std::move(*rhs.storage_.get()));
        }
    }
    return *this;
}

// \rSec3[movable.box.obs]{Observers}

//! \returns `true` if and only if `*this` contains a value.
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
constexpr bool movable_box<T>::has_value() const noexcept {
    return storage_.has_value();
}

//! \also
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
constexpr movable_box<T>::operator bool() const noexcept {
    return storage_.has_value();
}

//! \expects `*this` contains a value.
//! \returns A reference to the contained value.
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
constexpr T& movable_box<T>::operator*() & noexcept {
    return *storage_.get();
}

//! \also
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
constexpr const T& movable_box<T>::operator*() const& noexcept {
    return *storage_.get();
}

//! \expects `*this` contains a value.
//! \returns `std::move(`the contained value`)`.
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
constexpr T&& movable_box<T>::operator*() && noexcept {
    return std::move(*storage_.get());
}

//! \also
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
constexpr const T&& movable_box<T>::operator*() const&& noexcept {
    return std::move(*storage_.get());
}

//! \expects `*this` contains a value.
//! \returns `addressof(`the contained value`)`.
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
                                       constexpr T* movable_box<T>::operator->() noexcept {
    return storage_.get();
}

//! \also
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
                                       constexpr const T* movable_box<T>::operator->() const noexcept {
    return storage_.get();
}

} // namespace beman::movable_box

#endif // BEMAN_MOVABLE_BOX_USE_MODULES() &&
       // !defined(BEMAN_MOVABLE_BOX_INCLUDED_FROM_INTERFACE_UNIT)

#endif // BEMAN_MOVABLE_BOX_MOVABLE_BOX_HPP
