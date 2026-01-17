/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GlobalLeaderboardManager - Manages persistent global all-time leaderboard
*/

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include "protocol/Payloads.hpp"

namespace rtype::server {

/**
 * @brief Manages the global all-time leaderboard
 *
 * Handles:
 * - Loading/saving leaderboard data from/to JSON file
 * - Adding new scores (if they qualify for top 10)
 * - Retrieving current top 10 entries
 *
 * Thread-safe for concurrent access.
 */
class GlobalLeaderboardManager {
public:
    /**
     * @brief Construct a new GlobalLeaderboardManager
     * @param json_path Path to the JSON file for persistent storage
     */
    explicit GlobalLeaderboardManager(const std::string& json_path = "data/global_leaderboard.json");

    ~GlobalLeaderboardManager() = default;

    /**
     * @brief Load leaderboard data from JSON file
     * @return true if loaded successfully (or file doesn't exist yet)
     */
    bool load();

    /**
     * @brief Save leaderboard data to JSON file
     * @return true if saved successfully
     */
    bool save();

    /**
     * @brief Try to add a score to the leaderboard
     *
     * Score will be added only if it qualifies for top 10 (or less than 10 entries exist).
     * The leaderboard is automatically saved after adding a new score.
     *
     * @param name Player name
     * @param score Player score
     * @return true if score was added to leaderboard
     */
    bool try_add_score(const std::string& name, uint32_t score);

    /**
     * @brief Get current leaderboard entries
     * @return Vector of leaderboard entries (sorted by score, highest first)
     */
    std::vector<protocol::GlobalLeaderboardEntry> get_entries() const;

    /**
     * @brief Get number of entries in leaderboard
     */
    size_t get_entry_count() const;

private:
    std::string json_path_;
    std::vector<protocol::GlobalLeaderboardEntry> entries_;
    mutable std::mutex mutex_;

    static constexpr size_t MAX_ENTRIES = 10;

    /**
     * @brief Sort entries by score (highest first)
     */
    void sort_entries();

    /**
     * @brief Ensure data directory exists
     */
    bool ensure_directory_exists();
};

}  // namespace rtype::server
