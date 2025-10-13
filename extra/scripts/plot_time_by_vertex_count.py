#!/usr/bin/env python3
"""
Vertex Count Performance Visualization Script
Visualizes mean runtime by vertex count from benchmark.py JSON output

Usage:
    python3 plot_time_by_vertex_count.py <json_file> [--output-dir <dir>] [--title <title>]
    
Examples:
    # Visualize scalability by vertex count
    python3 plot_time_by_vertex_count.py consolidated_results.json
    
    # Custom output and title
    python3 plot_time_by_vertex_count.py results.json --output-dir plots --title "Solver Scalability Analysis"
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

def create_vertex_performance_plot(solver_data, output_dir, title=None):
    """Create a plot showing mean runtime by vertex count."""
    
    fig, ax = plt.subplots(figsize=(12, 8))
    colors = plt.cm.Set1(np.linspace(0, 1, len(solver_data)))
    
    # Group by solver and vertex count
    for i, (solver_name, solver_results) in enumerate(solver_data.items()):
        vertex_groups = defaultdict(list)
        
        # Group times by vertex count
        for result in solver_results:
            vertices = result['vertices']
            if vertices > 0:  # Only include games with vertex count info
                vertex_groups[vertices].append(result['time'])
        
        # Calculate mean times for each vertex count
        vertex_counts = sorted(vertex_groups.keys())
        mean_times = []
        
        for vertex_count in vertex_counts:
            times = vertex_groups[vertex_count]
            mean_times.append(np.mean(times))
        
        if vertex_counts:  # Only plot if we have vertex data
            # Plot mean with connecting lines (no error bars)
            ax.plot(vertex_counts, mean_times, 
                   marker='o', label=solver_name, linewidth=2,
                   markersize=6, alpha=0.8, color=colors[i])
    
    # Customize the vertex plot
    ax.set_xlabel('Number of Vertices', fontsize=12)
    ax.set_ylabel('Mean Runtime (seconds)', fontsize=12)
    
    if title:
        ax.set_title(title, fontsize=14, fontweight='bold')
    else:
        ax.set_title('Scalability: Mean Runtime by Vertex Count', fontsize=14, fontweight='bold')
    
    ax.legend(fontsize=11)
    ax.grid(True, alpha=0.3)
    
    # Use log scale if there's a wide range
    if len(solver_data) > 0:
        all_means = []
        for solver_results in solver_data.values():
            vertex_groups = defaultdict(list)
            for result in solver_results:
                if result['vertices'] > 0:
                    vertex_groups[result['vertices']].append(result['time'])
            for times in vertex_groups.values():
                all_means.append(np.mean(times))
        
        if all_means:
            time_range = max(all_means) / min(all_means) if min(all_means) > 0 else 1
            if time_range > 100:
                ax.set_yscale('log')
    
    plt.tight_layout()
    
    # Save the plot
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    output_file = Path(output_dir) / 'scalability_by_vertex_count.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Scalability plot saved to: {output_file}")
    
    plt.close()

def create_performance_plot(data, output_dir, title=None):
    """Create scalability plot from benchmark data."""
    
    # Extract results
    if 'results' in data:
        results = data['results']
    else:
        # Assume the data is already a list of results
        results = data
    
    # Group results by solver and game
    solver_data = defaultdict(list)
    
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
    
    if not solver_data:
        print("No successful results found in the data!")
        return
    
    # Create the plot
    create_vertex_performance_plot(solver_data, output_dir, title)
    
    # Print summary statistics
    print(f"\nOverall Performance Summary:")
    print(f"{'Solver':<25} {'Games Solved':<12} {'Avg Time (s)':<12} {'Min Time (s)':<12} {'Max Time (s)':<12}")
    print("-" * 75)
    
    for solver_name, solver_results in solver_data.items():
        times = [r['time'] for r in solver_results]
        avg_time = np.mean(times)
        min_time = min(times)
        max_time = max(times)
        games_solved = len(solver_results)
        
        print(f"{solver_name:<25} {games_solved:<12} {avg_time:<12.4f} {min_time:<12.4f} {max_time:<12.4f}")
    
    print(f"\nTotal solvers: {len(solver_data)}")

def main():
    parser = argparse.ArgumentParser(description='Visualize solver scalability by vertex count from benchmark JSON')
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
