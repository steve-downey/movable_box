::: wording

#### Assignment [movable.box.assign] {-}

```cpp
constexpr movable_box& operator=(const movable_box& rhs) noexcept($see below$)
  requires copy_constructible<T>;
```

[#]{.pnum} *Effects*: If `this == addressof(rhs)` is `true`, no effects. Otherwise:

- [#.#]{.pnum} If `copyable<T>` is modeled: if `*this` and `rhs` each contain a value, the contained value of `*this` is copy-assigned from `*rhs`; otherwise, if `rhs` contains a value, a contained value is direct-non-list-initialized with `*rhs`; otherwise, the contained value of `*this`, if any, is destroyed.
- [#.#]{.pnum} Otherwise: the contained value of `*this`, if any, is destroyed; then, if `rhs` contains a value, a contained value is direct-non-list-initialized with `*rhs`. If the initialization exits via an exception, `*this` contains no value.

[#]{.pnum} *Returns*: `*this`.

[#]{.pnum} *Remarks*: The exception specification is equivalent to: `is_nothrow_copy_constructible_v<T> && (!copyable<T> || is_nothrow_copy_assignable_v<T>)`.

[#]{.pnum} This operator is trivial if `is_trivially_copy_constructible_v<T>`, `is_trivially_copy_assignable_v<T>`, and `is_trivially_destructible_v<T>` are all `true`.

```cpp
constexpr movable_box& operator=(movable_box&& rhs) noexcept($see below$);
```

[#]{.pnum} *Effects*: If `this == addressof(rhs)` is `true`, no effects. Otherwise:

- [#.#]{.pnum} If `movable<T>` is modeled: if `*this` and `rhs` each contain a value, the contained value of `*this` is move-assigned from `*rhs`; otherwise, if `rhs` contains a value, a contained value is direct-non-list-initialized with `std::move(*rhs)`; otherwise, the contained value of `*this`, if any, is destroyed. `rhs.has_value()` is unchanged.
- [#.#]{.pnum} Otherwise: the contained value of `*this`, if any, is destroyed; then, if `rhs` contains a value, a contained value is direct-non-list-initialized with `std::move(*rhs)`. If the initialization exits via an exception, `*this` contains no value. `rhs.has_value()` is unchanged.

[#]{.pnum} *Returns*: `*this`.

[#]{.pnum} *Remarks*: The exception specification is equivalent to: `is_nothrow_move_constructible_v<T> && (!movable<T> || is_nothrow_move_assignable_v<T>)`.

[#]{.pnum} This operator is trivial if `is_trivially_move_constructible_v<T>`, `is_trivially_move_assignable_v<T>`, and `is_trivially_destructible_v<T>` are all `true`.

:::
