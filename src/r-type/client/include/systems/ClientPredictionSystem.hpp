/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ClientPredictionSystem - Client-side prediction for lag compensation
*/

#pragma once

#include <vector>
#include <deque>
#include <cstdint>
#include "protocol/Payloads.hpp"

namespace rtype::client {

/**
 * @brief Stores a predicted input for replay during reconciliation
 */
struct PredictedInput {
    uint32_t sequence_number;
    uint16_t input_flags;
    uint32_t timestamp;
};

/**
 * @brief Client-side prediction system for lag compensation
 *
 * This system implements client-side prediction by:
 * 1. Storing sent inputs in a buffer
 * 2. Immediately applying predicted movement locally
 * 3. Reconciling with server state when snapshots arrive
 */
class ClientPredictionSystem {
public:
    /**
     * @brief Construct a new ClientPredictionSystem
     * @param local_player_id The local player's ID
     */
    explicit ClientPredictionSystem(uint32_t local_player_id);

    /**
     * @brief Called when an input is sent to the server
     * Stores the input for potential replay during reconciliation
     * @param sequence Input sequence number
     * @param flags Input bitfield
     * @param timestamp Client timestamp
     */
    void on_input_sent(uint32_t sequence, uint16_t flags, uint32_t timestamp);

    /**
     * @brief Acknowledges that the server has processed inputs up to this sequence
     * Removes confirmed inputs from the pending buffer
     * @param last_processed_sequence Last sequence number processed by server
     */
    void acknowledge_input(uint32_t last_processed_sequence);

    /**
     * @brief Reset the prediction system state
     * Clears pending inputs. Call this on respawn or level change.
     */
    void reset();

    /**
     * @brief Get the pending inputs buffer (for reconciliation)
     * @return Const reference to the deque of pending inputs
     */
    const std::deque<PredictedInput>& get_pending_inputs() const { return pending_inputs_; }

    /**
     * @brief Get the local player ID
     */
    uint32_t get_local_player_id() const { return local_player_id_; }

private:
    uint32_t local_player_id_;
    std::deque<PredictedInput> pending_inputs_;
    static constexpr size_t MAX_PENDING_INPUTS = 64;  // Maximum buffered inputs
};

}  // namespace rtype::client