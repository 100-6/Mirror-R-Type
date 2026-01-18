#pragma once

#include "ui/UIButton.hpp"
#include "ui/UILabel.hpp"
#include "ui/UITextField.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include <memory>
#include <functional>
#include <vector>

namespace rtype::client {

class NetworkClient;

/**
 * @brief Modal dialog for entering room password
 */
class PasswordDialog {
public:
    using JoinCallback = std::function<void(uint32_t room_id, const std::string& password)>;
    using CancelCallback = std::function<void()>;

    PasswordDialog(int screen_width, int screen_height);

    void initialize();
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void draw(engine::IGraphicsPlugin* graphics);

    void show(uint32_t room_id);
    void hide();
    bool is_visible() const { return visible_; }

    void set_join_callback(JoinCallback callback) { on_join_ = callback; }
    void set_cancel_callback(CancelCallback callback) { on_cancel_ = callback; }

private:
    int screen_width_;
    int screen_height_;
    bool visible_ = false;
    uint32_t pending_room_id_ = 0;

    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UIButton>> buttons_;
    std::unique_ptr<UITextField> password_field_;

    JoinCallback on_join_;
    CancelCallback on_cancel_;
};

}  // namespace rtype::client
