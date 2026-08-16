// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/movable_box/config.hpp>
#include <catch2/catch_all.hpp>
#include <beman/movable_box/movable_box.hpp>

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

// This suite tests movable_box<T> against the proposed wording in
// papers/movable_box.md ([movable.box.general] / [movable.box.ctor] /
// [movable.box.dtor] / [movable.box.assign] / [movable.box.obs]), not
// against any particular implementation strategy. Storage-optimization
// (D8/[movable.box.general]p3, recommended practice) is exercised only where
// the facts supplied for this reference implementation guarantee a specific
// sizeof/triviality outcome.
//
// The box under test is beman::movable_box::movable_box by default. With
// BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_BOX defined (see the conformance target
// in CMakeLists.txt), the same suite instead runs against libstdc++'s
// exposition-only box, std::ranges::__detail::__box: the paper claims its
// proposed surface is a strict subset of every shipping implementation, so
// a semantic test failing there would be a bad sign for the wording. The
// few assertions that hold only for the *proposed public* type (deliberate
// interface narrowings, or spec details where libstdc++'s exposition-only
// box knowingly diverges) are marked as Catch2 expected failures under that
// macro, each with a comment saying why. This configuration requires C++23
// (P2494R2 movable-box semantics; in C++20 libstdc++'s __box still requires
// copy_constructible).

#if defined(BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_BOX)
    #include <ranges>
namespace box_under_test {
template <class T>
using movable_box = std::ranges::__detail::__box<T>;
} // namespace box_under_test
    #define BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_XFAIL "[!shouldfail]"
#else
namespace box_under_test {
using beman::movable_box::movable_box;
} // namespace box_under_test
    #define BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_XFAIL
#endif

namespace {

// ---------------------------------------------------------------------
// Helper payload types
// ---------------------------------------------------------------------

// A counter block shared by an instrumented payload type, reset at the
// start of each TEST_CASE/SECTION that relies on it.
struct Counters {
    int default_ctor = 0;
    int value_ctor   = 0;
    int copy_ctor    = 0;
    int move_ctor    = 0;
    int copy_assign  = 0;
    int move_assign  = 0;
    int dtor         = 0;

    void reset() { *this = Counters{}; }
};

// Fully regular (copyable and movable), instrumented payload.
struct Regular {
    inline static Counters counters{};

    int a = 0;
    int b = 0;

    Regular() { ++counters.default_ctor; }
    explicit Regular(int va) : a(va) { ++counters.value_ctor; }
    Regular(int va, int vb) : a(va), b(vb) { ++counters.value_ctor; }
    Regular(const Regular& o) : a(o.a), b(o.b) { ++counters.copy_ctor; }
    Regular(Regular&& o) noexcept : a(o.a), b(o.b) { ++counters.move_ctor; }
    Regular& operator=(const Regular& o) {
        a = o.a;
        b = o.b;
        ++counters.copy_assign;
        return *this;
    }
    Regular& operator=(Regular&& o) noexcept {
        a = o.a;
        b = o.b;
        ++counters.move_assign;
        return *this;
    }
    ~Regular() { ++counters.dtor; }
};

// Copy-constructible but not copyable: copy/move assignment are deleted, as
// for a type with a const member. Also not "movable" for the same reason.
// This is the shape that forces the synthesized destroy-then-construct
// assignment described in [movable.box.assign].
struct NonAssignable {
    inline static Counters counters{};

    int value;

    explicit NonAssignable(int v) : value(v) { ++counters.value_ctor; }
    NonAssignable(const NonAssignable& o) noexcept : value(o.value) { ++counters.copy_ctor; }
    NonAssignable(NonAssignable&& o) noexcept : value(o.value) { ++counters.move_ctor; }
    NonAssignable& operator=(const NonAssignable&) = delete;
    NonAssignable& operator=(NonAssignable&&)      = delete;
    ~NonAssignable() { ++counters.dtor; }
};

// A trivially copyable, fully regular aggregate payload.
struct TrivialPayload {
    int    a;
    double b;
};

// A small, literal, non-assignable struct with a const member: copy- and
// move-constructible, but not assignable at all, and both constructors are
// trivial (and nothrow), so per the recommended practice this payload need
// not force a discriminant. Used for the constexpr coverage.
struct ConstMember {
    const int tag;
    int       value;
    constexpr ConstMember(int t, int v) : tag(t), value(v) {}
};

// Move-only payload: no copy constructor at all.
struct MoveOnly {
    inline static Counters counters{};

    int value;

    explicit MoveOnly(int v) : value(v) { ++counters.value_ctor; }
    MoveOnly(const MoveOnly&)            = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&& o) noexcept : value(o.value) {
        ++counters.move_ctor;
        o.value = -1;
    }
    MoveOnly& operator=(MoveOnly&&) = delete;
    ~MoveOnly() { ++counters.dtor; }
};

// Neither copy- nor move-constructible: movable_box<Immovable> must be
// ill-formed per D7.
struct Immovable {
    Immovable()                            = default;
    Immovable(const Immovable&)            = delete;
    Immovable(Immovable&&)                 = delete;
    Immovable& operator=(const Immovable&) = delete;
    Immovable& operator=(Immovable&&)      = delete;
};

