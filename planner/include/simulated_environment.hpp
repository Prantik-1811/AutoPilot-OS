#pragma once

#include <string>

#include "action.hpp"
#include "state.hpp"

namespace autopilot {

struct ExecutionResult {
    bool applied;
    bool verified;
    std::string message;
};

class SimulatedEnvironment {
public:
    explicit SimulatedEnvironment(State initial_state);

    const State& observe() const noexcept;
    ExecutionResult execute(const Action& action);

private:
    State state_;
};

} // namespace autopilot
