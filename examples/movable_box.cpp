// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// The motivating use case for movable_box: a user-written view adaptor that
// stores a callable. Range adaptors must model std::ranges::view, which
// requires movable. Capturing lambdas are move-constructible but not
// assignable, so storing one directly breaks the view. movable_box<F>
// synthesizes the missing assignment operators -- exactly what the
// standard library's own *movable-box* does for transform_view, filter_view,
// and friends ([range.move.wrap]) -- and restores the concept.

#include <beman/movable_box/config.hpp>
#include <beman/movable_box/movable_box.hpp>

#include <concepts>
#include <functional>
#include <iostream>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

// A minimal transform_view-alike: lazily applies F to each element of V.
// UseBox selects how the callable is stored -- boxed (movable_box<F>) or
// raw (F) -- everything else about the two shapes is identical.
template <std::ranges::input_range V, std::copy_constructible F, bool UseBox>
    requires std::ranges::view<V> && std::is_object_v<F> &&
             std::regular_invocable<F&, std::ranges::range_reference_t<V>>
class basic_mapped_view : public std::ranges::view_interface<basic_mapped_view<V, F, UseBox>> {
    using Storage = std::conditional_t<UseBox, beman::movable_box::movable_box<F>, F>;

    static F& fun(Storage& s) {
        if constexpr (UseBox) {
            return *s; // movable_box::operator*
        } else {
            return s;
        }
    }

    class iterator {
      public:
        using Base            = std::ranges::iterator_t<V>;
        using value_type      = std::invoke_result_t<F&, std::ranges::range_reference_t<V>>;
        using difference_type = std::ranges::range_difference_t<V>;

        iterator() = default;
        iterator(Base cur, basic_mapped_view* parent) : cur_(cur), parent_(parent) {}

        auto      operator*() const { return std::invoke(fun(parent_->fun_), *cur_); }
        iterator& operator++() {
            ++cur_;
            return *this;
        }
        iterator operator++(int) {
            iterator tmp = *this;
            ++cur_;
            return tmp;
        }

        bool operator==(const iterator&) const = default;
        bool operator==(const std::ranges::sentinel_t<V>& s) const { return cur_ == s; }

      private:
        Base               cur_{};
        basic_mapped_view* parent_ = nullptr;
    };

  public:
    basic_mapped_view() = default;
    basic_mapped_view(V base, F fun) : base_(std::move(base)), fun_(std::move(fun)) {}

    iterator begin() { return iterator{std::ranges::begin(base_), this}; }
    auto     end() { return std::ranges::end(base_); }

  private:
    V       base_{};
    Storage fun_{};
};

// The paper's proposed shape: the callable lives in a movable_box.
template <class V, class F>
using mapped_view = basic_mapped_view<V, F, true>;

// What everyone writes first, and why it doesn't model view.
template <class V, class F>
using mapped_view_raw = basic_mapped_view<V, F, false>;

} // namespace

int main() {
    // --- Vignette 1: storing a capturing lambda in a view -----------------
    std::vector<int> data{1, 2, 3, 4, 5};
    int              factor = 10;
    auto             scale  = [factor](int x) { return x * factor; }; // capture => not assignable

    using F = decltype(scale);
    static_assert(!std::is_copy_assignable_v<F>);
    static_assert(!std::is_move_assignable_v<F>);
    static_assert(std::is_move_constructible_v<F>);

    using V     = std::ranges::ref_view<std::vector<int>>;
    using Boxed = mapped_view<V, F>;
    using Raw   = mapped_view_raw<V, F>;

    // Raw storage inherits F's missing assignment: not movable, not a view.
    static_assert(!std::ranges::view<Raw>);
    // movable_box synthesizes the assignment operators: movable, hence a view.
    static_assert(std::ranges::view<Boxed>);
    static_assert(std::copyable<Boxed>);

    Boxed mv(V(data), scale);
    for (int v : mv) {
        std::cout << v << ' ';
    }
    std::cout << '\n';

    // --- Vignette 2: synthesized assignment for a const-member type -------
    struct fixed_point {
        const int tag   = 0; // const member => the class is not copy-assignable
        int       value = 0;
    };
    static_assert(!std::is_copy_assignable_v<fixed_point>);

    beman::movable_box::movable_box<fixed_point> a;                     // default: {0, 0}
    beman::movable_box::movable_box<fixed_point> b(fixed_point{7, 42}); // direct-init from T&&
    static_assert(std::is_copy_assignable_v<decltype(a)>);              // the box is, though

    std::cout << "a = {" << a->tag << ", " << a->value << "}\n";
    a = b; // destroy-then-copy-construct, since fixed_point isn't copyable
    std::cout << "a = {" << a->tag << ", " << a->value << "}\n";
}