// Copy-constructible, but the copy constructor can be armed to throw on
// demand; copy assignment is deleted, so it is not copyable. Because the
// copy constructor is not nothrow and the type is not copyable, the
// recommended practice does not allow storing only a T: this payload forces
// a real, reachable empty state ([movable.box.general]p2/p3).
struct ThrowingCopyError {};

struct ThrowingCopy {
    inline static bool armed = false;

    int value;

    explicit ThrowingCopy(int v) : value(v) {}
    ThrowingCopy(const ThrowingCopy& o) : value(o.value) {
        if (armed) {
            armed = false;
            throw ThrowingCopyError{};
        }
    }
    ThrowingCopy(ThrowingCopy&&) noexcept        = default;
    ThrowingCopy& operator=(const ThrowingCopy&) = delete;
    ThrowingCopy& operator=(ThrowingCopy&&)      = delete;
    ~ThrowingCopy()                              = default;
};

// Representative types for the noexcept formulas in [movable.box.assign].

// Copyable, but copy assignment is not nothrow even though the copy
// constructor is.
struct ThrowingCopyAssign {
    int value;
    explicit ThrowingCopyAssign(int v) : value(v) {}
    ThrowingCopyAssign(const ThrowingCopyAssign& o) noexcept : value(o.value) {}
    ThrowingCopyAssign& operator=(const ThrowingCopyAssign& o) {
        value = o.value;
        return *this;
    }
    ThrowingCopyAssign(ThrowingCopyAssign&&) noexcept            = default;
    ThrowingCopyAssign& operator=(ThrowingCopyAssign&&) noexcept = default;
};

// Not nothrow move-constructible (even though it is otherwise movable).
struct ThrowingMove {
    int value;
    explicit ThrowingMove(int v) : value(v) {}
    ThrowingMove(const ThrowingMove& o) : value(o.value) {}
    ThrowingMove(ThrowingMove&& o) : value(o.value) {}
    ThrowingMove& operator=(const ThrowingMove&) = default;
    ThrowingMove& operator=(ThrowingMove&&)      = default;
};

// Movable, with a nothrow move constructor, but move assignment is not
// nothrow.
struct ThrowingMoveAssign {
    int value;
    explicit ThrowingMoveAssign(int v) : value(v) {}
    ThrowingMoveAssign(const ThrowingMoveAssign&)            = delete;
    ThrowingMoveAssign& operator=(const ThrowingMoveAssign&) = delete;
    ThrowingMoveAssign(ThrowingMoveAssign&& o) noexcept : value(o.value) {}
    ThrowingMoveAssign& operator=(ThrowingMoveAssign&& o) {
        value = o.value;
        return *this;
    }
};

// Movable but not copyable, with potentially-throwing construction: this
// forces boxed storage while still taking the movable<T> assignment branch.
struct BoxedMovable {
    inline static bool armed = false;

    int value;

    explicit BoxedMovable(int v) : value(v) {}
    BoxedMovable(const BoxedMovable& o) : value(o.value) {
        if (armed) {
            armed = false;
            throw ThrowingCopyError{};
        }
    }
    BoxedMovable(BoxedMovable&& o) : value(o.value) { o.value = -1; }
    BoxedMovable& operator=(const BoxedMovable&) = delete;
    BoxedMovable& operator=(BoxedMovable&& o) noexcept {
        value   = o.value;
        o.value = -1;
        return *this;
    }
};

// Detects whether box_under_test::movable_box<T> is a valid type: used
// to check the constraints in D7 without triggering a hard compile error
// for the negative cases.
template <class T>
concept can_box = requires { sizeof(box_under_test::movable_box<T>); };

// Detects whether Box provides a nested value_type: used so that a box
// lacking it (libstdc++'s __box) fails the check at runtime rather than
// failing to compile.
template <class Box>
concept box_has_value_type = requires { typename Box::value_type; };

// Checks Box::value_type against Expected when present. A template, so that
// when box_has_value_type<Box> is false, Box::value_type -- a dependent
// name -- is never instantiated instead of being a hard compile error.
template <class Box, class Expected>
void check_value_type_is_impl() {
    if constexpr (box_has_value_type<Box>) {
        CHECK(std::same_as<typename Box::value_type, Expected>);
    }
}

} // namespace

// ---------------------------------------------------------------------
// Interface presence (D6) and constraints (D7)
// ---------------------------------------------------------------------

// Proposed-public-interface only: value_type is part of the proposed
// synopsis; libstdc++'s no-discriminant __box specialization does not
// provide it (the exposition-only type has no reason to). Checked via
// box_has_value_type rather than excluded at compile time, so the absence
// is reported as a Catch2 expected failure instead of a build failure.
TEST_CASE("value_type names T", "[movable_box]" BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_XFAIL) {
    CHECK(box_has_value_type<box_under_test::movable_box<int>>);
    CHECK(box_has_value_type<box_under_test::movable_box<Regular>>);

    check_value_type_is_impl<box_under_test::movable_box<int>, int>();
    check_value_type_is_impl<box_under_test::movable_box<Regular>, Regular>();
}

