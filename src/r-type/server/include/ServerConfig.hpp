#pragma once

#include <cstdint>
#include "GameConfig.hpp"
#include <vector>

namespace rtype::server {

/**
 * @brief Serialize any POD struct to a byte vector
 */
template<typename T>
std::vector<uint8_t> serialize(const T& data) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&data);
    return std::vector<uint8_t>(bytes, bytes + sizeof(T));
}

/**
 * @brief Server configuration constants (alias to shared config for parity)
 */
namespace config = rtype::shared::config;

} // namespace rtype::server
