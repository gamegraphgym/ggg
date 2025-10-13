#pragma once

#include <ostream>
#include <string>

namespace ggg {
namespace solutions {

/**
 * @class ISolution
 * @brief Minimal solution type that records the winner of the initial state.
 *
 * Capability: I (Initial)
 */
class ISolution {
  public:
    int winner; ///< The winner of the solution: 0 or 1, -1 if unknown.

    /**
     * @brief Default constructor. Initializes as unsolved.
     */
    ISolution() : winner(-1) {}

    // Defaulted special member functions are sufficient.

    // No solved-state tracking.

    /**
     * @brief Serializes the solution to a JSON string.
     * @return JSON representation of the solution.
     */
    std::string to_json() const { return std::string("{") + "\"winner\":" + std::to_string(winner) + '}'; }
};

/**
 * @brief Stream insertion operator for ISolution.
 *
 * Outputs a human-readable description of the solution's winner state.
 *
 * @param os Output stream.
 * @param sol ISolution instance.
 * @return Reference to the output stream.
 */
inline std::ostream &operator<<(std::ostream &os, const ISolution &sol) {
    os << "Winner: " << sol.winner;
    return os;
}

} // namespace solutions
} // namespace ggg
