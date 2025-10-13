#pragma once

// Simplex utility moved from solvers/Simplex.hpp into the public library
// Full implementation copied from solvers/Simplex.hpp; kept non-namespaced to
// minimize API changes for existing code.

#include "libggg/utils/logging.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>

class Simplex {
  public:
    Simplex(const std::vector<std::vector<double>> &matrix_coeff,
            const std::vector<double> &obj_coeff_low,
            const std::vector<double> &obj_coeff_up,
            const std::vector<double> &var_low,
            const std::vector<double> &var_up,
            const std::vector<double> &obj_coeff) {
        const double penalty = 1e6;
        origVars = obj_coeff.size();
        int m = obj_coeff_low.size();
        // Step 1: Normalize RHS by flipping negative inequalities
        numVariables = origVars + 1; // x'_i plus shared W
        int w_index = origVars;
        numConstraints = 0;
        std::vector<std::vector<double>> constraints;
        std::vector<double> rhs;
        std::vector<double> slack_signs;
        // Step 2: Apply substitution x_i = x'_i - W to general constraints
        for (int i = 0; i < m; ++i) {
            if (std::isfinite(obj_coeff_low[i])) {
                double rhs_val = obj_coeff_low[i];
                std::vector<double> row(numVariables, 0.0);
                for (int j = 0; j < origVars; ++j) {
                    double aij = matrix_coeff[i][j];
                    row[j] = aij;
                    row[w_index] -= aij;
                }
                if (rhs_val < 0) {
                    for (double &val : row) {
                        val *= -1;
                    }
                    rhs_val *= -1;
                    slack_signs.push_back(+1.0);
                } else {
                    slack_signs.push_back(-1.0);
                }
                constraints.push_back(row);
                rhs.push_back(rhs_val);
                ++numConstraints;
            }
            if (std::isfinite(obj_coeff_up[i])) {
                double rhs_val = obj_coeff_up[i];
                std::vector<double> row(numVariables, 0.0);

                for (int j = 0; j < origVars; ++j) {
                    double aij = matrix_coeff[i][j];
                    row[j] = aij;
                    row[w_index] -= aij;
                }
                if (rhs_val < 0) {
                    for (double &val : row) {
                        val *= -1;
                    }
                    rhs_val *= -1;
                    slack_signs.push_back(-1.0);
                } else {
                    slack_signs.push_back(+1.0);
                }
                constraints.push_back(row);
                rhs.push_back(rhs_val);
                ++numConstraints;
            }
        }
        // Step 2 continued: Apply substitution to box constraints
        for (int i = 0; i < origVars; ++i) {
            if (std::isfinite(var_low[i])) {
                double rhs_val = var_low[i];
                std::vector<double> row(numVariables, 0.0);
                row[i] = -1.0;
                row[w_index] += 1.0;
                if (rhs_val < 0) {
                    for (double &val : row) {
                        val *= -1;
                    }
                    rhs_val *= -1;
                    slack_signs.push_back(+1.0);
                } else {
                    slack_signs.push_back(-1.0);
                }
                constraints.push_back(row);
                rhs.push_back(rhs_val);
                ++numConstraints;
            }
            if (std::isfinite(var_up[i])) {
                double rhs_val = var_up[i];
                std::vector<double> row(numVariables, 0.0);
                row[i] = +1.0;
                row[w_index] -= 1.0;

                if (rhs_val < 0) {
                    for (double &val : row) {
                        val *= -1;
                    }
                    rhs_val *= -1;
                    slack_signs.push_back(-1.0);
                } else {
                    slack_signs.push_back(+1.0);
                }

                constraints.push_back(row);
                rhs.push_back(rhs_val);
                ++numConstraints;
            }
        }
        // Step 3: Add slack variables and Step 4: add artificial variables (if needed)
        int extra_cols = 0;
        std::vector<bool> add_artificial(numConstraints, false);
        for (int i = 0; i < numConstraints; ++i) {
            if (slack_signs[i] < 0.0) {
                add_artificial[i] = true;
                extra_cols++;
                artificialVar++;
            }
        }
        int total_cols = numVariables + numConstraints + extra_cols + 1; // variables + slacks + artificial + RHS
        tableau.resize(numConstraints + 1, std::vector<double>(total_cols, 0.0));
        basis.resize(numConstraints);
        int artificial_offset = numVariables + numConstraints;
        int artificial_index = 0;
        for (int i = 0; i < numConstraints; ++i) {
            // Fill transformed constraint coefficients
            for (int j = 0; j < numVariables; ++j) {
                tableau[i][j] = constraints[i][j];
            }
            // Always insert the slack variable with its correct sign
            tableau[i][numVariables + i] = slack_signs[i];
            // Insert artificial variable only if required
            if (add_artificial[i]) {
                tableau[i][artificial_offset + artificial_index] = +1.0;
                basis[i] = artificial_offset + artificial_index;
                artificial_index++;
            } else {
                basis[i] = numVariables + i;
            }
            // RHS
            tableau[i].back() = rhs[i];
        }
        // Fill objective row
        for (int j = 0; j < origVars; ++j) {
            double cj = obj_coeff[j];
            tableau[numConstraints][j] = -cj;
            tableau[numConstraints][w_index] += cj; // Apply to W
        }
        // Add penalty terms for artificial variables
        artificial_index = 0;
        for (int i = 0; i < numConstraints; ++i) {
            if (add_artificial[i]) {
                tableau[numConstraints][artificial_offset + artificial_index] = penalty;
                artificial_index++;
            }
        }
    }

