#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace rtype::server {

class Server;  // Forward declaration

/**
 * @brief Admin command handler
 * Manages admin authentication and command execution
 */
class AdminManager {
public:
    explicit AdminManager(Server* server, const std::string& password_hash);

    bool verify_password(const std::string& password_hash) const;

    struct CommandResult {
        bool success;
        std::string message;
    };

    CommandResult execute_command(uint32_t admin_id, const std::string& command);

private:
    Server* server_;
    std::string admin_password_hash_;

    using CommandHandler = std::function<CommandResult(const std::vector<std::string>&)>;
    std::unordered_map<std::string, CommandHandler> commands_;

    void register_commands();
    std::vector<std::string> parse_command(const std::string& command);

    CommandResult cmd_help(const std::vector<std::string>& args);
    CommandResult cmd_list(const std::vector<std::string>& args);
    CommandResult cmd_kick(const std::vector<std::string>& args);
    CommandResult cmd_info(const std::vector<std::string>& args);

    // Tier 2 - Game control commands
    CommandResult cmd_pause(const std::vector<std::string>& args);
    CommandResult cmd_resume(const std::vector<std::string>& args);
    CommandResult cmd_clear_enemies(const std::vector<std::string>& args);
};

}
