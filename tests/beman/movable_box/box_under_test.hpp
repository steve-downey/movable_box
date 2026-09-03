// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_MOVABLE_BOX_TESTS_BOX_UNDER_TEST_HPP
#define BEMAN_MOVABLE_BOX_TESTS_BOX_UNDER_TEST_HPP

// Selects the movable-box the conformance suite in movable_box.test.cpp runs
// against, and defines the per-divergence expected-failure tags.
//
// The paper claims every shipping implementation can provide the proposed
// public interface as *its existing box*, with no layout or behavior change.
// The suite is the executable form of that claim. Four configurations:
//
// - Default: beman::movable_box::movable_box, the reference implementation.
//
// - BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_BOX: libstdc++'s exposition-only
//   std::ranges::__detail::__box, tested raw. Raw works there because the
//   optional-based primary template publicly inherits optional, which
//   happens to supply every proposed spelling (and, as the suite records,
//   a few unintended extras).
//
// - BEMAN_MOVABLE_BOX_TEST_LIBCXX_BOX: libc++'s std::ranges::__movable_box.
// - BEMAN_MOVABLE_BOX_TEST_MSVC_BOX: the Microsoft STL's
//   std::ranges::_Movable_box.
//   Neither of these can be tested raw: each lacks several proposed
//   spellings outright (see the shim comments below), so the suite runs
//   against a shim -- a derived class that adds exactly the missing
//   members and nothing else. The shim is the point, not a workaround: it
//   is the candidate additive adoption patch, machine-checked to add no
//   data members and change no triviality (the parity checks live in the
//   test file, under BEMAN_MOVABLE_BOX_TEST_VENDOR_SHIM). Semantic
//   divergences the shim deliberately does not paper over remain visible
//   and are tagged as expected failures below.
//
// Expected-failure tags. Each names one known divergence from the proposed
// wording; a configuration defines the tag to Catch2's [!shouldfail] when
// the divergence is present in that vendor's box. Anything failing without
// a tag is evidence against the paper's wording.
//
// - ..._XFAIL_VALUE_TYPE: the box has no member value_type.
//   libstdc++ only: its no-discriminant specialization lacks it. The
//   libc++/MS STL shims add value_type, so those configurations pass.
//
// - ..._XFAIL_DEFAULT_CTOR: the default constructor is not constrained on
//   default_initializable<T>. libstdc++ only: the optional-based primary
//   template inherits optional's unconstrained default constructor, so a
//   __box of a non-default-initializable T default-constructs to a
//   disengaged box. libc++ and MS STL constrain their own default
//   constructors correctly.
//
// - ..._XFAIL_IN_PLACE_CONSTRAINT: the in_place constructor is not
//   constrained on is_constructible_v<T, Args...>. MS STL only: its
//   in_place constructor template is unconstrained (and non-explicit), so
//   is_constructible_v reports true for argument lists T cannot be
//   constructed from. Unobservable through the views; additive to close.
//
// - ..._XFAIL_SELF_ASSIGN: self-assignment of a box of a copyable payload
//   has effects. All three vendors: whenever T is copyable the assignment
//   is defaulted (libstdc++, libc++) or unconditionally delegates to T's
//   assignment (MS STL), so T's assignment operator observes the
//   self-assignment. The proposed wording specifies no effects when
//   this == addressof(rhs).
//
// - ..._XFAIL_SELF_ASSIGN_MOVABLE: self-move-assignment of a
//   *discriminated* box of a movable (but not copyable) payload has
//   effects. libc++ and MS STL only: libc++'s primary template defaults
//   its move assignment whenever T models movable (deferring to
//   optional's, which has no self-check), and the MS STL's movable
//   branches assign unconditionally. libstdc++'s optional-based primary
//   defines its own guarded assignment operators and conforms here.
//
// - ..._XFAIL_NOEXCEPT_MOVE_ASSIGN: the move assignment's exception
//   specification is stronger than the proposed formula. All three
//   vendors, for a payload whose move constructor may throw but whose
//   move assignment cannot: the no-discriminant implementations follow
//   T's move *assignment* alone (defaulted in libstdc++/libc++,
//   documented "strengthened" in MS STL). Conforming per
//   [res.on.exception.handling] p5; the suite's noexcept(false)
//   assertions record the reference implementation, not a requirement.

#if defined(BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_BOX)

    #include <ranges>

namespace box_under_test {
template <class T>
using movable_box = std::ranges::__detail::__box<T>;
} // namespace box_under_test

    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_VALUE_TYPE "[!shouldfail]"
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_DEFAULT_CTOR "[!shouldfail]"
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_IN_PLACE_CONSTRAINT
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_SELF_ASSIGN "[!shouldfail]"
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_SELF_ASSIGN_MOVABLE
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_NOEXCEPT_MOVE_ASSIGN "[!shouldfail]"

#elif defined(BEMAN_MOVABLE_BOX_TEST_LIBCXX_BOX)

    #include <concepts>
    #include <ranges>
    #include <type_traits>
    #include <utility>

