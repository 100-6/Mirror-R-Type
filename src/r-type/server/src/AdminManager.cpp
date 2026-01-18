#include "AdminManager.hpp"
#include "Server.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace rtype::server {

AdminManager::AdminManager(Server* server, const std::string& password_hash)
    : server_(server)
    , admin_password_hash_(password_hash)
{
    register_commands();
}

bool AdminManager::verify_password(const std::string& password_hash) const
{
    return password_hash == admin_password_hash_;
}

void AdminManager::register_commands()
{
    commands_["help"] = [this](const auto& args) { return cmd_help(args); };
    commands_["list"] = [this](const auto& args) { return cmd_list(args); };
    commands_["kick"] = [this](const auto& args) { return cmd_kick(args); };
    commands_["info"] = [this](const auto& args) { return cmd_info(args); };
    commands_["pause"] = [this](const auto& args) { return cmd_pause(args); };
    commands_["resume"] = [this](const auto& args) { return cmd_resume(args); };
    commands_["clearenemies"] = [this](const auto& args) { return cmd_clear_enemies(args); };
}

std::vector<std::string> AdminManager::parse_command(const std::string& command)
{
    std::vector<std::string> tokens;
    std::istringstream iss(command);
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

AdminManager::CommandResult AdminManager::execute_command(uint32_t admin_id,
                                                          const std::string& command)
{
    auto tokens = parse_command(command);

    if (tokens.empty())
        return {false, "Empty command"};
    std::string cmd = tokens[0];
    if (!cmd.empty() && cmd[0] == '/')
        cmd = cmd.substr(1);
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    auto it = commands_.find(cmd);
    if (it == commands_.end())
        return {false, "Unknown command: " + cmd + ". Type 'help' for available commands."};
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    return it->second(args);
}

AdminManager::CommandResult AdminManager::cmd_help(const std::vector<std::string>& args)
{
    std::ostringstream oss;
    oss << "Available admin commands:\n"
        << "Tier 1 - Basic:\n"
        << "  help                 - Show this help\n"
        << "  list                 - List connected players\n"
        << "  kick <player_id>     - Kick a player\n"
        << "  info                 - Server statistics\n"
        << "\n"
        << "Tier 2 - Game Control:\n"
        << "  pause                - Pause all game sessions\n"
        << "  resume               - Resume all game sessions\n"
        << "  clearenemies [sid]   - Clear enemies (session_id optional)";
    return {true, oss.str()};
}

AdminManager::CommandResult AdminManager::cmd_list(const std::vector<std::string>& args)
{
    auto players = server_->get_connected_players();

    if (players.empty())
        return {true, "No players connected"};
    std::ostringstream oss;
    oss << "Connected players (" << players.size() << "):\n";
    for (const auto& player : players) {
        oss << "  [ID: " << player.player_id << "] " << player.player_name;
        if (player.in_game)
            oss << " (in game - session " << player.session_id << ")";
        oss << "\n";
    }
    return {true, oss.str()};
}

AdminManager::CommandResult AdminManager::cmd_kick(const std::vector<std::string>& args)
{
    if (args.empty())
        return {false, "Usage: kick <player_id>"};
    uint32_t player_id;

    try {
        player_id = std::stoul(args[0]);
    } catch (...) {
        return {false, "Invalid player ID: " + args[0]};
    }
    std::string reason = "Kicked by admin";
    if (args.size() > 1) {
        reason = "";
        for (size_t i = 1; i < args.size(); ++i) {
            reason += args[i];
            if (i < args.size() - 1) reason += " ";
        }
    }
    bool success = server_->kick_player(player_id, reason);
    if (success)
        return {true, "Player " + std::to_string(player_id) + " kicked"};
    else
        return {false, "Player " + std::to_string(player_id) + " not found"};
}

AdminManager::CommandResult AdminManager::cmd_info(const std::vector<std::string>& args)
{
    auto stats = server_->get_server_stats();

    std::ostringstream oss;
    oss << "Server Statistics:\n"
        << "  Uptime: " << stats.uptime_seconds << "s\n"
        << "  Connected Players: " << stats.connected_players << "\n"
        << "  Active Sessions: " << stats.active_sessions << "\n"
        << "  Total Connections: " << stats.total_connections;
    return {true, oss.str()};
}

AdminManager::CommandResult AdminManager::cmd_pause(const std::vector<std::string>& args)
{
    uint32_t paused_count = server_->pause_all_sessions();

    if (paused_count == 0)
        return {false, "No active game sessions to pause"};
    return {true, "Paused " + std::to_string(paused_count) + " game session(s)"};
}

AdminManager::CommandResult AdminManager::cmd_resume(const std::vector<std::string>& args)
{
    uint32_t resumed_count = server_->resume_all_sessions();

    if (resumed_count == 0)
        return {false, "No paused game sessions to resume"};
    return {true, "Resumed " + std::to_string(resumed_count) + " game session(s)"};
}

AdminManager::CommandResult AdminManager::cmd_clear_enemies(const std::vector<std::string>& args)
{
    if (!args.empty()) {
        uint32_t session_id;
        try {
            session_id = std::stoul(args[0]);
        } catch (...) {
            return {false, "Invalid session_id: " + args[0]};
        }
        bool success = server_->clear_enemies_in_session(session_id);
        if (success)
            return {true, "Cleared enemies from session " + std::to_string(session_id)};
        else
            return {false, "Session " + std::to_string(session_id) + " not found"};
    }
    uint32_t cleared_count = server_->clear_enemies_all_sessions();
    if (cleared_count == 0)
        return {false, "No active game sessions"};
    return {true, "Cleared enemies from " + std::to_string(cleared_count) + " session(s)"};
}

}
