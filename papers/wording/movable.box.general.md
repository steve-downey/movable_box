```cpp
namespace std {
  template<class T>
    requires move_constructible<T> && is_object_v<T>
  class movable_box {
  public:
    using value_type = T;

    // @[movable.box.ctor]@, constructors
    constexpr movable_box() noexcept(is_nothrow_default_constructible_v<T>)
      requires default_initializable<T>;
    constexpr explicit movable_box(const T& t)
      requires copy_constructible<T>;
    constexpr explicit movable_box(T&& t);
    template<class... Args>
      requires is_constructible_v<T, Args...>
    constexpr explicit movable_box(in_place_t, Args&&... args);

    constexpr movable_box(const movable_box& rhs)
      requires copy_constructible<T>;
    constexpr movable_box(movable_box&& rhs) noexcept(
        is_nothrow_move_constructible_v<T>);

    // @[movable.box.dtor]@, destructor
    constexpr ~movable_box();

    // @[movable.box.assign]@, assignment
    constexpr movable_box& operator=(const movable_box& rhs) noexcept($see below$)
      requires copy_constructible<T>;
    constexpr movable_box& operator=(movable_box&& rhs) noexcept($see below$);

    // @[movable.box.obs]@, observers
    constexpr bool has_value() const noexcept;
    constexpr explicit operator bool() const noexcept;

    constexpr T& operator*() & noexcept;
    constexpr const T& operator*() const& noexcept;
    constexpr T&& operator*() && noexcept;
    constexpr const T&& operator*() const&& noexcept;

    constexpr T* operator->() noexcept;
    constexpr const T* operator->() const noexcept;
  };
}
```