namespace box_under_test {

template <class T>
using raw_box = std::ranges::__movable_box<T>;

// Additive shim over libc++'s box. std::ranges::__movable_box provides the
// default and in_place constructors, the special member functions,
// lvalue operator* and operator->, and __has_value(). Missing relative to
// the proposed interface, added here: value_type; the constructors from
// const T& and T&&; has_value() and operator bool; the rvalue operator*
// overloads (the vendor's operator* overloads are not ref-qualified, so
// they are re-declared here as the proposed &/const& signatures rather
// than brought in by a using-declaration, which cannot mix ref-qualified
// overloads with unqualified ones).
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
struct movable_box : raw_box<T> {
  private:
    using base = raw_box<T>;

  public:
    using value_type = T;

    constexpr movable_box() = default;

    template <class... Args>
        requires std::is_constructible_v<base, std::in_place_t, Args...>
    constexpr explicit movable_box(std::in_place_t, Args&&... args)
        : base(std::in_place, std::forward<Args>(args)...) {}

    constexpr explicit movable_box(const T& t)
        requires std::copy_constructible<T>
        : base(std::in_place, t) {}
    constexpr explicit movable_box(T&& t) : base(std::in_place, std::move(t)) {}

    constexpr bool     has_value() const noexcept { return this->__has_value(); }
    constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr T&        operator*() & noexcept { return *static_cast<base&>(*this); }
    constexpr const T&  operator*() const& noexcept { return *static_cast<const base&>(*this); }
    constexpr T&&       operator*() && noexcept { return std::move(*static_cast<base&>(*this)); }
    constexpr const T&& operator*() const&& noexcept { return std::move(*static_cast<const base&>(*this)); }
};

} // namespace box_under_test

    #define BEMAN_MOVABLE_BOX_TEST_VENDOR_SHIM 1

    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_VALUE_TYPE
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_DEFAULT_CTOR
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_IN_PLACE_CONSTRAINT
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_SELF_ASSIGN "[!shouldfail]"
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_SELF_ASSIGN_MOVABLE "[!shouldfail]"
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_NOEXCEPT_MOVE_ASSIGN "[!shouldfail]"

#elif defined(BEMAN_MOVABLE_BOX_TEST_MSVC_BOX)

    #include <concepts>
    #include <memory>
    #include <ranges>
    #include <type_traits>
    #include <utility>

namespace box_under_test {

template <class T>
using raw_box = std::ranges::_Movable_box<T>;

// Additive shim over the Microsoft STL's box. std::ranges::_Movable_box
// provides the default and in_place constructors, the special member
// functions, lvalue operator*, and explicit operator bool. Missing
// relative to the proposed interface, added here: value_type; the
// constructors from const T& and T&&; has_value(); operator->; the rvalue
// operator* overloads (as for libc++, the vendor's operator* overloads
// are not ref-qualified and are re-declared with the proposed
// signatures). The forwarding in_place constructor deliberately mirrors
// the vendor's unconstrained one; the resulting trait-level divergence is
// tagged ..._XFAIL_IN_PLACE_CONSTRAINT rather than papered over.
template <class T>
    requires std::move_constructible<T> && std::is_object_v<T>
struct movable_box : raw_box<T> {
  private:
    using base = raw_box<T>;

  public:
    using value_type = T;

    constexpr movable_box() = default;

    template <class... Args>
        requires std::is_constructible_v<base, std::in_place_t, Args...>
    constexpr explicit movable_box(std::in_place_t, Args&&... args)
        : base(std::in_place, std::forward<Args>(args)...) {}

    constexpr explicit movable_box(const T& t)
        requires std::copy_constructible<T>
        : base(std::in_place, t) {}
    constexpr explicit movable_box(T&& t) : base(std::in_place, std::move(t)) {}

    constexpr bool has_value() const noexcept { return static_cast<bool>(static_cast<const base&>(*this)); }

    constexpr T&        operator*() & noexcept { return *static_cast<base&>(*this); }
    constexpr const T&  operator*() const& noexcept { return *static_cast<const base&>(*this); }
    constexpr T&&       operator*() && noexcept { return std::move(*static_cast<base&>(*this)); }
    constexpr const T&& operator*() const&& noexcept { return std::move(*static_cast<const base&>(*this)); }

    constexpr T*       operator->() noexcept { return std::addressof(**this); }
    constexpr const T* operator->() const noexcept { return std::addressof(**this); }
};

} // namespace box_under_test

    #define BEMAN_MOVABLE_BOX_TEST_VENDOR_SHIM 1

    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_VALUE_TYPE
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_DEFAULT_CTOR
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_IN_PLACE_CONSTRAINT "[!shouldfail]"
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_SELF_ASSIGN "[!shouldfail]"
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_SELF_ASSIGN_MOVABLE "[!shouldfail]"
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_NOEXCEPT_MOVE_ASSIGN "[!shouldfail]"

#else

    #include <beman/movable_box/movable_box.hpp>

namespace box_under_test {
using beman::movable_box::movable_box;
} // namespace box_under_test

    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_VALUE_TYPE
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_DEFAULT_CTOR
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_IN_PLACE_CONSTRAINT
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_SELF_ASSIGN
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_SELF_ASSIGN_MOVABLE
    #define BEMAN_MOVABLE_BOX_TEST_XFAIL_NOEXCEPT_MOVE_ASSIGN

#endif

#endif // BEMAN_MOVABLE_BOX_TESTS_BOX_UNDER_TEST_HPP