TEST_CASE("default constructor is present iff default_initializable<T>",
          "[movable_box]" BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_XFAIL) {
    STATIC_REQUIRE(std::default_initializable<int>);
    STATIC_REQUIRE(std::default_initializable<box_under_test::movable_box<int>>);

    // Delta-specification hazard, exactly as argued in D5: libstdc++'s
    // optional-based primary __box inherits optional's constructors --
    // including the default constructor -- so __box<ThrowingCopy>{} is
    // well-formed and produces a *disengaged* box even though ThrowingCopy
    // is not default_initializable. Unobservable through the views (their
    // own default constructors are constrained on default_initializable<T>),
    // but it is the kind of accident a standalone specification precludes.
    // Checked with a runtime CHECK rather than STATIC_REQUIRE so that the
    // divergence is a Catch2 expected failure, not a build failure, under
    // BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_BOX.
    CHECK_FALSE(std::default_initializable<ThrowingCopy>);
    CHECK_FALSE(std::default_initializable<box_under_test::movable_box<ThrowingCopy>>);
}

TEST_CASE("value constructors from const T& and T&& are explicit and non-template", "[movable_box]") {
    using Box = box_under_test::movable_box<Regular>;

    STATIC_REQUIRE(std::is_constructible_v<Box, const Regular&>);
    STATIC_REQUIRE_FALSE(std::is_convertible_v<const Regular&, Box>);

    STATIC_REQUIRE(std::is_constructible_v<Box, Regular&&>);
    STATIC_REQUIRE_FALSE(std::is_convertible_v<Regular&&, Box>);
}

TEST_CASE("in_place constructor is present and constrained on is_constructible_v<T, Args...>", "[movable_box]") {
    using Box = box_under_test::movable_box<Regular>;

    STATIC_REQUIRE(std::is_constructible_v<Box, std::in_place_t, int, int>);
    STATIC_REQUIRE_FALSE(std::is_constructible_v<Box, std::in_place_t, Regular*>);
}

TEST_CASE("copy constructor is present iff copy_constructible<T>", "[movable_box]") {
    STATIC_REQUIRE(std::copy_constructible<Regular>);
    STATIC_REQUIRE(std::copy_constructible<box_under_test::movable_box<Regular>>);

    STATIC_REQUIRE_FALSE(std::copy_constructible<MoveOnly>);
    STATIC_REQUIRE_FALSE(std::copy_constructible<box_under_test::movable_box<MoveOnly>>);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<box_under_test::movable_box<MoveOnly>>);
}

TEST_CASE("the box always models movable when T is move_constructible", "[movable_box]") {
    STATIC_REQUIRE(std::movable<box_under_test::movable_box<int>>);
    STATIC_REQUIRE(std::movable<box_under_test::movable_box<Regular>>);
    STATIC_REQUIRE(std::movable<box_under_test::movable_box<NonAssignable>>);
    STATIC_REQUIRE(std::movable<box_under_test::movable_box<MoveOnly>>);
    STATIC_REQUIRE(std::movable<box_under_test::movable_box<ThrowingCopy>>);
}

TEST_CASE("absent interface: no value assignment, no equality, move-only box is not copyable", "[movable_box]") {
    using RegularBox = box_under_test::movable_box<Regular>;

    // D9: value assignment (operator=(U&&)) is not part of the interface.
    STATIC_REQUIRE_FALSE(std::is_assignable_v<RegularBox&, Regular>);
    STATIC_REQUIRE_FALSE(std::is_assignable_v<RegularBox&, Regular&&>);

    // D9: comparison operators are not part of the interface.
    STATIC_REQUIRE_FALSE(std::equality_comparable<RegularBox>);

    // A box of a move-only type is not copyable, even though it is movable.
    using MoveOnlyBox = box_under_test::movable_box<MoveOnly>;
    STATIC_REQUIRE_FALSE(std::copyable<MoveOnlyBox>);
    STATIC_REQUIRE(std::movable<MoveOnlyBox>);
}

TEST_CASE("constraints: no reference types, no void, no immovable types (D7)", "[movable_box]") {
    STATIC_REQUIRE(can_box<int>);
    STATIC_REQUIRE(can_box<Regular>);

    STATIC_REQUIRE_FALSE(can_box<int&>);
    STATIC_REQUIRE_FALSE(can_box<void>);
    STATIC_REQUIRE_FALSE(can_box<Immovable>);
}

// ---------------------------------------------------------------------
// Construction semantics ([movable.box.ctor])
// ---------------------------------------------------------------------

TEST_CASE("default construction value-initializes the contained value", "[movable_box]") {
    Regular::counters.reset();

    box_under_test::movable_box<Regular> box;
    CHECK(box.has_value());
    CHECK(box->a == 0);
    CHECK(box->b == 0);
    CHECK(Regular::counters.default_ctor == 1);
}

TEST_CASE("construction from an lvalue copies, from an rvalue moves", "[movable_box]") {
    Regular src(1, 2);

    Regular::counters.reset();
    box_under_test::movable_box<Regular> from_lvalue(src);
    CHECK(Regular::counters.copy_ctor == 1);
    CHECK(Regular::counters.move_ctor == 0);
    CHECK(from_lvalue->a == 1);
    CHECK(from_lvalue->b == 2);

    Regular::counters.reset();
    box_under_test::movable_box<Regular> from_rvalue(Regular(5, 6));
    CHECK(Regular::counters.move_ctor == 1);
    CHECK(Regular::counters.copy_ctor == 0);
    CHECK(from_rvalue->a == 5);
    CHECK(from_rvalue->b == 6);
}

