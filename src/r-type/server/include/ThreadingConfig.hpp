/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ThreadingConfig - Configuration constants for threading system
*/

#pragma once

#include <cstdint>

namespace rtype::server::threading {

/**
 * @brief Number of worker threads in the session thread pool
 *
 * Recommended values:
 * - 4: Light load (2-4 concurrent sessions)
 * - 6: Medium load (4-6 concurrent sessions) - DEFAULT
 * - 8: Heavy load (6-10 concurrent sessions)
 */
constexpr size_t THREAD_POOL_SIZE = 6;

/**
 * @brief Interval (in seconds) for printing performance metrics
 *
 * Note: Metrics system not yet implemented, reserved for future use.
 */
constexpr uint64_t METRICS_PRINT_INTERVAL_SECONDS = 5;

}
