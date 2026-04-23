#!/bin/bash

# General Benchmark Script for Game Solvers
# No dependencies or libraries required beyond standard shell tools.
# Supports flat game directories and type-based subdirectories.
# Usage:
#   bash benchmark.sh <games_dir> <solver_dir> [--time TIMEOUT_SECONDS] [-o OUTPUT_FILE] [--solvers NAME1,NAME2,...]
# If no solver names are provided, prompts the user to choose solvers from the selected directory.

set -eu
shopt -s nullglob

script_dir="$(cd "$(dirname "$0")" && pwd)"

declare -A invalid_solvers
declare -A selected_solver_names
declare -A matched_solver_names
declare -a available_solver_names
declare -a available_solver_paths
declare -a solver_paths

selected_solver_count=0
solvers_dir=""
timeout_sec=300
output_file="$script_dir/results.json"

usage() {
  cat >&2 <<EOF
Usage: $0 <games_dir> <solver_dir> [--time TIMEOUT_SECONDS] [-o OUTPUT_FILE] [--solver NAME]... [--solvers NAME1,NAME2,...]

Options:
  --time SECONDS     Timeout per solver run (default: 300)
  -o, --output FILE  Output JSON file (default: $output_file)
  --solver NAME      Select a solver by name; may be repeated
  --solvers LIST     Select comma-separated solver names
  -h, --help         Show this help message

If no solver names are provided, the script lists the available solvers in the chosen
solver directory and asks which ones to run.
EOF
}

append_selected_solvers() {
  local value="$1"
  local solver_name trimmed_name

  IFS=',' read -r -a solver_names <<< "$value"
  for solver_name in "${solver_names[@]}"; do
    trimmed_name=$(printf "%s" "$solver_name" | xargs)
    [ -n "$trimmed_name" ] || continue
    if [ -z "${selected_solver_names[$trimmed_name]+x}" ]; then
      selected_solver_names["$trimmed_name"]=1
      selected_solver_count=$((selected_solver_count + 1))
    fi
  done
}

solver_is_selected() {
  local solver_name="$1"

  if [ $selected_solver_count -eq 0 ]; then
    return 0
  fi

  [ -n "${selected_solver_names[$solver_name]+x}" ]
}

