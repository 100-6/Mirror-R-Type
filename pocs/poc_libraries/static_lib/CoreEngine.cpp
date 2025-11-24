#include "CoreEngine.hpp"
#include <iostream>
#include <algorithm>

namespace RType {

CoreEngine::CoreEngine() : nextId_(1) {
    std::cout << "[CoreEngine] Constructor - Static library linked at compile time\n";
}

CoreEngine::~CoreEngine() {
    std::cout << "[CoreEngine] Destructor\n";
}

void CoreEngine::initialize() {
    std::cout << "[CoreEngine] Initializing core engine (STATIC .a)\n";
    std::cout << "  → Code embedded in executable\n";
    std::cout << "  → No runtime dependencies\n";
    std::cout << "  → Fast function calls (no indirection)\n";
}

void CoreEngine::shutdown() {
    std::cout << "[CoreEngine] Shutting down\n";
    entities_.clear();
}

std::shared_ptr<Entity> CoreEngine::createEntity(const std::string& name) {
    auto entity = std::make_shared<Entity>(nextId_++, name);
    entities_.push_back(entity);
    std::cout << "[CoreEngine] Created entity #" << entity->getId()
              << ": " << entity->getName() << "\n";
    return entity;
}

void CoreEngine::removeEntity(int id) {
    auto it = std::remove_if(entities_.begin(), entities_.end(),
        [id](const auto& entity) { return entity->getId() == id; });

    if (it != entities_.end()) {
        std::cout << "[CoreEngine] Removed entity #" << id << "\n";
        entities_.erase(it, entities_.end());
    }
}

std::vector<std::shared_ptr<Entity>> CoreEngine::getEntities() const {
    return entities_;
}

} // namespace RType
