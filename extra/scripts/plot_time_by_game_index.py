#!/usr/bin/env python3
"""
Game Index Performance Visualization Script
Visualizes individual game performance from benchmark.py JSON output

Usage:
    python3 plot_time_by_game_index.py <json_file> [--output-dir <dir>] [--title <title>]
    
Examples:
    # Visualize individual game performance
    python3 plot_time_by_game_index.py consolidated_results.json
    
    # Custom output and title
    python3 plot_time_by_game_index.py results.json --output-dir plots --title "Parity Game Solvers"
"""

import json
import matplotlib.pyplot as plt
import argparse
import numpy as np
from pathlib import Path
from collections import defaultdict

def load_benchmark_data(json_file):
    """Load benchmark data from JSON file."""
    with open(json_file, 'r') as f:
        return json.load(f)

def create_individual_performance_plot(solver_data, sorted_games, output_dir, title=None):
    """Create individual game performance plot."""
    
    fig, ax = plt.subplots(figsize=(14, 8))
    colors = plt.cm.Set1(np.linspace(0, 1, len(solver_data)))
    
    for i, (solver_name, solver_results) in enumerate(solver_data.items()):
        # Create a mapping of game to time for this solver
        game_to_time = {r['game']: r['time'] for r in solver_results}
        
        # Get times for all games (None if game not solved by this solver)
        x_positions = []
        y_times = []
        
        for j, game in enumerate(sorted_games):
            if game in game_to_time:
                x_positions.append(j)
                y_times.append(game_to_time[game])
        
        # Plot the line for this solver
        ax.plot(x_positions, y_times, 
                marker='o', label=solver_name, linewidth=2, 
                markersize=4, alpha=0.8, color=colors[i])
    
    # Customize the plot
    ax.set_xlabel('Game Index', fontsize=12)
    ax.set_ylabel('Runtime (seconds)', fontsize=12)
    
    if title:
        ax.set_title(title, fontsize=14, fontweight='bold')
    else:
        ax.set_title('Individual Game Performance', fontsize=14, fontweight='bold')
    
    ax.legend(fontsize=11)
    ax.grid(True, alpha=0.3)
    
    # Use log scale if there's a wide range of values
    all_times = [r['time'] for solver_results in solver_data.values() 
                 for r in solver_results]
    if len(all_times) > 0:
        time_range = max(all_times) / min(all_times) if min(all_times) > 0 else 1
        if time_range > 100:  # Use log scale if range is > 100x
            ax.set_yscale('log')
    
    # Format x-axis
    ax.set_xticks(range(0, len(sorted_games), max(1, len(sorted_games) // 10)))
    
    plt.tight_layout()
    
    # Save the plot
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    output_file = Path(output_dir) / 'individual_game_performance.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Individual game performance plot saved to: {output_file}")
    
    plt.close()

def create_performance_plot(data, output_dir, title=None):
    """Create individual game performance plot from benchmark data."""
    
    # Extract results
    if 'results' in data:
        results = data['results']
    else:
        # Assume the data is already a list of results
        results = data
    
    # Group results by solver and game
    solver_data = defaultdict(list)
    game_names = set()
    
    # Collect successful results only
    for result in results:
        if result.get('status') == 'success' and 'time' in result:
            solver = result['solver']
            game = result['game']
            time_val = result['time']
            
            solver_data[solver].append({
                'game': game,
                'time': time_val,
                'vertices': result.get('vertices', 0),
                'edges': result.get('edges', 0)
            })
            game_names.add(game)
    
    if not solver_data:
        print("No successful results found in the data!")
        return
    
    # Sort games by name for consistent ordering
    sorted_games = sorted(game_names)
    
    # Create the plot
    create_individual_performance_plot(solver_data, sorted_games, output_dir, title)
    
    # Print summary statistics
    print(f"\nPerformance Summary:")
    print(f"{'Solver':<25} {'Games Solved':<12} {'Avg Time (s)':<12} {'Min Time (s)':<12} {'Max Time (s)':<12}")
    print("-" * 75)
    
    for solver_name, solver_results in solver_data.items():
        times = [r['time'] for r in solver_results]
        avg_time = np.mean(times)
        min_time = min(times)
        max_time = max(times)
        games_solved = len(solver_results)
        
        print(f"{solver_name:<25} {games_solved:<12} {avg_time:<12.4f} {min_time:<12.4f} {max_time:<12.4f}")
    
    print(f"\nTotal games tested: {len(sorted_games)}")
    print(f"Total solvers: {len(solver_data)}")

def main():
    parser = argparse.ArgumentParser(description='Visualize individual game performance from benchmark JSON')
    parser.add_argument('json_file', help='JSON file with benchmark results (from benchmark.py)')
    parser.add_argument('--output-dir', '-o', default='.', 
                       help='Directory to save plots (default: current directory)')
    parser.add_argument('--title', '-t', 
                       help='Custom title for the plot')
    
    args = parser.parse_args()
    
    # Load data
    try:
        data = load_benchmark_data(args.json_file)
    except FileNotFoundError:
        print(f"Error: File '{args.json_file}' not found.")
        return 1
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in '{args.json_file}': {e}")
        return 1
    
    # Generate visualization
    create_performance_plot(data, args.output_dir, args.title)
    
    print(f"\n✅ Visualization complete! Output saved to: {Path(args.output_dir).absolute()}")
    return 0

if __name__ == '__main__':
    exit(main())
