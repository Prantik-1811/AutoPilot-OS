#include "action.hpp"

#include <stdexcept>
#include <type_traits>

namespace autopilot {
namespace {

const std::array<ActionDefinition, 2> kDefinitions{{
    {
        ActionKind::CreateDirectory,
        "CreateDirectory",
        {{"path", ParameterType::Path}},
        {"path is within an allow-listed root", "parent directory exists", "path is unused"},
        {"directory exists at path"},
        1.0,
        0.0,
        false,
        "re-observe path and confirm it is a directory"
    },
    {
        ActionKind::MoveFile,
        "MoveFile",
        {{"source", ParameterType::Path}, {"destination", ParameterType::Path}},
        {
            "source file exists",
            "destination parent directory exists",
            "source and destination are within allow-listed roots",
            "destination is unused"
        },
        {
            "source is absent",
            "destination exists with the original size and content hash"
        },
        1.0,
        0.0,
        true,
        "re-observe both paths and compare size and SHA-256"
    }
}};

} // namespace

std::span<const ActionDefinition> ActionRegistry::definitions() const noexcept {
    return kDefinitions;
}

const ActionDefinition* ActionRegistry::find(std::string_view name) const noexcept {
    for (const auto& definition : kDefinitions) {
        if (definition.name == name) {
            return &definition;
        }
    }
    return nullptr;
}

const ActionDefinition& definitionFor(const ActionRegistry& registry, const Action& action) {
    const auto kind = std::visit([](const auto& typed_action) {
        using T = std::decay_t<decltype(typed_action)>;
        if constexpr (std::is_same_v<T, CreateDirectory>) {
            return ActionKind::CreateDirectory;
        } else {
            return ActionKind::MoveFile;
        }
    }, action);

    for (const auto& definition : registry.definitions()) {
        if (definition.kind == kind) {
            return definition;
        }
    }
    throw std::logic_error("Typed action has no registry definition.");
}

} // namespace autopilot
