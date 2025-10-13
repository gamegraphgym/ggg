#!/bin/bash

# General Benchmark Script for Game Solvers
# no dependancies or libraries required
# Flat structure: all games in a single directory
# SubDir structure: games in type-subdirectories
# Usage: bash benchmark.sh <games_dir> <solver_dir> [timeout] [output_file]

set -eu
shopt -s nullglob

if [ $# -lt 2 ]; then
  echo "Usage: $0 <games_dir> <solvers_dir> [timeout_seconds] [output_file]" >&2
  exit 1
fi

games_dir="$1"
solvers_dir="$2"
timeout_sec="${3:-300}"

script_dir="$(cd "$(dirname "$0")" && pwd)"
if [ $# -ge 4 ]; then
  output_file="$4"
else
  output_file="$script_dir/results.json"
fi

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
  if output=$(timeout "$timeout_sec" "$solver_path" --time-only < "$game" 2>&1); then
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
files=("$games_dir"/*.dot)
if [ ${#files[@]} -gt 0 ]; then
  # Flat structure
  for solver_path in "$solvers_dir"/*; do
    [ -x "$solver_path" ] || continue
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
    for solver_path in "$solvers_dir"/*; do
      [ -x "$solver_path" ] || continue
      for game in "${subfiles[@]}"; do
        run_solver "$solver_path" "$game" "$type"
      done
    done
  done
fi

results+="\n]"
printf "%b\n" "$results" > "$output_file"
echo "Results written to $output_file"