TEST_CASE("in_place construction forwards multiple arguments", "[movable_box]") {
    Regular::counters.reset();

    box_under_test::movable_box<Regular> box(std::in_place, 3, 4);
    CHECK(box->a == 3);
    CHECK(box->b == 4);
    CHECK(Regular::counters.value_ctor == 1);
    CHECK(Regular::counters.default_ctor == 0);
}

TEST_CASE("copy construction copies, move construction moves the contained value", "[movable_box]") {
    box_under_test::movable_box<Regular> a(std::in_place, 7, 8);

    Regular::counters.reset();
    box_under_test::movable_box<Regular> b(a);
    CHECK(Regular::counters.copy_ctor == 1);
    CHECK(b->a == 7);
    CHECK(b->b == 8);
    CHECK(a.has_value());

    Regular::counters.reset();
    box_under_test::movable_box<Regular> c(std::move(a));
    CHECK(Regular::counters.move_ctor == 1);
    CHECK(c->a == 7);
    // [movable.box.ctor]p8: rhs.has_value() is unchanged by move construction.
    CHECK(a.has_value());
}

TEST_CASE("dereference returns references to the same object", "[movable_box]") {
    box_under_test::movable_box<Regular> box(std::in_place, 1, 2);
    const auto&                          cbox = box;

    CHECK(std::addressof(*box) == box.operator->());
    CHECK(std::addressof(*cbox) == cbox.operator->());
    CHECK(static_cast<const void*>(std::addressof(*box)) == static_cast<const void*>(std::addressof(*cbox)));

    // Repeated dereference returns the same address.
    CHECK(std::addressof(*box) == std::addressof(*box));
}

TEST_CASE("rvalue dereference returns T&&", "[movable_box]") {
    using Box = box_under_test::movable_box<Regular>;

    STATIC_REQUIRE(std::same_as<decltype(*std::declval<Box&&>()), Regular&&>);
    STATIC_REQUIRE(std::same_as<decltype(*std::declval<const Box&&>()), const Regular&&>);
    STATIC_REQUIRE(std::same_as<decltype(*std::declval<Box&>()), Regular&>);
    STATIC_REQUIRE(std::same_as<decltype(*std::declval<const Box&>()), const Regular&>);

    Box box(std::in_place, 1, 2);
    Regular::counters.reset();
    Regular moved_out(*std::move(box));
    CHECK(Regular::counters.move_ctor == 1);
    CHECK(Regular::counters.copy_ctor == 0);
    CHECK(moved_out.a == 1);
}

// ---------------------------------------------------------------------
// Synthesized assignment ([movable.box.assign])
// ---------------------------------------------------------------------

TEST_CASE("copy assignment of a non-assignable T destroys then copy-constructs", "[movable_box]") {
    box_under_test::movable_box<NonAssignable> a(std::in_place, 1);
    box_under_test::movable_box<NonAssignable> b(std::in_place, 2);

    NonAssignable::counters.reset();
    a = b;
    CHECK(NonAssignable::counters.dtor == 1);
    CHECK(NonAssignable::counters.copy_ctor == 1);
    CHECK(NonAssignable::counters.copy_assign == 0);
    CHECK(a->value == 2);
    CHECK(b->value == 2);
}

TEST_CASE("move assignment of a non-movable T destroys then move-constructs", "[movable_box]") {
    box_under_test::movable_box<NonAssignable> a(std::in_place, 1);
    box_under_test::movable_box<NonAssignable> b(std::in_place, 2);

    NonAssignable::counters.reset();
    a = std::move(b);
    CHECK(NonAssignable::counters.dtor == 1);
    CHECK(NonAssignable::counters.move_ctor == 1);
    CHECK(NonAssignable::counters.move_assign == 0);
    CHECK(a->value == 2);
    // [movable.box.assign]p4.2: rhs.has_value() is unchanged.
    CHECK(b.has_value());
}

TEST_CASE("self-assignment is a no-op", "[movable_box]") {
    box_under_test::movable_box<NonAssignable> a(std::in_place, 42);

    NonAssignable::counters.reset();
    auto& self_ref = a;
    a              = self_ref;
    CHECK(NonAssignable::counters.dtor == 0);
    CHECK(NonAssignable::counters.copy_ctor == 0);
    CHECK(a->value == 42);

    a = std::move(self_ref);
    CHECK(NonAssignable::counters.dtor == 0);
    CHECK(NonAssignable::counters.move_ctor == 0);
    CHECK(a->value == 42);

    box_under_test::movable_box<ThrowingCopy> throwing(std::in_place, 7);
    auto&                                    throwing_ref = throwing;

    ThrowingCopy::armed = true;
    throwing            = throwing_ref;
    CHECK(ThrowingCopy::armed);
    CHECK(throwing.has_value());
    CHECK(throwing->value == 7);

    throwing = std::move(throwing_ref);
    CHECK(ThrowingCopy::armed);
    CHECK(throwing.has_value());
    CHECK(throwing->value == 7);
    ThrowingCopy::armed = false;
}

