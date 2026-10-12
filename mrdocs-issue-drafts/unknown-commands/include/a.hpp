namespace demo {

/// \effects One-line effects.
int f1(int x);

/// \constraints `x` is positive.
/// \effects Effects text.
int f2(int x);

/// \effects Intro. Otherwise:
/// \item First item.
/// \item Second item.
int f3(int x);

/// \constraints-in-decl
/// \effects Effects text.
int f4(int x);

/// \omit
int f5(int x);

} // namespace demo
