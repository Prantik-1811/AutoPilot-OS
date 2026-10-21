#include "simulated_environment.hpp"

#include <type_traits>
#include <utility>

namespace autopilot {

SimulatedEnvironment::SimulatedEnvironment(State initial_state)
    : state_(std::move(initial_state)) {}

const State& SimulatedEnvironment::observe() const noexcept {
    return state_;
}

ExecutionResult SimulatedEnvironment::execute(const Action& action) {
    return std::visit([this](const auto& typed_action) -> ExecutionResult {
        using T = std::decay_t<decltype(typed_action)>;
        State candidate = state_;

        if constexpr (std::is_same_v<T, CreateDirectory>) {
            if (!candidate.isAllowed(typed_action.path)) {
                return {false, false, "Directory is outside all allow-listed roots."};
            }
            if (candidate.directoryExists(typed_action.path) || candidate.fileExists(typed_action.path)) {
                return {false, false, "Directory path is already in use."};
            }
            if (!candidate.directoryExists(typed_action.path.parent())) {
                return {false, false, "Parent directory does not exist."};
            }

            candidate.addDirectory(typed_action.path);
            if (!candidate.directoryExists(typed_action.path)) {
                return {false, false, "Directory postcondition failed."};
            }
            state_ = std::move(candidate);
            return {true, true, "Directory exists at the requested path."};
        } else {
            if (!candidate.isAllowed(typed_action.source) ||
                !candidate.isAllowed(typed_action.destination)) {
                return {false, false, "Source and destination must be within allow-listed roots."};
            }
            if (typed_action.source == typed_action.destination) {
                return {false, false, "Source and destination must differ."};
            }
            const auto* source_record = candidate.file(typed_action.source);
            if (source_record == nullptr) {
                return {false, false, "Source file does not exist."};
            }
            if (candidate.fileExists(typed_action.destination) ||
                candidate.directoryExists(typed_action.destination)) {
                return {false, false, "Destination path is already in use."};
            }
            if (!candidate.directoryExists(typed_action.destination.parent())) {
                return {false, false, "Destination parent directory does not exist."};
            }

            const FileRecord original_record = *source_record;
            candidate.files_.erase(typed_action.source);
            candidate.addFile(typed_action.destination, original_record);
            const auto* destination_record = candidate.file(typed_action.destination);
            if (candidate.fileExists(typed_action.source) ||
                destination_record == nullptr ||
                *destination_record != original_record) {
                return {false, false, "Move postcondition failed."};
            }
            state_ = std::move(candidate);
            return {true, true, "Source is absent and destination content matches."};
        }
    }, action);
}

} // namespace autopilot
