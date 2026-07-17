---
title: "`std::movable_box`: Give Users the Ranges Movable Wrapper"
document: DTBD
date: today
audience: LEWG
author:
  - name: Steve Downey
    email: <sdowney@gmail.com>
toc: true
toc-depth: 2
include-before:
  - |
    ```{=latex}
    \setlength{\emergencystretch}{10em}
    ```
references:
  - id: range-v3
    citation-label: range-v3
    title: "range-v3: Range library for C++14/17/20"
    author:
      - family: Niebler
        given: Eric
    URL: https://github.com/ericniebler/range-v3
  - id: beman-movable-box
    citation-label: beman.movable_box
    title: "beman.movable_box: A non-exposition-only movable-box"
    author:
      - family: Downey
        given: Steve
    URL: https://github.com/bemanproject/movable_box
  - id: rangemovewrap
    citation-label: range.move.wrap
    title: "Working Draft, Programming Languages — C++, [range.move.wrap]"
    URL: https://eel.is/c++draft/range.move.wrap
---

<!-- Paper number is TBD; "DTBD" is a placeholder until a number is assigned. -->

# Abstract

The exposition-only class template *movable-box* is the workhorse of the
range adaptors. It is how the standard library stores a predicate, a
projection, or a value inside a view type and still satisfies the
assignability the `view` concept demands. Every implementation ships one.
Every ranges-adjacent library reimplements one. Anyone writing a
conforming view outside the standard library must write one.

This paper proposes making the type available to users as
`std::movable_box`, with two deliberate constraints on the proposal
itself:

1. **The interface is exactly the usage surface of clause [ranges].**
   The type supports precisely the operations the standard library's
   views perform on *movable-box* today — no more. It is a tool for
   implementing range types, not a new general-purpose maybe-type.

2. **The specification is standalone.** [range.move.wrap] specifies
   *movable-box* as a delta against `optional` as a wording convenience.
   That convenience reads as a structural mandate, and it is not one:
   shipping implementations differ on whether `optional` is involved at
   all. The proposed wording is self-contained and does not mention
   `optional`.

The semantics of the ranges library are unchanged; the exposition-only
name is redefined as an alias for the public type, and the changes to
clause [ranges] are borderline editorial.

# Revision History

## R0

Initial revision. Paper number to be assigned.

# Motivation and Scope

## The problem *movable-box* solves

Function objects in C++ are frequently not assignable. A lambda with a
capture has a deleted copy assignment operator. Types with `const` or
reference members are not assignable. Yet the range adaptors need to
store such objects inside views, and views must model `movable`.

*movable-box*`<T>` ([range.move.wrap]) augments a `move_constructible`
object type with assignability: if `T` is copy constructible but not
`copyable`, copy assignment is synthesized as destroy-then-copy-construct;
if `T` is not `movable`, move assignment is synthesized as
destroy-then-move-construct. A disengaged state exists, but it is an
exception artifact — reachable only when a synthesized operation exits
via an exception — not a domain state. In this respect the type is the
inverse of `optional`: its default state is engaged (a value-initialized
`T`), and emptiness is never requested, only suffered.

## Everyone already has one

The standard library uses *movable-box* pervasively. At least the
following are specified in terms of it: `single_view`, `repeat_view`,
`filter_view`, `transform_view`, `take_while_view`, `drop_while_view`,
`zip_transform_view`, `adjacent_transform_view`, and `chunk_by_view`.

Consequently every implementation ships an internal version: libstdc++
has `std::ranges::__detail::__box`, libc++ has `__movable_box`, and the
Microsoft STL has `_Movable_box`. The design originates in range-v3's
`semiregular_box` [@range-v3], which user code has been borrowing or
reimplementing for the better part of a decade.

Users writing their own views and range adaptors — a task the ranges
design explicitly intends to support, and for which `view_interface` and
`range_adaptor_closure` were made public — need exactly this component
to store their callables and values. Today their choices are:

1. Reimplement it, including the subtle valueless-by-exception corner
   cases and the storage optimization.
2. Reach into implementation detail namespaces, with the usual
   consequences.