    void perform_pivot(int pivot_row, int pivot_col) {
        int total_cols = tableau[0].size();
        double pivot_element = tableau[pivot_row][pivot_col];
        if (std::fabs(pivot_element) < 1e-8) {
            throw std::runtime_error("Invalid pivot: element too close to zero");
        }
        // Step 1: Normalize the pivot row
        for (int col = 0; col < total_cols; ++col) {
            tableau[pivot_row][col] /= pivot_element;
        }
        // Step 2: Eliminate pivot column from all other rows
        for (int row = 0; row < tableau.size(); ++row) {
            if (row == pivot_row) {
                continue;
            }
            double factor = tableau[row][pivot_col];
            for (int col = 0; col < total_cols; ++col) {
                tableau[row][col] -= factor * tableau[pivot_row][col];
            }
        }
        // Step 3: Update basis
        basis[pivot_row] = pivot_col;
    }

    auto remove_artificial_variables() -> bool {
        int artificial_offset = numVariables + numConstraints;
        int total_cols = tableau[0].size();
        for (int i = 0; i < numConstraints; ++i) {
            int basic_var = basis[i];
            // Only target rows with artificial variables in the basis
            if (basic_var >= artificial_offset) {
                double artificial_cost = tableau[numConstraints][basic_var];
                if (std::fabs(artificial_cost) >= 1e-8) {
                    int pivot_row = -1;
                    double min_ratio = 1e20;
                    for (int i = 0; i < numConstraints; ++i) {
                        double coeff = tableau[i][basic_var];
                        double rhs_val = tableau[i].back();
                        if (coeff > 1e-8) {
                            double ratio = rhs_val / coeff;
                            if (ratio < min_ratio) {
                                min_ratio = ratio;
                                pivot_row = i;
                            }
                        }
                    }
                    perform_pivot(pivot_row, basic_var);
                    return true;
                }
            }
        }
        return false; // All penalized artificials handled or skipped temporarily
    }

    auto calculate_simplex() -> bool {
        // Step 2: Standard Simplex optimization
        int total_cols = tableau[0].size();
        int pivot_col = -1;
        double most_negative = -1e-8;
        for (int col = 0; col < total_cols - 1; ++col) {
            double cost = tableau[numConstraints][col];
            if (cost < most_negative) {
                most_negative = cost;
                pivot_col = col;
            }
        }
        if (pivot_col == -1) {
            return false; // Optimal
        }
        // Ratio test
        int pivot_row = -1;
        double min_ratio = 1e20;
        for (int i = 0; i < numConstraints; ++i) {
            double coeff = tableau[i][pivot_col];
            double rhs_val = tableau[i].back();
            if (coeff > 1e-8) {
                double ratio = rhs_val / coeff;
                if (ratio < min_ratio) {
                    min_ratio = ratio;
                    pivot_row = i;
                }
            }
        }
        if (pivot_row != -1) {
            perform_pivot(pivot_row, pivot_col);
            return true;
        }
        return false;
    }

    void set_objective_at(int i) {
        int obj_row_index = tableau.size() - 1;
        // Assuming tableau is a 2D vector and the first row [0] is the objective function
        if (obj_row_index < 0 || i < 0 || i >= tableau[obj_row_index].size()) {
            return;
        }
        // Erase the objective function: set all values to 0
        std::fill(tableau[obj_row_index].begin(), tableau[obj_row_index].end(), 0.0);
        // Assign x at column i
        tableau[obj_row_index][i] = pivotValue;
    }

