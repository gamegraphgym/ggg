#pragma once
#include <concepts>
#include <stdexcept>
#include <string>

namespace ggg {
namespace graphs {

/**
 * @brief Custom exception for graph validation failures
 *
 * This exception is thrown when a graph fails validation checks.
 * It provides a descriptive message indicating what validation rule was violated.
 */
class GraphValidationError : public std::runtime_error {
  public:
    explicit GraphValidationError(const std::string &msg)
        : std::runtime_error(msg) {}
};

/**
 * @brief Concept defining a graph validator
 *
 * A Validator is any type that provides a static validate() method
 * accepting a const reference to a GraphType and returning void.
 * The validate() method should throw GraphValidationError if validation fails.
 *
 * @tparam V The validator type to check
 * @tparam GraphType The graph type the validator operates on
 */
template <typename V, typename GraphType>
concept Validator = requires(const GraphType &graph) {
    { V::validate(graph) } -> std::same_as<void>;
};

/**
 * @brief No-op validator that accepts any graph
 *
 * This validator performs no checks and always succeeds.
 * Useful when validation is not needed or as a placeholder.
 */
struct NoOpValidator {
    template <typename GraphType>
    static void validate(const GraphType &) {}
};

/**
 * @brief Composite validator that runs multiple validators in sequence
 *
 * Combines multiple validators into a single validator that runs all
 * constituent validators in order. Validation fails on the first
 * validator that throws an exception.
 *
 * @tparam GraphType The graph type to validate
 * @tparam Validators Variadic pack of validator types satisfying Validator concept
 *
 * @example
 * using MyValidator = CompositeValidator<
 *     ParityGraph,
 *     PlayerValidator<0, 1>,
 *     PriorityValidator<0>
 * >;
 * MyValidator::validate(graph);
 */
template <typename GraphType, Validator<GraphType>... Validators>
struct CompositeValidator {
    static void validate(const GraphType &graph) {
        (Validators::validate(graph), ...);
    }
};

} // namespace graphs
} // namespace ggg
