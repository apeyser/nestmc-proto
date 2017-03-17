#pragma once

#include <iterator>
#include <stdexcept>
#include <type_traits>

#include <util/either.hpp>
#include <util/meta.hpp>
#include <util/partition_iterator.hpp>
#include <util/range.hpp>

namespace nest {
namespace mc {
namespace util {

struct invalid_partition: std::runtime_error {
    explicit invalid_partition(const std::string& what): std::runtime_error(what) {}
    explicit invalid_partition(const char* what): std::runtime_error(what) {}
};

/*
 * Present a sequence with monotically increasing values as a partition,
 */
template <typename I>
class partition_range: public range<partition_iterator<I>> {
    using base = range<partition_iterator<I>>;
    using inner_value_type = typename std::iterator_traits<I>::value_type;

public:
    using typename base::iterator;
    using typename base::value_type;
    using typename base::size_type;
    using base::left;
    using base::right;
    using base::front;
    using base::back;
    using base::empty;

    // `npos` is returned by the `index()` method if the search fails;
    // analogous to `std::string::npos`.
    static constexpr size_type npos = static_cast<size_type>(-1);

    partition_range() = default;

    template <typename Seq>
    partition_range(const Seq& s): base{std::begin(s), upto(std::begin(s), std::end(s))} {
        EXPECTS(is_valid());
    }

    // explicitly check invariants
    void validate() const {
        auto ok = is_valid();
        if (!ok) {
            throw invalid_partition(ok.second());
        }
    }

    // find half-open sub-interval containing x
    iterator find(const inner_value_type& x) const {
        if (empty()) {
            return right;
        }

        auto divs = divisions();
        auto i = std::upper_bound(divs.left, divs.right, x);
        if (i==divs.left || i==divs.right) {
            return right;
        }
        return iterator{std::prev(i)};
    }

    size_type index(const inner_value_type& x) const {
        iterator i = find(x);
        return i==right? npos: i-left;
    }

    // access to underlying divisions
    range<I> divisions() const {
        return {left.get(), std::next(right.get())};
    }

    // global upper and lower bounds of partition
    value_type bounds() const {
        return {front().first, back().second};
    }

private:
    either<bool, std::string> is_valid() const {
        if (!std::is_sorted(left.get(), right.get())) {
            return std::string("offsets are not monotonically increasing");
        }
        else {
            return true;
        }
    }
};


template <
    typename Seq,
    typename SeqIter = typename sequence_traits<Seq>::const_iterator,
    typename = enable_if_t<is_forward_iterator<SeqIter>::value>
>
partition_range<SeqIter> partition_view(const Seq& r) {
    return partition_range<SeqIter>(r);
}

/*
 * Construct a monotonically increasing sequence in a provided
 * container representing a partition from a sequence of subset sizes.
 *
 * If the first parameter is `partition_in_place`, the provided
 * container `divisions` will not be resized, and the partition will 
 * be of length `util::size(divisions)-1` or zero if `divisions` is
 * empty.
 *
 * Otherwise, `divisions` will be be resized to `util::size(sizes)+1`
 * and represent a partition of length `util::size(sizes)`.
 *
 * Returns a partition view over `divisions`.
 */

struct partition_in_place_t {
    constexpr partition_in_place_t() {}
};

constexpr partition_in_place_t partition_in_place;

template <
    typename Part,
    typename Sizes,
    typename T = typename sequence_traits<Part>::value_type
>
partition_range<typename sequence_traits<Part>::const_iterator>
make_partition(partition_in_place_t, Part& divisions, const Sizes& sizes, T from=T{}) {
    auto pi = std::begin(divisions);
    auto pe = std::end(divisions);
    auto si = std::begin(sizes);
    auto se = std::end(sizes);

    if (pi!=pe) {
        *pi++ = from;
        while (pi!=pe && si!=se) {
            from += *si++;
            *pi++ = from;
        }
        while (pi!=pe) {
            *pi++ = from;
        }
    }
    return partition_view(divisions);
}

template <
    typename Part,
    typename Sizes,
    typename T = typename sequence_traits<Part>::value_type
>
partition_range<typename sequence_traits<Part>::const_iterator>
make_partition(Part& divisions, const Sizes& sizes, T from=T{}) {
    divisions.resize(size(sizes)+1);

    // (would use std::inclusive_scan in C++17)
    auto pi = std::begin(divisions);
    for (const auto& s: sizes) {
        *pi++ = from;
        from += s;
    }
    *pi = from;

    return partition_view(divisions);
}

struct partition_functional_t {
    constexpr partition_functional_t() {}
};
constexpr partition_functional_t partition_functional;

template <
    typename Part,
    typename Range,
    typename Func,
    typename T = typename sequence_traits<Part>::value_type
>
partition_range<typename sequence_traits<Part>::const_iterator>
make_partition(partition_functional_t,
               Part& divisions, const Range& r, Func f, T from=T{}) {
    divisions.resize(size(r)+1);

    auto pi = std::begin(divisions);
    for (const auto& i: r) {
        *pi++ = from;
        from += f(i);
    }
    *pi = from;

    return partition_view(divisions);
}


} // namespace util
} // namespace mc
} // namespace nest