TEST_CASE("self-assignment of a copyable payload has no effects",
          "[movable_box]" BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_XFAIL) {
    // [movable.box.assign] specifies no effects for this == addressof(rhs).
    // libstdc++'s exposition-only no-discriminant __box specialization defaults
    // assignment for copyable T, so T's assignment operator still observes the
    // self-assignment.
    box_under_test::movable_box<Regular> regular(std::in_place, 1, 2);
    auto&                               regular_ref = regular;

    Regular::counters.reset();
    regular = regular_ref;
    CHECK(Regular::counters.copy_assign == 0);
    CHECK(Regular::counters.copy_ctor == 0);
    CHECK(regular->a == 1);
    CHECK(regular->b == 2);

    regular = std::move(regular_ref);
    CHECK(Regular::counters.move_assign == 0);
    CHECK(Regular::counters.move_ctor == 0);
    CHECK(regular->a == 1);
    CHECK(regular->b == 2);
}

TEST_CASE("copy assignment of a fully copyable T uses T's copy assignment operator", "[movable_box]") {
    box_under_test::movable_box<Regular> a(std::in_place, 1, 1);
    box_under_test::movable_box<Regular> b(std::in_place, 2, 2);

    Regular::counters.reset();
    a = b;
    CHECK(Regular::counters.copy_assign == 1);
    CHECK(Regular::counters.copy_ctor == 0);
    CHECK(Regular::counters.dtor == 0);
    CHECK(a->a == 2);
    CHECK(a->b == 2);
}

TEST_CASE("move assignment of a fully movable T uses T's move assignment operator", "[movable_box]") {
    box_under_test::movable_box<Regular> a(std::in_place, 1, 1);
    box_under_test::movable_box<Regular> b(std::in_place, 2, 2);

    Regular::counters.reset();
    a = std::move(b);
    CHECK(Regular::counters.move_assign == 1);
    CHECK(Regular::counters.move_ctor == 0);
    CHECK(Regular::counters.dtor == 0);
    CHECK(a->a == 2);
    CHECK(a->b == 2);
}

TEST_CASE("assignment of a boxed movable T handles empty targets and sources", "[movable_box]") {
    STATIC_REQUIRE(std::movable<BoxedMovable>);
    STATIC_REQUIRE(std::copy_constructible<BoxedMovable>);
    STATIC_REQUIRE_FALSE(std::copyable<BoxedMovable>);
    STATIC_REQUIRE_FALSE(std::is_nothrow_copy_constructible_v<BoxedMovable>);
    STATIC_REQUIRE_FALSE(std::is_nothrow_move_constructible_v<BoxedMovable>);

    box_under_test::movable_box<BoxedMovable> empty(std::in_place, 1);
    box_under_test::movable_box<BoxedMovable> source(std::in_place, 2);

    BoxedMovable::armed = true;
    CHECK_THROWS_AS(empty = source, ThrowingCopyError);
    REQUIRE_FALSE(empty.has_value());

    empty = std::move(source);
    CHECK(empty.has_value());
    CHECK(empty->value == 2);
    // [movable.box.assign]p5.1: rhs.has_value() is unchanged.
    CHECK(source.has_value());

    box_under_test::movable_box<BoxedMovable> engaged_target(std::in_place, 6);
    box_under_test::movable_box<BoxedMovable> engaged_source(std::in_place, 7);

    engaged_target = std::move(engaged_source);
    CHECK(engaged_target.has_value());
    CHECK(engaged_target->value == 7);
    // [movable.box.assign]p5.1: rhs.has_value() is unchanged.
    CHECK(engaged_source.has_value());

    box_under_test::movable_box<BoxedMovable> empty_source(std::in_place, 3);
    box_under_test::movable_box<BoxedMovable> copy_source(std::in_place, 4);

    BoxedMovable::armed = true;
    CHECK_THROWS_AS(empty_source = copy_source, ThrowingCopyError);
    REQUIRE_FALSE(empty_source.has_value());

    empty_source = copy_source;
    CHECK(empty_source.has_value());
    CHECK(empty_source->value == 4);

    box_under_test::movable_box<BoxedMovable> empty_move_source(std::in_place, 8);

    BoxedMovable::armed = true;
    CHECK_THROWS_AS(empty_move_source = copy_source, ThrowingCopyError);
    REQUIRE_FALSE(empty_move_source.has_value());

    box_under_test::movable_box<BoxedMovable> copy_from_empty_target(std::in_place, 9);

    copy_from_empty_target = empty_move_source;
    CHECK_FALSE(copy_from_empty_target.has_value());

    box_under_test::movable_box<BoxedMovable> target(std::in_place, 5);
    auto&                                    self = target;

    target = self;
    CHECK(target.has_value());
    CHECK(target->value == 5);

    target = std::move(self);
    CHECK(target.has_value());
    CHECK(target->value == 5);

    target = std::move(empty_move_source);
    CHECK_FALSE(target.has_value());
    CHECK_FALSE(empty_move_source.has_value());
}

// ---------------------------------------------------------------------
// Valueless-by-exception ([movable.box.general]p2)
// ---------------------------------------------------------------------

