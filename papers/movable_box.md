---
title: "`std::movable_box`: Give Users the Ranges Movable Wrapper"
document: P4346R0
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

# Abstract

The exposition-only class template *movable-box* is the workhorse of the
range adaptors. It is how the standard library stores a predicate, a
projection, or a value inside a view type and still satisfies the
assignability the `view` concept demands. Every implementation ships one.
Every ranges-adjacent library reimplements one. Anyone writing a
conforming view outside the standard library must write one.

This paper proposes making the type available to users as
`std::movable_box`, with two constraints on the proposal itself:

1. **The interface is exactly the usage surface of clause [ranges].**
   The type supports the operations the standard library's views
   perform on *movable-box* today, and no more. It is a tool for
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

Initial revision.

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
exception artifact (reachable only when a synthesized operation exits
via an exception), not a domain state. In this respect the type is the
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

Users writing their own views and range adaptors need this component
to store their callables and values. Writing them is a task the ranges
design intends to support; `view_interface` and `range_adaptor_closure`
were made public for it. Today their choices are:

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

That is the entire interface proposed. Everything else `optional` offers
is absent; see the design decisions and the consolidated non-goals in
[D9](#d9-non-goals-consolidated).

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
This is the warrant for the interface in D6 and the non-goals in D9.
An audit of what the views happen to call is not.

**A drop-in for what ships now.** A vendor must be able to implement
`std::movable_box` as their existing box and alias the exposition-only
name to it, with no view changing layout or behavior. This is why the
wording mandates no structure (D5) and why nothing in the interface can
make the empty state reachable on demand (D6).

**Adoption is a one-time cost.** It should be a rename plus a bounded,
mechanical set of edits, after which library implementers owe this
component nothing further. A delta specification fails this test on its
own: it makes every future change to `optional` into a standing question
about `movable_box` (D5).

The drop-in principle is a condition on the paper; the other two are
preferences. If adopting `std::movable_box` would mandate an ABI break
in libstdc++, libc++, or the Microsoft STL, the proposal should be
rejected, and I will withdraw it. There is no version of this component
worth a break: the whole claim is that the type already exists and users
cannot reach it. A vocabulary type is not worth breaking the ABI of
every view in the standard library. See
[Implementation Experience](#implementation-experience) for what has
been confirmed and what has not.

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
describes what the wrapper guarantees (the result always models
`movable`) and matches the name users of the working draft and of
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
would be defensible, given the ranges-shaped scope of this proposal.
However, I have a weak preference for `std`, matching the other small
vocabulary components that view authors reach for — `in_place_t`, and
the concepts of [concepts] — and avoiding a second-class name if other
library clauses later adopt it. LEWG direction welcome.

## D4: Header

Proposed: `<utility>`.

With the specification decoupled from `optional` (D5), `<optional>`
loses its claim. The type's constituency includes every author of a
view type, all of whom include `<ranges>`; but per D3 the component is
not ranges-specific. `<utility>` is where small, broadly applicable
vocabulary components live, and the addition is tiny.

Alternatives: a new `<movable_box>` header (defensible under the
one-component-one-header trend; I am not opposed), `<optional>`
(rejected: implies a structural relationship this paper explicitly
disclaims), `<ranges>` (rejected per D3).

## D5: Standalone specification — `optional` is not involved

[range.move.wrap] says *movable-box* "behaves exactly like
`optional<T>`" with enumerated differences. For an exposition-only type
whose every use is visible to the specification's authors, that is an
economical technique. However, for a public type it is the wrong shape,
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
disengaged state's semantics are stated where they belong, as
postconditions of the synthesized assignments, rather than inherited
from a type whose disengaged state is a first-class feature.

## D6: Interface — the observed usage surface

Proposed: the operations enumerated in [Scope](#scope-exactly-the-standard-librarys-usage-no-more):
default construction, direct construction from `const T&`/`T&&`,
`in_place` construction, the special member functions with the
synthesized assignments, `operator*`, `operator->`, `has_value`, and
`operator bool`.

Two things point the same way:

- **Loud failure.** The goal is to hand range-type implementers the tool
  the standard library uses. It is not to grow a second `optional`. An
  operation is omitted where offering it would answer a question the
  type has no business answering — and answer it plausibly, which is
  worse than not answering. Absent operations are a compile error at the
  point of misuse. That the views do not call them is corroboration, not
  the argument: the views are not the constituency this paper is for.

- **The storage optimization.** [range.move.wrap] p2 recommends storing
  only a `T`, with no discriminant, whenever the synthesized
  assignments cannot create an empty state. That is only possible
  because nothing can disengage the box on purpose. Exposing
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
from `T` itself, so implicit conversion and a templated converting
constructor (with its attendant constraint and CTAD questions) are not
needed and not proposed.

## D7: Constraints

Unchanged: `move_constructible<T> && is_object_v<T>`.

The constraint implies two boundaries rather than stating them. Both
will be asked about, so:

- **No references.** With `optional<T&>` adopted for C++26, the
  question will be asked. `movable_box<T&>` remains ill-formed:
  synthesizing assignability over a reference is incoherent (rebind
  vs. assign-through is the swamp this type exists to avoid), and no
  view needs it.
- **No immovable types.** `move_constructible` is the floor [@P2494R2]
  chose. Boxing a pinned type is a different component with different
  costs, closer to `indirect` ([@P3019R11]) territory, and is out of
  scope.

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
that implies. The assignment operators are also specified trivial under
the conditions of [optional.assign], so trivially copyable (and
trivially assignable) payloads yield trivially copyable boxes — and,
transitively, trivially copyable views.

## D9: Non-goals, consolidated

The following are absent. Each is additive, and none is precluded for
the future, but the reason each is absent now is that
supplying it would answer a question over the empty state, which is an
exception artifact and not a domain state. A wrong answer that compiles
is worse than no answer.

- Value assignment (`operator=(U&&)`) — also inconsistent as specified
  for `optional`, whose version is constrained on `is_assignable_v`,
  which would vanish for the types this box serves.
- `emplace` and `reset` — used only *within* the specification of the
  synthesized assignments; not part of the public interface. `reset`
  would also make the empty state reachable on demand, forcing a
  discriminant into every specialization (D6).
- `value()`, `value_or()`, monadic operations, `nullopt` in any form,
  `bad_optional_access` — the empty state is not a domain state, and
  each of these is an operation for handling one that is.
- Comparison operators and `hash` support — `optional`'s comparisons
  order the disengaged state below every value, and its `hash`
  specialization hashes it. Here that would be
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
  match what ships, and I would not oppose adding it.
- A `copyable_box` — [@P2494R2] subsumed it: when
  `copy_constructible<T>` holds, `movable_box<T>` is copyable.
- *non-propagating-cache* — the box's sibling in [range.adaptor.helpers]
  has different semantics (copies produce empty) and is not proposed.
  This paper is not a general program to standardize exposition-only
  helpers; it is a claim that this one component has proven, portable
  demand.

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

Three independent implementations of this type ship today inside
libstdc++, libc++, and the Microsoft STL, exercised by the entire range
adaptor test suite of each. range-v3's `semiregular_box` [@range-v3] is
the design's origin and has a decade of field experience.

A reference implementation of the proposed interface exists as
`beman.movable_box` [@beman-movable-box], with a test suite written
against the wording below rather than against the implementation. That
repository is not public yet; the cited URL is where it will be
published. The same suite compiles and runs, unmodified, against
libstdc++'s exposition-only `std::ranges::__detail::__box`.
Thirty-four of its thirty-eight test cases pass there.

The four failures are not defects. [range.move.wrap] specifies the
exposition-only type well, and an exposition-only type owes correct
behavior only where the library uses it; every difference below is
either unobservable through the views, or is not required by the current
exposition-only wording. The question this paper has to answer is not
whether they are bugs. It is what adopting the public interface would
cost a vendor: whether the differences can be closed without breaking
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
  `default_initializable<T>`. However, the `optional`-based primary
  template publicly inherits `optional`'s constructors, and
  `optional`'s default constructor is unconstrained. Where `T` is
  default-initializable the box's own constructor is preferred and the
  box is engaged; where it is not, the box's own constructor drops out
  of the overload set and the inherited one is all that is left. This
  is the delta-specification
  hazard of D5, caught in the wild: the interface `optional` exports was
  never chosen, only inherited. The views cannot reach it; their own
  default constructors are already constrained on
  `default_initializable<T>`, so within [ranges] the box is correct.

- **The move assignment is stronger than specified.** For a movable `T`
  whose move constructor may throw, the no-discriminant specialization
  defaults move assignment, so its exception specification follows `T`'s
  move *assignment* alone and is `noexcept(true)`, where the
  `optional`-based primary template (and [range.move.wrap]) give
  `noexcept(false)`. libstdc++'s two specializations disagree with each
  other, and both conform: [res.on.exception.handling] p5 permits an
  implementation to strengthen any non-virtual function's exception
  specification. This one needs no work at all, and no conformance test
  may demand otherwise. It is recorded here because the paper's own
  suite asserted an exact `noexcept` and thereby claimed more than the
  wording can give.

- **Self-assignment of a copyable payload has effects.** The
  no-discriminant specialization defaults assignment for copyable `T`, so
  self-assignment of the box can invoke `T`'s assignment operator. The
  proposed wording specifies no effects when `this == addressof(rhs)`.
  Unlike the first two differences, this can be observed through a view:
  for example, a stateful predicate stored in `filter_view` can count
  assignments and expose that count through `pred()`. This is not a
  libstdc++ defect; the current exposition-only wording does not promise
  the public type's self-assignment postcondition. It is adoption work
  if the public interface keeps that guarantee.

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

So the current libstdc++ result is narrower than "rename plus two
mechanical edits." The interface accidents above can be closed with ABI
intact, and the stronger exception specification needs no change, but
the public self-assignment guarantee needs either an implementation
change or a wording decision. What carries the ABI result is still that
the vendor can implement `std::movable_box` as *their* box and alias the
exposition-only name to it. That is only available because the proposed
wording is standalone (D5) and mandates no structure. A specification
that made `movable_box` an `optional` with patches would put every
view's layout back in play.

The same suite has since been run against the other two
implementations: libc++'s `std::ranges::__movable_box` (LLVM 17 through
22) and the Microsoft STL's `std::ranges::_Movable_box` (the latest
MSVC), built and executed as conformance targets in the reference
implementation's CI [@beman-movable-box]. Neither box can host the
suite raw. libstdc++'s could, but by an accident of structure: it
publicly inherits from `optional`, which happens to supply every
proposed spelling. libc++ and Microsoft wrote theirs freestanding, and
each is *narrower* than the proposed interface in spelling:

- `value_type` is absent from both.
- Both construct only from `in_place`: neither has the constructors
  from `const T&` and `T&&`.
- Neither spells `has_value()`; libc++ has `__has_value()`, the
  Microsoft STL only `explicit operator bool`.
- The Microsoft STL has no `operator->` (its `single_view::data` is
  written as `addressof(*value_)` instead).
- Neither has the rvalue `operator*` overloads, and the lvalue
  overloads are not ref-qualified.

Every entry on that list is additive: a member function or a nested
typedef, no state, no layout. The conformance targets make that claim
executable. The suite runs against a shim deriving from the vendor's
box and adding exactly the missing members, and static assertions
check, for every payload type in the suite, that the shim changes
neither size nor alignment nor the availability, triviality, or
exception specification of any special member. The shim is the
candidate adoption patch; the parity checks are the demonstration that
it is free.

The semantic divergences that remain fall into two families:

- **Self-assignment.** The proposed wording specifies no effects when
  `this == addressof(rhs)`. For a copyable payload, none of the three
  implementations provides that today: whenever `T` models `copyable`
  (respectively `movable`), all three defer to `T`'s own assignment
  operator — defaulted in libstdc++'s and libc++'s no-discriminant
  specializations, unconditional in the Microsoft STL's branches — so
  `T`'s assignment observes the self-assignment. The cross-checks also
  surfaced a case the libstdc++ run could not: in the *discriminated*
  box of a movable payload, libc++ (which defaults move assignment
  whenever `T` models `movable`, deferring to `optional`'s, which has
  no self-check) and the Microsoft STL (which assigns unconditionally)
  both let `T`'s move assignment observe a self-move — a move
  assignment that wipes its source then loses the value — while
  libstdc++'s primary defines its own guarded assignment operators and
  conforms. All three do guard the synthesized destroy-then-construct
  branches, where the self-check keeps self-assignment from destroying
  the operand outright.
- **Exception-specification strengthening.** All three give the move
  assignment a stronger `noexcept` than the proposed formula for a
  payload whose move constructor throws but whose move assignment does
  not (the Microsoft STL documents its formulas as "strengthened"
  in-source). As for libstdc++ above, this is conforming under
  [res.on.exception.handling] p5 and needs no work.

Two smaller items are vendor-specific. Neither of libstdc++'s interface
accidents recurs elsewhere — libc++ and the Microsoft STL both
constrain their default constructors correctly. The Microsoft STL's
`in_place` constructor template, however, is unconstrained and
non-`explicit`, so `is_constructible_v` answers `true` for argument
lists `T` cannot be constructed from; unobservable through the views,
and additive to close.

The drop-in claim now stands as follows, for all three implementations:
the missing interface is demonstrated additive with layouts unchanged,
and the residue is one wording decision plus a conforming
strengthening. The self-assignment postcondition of the proposed
[movable.box.assign] is provided by none of the three for copyable
payloads, and by only libstdc++ for the movable discriminated case, so
LEWG should decide it explicitly (Open Questions, item 5): keep the
guarantee, with vendors adding one cheap, ABI-free self-check per
assignment operator, or weaken the wording to defer to `T`'s assignment
when `copyable<T>` / `movable<T>` is modeled, matching everything that
ships. The libstdc++ header-patch experiment above remains the deepest
of the three results — it also rebuilt every view against the patched
box — and repeating it for libc++ and the Microsoft STL is available as
further confirmation; but the interface-level question it answered is
now answered for all three.

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

### Movable wrapper [movable.box] {-}

#### General [movable.box.general] {-}

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

::: {.include file="wording/movable.box.general.md"}
:::

::: {.include file="wording/movable.box.ctor.md"}
:::

::: {.include file="wording/movable.box.dtor.md"}
:::

::: {.include file="wording/movable.box.assign.md"}
:::

::: {.include file="wording/movable.box.obs.md"}
:::

:::

*Drafting note*: paragraphs [movable.box.assign] 1.1/1.2 and 5.1/5.2
reproduce, in standalone form, the semantics of [range.move.wrap]
(1.3)/(1.4) together with the `optional` assignment semantics those
paragraphs defer to when `copyable<T>`/`movable<T>` are modeled. Three
of the four combined exception specifications reduce to the current
ones: `is_nothrow_copy_constructible_v<T>` when `copyable<T>` is not
modeled, `is_nothrow_move_constructible_v<T>` when `movable<T>` is not,
and `optional`'s own
`is_nothrow_move_constructible_v<T> && is_nothrow_move_assignable_v<T>`
when it is. The fourth does not: when `copyable<T>` is modeled the
current wording defers to `optional`'s copy assignment, which carries no
exception specification at all, so [movable.box.assign] p3 strengthens
it. The strengthening is deliberate, and is one an implementation was
already free to make under [res.on.exception.handling]; no currently
conforming program can observe the difference. LWG review should confirm
the other three equivalences and this one departure. The triviality
*Remarks* on the assignment operators follow the conditions of
[optional.assign] and keep `movable_box<T>` — and every view storing
one — trivially copyable for trivially copyable, trivially assignable
`T`; this is new relative to the exposition-only specification, whose
triviality *Remarks* cover only the constructors and the destructor,
and the reference implementation implements it.

The constructor triviality *Remarks* hold whether or not the box carries
a discriminant. Trivial copy and move constructors cannot throw, but a
`T` with one of them trivial can still need the empty state when the
other may throw: a non-assignable `T` with a trivial copy constructor
and a potentially-throwing move constructor, for instance. The box's
copy constructor must then be trivial in the discriminated layout too.
Copying the contained value and the discriminant as they stand gives the
specified effects whether or not a value is present, so this costs
nothing. `optional`'s constructors already behave this way, and both
libstdc++ and libc++ inherit it by falling back to `optional`; the
reference implementation defaults the corresponding constructor
directly.

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
5. Self-assignment: the proposed [movable.box.assign] specifies no
   effects when `this == addressof(rhs)`. No shipping implementation
   provides that where the box defers to `T`'s own assignment operator
   (see [Implementation Experience](#implementation-experience)). Keep
   the guarantee — vendors add a self-check per assignment operator —
   or defer to `T`'s assignment semantics when `copyable<T>` /
   `movable<T>` is modeled, matching what ships?

# Acknowledgements

Thanks to Eric Niebler for `semiregular_box` and the ranges design, and
to the authors of [@P2325R3] and [@P2494R2], whose work converged this
type to its current, stable form.
