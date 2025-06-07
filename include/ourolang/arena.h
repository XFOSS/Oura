#pragma once
#include <memory_resource>
#include <vector>

namespace ouro {

class Arena {
    static constexpr std::size_t DEFAULT_SIZE = 1024 * 1024; // 1MB
    std::pmr::monotonic_buffer_resource resource;
public:
    Arena() : resource(DEFAULT_SIZE) {}

    std::pmr::memory_resource* get_resource() { return &resource; }
};

} // namespace ouro
