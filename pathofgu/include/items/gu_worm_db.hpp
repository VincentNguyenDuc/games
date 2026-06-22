#pragma once

#include "items/gu_worm.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class IGuWormDatabase {
public:
    virtual ~IGuWormDatabase() = default;

    virtual std::shared_ptr<GuWormDefinition> get(const std::string& name) const = 0;
    virtual std::vector<std::shared_ptr<GuWormDefinition>> all() const = 0;
    virtual std::vector<std::shared_ptr<GuWormDefinition>> by_rank(int rank) const = 0;
    virtual std::vector<std::shared_ptr<GuWormDefinition>> by_type(GuWormType type) const = 0;
};

class InMemoryGuWormDatabase : public IGuWormDatabase {

public:
    InMemoryGuWormDatabase();

    std::shared_ptr<GuWormDefinition> get(const std::string& name) const override;
    std::vector<std::shared_ptr<GuWormDefinition>> all() const override;
    std::vector<std::shared_ptr<GuWormDefinition>> by_rank(int rank) const override;
    std::vector<std::shared_ptr<GuWormDefinition>> by_type(GuWormType type) const override;
    void add(GuWormDefinition def);

private:
    std::unordered_map<std::string, std::shared_ptr<GuWormDefinition>> entries_;
};
