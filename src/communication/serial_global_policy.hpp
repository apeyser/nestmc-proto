#pragma once

#include <type_traits>
#include <vector>

#include <cstdint>

#include <communication/spike.hpp>

namespace nest {
namespace mc {
namespace communication {

struct serial_global_policy {
    using id_type = uint32_t;

    std::vector<spike<id_type>> const
    static gather_spikes(const std::vector<spike<id_type>>& local_spikes) {
        return local_spikes;
    }

    static int id() {
        return 0;
    }

    static int size() {
        return 1;
    }

    template <typename T>
    static T min(T value) {
        return value;
    }

    template <typename T>
    static T max(T value) {
        return value;
    }

    template <
        typename T,
        typename = typename std::enable_if<std::is_integral<T>::value>
    >
    static std::vector<T> make_map(T local) {
        return {T(0), local};
    }

    static void setup(int& argc, char**& argv) {}
    static void teardown() {}
    static const char* name() { return "serial"; }
};

} // namespace communication
} // namespace mc
} // namespace nest