TEST_CASE("copy assignment exception during the synthesized construct leaves the target empty", "[movable_box]") {
    box_under_test::movable_box<ThrowingCopy> a(std::in_place, 1);
    box_under_test::movable_box<ThrowingCopy> b(std::in_place, 2);

    ThrowingCopy::armed = true;
    CHECK_THROWS_AS(a = b, ThrowingCopyError);

    CHECK_FALSE(a.has_value());
    CHECK(static_cast<bool>(a) == a.has_value());

    // rhs is untouched by the source-side exception.
    CHECK(b.has_value());
    CHECK(b->value == 2);
}

TEST_CASE("valueless state propagates through copy and move construction", "[movable_box]") {
    box_under_test::movable_box<ThrowingCopy> a(std::in_place, 1);
    box_under_test::movable_box<ThrowingCopy> b(std::in_place, 2);

    ThrowingCopy::armed = true;
    CHECK_THROWS_AS(a = b, ThrowingCopyError);
    REQUIRE_FALSE(a.has_value());

    box_under_test::movable_box<ThrowingCopy> c(a); // copy from an empty box
    CHECK_FALSE(c.has_value());

    box_under_test::movable_box<ThrowingCopy> d(std::move(a)); // move from an empty box
    CHECK_FALSE(d.has_value());
    // [movable.box.ctor]p8: rhs.has_value() is unchanged by move construction.
    CHECK_FALSE(a.has_value());
}

TEST_CASE("engaged discriminated boxes copy and move construct the contained value", "[movable_box]") {
    box_under_test::movable_box<ThrowingCopy> a(std::in_place, 1);

    box_under_test::movable_box<ThrowingCopy> b(a);
    CHECK(b.has_value());
    CHECK(b->value == 1);

    box_under_test::movable_box<ThrowingCopy> c(std::move(a));
    CHECK(c.has_value());
    CHECK(c->value == 1);
    // [movable.box.ctor]p8: rhs.has_value() is unchanged by move construction.
    CHECK(a.has_value());
}

TEST_CASE("assigning an engaged box into an empty box re-engages it", "[movable_box]") {
    box_under_test::movable_box<ThrowingCopy> a(std::in_place, 1);
    box_under_test::movable_box<ThrowingCopy> b(std::in_place, 2);

    ThrowingCopy::armed = true;
    CHECK_THROWS_AS(a = b, ThrowingCopyError);
    REQUIRE_FALSE(a.has_value());

    a = b; // not armed this time: succeeds.
    CHECK(a.has_value());
    CHECK(a->value == 2);
    CHECK(static_cast<bool>(a) == a.has_value());

    box_under_test::movable_box<ThrowingCopy> c(std::in_place, 3);

    ThrowingCopy::armed = true;
    CHECK_THROWS_AS(c = b, ThrowingCopyError);
    REQUIRE_FALSE(c.has_value());

    c = std::move(b);
    CHECK(c.has_value());
    CHECK(c->value == 2);
    // [movable.box.assign]p5.1: rhs.has_value() is unchanged.
    CHECK(b.has_value());
}

TEST_CASE("assigning an empty box into an engaged box disengages it", "[movable_box]") {
    box_under_test::movable_box<ThrowingCopy> empty(std::in_place, 1);
    box_under_test::movable_box<ThrowingCopy> source(std::in_place, 2);

    ThrowingCopy::armed = true;
    CHECK_THROWS_AS(empty = source, ThrowingCopyError);
    REQUIRE_FALSE(empty.has_value());

    box_under_test::movable_box<ThrowingCopy> copy_target(std::in_place, 3);
    copy_target = empty;
    CHECK_FALSE(copy_target.has_value());

    box_under_test::movable_box<ThrowingCopy> move_target(std::in_place, 4);
    move_target = std::move(empty);
    CHECK_FALSE(move_target.has_value());
    // [movable.box.assign]p4.2: rhs.has_value() is unchanged.
    CHECK_FALSE(empty.has_value());
}

// ---------------------------------------------------------------------
// Triviality and storage propagation (D8, and the recommended-practice
// storage optimization this reference implementation follows)
// ---------------------------------------------------------------------

TEST_CASE("movable_box<int> propagates triviality and storage of int", "[movable_box]") {
    using Box = box_under_test::movable_box<int>;
    STATIC_REQUIRE(std::is_trivially_copy_constructible_v<Box>);
    STATIC_REQUIRE(std::is_trivially_move_constructible_v<Box>);
    STATIC_REQUIRE(std::is_trivially_destructible_v<Box>);
    STATIC_REQUIRE(std::is_trivially_copy_assignable_v<Box>);
    STATIC_REQUIRE(std::is_trivially_move_assignable_v<Box>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<Box>);
    STATIC_REQUIRE(sizeof(Box) == sizeof(int));
}