    void update_objective_row(const std::vector<double> &new_obj_coeff, double new_rhs) {
        int w_index = origVars;
        int total_cols = tableau[0].size();
        // Clear current objective row (excluding RHS)
        for (int j = 0; j < total_cols - 1; ++j) {
            tableau[numConstraints][j] = 0.0;
        }
        // Apply new objective coefficients for transformed variables
        for (int j = 0; j < origVars; ++j) {
            double cj = new_obj_coeff[j];
            tableau[numConstraints][j] = -cj;       // x'_j gets -c_j
            tableau[numConstraints][w_index] += cj; // shared W gets +c_j
        }
        // Override objective RHS value
        tableau[numConstraints].back() = new_rhs;
    }

    void normalize_objective_row() {
        int total_cols = tableau[0].size();
        int total_rows = tableau.size();
        int objective_row = numConstraints;
        for (int col = 0; col < total_cols - 1; ++col) {
            double cost = tableau[objective_row][col];
            if (std::fabs(cost) < 1e-8) {
                continue; // Skip zero or negligible cost
            }
        }
    }

    void purge_artificial_columns() {
        int artificial_offset = numVariables + numConstraints;
        int total_cols = tableau[0].size();
        int total_rows = tableau.size();
        // Clear each artificial column across all rows
        for (int col = artificial_offset; col < total_cols - 1; ++col) {
            for (int row = 0; row < total_rows; ++row) {
                tableau[row][col] = 0.0;
            }
        }
    }

    auto fixed_calculate_simplex(int pivot_col) -> bool {
        // Ratio test
        int pivot_row = -1;
        double min_ratio = 1e20;
        for (int i = 0; i < numConstraints; ++i) {
            double coeff = tableau[i][pivot_col];
            double rhs_val = tableau[i].back();
            if (coeff > 1e-8) {
                double ratio = rhs_val / coeff;
                if (ratio < min_ratio) {
                    min_ratio = ratio;
                    pivot_row = i;
                }
            }
        }
        if (pivot_row != -1) {
            perform_pivot(pivot_row, pivot_col);
            return true;
        }
        return false;
    }

    auto all_variables_have_non_basic_slack(const int *actions) -> bool {
        int num_slack_vars = numConstraints; // each constraint gets one
        int slack_offset = origVars + 1;
        int artificial_offset = slack_offset + num_slack_vars;
        int rhs_column = artificial_offset + artificialVar;
        std::vector<bool> has_non_basic_slack(origVars, false);
        for (int s = 0; s < num_slack_vars; ++s) {
            int col = slack_offset + s;
            int var = *(actions + s);
            if (var < 0 || var >= origVars || col >= rhs_column) {
                continue;
            }
            bool is_basic = std::find(basis.begin(), basis.end(), col) != basis.end();
            if (!is_basic) {
                has_non_basic_slack[var] = true;
            }
        }
        for (int i = 0; i < origVars; ++i) {
            if (!has_non_basic_slack[i]) {
                return false;
            }
        }
        return true;
    }

