/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GlobalLeaderboardManager implementation
*/

#include "GlobalLeaderboardManager.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <sstream>

namespace rtype::server {

GlobalLeaderboardManager::GlobalLeaderboardManager(const std::string& json_path)
    : json_path_(json_path) {
}

bool GlobalLeaderboardManager::ensure_directory_exists() {
    std::filesystem::path path(json_path_);
    std::filesystem::path dir = path.parent_path();

    if (dir.empty()) {
        return true;  // No directory in path
    }

    try {
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
            std::cout << "[GlobalLeaderboard] Created directory: " << dir << "\n";
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[GlobalLeaderboard] Failed to create directory: " << e.what() << "\n";
        return false;
    }
}

bool GlobalLeaderboardManager::load() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ifstream file(json_path_);
    if (!file.is_open()) {
        std::cout << "[GlobalLeaderboard] No existing leaderboard file, starting fresh\n";
        return true;  // File doesn't exist yet, that's OK
    }

    try {
        entries_.clear();

        // Simple JSON parsing (no external library dependency)
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();

        // Find leaderboard array
        size_t pos = content.find("\"leaderboard\"");
        if (pos == std::string::npos) {
            std::cout << "[GlobalLeaderboard] Empty or invalid JSON file\n";
            return true;
        }

        // Find the array start
        pos = content.find('[', pos);
        if (pos == std::string::npos) {
            return true;
        }

        // Parse each entry
        while (true) {
            size_t entry_start = content.find('{', pos);
            if (entry_start == std::string::npos) break;

            size_t entry_end = content.find('}', entry_start);
            if (entry_end == std::string::npos) break;

            std::string entry_str = content.substr(entry_start, entry_end - entry_start + 1);

            protocol::GlobalLeaderboardEntry entry;

            // Parse player_name
            size_t name_pos = entry_str.find("\"player_name\"");
            if (name_pos != std::string::npos) {
                size_t val_start = entry_str.find(':', name_pos);
                size_t quote_start = entry_str.find('"', val_start);
                size_t quote_end = entry_str.find('"', quote_start + 1);
                if (quote_start != std::string::npos && quote_end != std::string::npos) {
                    std::string name = entry_str.substr(quote_start + 1, quote_end - quote_start - 1);
                    entry.set_name(name);
                }
            }

            // Parse best_score
            size_t score_pos = entry_str.find("\"best_score\"");
            if (score_pos != std::string::npos) {
                size_t val_start = entry_str.find(':', score_pos);
                size_t val_end = entry_str.find_first_of(",}", val_start);
                if (val_start != std::string::npos && val_end != std::string::npos) {
                    std::string score_str = entry_str.substr(val_start + 1, val_end - val_start - 1);
                    // Trim whitespace
                    score_str.erase(0, score_str.find_first_not_of(" \t\n\r"));
                    score_str.erase(score_str.find_last_not_of(" \t\n\r") + 1);
                    entry.score = static_cast<uint32_t>(std::stoul(score_str));
                }
            }

            // Parse timestamp
            size_t ts_pos = entry_str.find("\"timestamp\"");
            if (ts_pos != std::string::npos) {
                size_t val_start = entry_str.find(':', ts_pos);
                size_t val_end = entry_str.find_first_of(",}", val_start);
                if (val_start != std::string::npos && val_end != std::string::npos) {
                    std::string ts_str = entry_str.substr(val_start + 1, val_end - val_start - 1);
                    // Trim whitespace
                    ts_str.erase(0, ts_str.find_first_not_of(" \t\n\r"));
                    ts_str.erase(ts_str.find_last_not_of(" \t\n\r") + 1);
                    entry.timestamp = static_cast<uint32_t>(std::stoul(ts_str));
                }
            }

            if (entry.score > 0) {
                entries_.push_back(entry);
            }

            pos = entry_end + 1;

            // Check for array end
            size_t next_brace = content.find('{', pos);
            size_t array_end = content.find(']', pos);
            if (array_end != std::string::npos &&
                (next_brace == std::string::npos || array_end < next_brace)) {
                break;
            }
        }

        sort_entries();
        std::cout << "[GlobalLeaderboard] Loaded " << entries_.size() << " entries\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[GlobalLeaderboard] Failed to parse JSON: " << e.what() << "\n";
        entries_.clear();
        return false;
    }
}

bool GlobalLeaderboardManager::save() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!ensure_directory_exists()) {
        return false;
    }

    std::ofstream file(json_path_);
    if (!file.is_open()) {
        std::cerr << "[GlobalLeaderboard] Failed to open file for writing: " << json_path_ << "\n";
        return false;
    }

    try {
        file << "{\n";
        file << "    \"leaderboard\": [\n";

        for (size_t i = 0; i < entries_.size(); ++i) {
            const auto& entry = entries_[i];
            std::string name(entry.player_name, strnlen(entry.player_name, sizeof(entry.player_name)));

            file << "        {\"player_name\": \"" << name
                 << "\", \"best_score\": " << entry.score
                 << ", \"timestamp\": " << entry.timestamp << "}";

            if (i < entries_.size() - 1) {
                file << ",";
            }
            file << "\n";
        }

        file << "    ]\n";
        file << "}\n";

        file.close();
        std::cout << "[GlobalLeaderboard] Saved " << entries_.size() << " entries to " << json_path_ << "\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[GlobalLeaderboard] Failed to write JSON: " << e.what() << "\n";
        return false;
    }
}

bool GlobalLeaderboardManager::try_add_score(const std::string& name, uint32_t score) {
    if (score == 0) {
        return false;  // Don't add zero scores
    }

    bool added = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if this score qualifies for leaderboard
        if (entries_.size() >= MAX_ENTRIES) {
            // Check if score beats the lowest entry
            if (score <= entries_.back().score) {
                return false;  // Score doesn't qualify
            }
        }

        // Create new entry
        protocol::GlobalLeaderboardEntry new_entry;
        new_entry.set_name(name);
        new_entry.score = score;
        new_entry.timestamp = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );

        entries_.push_back(new_entry);
        sort_entries();

        // Trim to max entries
        if (entries_.size() > MAX_ENTRIES) {
            entries_.resize(MAX_ENTRIES);
        }

        std::cout << "[GlobalLeaderboard] Added score: " << name << " - " << score << "\n";
        added = true;
    }

    // Save after releasing the lock
    if (added) {
        save();
    }

    return added;
}

std::vector<protocol::GlobalLeaderboardEntry> GlobalLeaderboardManager::get_entries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
}

size_t GlobalLeaderboardManager::get_entry_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

void GlobalLeaderboardManager::sort_entries() {
    // Sort by score (highest first)
    std::sort(entries_.begin(), entries_.end(),
        [](const protocol::GlobalLeaderboardEntry& a, const protocol::GlobalLeaderboardEntry& b) {
            return a.score > b.score;
        });
}

}  // namespace rtype::server