TEST_CASE("a trivially copyable struct payload keeps the whole box trivially copyable", "[movable_box]") {
    // QoI beyond the paper's normative Remarks (which cover only the
    // constructors and destructor): when T is assignable and fully trivial,
    // the synthesized assignments reduce to copying the object
    // representation, so they are defaulted and the box -- and any view
    // storing one -- stays trivially copyable, matching libstdc++'s box.
    using Box = box_under_test::movable_box<TrivialPayload>;
    STATIC_REQUIRE(std::is_trivially_copyable_v<TrivialPayload>);
    STATIC_REQUIRE(std::is_trivially_copy_constructible_v<Box>);
    STATIC_REQUIRE(std::is_trivially_move_constructible_v<Box>);
    STATIC_REQUIRE(std::is_trivially_destructible_v<Box>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<Box>);
    STATIC_REQUIRE(sizeof(Box) == sizeof(TrivialPayload));
}

TEST_CASE("assignment stays user-provided (non-trivial) whenever the synthesized semantics are observable",
          "[movable_box]") {
    // A trivially copyable but non-assignable payload: the box must
    // synthesize destroy-then-construct, which cannot be a defaulted
    // (member-wise) assignment, so only the constructors and destructor are
    // trivial.
    using ConstBox = box_under_test::movable_box<ConstMember>;
    STATIC_REQUIRE(std::is_trivially_copyable_v<ConstMember>);
    STATIC_REQUIRE(std::is_copy_assignable_v<ConstBox>);
    STATIC_REQUIRE_FALSE(std::is_trivially_copy_assignable_v<ConstBox>);
    STATIC_REQUIRE_FALSE(std::is_trivially_move_assignable_v<ConstBox>);

    // A discriminated box: assignment manages the engaged flag, never trivial.
    using ThrowingBox = box_under_test::movable_box<ThrowingCopy>;
    STATIC_REQUIRE(std::is_copy_assignable_v<ThrowingBox>);
    STATIC_REQUIRE_FALSE(std::is_trivially_copy_assignable_v<ThrowingBox>);
    STATIC_REQUIRE_FALSE(std::is_trivially_move_assignable_v<ThrowingBox>);

    // A payload with its own non-trivial assignment: the box's assignment
    // defers to it and is therefore also non-trivial.
    using RegularBox = box_under_test::movable_box<Regular>;
    STATIC_REQUIRE(std::is_copy_assignable_v<RegularBox>);
    STATIC_REQUIRE_FALSE(std::is_trivially_copy_assignable_v<RegularBox>);
}

TEST_CASE("destructor triviality propagates from T even when a discriminant is present", "[movable_box]") {
    // ThrowingCopy's destructor is trivial even though its copy constructor
    // is not, and even though this payload forces a discriminant.
    STATIC_REQUIRE(std::is_trivially_destructible_v<ThrowingCopy>);
    STATIC_REQUIRE(std::is_trivially_destructible_v<box_under_test::movable_box<ThrowingCopy>>);
    STATIC_REQUIRE_FALSE(std::is_trivially_copy_constructible_v<box_under_test::movable_box<ThrowingCopy>>);
}

TEST_CASE("a payload that forces a discriminant has a strictly larger box (recommended practice)", "[movable_box]") {
    // ThrowingCopy is copy_constructible but neither copyable nor
    // (nothrow_move_constructible && nothrow_copy_constructible), so the
    // recommended practice does not permit storing only a ThrowingCopy.
    STATIC_REQUIRE(std::copy_constructible<ThrowingCopy>);
    STATIC_REQUIRE_FALSE(std::copyable<ThrowingCopy>);
    STATIC_REQUIRE_FALSE(std::is_nothrow_copy_constructible_v<ThrowingCopy>);

    STATIC_REQUIRE(sizeof(box_under_test::movable_box<ThrowingCopy>) > sizeof(ThrowingCopy));
}

TEST_CASE("move-only payloads with a nothrow move constructor need no discriminant either", "[movable_box]") {
    // MoveOnly is not copy_constructible, but its move constructor is
    // nothrow, so the recommended practice's "Otherwise" clause applies.
    STATIC_REQUIRE_FALSE(std::copy_constructible<MoveOnly>);
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<MoveOnly>);
    STATIC_REQUIRE(sizeof(box_under_test::movable_box<MoveOnly>) == sizeof(MoveOnly));
}

TEST_CASE("noexcept: move constructor is noexcept iff T's move constructor is", "[movable_box]") {
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<Regular>);
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<box_under_test::movable_box<Regular>>);

    STATIC_REQUIRE_FALSE(std::is_nothrow_move_constructible_v<ThrowingMove>);
    STATIC_REQUIRE_FALSE(std::is_nothrow_move_constructible_v<box_under_test::movable_box<ThrowingMove>>);
}

TEST_CASE("noexcept: copy assignment matches is_nothrow_copy_constructible_v<T> && "
          "(!copyable<T> || is_nothrow_copy_assignable_v<T>)",
          "[movable_box]") {
    using TrivialBox = box_under_test::movable_box<TrivialPayload>;
    STATIC_REQUIRE(noexcept(std::declval<TrivialBox&>() = std::declval<const TrivialBox&>()));

    using ThrowingCopyBox = box_under_test::movable_box<ThrowingCopy>;
    STATIC_REQUIRE_FALSE(noexcept(std::declval<ThrowingCopyBox&>() = std::declval<const ThrowingCopyBox&>()));

    using ThrowingAssignBox = box_under_test::movable_box<ThrowingCopyAssign>;
    STATIC_REQUIRE_FALSE(noexcept(std::declval<ThrowingAssignBox&>() = std::declval<const ThrowingAssignBox&>()));
}