    auto evaluate_candidate_slack(const int *actions) -> int {
        int slack_offset = numVariables;
        int artificial_offset = tableau[0].size();
        int num_slack = artificial_offset - slack_offset;
        // Identify slack variables in the basis
        std::vector<int> slack_in_basis;
        for (int col : basis) {
            if (col >= slack_offset && col < artificial_offset) {
                slack_in_basis.push_back(col);
            }
        }
        std::vector<bool> tried(num_slack, false);
        // Keep track of which slack candidates we've examined
        bool found_valid = false;
        int candidate_col = 0;
        while (true) {
            candidate_col = -1;
            // Search for next untried candidate slack variable
            for (int i = 0; i < num_slack; ++i) {
                if (tried[i]) {
                    continue;
                }
                int col = slack_offset + i;
                // Skip if in basis
                if (std::find(slack_in_basis.begin(), slack_in_basis.end(), col) != slack_in_basis.end()) {
                    tried[i] = true;
                    // mark as tried even if it’s in basis
                    continue;
                }
                bool has_positive = false;
                for (int row = 0; row < numConstraints; ++row) {
                    if (tableau[row][col] > 0) {
                        has_positive = true;
                        break;
                    }
                }
                if (has_positive) {
                    candidate_col = col;
                    tried[i] = true;
                    break;
                }
                tried[i] = true;
            }
            if (candidate_col == -1 || candidate_col > numVariables + numConstraints) {
                break;
            }
            int candidate_slack_index = candidate_col - slack_offset;
            int candidate_var = *(actions + candidate_slack_index);
            // Find other slack variables in the basis associated with different variables
            std::map<int, std::pair<int, double>> best_slack_per_var;
            // var -> {col, rhs}
            for (int col : slack_in_basis) {
                int slack_index = col - slack_offset;
                int var = *(actions + slack_index);
                if (var == candidate_var) {
                    continue;
                }
                for (int row = 0; row < numConstraints; ++row) {
                    if (std::fabs(tableau[row][col] - 1.0) < 1e-8) {
                        double rhs = tableau[row].back();
                        auto it = best_slack_per_var.find(var);
                        if (it == best_slack_per_var.end() || rhs < it->second.second) {
                            best_slack_per_var[var] = {col, rhs};
                        }
                    }
                }
            }
            std::vector<int> other_slack_cols;
            other_slack_cols.reserve(best_slack_per_var.size());
            for (const auto &[var, pair] : best_slack_per_var) {
                other_slack_cols.push_back(pair.first);
            }
            // Sum candidate values in rows where other slacks = 1
            double sum = 0.0;
            for (int row = 0; row < numConstraints; ++row) {
                for (int slack_col : other_slack_cols) {
                    double slack_value = tableau[row][slack_col];
                    if (std::fabs(slack_value - 1.0) < 1e-8) {
                        double candidate_value = tableau[row][candidate_col];
                        sum += candidate_value;
                    }
                }
            }
            if (sum > 1e-8) // warning negative, dirty values close to 0
            {
                pivotValue = -sum; // swap sign
                found_valid = true;
                break;
            }
        }
        if (!found_valid) {
            return -1;
        }
        return candidate_col;
    }

    void get_full_results(std::vector<double> &x_out, double &objective, bool use_original_variables) const {
        int total_variables = tableau[0].size() - 1; // excludes RHS
        x_out.resize(use_original_variables ? numVariables - 1 : total_variables);
        // Initialize all variables to 0
        for (double &i : x_out) {
            i = 0.0;
        }
        // Step 1: Extract basic variable values
        std::vector<double> x_full;
        x_full.resize(total_variables);
        for (int i = 0; i < total_variables; ++i) {
            x_full[i] = 0.0;
        }
        for (int i = 0; i < numConstraints; ++i) {
            int var_index = basis[i];
            if (var_index < total_variables) {
                x_full[var_index] = tableau[i].back();
            }
        }
        // Step 2: If requested, transform back to original variables: x_i = x'_i - W
        if (use_original_variables) {
            double w = x_full[numVariables - 1]; // shared W is last among x'_i
            for (int i = 0; i < numVariables - 1; ++i) {
                x_out[i] = x_full[i] - w;
            }
        } else {
            // Otherwise return raw tableau variable values
            for (int i = 0; i < total_variables; ++i) {
                x_out[i] = x_full[i];
            }
        }
        // Objective value
        objective = tableau[numConstraints].back();
    }

    void print_tableau() const {
        LGG_INFO("Tableau Matrix:");
        for (const auto &row : tableau) {
            for (double val : row) {
                LGG_INFO(std::setw(10), std::fixed, std::setprecision(4), val);
            }
        }
    }

    void print_basis() const {
        LGG_INFO("Basis Variables:");
        for (int i = 0; i < basis.size(); ++i) {
            LGG_INFO("Row ", i, " -> x_", basis[i]);
        }
    }

    void print_problem() const {
        LGG_INFO("Problem Summary:");
        LGG_INFO("Variables: ", numVariables);
        LGG_INFO("Constraints: ", numConstraints);
        LGG_INFO("Tableau size: ", tableau.size(), " x ", tableau[0].size());
    }

    void print_constraints_info() const {
        LGG_INFO("Constraints Info:");
        for (int i = 0; i < constraintType.size(); ++i) {
            LGG_INFO("Constraint ", i, ": ", constraintType[i]);
        }
    }

  private:
    std::vector<std::vector<double>> tableau;
    std::vector<int> basis;
    std::vector<std::string> constraintType;
    int numConstraints;
    int numVariables;
    double pivotValue = 0.0;
    int artificialVar = 0;
    int origVars;
};