namespace demo {

template <class T>
class box {
  public:
    /// Copy assignment.
    ///
    /// The exception specification is shown as `noexcept(see below)`
    /// in the generated specification; the condition is given in prose.
    ///
    /// @seebelow noexcept
    box& operator=(const box& rhs) noexcept(noexcept(T(rhs.value_)));

    /// Move assignment, documented without the command.
    box& operator=(box&& rhs) noexcept(noexcept(T(static_cast<T&&>(rhs.value_))));

  private:
    T value_;
};

} // namespace demo