collect_available_solvers() {
  local solver_path solver_name

  available_solver_names=()
  available_solver_paths=()

  if [ ! -d "$solvers_dir" ]; then
    echo "Solver directory does not exist: $solvers_dir" >&2
    exit 1
  fi

  for solver_path in "$solvers_dir"/*; do
    [ -x "$solver_path" ] || continue
    solver_name=$(basename "$solver_path")
    available_solver_names+=("$solver_name")
    available_solver_paths+=("$solver_path")
  done

  if [ ${#available_solver_paths[@]} -eq 0 ]; then
    echo "No executable solvers found in $solvers_dir" >&2
    exit 1
  fi
}

prompt_for_solvers() {
  local choice choices index solver_name prompt_choice

  if [ $selected_solver_count -gt 0 ]; then
    return
  fi

  echo "Available solvers in $solvers_dir:"
  for index in "${!available_solver_names[@]}"; do
    printf "  [%d] %s\n" "$((index + 1))" "${available_solver_names[$index]}"
  done

  while true; do
    printf "Select solver numbers or names (comma-separated, or 'all'): "
    IFS= read -r choice
    prompt_choice=$(printf "%s" "$choice" | xargs)

    if [ -z "$prompt_choice" ] || [ "$prompt_choice" = "all" ]; then
      for solver_name in "${available_solver_names[@]}"; do
        append_selected_solvers "$solver_name"
      done
      break
    fi

    IFS=',' read -r -a choices <<< "$prompt_choice"
    for choice in "${choices[@]}"; do
      prompt_choice=$(printf "%s" "$choice" | xargs)
      [ -n "$prompt_choice" ] || continue

      if [[ "$prompt_choice" =~ ^[0-9]+$ ]]; then
        index=$((prompt_choice - 1))
        if [ $index -ge 0 ] && [ $index -lt ${#available_solver_names[@]} ]; then
          append_selected_solvers "${available_solver_names[$index]}"
          continue
        fi
      fi

      for solver_name in "${available_solver_names[@]}"; do
        if [ "$solver_name" = "$prompt_choice" ]; then
          append_selected_solvers "$solver_name"
          prompt_choice=""
          break
        fi
      done

      if [ -n "$prompt_choice" ]; then
        echo "Unknown solver selection: $prompt_choice" >&2
      fi
    done

    if [ $selected_solver_count -gt 0 ]; then
      break
    fi

    echo "Please select at least one valid solver." >&2
  done
}

positional_args=()
while [ $# -gt 0 ]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    --time)
      [ $# -ge 2 ] || {
        echo "Missing value for --time" >&2
        usage
        exit 1
      }
      timeout_sec="$2"
      shift 2
      ;;
    -o|--output)
      [ $# -ge 2 ] || {
        echo "Missing value for --output" >&2
        usage
        exit 1
      }
      output_file="$2"
      shift 2
      ;;
    --solver)
      [ $# -ge 2 ] || {
        echo "Missing value for --solver" >&2
        usage
        exit 1
      }
      append_selected_solvers "$2"
      shift 2
      ;;
    --solvers)
      [ $# -ge 2 ] || {
        echo "Missing value for --solvers" >&2
        usage
        exit 1
      }
      append_selected_solvers "$2"
      shift 2
      ;;
    --)
      shift
      positional_args+=("$@")
      break
      ;;
    *)
      positional_args+=("$1")
      shift
      ;;
  esac
done

set -- "${positional_args[@]}"

if [ $# -ne 2 ]; then
  usage
  exit 1
fi

games_dir="$1"
solvers_dir="$2"

results="[\n"
first=1

# Count vertices and edges
count_dot() {
  local file="$1"
  local edges=0 vertices=0
  while IFS= read -r line; do
    case "$line" in
      *"digraph"*) continue ;;
      *"->"*) edges=$((edges+1)); continue ;;
    esac
    case "$line" in
      *"["*"]"*) vertices=$((vertices+1)) ;;
    esac
  done < "$file"
  echo "$vertices $edges"
}

# Parse solver output time
parse_time() {
  local output="$1"
  local fallback="$2"

  if printf "%s\n" "$output" | grep -q "Time to solve:"; then
    local t unit
    t=$(printf "%s\n" "$output" | grep "Time to solve:" | awk '{print $4}')
    unit=$(printf "%s\n" "$output" | grep "Time to solve:" | awk '{print $5}')
    if [ "$unit" = "ms" ]; then
      awk "BEGIN { printf \"%.9f\\n\", $t/1000 }" | sed 's/^\./0./'
    else
      printf "%.9f\n" "$t" | sed 's/^\./0./'
    fi
  elif printf "%s\n" "$output" | grep -q "^Time:"; then
    val=$(printf "%s\n" "$output" | grep "^Time:" | awk '{print $2}' | sed 's/s//')
    printf "%.9f\n" "$val" | sed 's/^\./0./'
  else
    printf "%.9f\n" "$fallback" | sed 's/^\./0./'
  fi
}

check_solver() {
  local solver_path="$1"
  local output rc first_line solver

  solver=$(basename "$solver_path")
  set +e
  output=$(timeout 5 "$solver_path" --help 2>&1)
  rc=$?
  set -e

  if [ $rc -eq 0 ]; then
    return 0
  fi

  if [ $rc -eq 1 ] && printf "%s\n" "$output" | grep -Eqi 'usage|options|help'; then
    return 0
  fi

  if [ $rc -eq 124 ]; then
    return 0
  fi

  first_line=$(printf "%s\n" "$output" | head -n 1)
  if [ -z "$first_line" ]; then
    first_line="unknown startup failure"
  fi

  invalid_solvers["$solver_path"]="$first_line"
  echo "Skipping $solver: $first_line" >&2
  return 1
}

collect_solvers() {
  local solver_path solver_name index

  solver_paths=()
  for index in "${!available_solver_paths[@]}"; do
    solver_path="${available_solver_paths[$index]}"
    solver_name="${available_solver_names[$index]}"
    solver_is_selected "$solver_name" || continue
    matched_solver_names["$solver_name"]=1
    check_solver "$solver_path" || continue
    solver_paths+=("$solver_path")
  done

  if [ $selected_solver_count -gt 0 ]; then
    for solver_name in "${!selected_solver_names[@]}"; do
      if [ -z "${matched_solver_names[$solver_name]+x}" ]; then
        echo "Requested solver not found: $solver_name" >&2
      fi
    done
  fi

  if [ ${#solver_paths[@]} -eq 0 ]; then
    echo "No runnable solvers found in $solvers_dir" >&2
    exit 1
  fi
}

# Solver execution function
run_solver() {
  local solver_path="$1" game="$2" type="$3"
  local solver game_name vertices edges start end elapsed output status time_val rc

  solver=$(basename "$solver_path")
  game_name=$(basename "$game" .dot)

  set -- $(count_dot "$game")
  vertices=$1
  edges=$2

  start=$(date +%s.%N)
  if output=$(timeout "$timeout_sec" "$solver_path" --time-only "$game" 2>&1); then
    end=$(date +%s.%N)
    elapsed=$(echo "$end - $start" | bc | sed 's/^\./0./')
    status="success"
    time_val=$(parse_time "$output" "$elapsed")
  else
    rc=$?
    if [ $rc -eq 124 ]; then
      status="timeout"
      time_val="$timeout_sec"
    else
      status="failed"
      time_val="null"
    fi
  fi

  [ $first -eq 0 ] && results+=",\n"
  first=0

  json="  {\"solver\":\"$solver\",\"game\":\"$game_name\""
  [ -n "$type" ] && json+=",\"type\":\"$type\""
  json+=",\"status\":\"$status\",\"time\":$time_val,\"vertices\":$vertices,\"edges\":$edges,\"timestamp\":$(date +%s)}"
  results+="$json"

  echo "Processed $game_name with $solver ($status)"
}

# Directory structure
collect_available_solvers
prompt_for_solvers
collect_solvers

files=("$games_dir"/*.dot)
if [ ${#files[@]} -gt 0 ]; then
  # Flat structure
  for solver_path in "${solver_paths[@]}"; do
    for game in "${files[@]}"; do
      [ -f "$game" ] || continue
      run_solver "$solver_path" "$game" ""
    done
  done
else
  # Subdir structure
  for subdir in "$games_dir"/*; do
    [ -d "$subdir" ] || continue
    type=$(basename "$subdir")
    subfiles=("$subdir"/*.dot)
    if [ ${#subfiles[@]} -eq 0 ]; then
      echo "No .dot files in $subdir"
      continue
    fi
    for solver_path in "${solver_paths[@]}"; do
      for game in "${subfiles[@]}"; do
        run_solver "$solver_path" "$game" "$type"
      done
    done
  done
fi

results+="\n]"
printf "%b\n" "$results" > "$output_file"
echo "Results written to $output_file"