3. Depend on range-v3 or another library for one small class template.

None of these is good. The type is nearly fully specified in the working
draft already; the standard is simply keeping it to itself.

## Scope: exactly the standard library's usage, no more

The target user is an implementer of new or existing range types. So the
proposal standardizes the *observed usage surface* of *movable-box* in
clause [ranges]. An audit gives:

- **Default construction**, via the views' defaulted default
  constructors (constrained on `default_initializable`).
- **Direct-initialization from an lvalue or rvalue of `T`** — view
  constructors are specified as "Initializes *fun_* with
  `std::move(fun)`" and "Initializes *value_* with `t`".
- **`in_place` construction from arguments** (`single_view`,
  `repeat_view`).
- **Copy/move construction and assignment**, via the views' implicitly
  generated special member functions — this synthesized assignability
  is the type's reason to exist.
- **Dereference**, `*box_`, in both `const` and non-`const` contexts.
- **`operator->`** (`single_view::data` is specified as
  `return value_.operator->();`).
- **Engagement queries**, which appear only in *Preconditions*
  (e.g., `filter_view::pred()`), motivating `has_value()` and
  `operator bool`.

That is the entire interface proposed. Everything else `optional`
offers is deliberately absent; see the design decisions and the
consolidated non-goals in [D9](#d9-non-goals-consolidated).

## History

The wrapper entered the working draft with the ranges design (as
exposition-only *semiregular-box*, [range.semi.wrap] in C++20).
[@P2325R3] removed the requirement that views be default constructible
and renamed the wrapper *copyable-box*. [@P2494R2] relaxed the range
adaptors to admit move-only function objects, arriving at today's
*movable-box*. The specification has been stable since; the type has
shipped, in essentially its current form, in three major implementations
across two standard cycles.

# Design Decisions

The guiding principle is: standardize the type that exists, for the use
cases the standard library keeps finding for it. Where the existing
specification leaves a choice, prefer the resolution already made by
[range.move.wrap] [@rangemovewrap] and by shipping implementations.

Three principles decide the rest of this section, and are worth stating
before the decisions that follow from them.

**Fail loudly, not subtly.** The type is proposed for the use cases it
already matches. Used outside them, it must not compile — not compile
and quietly do the wrong thing. An operation is omitted when supplying
it would give an answer that is defensible in isolation and wrong in
context; the empty state drives most of these, because it is an
exception artifact rather than a domain state, and any operation whose
semantics turn on it computes over something that should not have been
reachable. `movable_box<T&>` is ill-formed for the same reason (D7).
This, and not an audit of what the views happen to call, is the warrant
for the interface in D6 and the non-goals in D9.

**A drop-in for what ships now.** A vendor must be able to implement
`std::movable_box` as their existing box and alias the exposition-only
name to it, with no view changing layout or behavior. This is why the
wording mandates no structure (D5) and why nothing in the interface can
make the empty state reachable on demand (D6).

This one is a condition on the paper, not a preference expressed by it.
If adopting `std::movable_box` would mandate an ABI break in libstdc++,
libc++, or the Microsoft STL, the proposal should be rejected, and the
author will withdraw it. There is no version of this component worth a
break: the whole claim is that the type already exists and users cannot
reach it. A vocabulary type is not worth relaying every view in the
standard library. See [Implementation Experience](#implementation-experience)
for what has been confirmed and what has not.

**A one-time cost, not an ongoing one.** Adoption should be a rename
plus a bounded, mechanical set of edits, after which library
implementors owe this component nothing further. A delta specification
fails this test on its own: it makes every future change to `optional`
into a standing question about `movable_box` (D5).

## D1: No semantic changes

`std::movable_box<T>`, restricted to the interface above, has exactly
the semantics specified in [range.move.wrap]. The exposition-only
*movable-box* is redefined as an alias for it, so clause [ranges]
changes are borderline editorial and no implementation needs to change
behavior; vendors may literally rename (or alias) their internal type.

## D2: Name

Proposed: `movable_box`.

The working draft has called this type *semiregular-box*, *copyable-box*,
and *movable-box* as its constraints evolved; the current name accurately
describes what the wrapper guarantees — the result always models
`movable` — and matches the name users of the working draft and of
cppreference already know.

Alternatives considered:

- `box` — attractive and short, but underspecified, and squats on a name
  with broader plausible meanings.
- `value_box` / `assignable_box` — novel names for a known type; no
  constituency.
- `semiregular_box` — range-v3's name, but inaccurate since [@P2325R3]:
  the type is not required to be default constructible or copyable.

## D3: Namespace

Proposed: `namespace std`.

Although the specification currently lives in [ranges], nothing about
the type's semantics is ranges-specific. Placing it in `std::ranges`
would be defensible given the deliberately ranges-shaped scope of this
proposal; the author weakly prefers `std`, matching `view_interface`'s
sibling vocabulary components (`in_place_t`, the concepts) and avoiding
a second-class name if other library clauses later adopt it. LEWG
direction welcome.

## D4: Header

Proposed: `<utility>`.

With the specification decoupled from `optional` (D5), `<optional>`
loses its claim. The type's constituency includes every author of a
view type, all of whom include `<ranges>`; but per D3 the component is
not ranges-specific. `<utility>` is where small, broadly applicable
vocabulary components live, and the addition is tiny.

Alternatives: a new `<movable_box>` header (defensible under the
one-component-one-header trend; the author is not opposed), `<optional>`
(rejected: implies a structural relationship this paper explicitly
disclaims), `<ranges>` (rejected per D3).

## D5: Standalone specification — `optional` is not involved

[range.move.wrap] says *movable-box* "behaves exactly like
`optional<T>`" with enumerated differences. For an exposition-only type
whose full usage surface is visible to the specification's authors, that
is an economical technique. For a public type it is the wrong shape,
twice over:

- **It reads as a structural mandate.** Users (and conceivably
  implementers) infer that a `movable_box` is an `optional` with
  patches. Shipping implementations do not agree: some build their box
  on `optional`, some store a `T` (or a union) directly, per the
  storage optimization. The specification should not privilege either
  structure.

- **It imports interface by default.** Every member `optional` gains
  in the future would silently become a question about `movable_box`.
  A delta specification of a public type is a standing maintenance
  hazard.

The proposed wording is therefore self-contained: it defines "contains
a value" for `movable_box` directly, specifies each member's effects in
its own right, and never references `optional`. This costs perhaps a
page of wording and buys structural independence. It also means the
disengaged state's semantics are stated where they belong — as
postconditions of the synthesized assignments — rather than inherited
from a type whose disengaged state is a first-class feature.

## D6: Interface — the observed usage surface

Proposed: exactly the operations enumerated in [Scope](#scope-exactly-the-standard-librarys-usage-no-more):
default construction, direct construction from `const T&`/`T&&`,
`in_place` construction, the special member functions with the
synthesized assignments, `operator*`, `operator->`, `has_value`, and
`operator bool`.

Two things point the same way:

- **Loud failure.** The goal is to hand range-type implementers the tool
  the standard library uses, not to grow a second `optional`. An
  operation is omitted where offering it would answer a question the
  type has no business answering — and answer it plausibly, which is
  worse than not answering. Absent operations are a compile error at the
  point of misuse. That the views do not call them is corroboration, not
  the argument: the views are not the constituency this paper is for.

- **The storage optimization.** [range.move.wrap] p2's recommended
  practice — store only a `T`, no discriminant, whenever the
  synthesized assignments cannot create an empty state — is only
  possible because nothing can disengage the box on purpose. Exposing
  `reset()` or `nullopt` construction would make the empty state
  reachable on demand, forcing a discriminant into every
  specialization and breaking the ABI of every shipping view. The
  restricted interface is what makes aliasing the exposition-only name
  to the public type a no-op.

The engagement observers are retained even though views use them only
in preconditions: the valueless-by-exception state is real, and a type
that can be empty but cannot be asked is a trap. This mirrors the
honesty of `variant::valueless_by_exception`.

The constructors from `const T&` and `T&&` are proposed as `explicit`
and non-template. The ranges clauses use only direct-initialization
from exactly `T`, so implicit conversion and a templated converting
constructor (with its attendant constraint and CTAD questions) are not
needed and not proposed.

## D7: Constraints

Unchanged: `move_constructible<T> && is_object_v<T>`.

The constraint implies two boundaries rather than stating them. Both
will be asked about, so:

- **No references.** With `optional<T&>` adopted for C++26, the
  question will be asked. `movable_box<T&>` remains ill-formed:
  synthesizing assignability over a reference is incoherent (rebind
  vs. assign-through is precisely the swamp this type exists to avoid),
  and no view needs it.
- **No immovable types.** `move_constructible` is a floor chosen
  deliberately by [@P2494R2]. Boxing a pinned type is a different
  component with different costs — closer to `indirect` ([@P3019R11])
  territory — and is out of scope.

## D8: Storage and triviality

The recommended practice of [range.move.wrap] p2 is carried over
verbatim. Layout is unspecified generally, so it stays a
quality-of-implementation matter.

Because the specification is standalone (D5), the triviality guarantees
that were previously inherited must now be stated: the destructor is
trivial if `is_trivially_destructible_v<T>` is `true`, and the copy/move
constructors are trivial if `T`'s corresponding constructors are
trivial. These are implementable in the presence of a discriminant
(as `optional` demonstrates) and matter in practice: a trivially
copyable `movable_box<T>` keeps views of trivially copyable payloads
trivially copyable, with the ABI and calling-convention consequences
that implies. The assignment operators are additionally specified
trivial under the conditions of [optional.assign], so trivially
copyable (and trivially assignable) payloads yield trivially copyable
boxes — and, transitively, trivially copyable views.

## D9: Non-goals, consolidated

The following are deliberately absent. Each is additive, and none is
precluded for the future — but the reason each is absent now is that
supplying it would answer a question over the empty state, which is an
exception artifact and not a domain state. A wrong answer that compiles
is worse than no answer.

- Value assignment (`operator=(U&&)`) — also inconsistent as specified
  for `optional`, whose version is constrained on `is_assignable_v`,
  which would vanish for exactly the types this box serves.
- `emplace` and `reset` — used only *within* the specification of the
  synthesized assignments; not part of the public surface. `reset` would
  additionally make the empty state reachable on demand, forcing a
  discriminant into every specialization (D6).
- `value()`, `value_or()`, monadic operations, `nullopt` in any form,
  `bad_optional_access` — the empty state is not a domain state, and
  each of these is an operation for handling one that is.
- Comparison operators and `hash` support — `optional`'s order the
  disengaged state below every value, and hash it. Here that would be
  ordering and hashing on an exception artifact: an answer that is
  well-defined, cheap to provide, and meaningless. Two boxes that have
  each survived a throwing assignment are not thereby equal.
- Member or free `swap` — `std::swap` works through the synthesized
  move operations, and notably works even when `T` itself is not
  swappable; no bespoke overload is needed.
- Deduction guides — views name the box's type parameter explicitly;
  so should other users. Class template argument deduction from the
  constructors is unaffected.
- An `initializer_list` overload of the `in_place` constructor — no
  view uses one. This is the weakest entry in this list: it answers no
  question about emptiness, and its absence is a compile error for a use
  the type is meant for rather than one it is not. It is omitted to
  match what ships, and the author would not oppose adding it.
- A `copyable_box` — [@P2494R2] subsumed it: when
  `copy_constructible<T>` holds, `movable_box<T>` is copyable.
- *non-propagating-cache* — the box's sibling in [range.adaptor.helpers]
  has its own deliberately different semantics (copies produce empty)
  and is not proposed. This paper is not a general program to
  standardize exposition-only machinery; it is a claim that this one
  component has proven, portable demand.

## D10: `constexpr`

Fully `constexpr`, as the exposition-only type already must be for the
views to be usable in constant expressions.

## D11: Feature test macro

`__cpp_lib_movable_box`, in `<utility>` and `<version>`.

## D12: Freestanding

The proposed interface has no throwing accessors and no dependency on
`bad_optional_access`; nothing obstructs marking the subclause
freestanding. To be confirmed during wording review.

# Impact on the Standard

A pure library extension: one new class template, one feature test
macro, and an editorial redefinition of the exposition-only
*movable-box* as an alias for the new public type. No existing valid
program changes meaning. No implementation is required to change the
behavior, layout, or ABI of any ranges component.

# Implementation Experience

Three independent implementations of exactly this type ship today inside
libstdc++, libc++, and the Microsoft STL, exercised by the entire range
adaptor test surface of each. range-v3's `semiregular_box` [@range-v3]
is the design's origin and has a decade of field experience.

A reference implementation of the proposed interface is available as
`beman.movable_box` [@beman-movable-box], with a test suite written
against the wording below rather than against the implementation. The
same suite compiles and runs, unmodified, against libstdc++'s
exposition-only `std::ranges::__detail::__box`. Thirty of its
thirty-three test cases pass there.

The three failures are not defects. [range.move.wrap] specifies the
exposition-only type well, and an exposition-only type owes correct
behavior only where the library uses it; every difference below is
unobservable through the views, which is exactly why each has survived
two standard cycles. The question this paper has to answer is not
whether they are bugs. It is what adopting the public interface would
cost a vendor — whether the differences can be closed without breaking
any currently conforming program, and without moving any layout.

- **`value_type` is absent.** The no-discriminant `__box`
  specialization does not provide it. No view names it.
- **A box can be default-constructed empty.** `__box<T>{}` is
  well-formed and yields a *disengaged* box even where `T` is not
  `default_initializable`:

  ```cpp
  struct Bad {
    Bad(const Bad&);
    Bad& operator=(const Bad&) = delete;
  };

  static_assert(!std::default_initializable<Bad>);
  static_assert(std::default_initializable<__box<Bad>>);  // ?
  constexpr __box<Bad> b;
  static_assert(!b);                                      // empty on arrival
  ```

  The box's own default constructor is constrained correctly, on
  `default_initializable<T>`. But the `optional`-based primary template
  publicly inherits `optional`'s constructors, and `optional`'s default
  constructor is unconstrained. Where `T` is default-initializable the
  box's own constructor is preferred and the box is engaged; where it is
  not, the box's own constructor drops out of the overload set and the
  inherited one is all that is left. This is the delta-specification
  hazard of D5, caught in the wild: the interface `optional` exports was
  never chosen, only inherited. The views cannot reach it — their own
  default constructors are already constrained on
  `default_initializable<T>` — so within [ranges] the box is correct.

- **The move assignment is stronger than specified.** For a movable `T`
  whose move constructor may throw, the no-discriminant specialization
  defaults move assignment, so its exception specification follows `T`'s
  move *assignment* alone and is `noexcept(true)`, where the
  `optional`-based primary template — and [range.move.wrap] — give
  `noexcept(false)`. libstdc++'s two specializations disagree with each
  other, and both conform: [res.on.exception.handling] p5 permits an
  implementation to strengthen any non-virtual function's exception
  specification. This one needs no work at all, and no conformance test
  may demand otherwise. It is recorded here because the paper's own
  suite asserted an exact `noexcept` and thereby claimed more than the
  wording can give.

The first two were closed experimentally against libstdc++ 15, by
replacing the primary template's `using optional<T>::optional;` with the
four constructors the proposed interface actually has, and adding
`value_type` to both templates. All nine views specified in terms of
*movable-box* still compile and run; `__box<Bad>` becomes correctly
non-default-initializable; and `sizeof` is unchanged for every box and
every view tested, `[[no_unique_address]]` and the `optional` base
included. No currently conforming program can observe either change:
`__box` is a reserved name, and neither difference is reachable through
a view.

So the answer for libstdc++ is that adoption is a rename plus two
mechanical edits, ABI intact. Note what carries that result: the vendor
implements `std::movable_box` as *their* box and aliases the
exposition-only name to it. That is only available because the proposed
wording is standalone (D5) and mandates no structure. A specification
that made `movable_box` an `optional` with patches would put every
view's layout back in play.

This has been confirmed for one implementation of three. libc++'s
`std::ranges::__movable_box` and the Microsoft STL's `_Movable_box` have
not been tested, and the author intends to run the suite against both
before asking LEWG to advance the paper.

That is a condition on the proposal rather than an outstanding chore.
Per the drop-in principle, a required ABI break in any of the three sinks
it. The libstdc++ result is therefore evidence that adoption *can* be
free, not that it is; two thirds of the claim is unverified, and the
paper should not be advanced on one data point. LEWG should read the
result above as a demonstration of method — patch the vendor's header,
compile every view specified in terms of *movable-box*, compare layouts
— and not yet as a conclusion.

# Proposed Wording

Wording is relative to the current working draft. The specification is
self-contained and follows the drafting style of [optional] and
[expected] without referencing either.

## [utility.syn]

Add to the header `<utility>` synopsis:

::: add

```cpp
// [movable.box], class template movable_box
template<class T>
  requires move_constructible<T> && is_object_v<T>
  class movable_box;
```

:::

## New subclause [movable.box]

Add a new subclause to [utility] (placement editorial):

::: add

**Movable wrapper [movable.box]**

**General [movable.box.general]**

[1]{.pnum} A `movable_box<T>` either *contains a value* of type `T` or
contains no value. When a `movable_box<T>` contains a value, the
contained value is allocated within the storage of the `movable_box`.
Implementations are not permitted to use additional storage, such as
dynamic memory, to allocate the contained value.

[2]{.pnum} A `movable_box` contains a value after any of its
constructors completes without an exception, unless it was copy or move
constructed from a `movable_box` that contains no value. A `movable_box`
can come to contain no value only as specified for its assignment
operators, or by being copy or move constructed from a `movable_box`
containing no value.
[*Note*: A `movable_box<T>` contains no value only as a consequence of
an exception thrown during a copy or move assignment; the state then
propagates through subsequent copies, moves, and assignments until a
value is assigned. — *end note*]

[3]{.pnum} *Recommended practice*: If `copy_constructible<T>` is modeled,
`movable_box<T>` should store only a `T` if either `T` models `copyable`,
or `is_nothrow_move_constructible_v<T> &&`
`is_nothrow_copy_constructible_v<T>` is `true`. Otherwise, `movable_box<T>` should store only a `T` if either
`T` models `movable` or `is_nothrow_move_constructible_v<T>` is `true`.

```cpp
namespace std {
  template<class T>
    requires move_constructible<T> && is_object_v<T>
  class movable_box {
  public:
    using value_type = T;

    // [movable.box.ctor], constructors
    constexpr movable_box() noexcept(is_nothrow_default_constructible_v<T>)
      requires default_initializable<T>;
    constexpr explicit movable_box(const T& t) requires copy_constructible<T>;
    constexpr explicit movable_box(T&& t);
    template<class... Args>
      constexpr explicit movable_box(in_place_t, Args&&... args);

    constexpr movable_box(const movable_box& rhs) requires copy_constructible<T>;
    constexpr movable_box(movable_box&& rhs)
      noexcept(is_nothrow_move_constructible_v<T>);

    // [movable.box.dtor], destructor
    constexpr ~movable_box();

    // [movable.box.assign], assignment
    constexpr movable_box& operator=(const movable_box& rhs)
      noexcept(@_see below_@) requires copy_constructible<T>;
    constexpr movable_box& operator=(movable_box&& rhs)
      noexcept(@_see below_@);

    // [movable.box.obs], observers
    constexpr bool has_value() const noexcept;
    constexpr explicit operator bool() const noexcept;

    constexpr T& operator*() & noexcept;
    constexpr const T& operator*() const & noexcept;
    constexpr T&& operator*() && noexcept;
    constexpr const T&& operator*() const && noexcept;

    constexpr T* operator->() noexcept;
    constexpr const T* operator->() const noexcept;
  };
}
```

**Constructors [movable.box.ctor]**

```cpp
constexpr movable_box() noexcept(is_nothrow_default_constructible_v<T>)
  requires default_initializable<T>;
```

[1]{.pnum} *Effects*: Equivalent to `movable_box(in_place)`.
[*Note*: The contained value is value-initialized. — *end note*]

```cpp
constexpr explicit movable_box(const T& t) requires copy_constructible<T>;
```

[2]{.pnum} *Effects*: Direct-non-list-initializes the contained value
with `t`.

```cpp
constexpr explicit movable_box(T&& t);
```

[3]{.pnum} *Effects*: Direct-non-list-initializes the contained value
with `std::move(t)`.

```cpp
template<class... Args>
  constexpr explicit movable_box(in_place_t, Args&&... args);
```

[4]{.pnum} *Constraints*: `is_constructible_v<T, Args...>` is `true`.

[5]{.pnum} *Effects*: Direct-non-list-initializes the contained value
with `std::forward<Args>(args)...`.

```cpp
constexpr movable_box(const movable_box& rhs) requires copy_constructible<T>;
```

[6]{.pnum} *Effects*: If `rhs` contains a value, direct-non-list-initializes
the contained value with `*rhs`; otherwise, `*this` contains no value.

[7]{.pnum} *Remarks*: This constructor is trivial if
`is_trivially_copy_constructible_v<T>` is `true`.

```cpp
constexpr movable_box(movable_box&& rhs)
  noexcept(is_nothrow_move_constructible_v<T>);
```

[8]{.pnum} *Effects*: If `rhs` contains a value, direct-non-list-initializes
the contained value with `std::move(*rhs)`; otherwise, `*this` contains
no value. `rhs.has_value()` is unchanged.

[9]{.pnum} *Remarks*: This constructor is trivial if
`is_trivially_move_constructible_v<T>` is `true`.

**Destructor [movable.box.dtor]**

```cpp
constexpr ~movable_box();
```

[1]{.pnum} *Effects*: Destroys the contained value, if any.

[2]{.pnum} *Remarks*: This destructor is trivial if
`is_trivially_destructible_v<T>` is `true`.

**Assignment [movable.box.assign]**

```cpp
constexpr movable_box& operator=(const movable_box& rhs)
  noexcept(@_see below_@) requires copy_constructible<T>;
```

[1]{.pnum} *Effects*: If `this == addressof(rhs)` is `true`, no effects.
Otherwise:

- [1.1]{.pnum} If `copyable<T>` is modeled: if `*this` and `rhs` each
  contain a value, the contained value of `*this` is copy-assigned from
  `*rhs`; otherwise, if `rhs` contains a value, a contained value is
  direct-non-list-initialized with `*rhs`; otherwise, the contained
  value of `*this`, if any, is destroyed.

- [1.2]{.pnum} Otherwise: the contained value of `*this`, if any, is
  destroyed; then, if `rhs` contains a value, a contained value is
  direct-non-list-initialized with `*rhs`. If the initialization exits
  via an exception, `*this` contains no value.

[2]{.pnum} *Returns*: `*this`.

[3]{.pnum} *Remarks*: The exception specification is equivalent to:
`is_nothrow_copy_constructible_v<T> &&`
`(!copyable<T> || is_nothrow_copy_assignable_v<T>)`.

[4]{.pnum} *Remarks*: This operator is trivial if
`is_trivially_copy_constructible_v<T>`,
`is_trivially_copy_assignable_v<T>`, and
`is_trivially_destructible_v<T>` are all `true`.

```cpp
constexpr movable_box& operator=(movable_box&& rhs)
  noexcept(@_see below_@);
```

[5]{.pnum} *Effects*: If `this == addressof(rhs)` is `true`, no effects.
Otherwise:

- [5.1]{.pnum} If `movable<T>` is modeled: if `*this` and `rhs` each
  contain a value, the contained value of `*this` is move-assigned from
  `*rhs`; otherwise, if `rhs` contains a value, a contained value is
  direct-non-list-initialized with `std::move(*rhs)`; otherwise, the
  contained value of `*this`, if any, is destroyed. `rhs.has_value()`
  is unchanged.

- [5.2]{.pnum} Otherwise: the contained value of `*this`, if any, is
  destroyed; then, if `rhs` contains a value, a contained value is
  direct-non-list-initialized with `std::move(*rhs)`. If the
  initialization exits via an exception, `*this` contains no value.
  `rhs.has_value()` is unchanged.

[6]{.pnum} *Returns*: `*this`.

[7]{.pnum} *Remarks*: The exception specification is equivalent to:
`is_nothrow_move_constructible_v<T> &&`
`(!movable<T> || is_nothrow_move_assignable_v<T>)`.

[8]{.pnum} *Remarks*: This operator is trivial if
`is_trivially_move_constructible_v<T>`,
`is_trivially_move_assignable_v<T>`, and
`is_trivially_destructible_v<T>` are all `true`.

**Observers [movable.box.obs]**

```cpp
constexpr bool has_value() const noexcept;
constexpr explicit operator bool() const noexcept;
```

[1]{.pnum} *Returns*: `true` if and only if `*this` contains a value.

```cpp
constexpr T& operator*() & noexcept;
constexpr const T& operator*() const & noexcept;
```

[2]{.pnum} *Preconditions*: `*this` contains a value.

[3]{.pnum} *Returns*: A reference to the contained value.

```cpp
constexpr T&& operator*() && noexcept;
constexpr const T&& operator*() const && noexcept;
```

[4]{.pnum} *Preconditions*: `*this` contains a value.

[5]{.pnum} *Returns*: `std::move(`the contained value`)`.

```cpp
constexpr T* operator->() noexcept;
constexpr const T* operator->() const noexcept;
```

[6]{.pnum} *Preconditions*: `*this` contains a value.

[7]{.pnum} *Returns*: `addressof(`the contained value`)`.

:::

*Drafting note*: paragraphs [movable.box.assign] 1.1/1.2 and 5.1/5.2
reproduce, in standalone form, the semantics of [range.move.wrap]
(1.3)/(1.4) together with the `optional` assignment semantics those
paragraphs defer to when `copyable<T>`/`movable<T>` are modeled. The
combined exception specifications reduce to the current ones in each
branch. LWG review should confirm the equivalence. The triviality
*Remarks* on the assignment operators follow the conditions of
[optional.assign] and keep `movable_box<T>` — and every view storing
one — trivially copyable for trivially copyable, trivially assignable
`T`; this is new relative to the exposition-only specification, whose
triviality *Remarks* cover only the constructors and the destructor,
and the reference implementation implements it.

## [range.move.wrap]

Replace the contents of [range.move.wrap] as follows:

[1]{.pnum} Many types in this subclause are specified in terms of an
exposition-only class template *movable-box*. [*movable-box*`<T>`
denotes `std::movable_box<T>` ([movable.box]).]{.add}

::: rm

[1]{.pnum} *movable-box*`<T>` behaves exactly like `optional<T>` with
the following differences: [...]

[2]{.pnum} *Recommended practice*: [...]

:::

*Drafting note*: paragraphs 1.1–1.4 and 2 are struck in their entirety;
their content is subsumed by [movable.box]. All uses of *movable-box*
elsewhere in [ranges] are unchanged, as are the semantics of every view
specified in terms of it. The operations clause [ranges] performs on
*movable-box* — default construction, direct-initialization from
`const T&` and `T&&`, `in_place` construction, the special member
functions, `operator*`, `operator->` (`single_view::data`), and
`has_value` in preconditions — are all provided by [movable.box];
this claim should be re-audited against the working draft during
wording review.

## [version.syn]

Add:

::: add

```cpp
#define __cpp_lib_movable_box 20XXXXL // also in <utility>
```

:::

# Open Questions for LEWG

1. Header: `<utility>` or a new `<movable_box>` (D4)?
2. Namespace: `std` or `std::ranges` (D3)?
3. Are the value constructors correctly `explicit` and non-template
   (D6)?
4. Does LEWG endorse the standalone specification (D5) and the
   usage-surface-only interface (D6, D9) as the scope of the paper?

# Acknowledgements

Thanks to Eric Niebler for `semiregular_box` and the ranges design, and
to the authors of [@P2325R3] and [@P2494R2], whose work converged this
type to its current, stable form.
