::: wording

#### Destructor [movable.box.dtor] {-}

```cpp
constexpr ~movable_box();
```

[#]{.pnum} *Effects*: Destroys the contained value, if any.

[#]{.pnum} *Remarks*: This destructor is trivial if `is_trivially_destructible_v<T>` is `true`.

:::
