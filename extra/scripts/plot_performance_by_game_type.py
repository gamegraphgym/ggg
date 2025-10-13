#!/usr/bin/env python3
"""
Subdirectory Structure Game Type Visualization Script
Visualizes mean game performance from benchmark.py JSON output with subdirectory structure

Usage:
    python3 plot_time_by_game_index.py <json_file>
"""

import os
import sys
import json
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import LogFormatter
from matplotlib.patches import Rectangle

def create_performance_by_game_type(json_path):
    """Create visualization for performance by game type, saving plot in same directory as input JSON."""

    output_dir = os.path.dirname(os.path.abspath(json_path))

    # Load JSON array
    with open(json_path, 'r') as f:
        obj = json.load(f)

    if isinstance(obj, dict) and 'results' in obj:
        raw_results = obj['results']
    elif isinstance(obj, list):
        raw_results = obj
    elif isinstance(obj, dict):
        raw_results = [obj]
    else:
        raise ValueError("Unexpected JSON structure")

    # Normalize records
    data = []
    for r in raw_results:
        if not all(k in r for k in ('solver', 'game', 'type', 'status')):
            continue
        data.append({
            'solver': r['solver'],
            'game': r['game'],
            'game_type': r.get('type'),
            'status': r['status'],
            'time': r.get('time', None),
            'vertices': r.get('vertices'),
            'edges': r.get('edges'),
        })

    df = pd.DataFrame(data)
    print(f"Loaded {len(df)} results for analysis")

    if df.empty:
        print("No data found to plot.")
        return

    # Compute statistics
    stats_rows = []
    for (solver, game_type), group in df.groupby(['solver', 'game_type']):
        total = len(group)
        successes = (group['status'] == 'success')
        timeouts = (group['status'] == 'timeout')
        failures  = (group['status'] == 'failed')

        success_count = int(successes.sum())
        timeout_count = int(timeouts.sum())
        failed_count  = int(failures.sum())

        mean_time = group.loc[successes | timeouts, 'time'].mean()
        stats_rows.append({
            'solver': solver,
            'game_type': game_type if pd.notna(game_type) else 'Unknown',
            'mean_time': mean_time,
            'timeout_count': timeout_count,
            'success_count': success_count,
            'failed_count': failed_count,
            'total_count': total,
        })

    stats_df = pd.DataFrame(stats_rows)

    if stats_df.empty:
        print("No aggregated data to plot.")
        return

    # Pivot for plotting
    time_pivot = stats_df.pivot(index='game_type', columns='solver', values='mean_time')
    timeout_pivot = stats_df.pivot(index='game_type', columns='solver', values='timeout_count')
    time_pivot = time_pivot.sort_index().sort_index(axis=1)
    timeout_pivot = timeout_pivot.reindex_like(time_pivot)
    epsilon = 1e-6
    safe_time = time_pivot.copy()
    safe_time = safe_time.where(safe_time > 0, epsilon)
    safe_time = safe_time.fillna(epsilon)

    # Plot
    fig, ax = plt.subplots(figsize=(14, 9))
    safe_time.plot(kind='bar', ax=ax, width=0.8, alpha=0.85)
    ax.set_xlabel('Game type', fontsize=14)
    ax.set_ylabel('Mean time (seconds)', fontsize=14)
    ax.set_title('Solver performance by game type with timeout counts', fontsize=16)
    ax.set_yscale('log')
    ax.set_ylim(bottom=1e-2)
    ax.tick_params(axis='x', rotation=45, labelsize=11)
    ax.tick_params(axis='y', labelsize=11)
    ax.grid(True, axis='y', alpha=0.3)
    ax.legend(title='Solver', fontsize=11, title_fontsize=12)
    ax.yaxis.set_major_formatter(LogFormatter(base=10, labelOnlyBase=False))

    # Overlay timeout indicators
    n_game_types = len(time_pivot.index)
    n_solvers = len(time_pivot.columns)
    if n_game_types > 0 and n_solvers > 0:
        bar_width = 0.8 / max(n_solvers, 1)
        y_min, y_max = ax.get_ylim()
        overlay_h = max(epsilon * 10, y_min * 10 if y_min > 0 else epsilon * 10)

        for i, (game_type, row) in enumerate(timeout_pivot.iterrows()):
            for j, (solver, tcount) in enumerate(row.items()):
                if pd.notna(tcount) and tcount > 0:
                    x_pos = i + (j - n_solvers/2 + 0.5) * bar_width
                    ax.bar(x_pos, overlay_h, bar_width * 0.6,
                           color='darkred', alpha=0.9,
                           label='Timeout count' if i == 0 and j == 0 else "")
                    ax.text(x_pos, overlay_h * 2, f'{int(tcount)}',
                            ha='center', va='bottom', fontsize=9,
                            fontweight='bold', color='darkred')

    if (timeout_pivot.fillna(0).values > 0).any():
        handles, labels = ax.get_legend_handles_labels()
        red_bar = Rectangle((0, 0), 1, 1, facecolor='darkred', alpha=0.9)
        handles.append(red_bar)
        labels.append('Timeout count')
        ax.legend(handles, labels, title='Solver performance', fontsize=11, title_fontsize=12)

    plt.tight_layout()
    output_file = os.path.join(output_dir, 'game_type_performance.png')
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Performance by game type saved to: {output_file}")

    # Summary
    print("\nSummary statistics:")
    for solver in stats_df['solver'].dropna().unique():
        solver_stats = stats_df[stats_df['solver'] == solver]
        total_games = int(solver_stats['total_count'].sum())
        total_timeouts = int(solver_stats['timeout_count'].sum())
        rate = (total_timeouts / total_games * 100) if total_games > 0 else 0
        print(f"{solver}: {total_timeouts}/{total_games} timeouts ({rate:.1f}%)")

    plt.close()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python plot_performance_by_game_type.py /path/to/results.json")
        sys.exit(1)
    create_performance_by_game_type(sys.argv[1])