TEST_CASE("noexcept: move assignment matches is_nothrow_move_constructible_v<T> && "
          "(!movable<T> || is_nothrow_move_assignable_v<T>)",
          "[movable_box]" BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_XFAIL) {
    using RegularBox = box_under_test::movable_box<Regular>;
    STATIC_REQUIRE(noexcept(std::declval<RegularBox&>() = std::declval<RegularBox>()));

    // The assertions below that a box is *not* noexcept are stricter than the
    // wording can be: [res.on.exception.handling] p5 lets an implementation
    // strengthen any non-virtual function's exception specification, so no
    // conformance test may require noexcept(false). They record what this
    // reference implementation does, not what the proposal guarantees.
    //
    // libstdc++ exercises exactly that freedom: for movable T its
    // no-discriminant __box defaults the move assignment, whose noexcept
    // follows T's move *assignment* alone -- stronger than the
    // optional-derived formula when T's move constructor throws but its move
    // assignment does not. Its own optional-based primary template gives the
    // weaker answer, so the two specializations disagree, and both conform.
    // Checked with a runtime CHECK_FALSE rather than STATIC_REQUIRE_FALSE so
    // that the strengthening is a Catch2 expected failure, not a build failure,
    // under BEMAN_MOVABLE_BOX_TEST_LIBSTDCXX_BOX.
    using ThrowingMoveBox = box_under_test::movable_box<ThrowingMove>;
    CHECK_FALSE(noexcept(std::declval<ThrowingMoveBox&>() = std::declval<ThrowingMoveBox>()));

    using ThrowingMoveAssignBox = box_under_test::movable_box<ThrowingMoveAssign>;
    STATIC_REQUIRE_FALSE(noexcept(std::declval<ThrowingMoveAssignBox&>() = std::declval<ThrowingMoveAssignBox>()));
}

// ---------------------------------------------------------------------
// constexpr (D10)
// ---------------------------------------------------------------------

namespace {

constexpr int exercise_int_box() {
    box_under_test::movable_box<int> a; // a == 0 (value-initialized)
    box_under_test::movable_box<int> b(std::in_place, 5);
    box_under_test::movable_box<int> c(7); // movable_box(T&&)

    a = b;                                            // copy assignment: a == 5
    box_under_test::movable_box<int> d(a);            // copy construction: d == 5
    a = std::move(c);                                 // move assignment: a == 7
    box_under_test::movable_box<int> e(std::move(d)); // move construction: e == 5

    return *a + *e; // 7 + 5
}
static_assert(exercise_int_box() == 12);

constexpr int exercise_const_member_box() {
    box_under_test::movable_box<ConstMember> a(std::in_place, 1, 10);
    box_under_test::movable_box<ConstMember> b(std::in_place, 2, 20);

    box_under_test::movable_box<ConstMember> copy_ctor_result(a);
    a = b; // synthesized destroy-then-copy-construct: ConstMember is not assignable
    box_under_test::movable_box<ConstMember> move_ctor_result(std::move(b));
    a = std::move(move_ctor_result); // synthesized destroy-then-move-construct

    return copy_ctor_result->value + a->value;
}
static_assert(exercise_const_member_box() == 10 + 20);

} // namespace

TEST_CASE("constexpr coverage compiles and evaluates as expected", "[movable_box]") {
    STATIC_REQUIRE(exercise_int_box() == 12);
    STATIC_REQUIRE(exercise_const_member_box() == 30);
}

// ---------------------------------------------------------------------
// Capturing lambda end-to-end
// ---------------------------------------------------------------------

TEST_CASE("a capturing lambda can be boxed, copy-assigned, and invoked through * and ->", "[movable_box]") {
    // A single lambda-expression (one source location) makes every value it
    // produces the same closure type, so this factory is what lets box_a
    // and box_b below share a movable_box<LambdaT> specialization.
    auto make_adder = [](int captured) { return [captured](int y) { return captured + y; }; };
    using LambdaT   = decltype(make_adder(0));

    STATIC_REQUIRE(std::copy_constructible<LambdaT>);
    STATIC_REQUIRE_FALSE(std::copyable<LambdaT>);

    auto lambda_a = make_adder(5);
    auto lambda_b = make_adder(100);

    box_under_test::movable_box<LambdaT> box_a(lambda_a);
    box_under_test::movable_box<LambdaT> box_b(lambda_b);

    CHECK((*box_a)(1) == 6);

    box_a = box_b; // synthesized destroy-then-copy-construct

    auto& self = box_a;
    box_a      = self;
    CHECK((*box_a)(3) == 103);

    box_a = std::move(self);
    CHECK((*box_a)(3) == 103);
    CHECK((*box_a.operator->())(3) == 103);

    auto lambda_c = make_adder(200);
    box_under_test::movable_box<LambdaT> box_c(lambda_c);

    box_a = std::move(box_c); // synthesized destroy-then-move-construct
    CHECK((*box_a)(4) == 204);
    // [movable.box.assign]p4.2: rhs.has_value() is unchanged.
    CHECK(box_c.has_value());
}
