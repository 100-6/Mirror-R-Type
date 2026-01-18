#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace bagario {

/**
 * @brief Simple INI file parser and writer
 */
class ConfigManager {
public:
    /**
     * @brief Load an INI file
     * @param filepath Path to the INI file
     * @return true if loaded successfully
     */
    bool load(const std::string& filepath) {
        filepath_ = filepath;
        data_.clear();

        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        std::string current_section;
        std::string line;

        while (std::getline(file, line)) {
            line = trim(line);

            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue;
            }

            if (line[0] == '[' && line.back() == ']') {
                current_section = line.substr(1, line.size() - 2);
                continue;
            }

            size_t eq_pos = line.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = trim(line.substr(0, eq_pos));
                std::string value = trim(line.substr(eq_pos + 1));

                std::string full_key = current_section.empty() ? key : current_section + "." + key;
                data_[full_key] = value;
            }
        }

        return true;
    }

    /**
     * @brief Save to an INI file
     * @param filepath Path to save (uses loaded path if empty)
     * @return true if saved successfully
     */
    bool save(const std::string& filepath = "") {
        std::string path = filepath.empty() ? filepath_ : filepath;
        if (path.empty()) {
            return false;
        }

        std::filesystem::path dir = std::filesystem::path(path).parent_path();
        if (!dir.empty() && !std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }

        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }

        std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> sections;

        for (const auto& [key, value] : data_) {
            size_t dot_pos = key.find('.');
            if (dot_pos != std::string::npos) {
                std::string section = key.substr(0, dot_pos);
                std::string name = key.substr(dot_pos + 1);
                sections[section].emplace_back(name, value);
            } else {
                sections[""].emplace_back(key, value);
            }
        }

        if (sections.count("") && !sections[""].empty()) {
            for (const auto& [key, value] : sections[""]) {
                file << key << " = " << value << "\n";
            }
            file << "\n";
        }

        for (const auto& [section, pairs] : sections) {
            if (section.empty()) continue;
            file << "[" << section << "]\n";
            for (const auto& [key, value] : pairs) {
                file << key << " = " << value << "\n";
            }
            file << "\n";
        }

        return true;
    }

    std::string get_string(const std::string& key, const std::string& default_value = "") const {
        auto it = data_.find(key);
        return it != data_.end() ? it->second : default_value;
    }

    int get_int(const std::string& key, int default_value = 0) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            try {
                return std::stoi(it->second);
            } catch (...) {}
        }
        return default_value;
    }

    bool get_bool(const std::string& key, bool default_value = false) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            std::string val = it->second;
            for (char& c : val) c = std::tolower(c);
            return val == "true" || val == "1" || val == "yes" || val == "on";
        }
        return default_value;
    }

    void set(const std::string& key, const std::string& value) {
        data_[key] = value;
    }

    void set(const std::string& key, int value) {
        data_[key] = std::to_string(value);
    }

    void set(const std::string& key, bool value) {
        data_[key] = value ? "true" : "false";
    }

private:
    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    std::string filepath_;
    std::unordered_map<std::string, std::string> data_;
};

}  // namespace bagario
