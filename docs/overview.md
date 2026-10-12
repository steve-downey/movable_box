# Overview

## Motivation

Function objects in C++ are frequently not assignable.
A lambda with a capture has a deleted copy assignment operator.
Types with `const` or reference members are not assignable.
Yet the range adaptors need to store such objects inside views, and views must model `movable`.

The exposition-only class template *movable-box* ([range.move.wrap]) is how the standard library does that.
Every implementation ships one.
Every ranges-adjacent library reimplements one.
Anyone writing a conforming view outside the standard library must write one.

`beman::movable_box::movable_box<T>` is that type, made available to users, as proposed for the standard as `std::movable_box`.

## Semantics

`movable_box<T>` augments a `move_constructible` object type with assignability.
If `T` is copy constructible but not `copyable`, copy assignment is synthesized as destroy-then-copy-construct.
If `T` is not `movable`, move assignment is synthesized as destroy-then-move-construct.

A disengaged state exists, but it is an exception artifact, reachable only when a synthesized operation exits via an exception, not a domain state.
In this respect the type is the inverse of `optional`: its default state is engaged (a value-initialized `T`), and emptiness is never requested, only suffered.

## Interface

The interface is exactly the usage surface of *movable-box* in clause [ranges], and no more:

- default construction, constrained on `default_initializable`;
- direct-initialization from an lvalue or rvalue of `T`;
- `in_place` construction from arguments;
- copy and move construction and assignment;
- dereference, `*box`, in both `const` and non-`const` contexts;
- `operator->`;
- engagement queries, `has_value()` and `operator bool`.

It is a tool for implementing range types, not a new general-purpose maybe-type.

## Storage and triviality

The destructor is trivial if `is_trivially_destructible_v<T>` is `true`, and the copy and move constructors are trivial if `T`'s corresponding constructors are trivial.
The assignment operators are trivial under the conditions of [optional.assign].
So a trivially copyable payload yields a trivially copyable box, and, transitively, a trivially copyable view.

## Further reading

The proposal is [papers/movable_box.md](https://github.com/bemanproject/movable_box/blob/main/papers/movable_box.md).
