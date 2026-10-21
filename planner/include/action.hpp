#pragma once

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "state.hpp"

namespace autopilot {

enum class ActionKind {
    CreateDirectory,
    MoveFile
};

enum class ParameterType {
    Path
};

struct ParameterDefinition {
    std::string_view name;
    ParameterType type;
};

struct ActionDefinition {
    ActionKind kind;
    std::string_view name;
    std::vector<ParameterDefinition> parameters;
    std::vector<std::string_view> preconditions;
    std::vector<std::string_view> effects;
    double cost;
    double risk;
    bool reversible;
    std::string_view verifier;
};

struct CreateDirectory {
    VirtualPath path;
};

struct MoveFile {
    VirtualPath source;
    VirtualPath destination;
};

using Action = std::variant<CreateDirectory, MoveFile>;

class ActionRegistry {
public:
    std::span<const ActionDefinition> definitions() const noexcept;
    const ActionDefinition* find(std::string_view name) const noexcept;
};

const ActionDefinition& definitionFor(const ActionRegistry& registry, const Action& action);

} // namespace autopilot
