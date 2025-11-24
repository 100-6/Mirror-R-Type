
#pragma once
#include <string>
#include <memory>
#include <vector>

namespace RType {

class Entity {
public:
    Entity(int id, const std::string& name) : id_(id), name_(name) {}

    int getId() const { return id_; }
    const std::string& getName() const { return name_; }

private:
    int id_;
    std::string name_;
};

class CoreEngine {
public:
    CoreEngine();
    ~CoreEngine();

    void initialize();
    void shutdown();

    std::shared_ptr<Entity> createEntity(const std::string& name);
    void removeEntity(int id);
    std::vector<std::shared_ptr<Entity>> getEntities() const;

    std::string getLibraryType() const { return "STATIC LIBRARY (.a)"; }

private:
    std::vector<std::shared_ptr<Entity>> entities_;
    int nextId_;
};

} // namespace RType
