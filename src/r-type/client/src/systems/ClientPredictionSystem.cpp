/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ClientPredictionSystem - Implementation
*/

#include "systems/ClientPredictionSystem.hpp"
#include <iostream>

namespace rtype::client {

ClientPredictionSystem::ClientPredictionSystem(uint32_t local_player_id)
    : local_player_id_(local_player_id)
{
    std::cout << "[ClientPredictionSystem] Initialized for player " << local_player_id << "\n";
}

void ClientPredictionSystem::on_input_sent(uint32_t sequence, uint16_t flags, uint32_t timestamp)
{
    // Store the input for potential replay during reconciliation
    pending_inputs_.push_back({sequence, flags, timestamp});

    // Limit buffer size to prevent memory issues
    if (pending_inputs_.size() > MAX_PENDING_INPUTS) {
        std::cout << "[ClientPredictionSystem] WARNING: Input buffer full, dropping oldest input\n";
        pending_inputs_.pop_front();
    }
}

void ClientPredictionSystem::acknowledge_input(uint32_t last_processed_sequence)
{
    // Remove all inputs that have been confirmed by the server
    size_t removed_count = 0;
    while (!pending_inputs_.empty() &&
           pending_inputs_.front().sequence_number <= last_processed_sequence) {
        pending_inputs_.pop_front();
        removed_count++;
    }

    if (removed_count > 0) {
        std::cout << "[ClientPredictionSystem] Acknowledged " << removed_count
                  << " inputs up to seq " << last_processed_sequence
                  << " (remaining: " << pending_inputs_.size() << ")\n";
    }
}

}  // namespace rtype::client