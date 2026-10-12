::: wording

#### Observers [movable.box.obs] {-}

```cpp
constexpr bool has_value() const noexcept;
constexpr explicit operator bool() const noexcept;
```

[#]{.pnum} *Returns*: `true` if and only if `*this` contains a value.

```cpp
constexpr T& operator*() & noexcept;
constexpr const T& operator*() const& noexcept;
```

[#]{.pnum} *Preconditions*: `*this` contains a value.

[#]{.pnum} *Returns*: A reference to the contained value.

```cpp
constexpr T&& operator*() && noexcept;
constexpr const T&& operator*() const&& noexcept;
```

[#]{.pnum} *Preconditions*: `*this` contains a value.

[#]{.pnum} *Returns*: `std::move(`the contained value`)`.

```cpp
constexpr T* operator->() noexcept;
constexpr const T* operator->() const noexcept;
```

[#]{.pnum} *Preconditions*: `*this` contains a value.

[#]{.pnum} *Returns*: `addressof(`the contained value`)`.

:::
