::: wording

#### Constructors [movable.box.ctor] {-}

```cpp
constexpr movable_box() noexcept(is_nothrow_default_constructible_v<T>)
  requires default_initializable<T>;
```

[#]{.pnum} *Effects*: Equivalent to `movable_box(in_place)`.

```cpp
constexpr explicit movable_box(const T& t)
  requires copy_constructible<T>;
```

[#]{.pnum} *Effects*: Direct-non-list-initializes the contained value with `t`.

```cpp
constexpr explicit movable_box(T&& t);
```

[#]{.pnum} *Effects*: Direct-non-list-initializes the contained value with `std::move(t)`.

```cpp
template<class... Args> constexpr explicit movable_box(in_place_t, Args&&... args);
```

[#]{.pnum} *Constraints*: `is_constructible_v<T, Args...>` is `true`.

[#]{.pnum} *Effects*: Direct-non-list-initializes the contained value with `std::forward<Args>(args)...`.

```cpp
constexpr movable_box(const movable_box& rhs)
  requires copy_constructible<T>;
```

[#]{.pnum} *Effects*: If `rhs` contains a value, direct-non-list-initializes the contained value with `*rhs`; otherwise, `*this` contains no value.

[#]{.pnum} *Remarks*: This constructor is trivial if `is_trivially_copy_constructible_v<T>` is `true`.

```cpp
constexpr movable_box(movable_box&& rhs) noexcept(is_nothrow_move_constructible_v<T>);
```

[#]{.pnum} *Effects*: If `rhs` contains a value, direct-non-list-initializes the contained value with `std::move(*rhs)`; otherwise, `*this` contains no value. `rhs.has_value()` is unchanged.

[#]{.pnum} *Remarks*: This constructor is trivial if `is_trivially_move_constructible_v<T>` is `true`.

:::
