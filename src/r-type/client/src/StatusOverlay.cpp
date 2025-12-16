#include "StatusOverlay.hpp"

namespace rtype::client {

StatusOverlay::StatusOverlay(Registry& registry, Entity status_entity)
    : registry_(registry)
    , status_entity_(status_entity)
    , connection_("Disconnected")
    , lobby_("Lobby: -")
    , session_("Waiting")
    , ping_ms_(-1) {
}

void StatusOverlay::set_connection(const std::string& status) {
    connection_ = status;
}

void StatusOverlay::set_lobby(const std::string& status) {
    lobby_ = status;
}

void StatusOverlay::set_session(const std::string& status) {
    session_ = status;
}

void StatusOverlay::set_ping(int ping_ms) {
    ping_ms_ = ping_ms;
}

void StatusOverlay::refresh() {
    auto& texts = registry_.get_components<UIText>();
    if (!texts.has_entity(status_entity_))
        return;

    std::ostringstream oss;
    oss << connection_;
    if (!lobby_.empty())
        oss << " | " << lobby_;
    if (!session_.empty())
        oss << " | " << session_;
    if (ping_ms_ >= 0)
        oss << " | Ping: " << ping_ms_ << "ms";

    UIText& text = texts[status_entity_];
    text.text = oss.str();
}

}
