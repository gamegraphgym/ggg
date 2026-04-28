#!/usr/bin/env python3
"""
hoa_to_dot.py

Convert parity games from HOA (Hanoi Omega-Automata) format to DOT format.

Quick usage:
  1) Single file -> stdout
      ./hoa_to_dot.py game.ehoa > game.dot

  2) Single file -> file
      ./hoa_to_dot.py game.ehoa -o game.dot

  3) Directory batch conversion
      ./hoa_to_dot.py ./hoa_games -o ./dot_games --recursive

Notes:
  - Default directory extensions: .ehoa,.hoa,.aut
  - Use --input-exts to override, e.g. --input-exts .ehoa,.txt
"""

from __future__ import annotations

import argparse
import dataclasses
from pathlib import Path
import re
import sys
from typing import Dict, Iterable, List, Optional, Sequence, Set, Tuple


@dataclasses.dataclass(frozen=True)
class Node:
    node_id: int
    priority: int
    player: int
    successors: Tuple[int, ...]
    name: str


def _parse_hoa_header(lines: Iterable[str]) -> Tuple[Optional[int], Dict[str, str], bool]:
    """Parse HOA header key-value pairs.
    
    Returns: (max_priority, header, is_generalized_buchi)
    """
    header: Dict[str, str] = {}
    max_priority: Optional[int] = None
    is_gen_buchi: bool = False
    
    for line in lines:
        line = line.strip()
        if line == "--BODY--":
            break
        if not line or line.startswith("#"):
            continue
        
        if ":" in line:
            key, _, value = line.partition(":")
            header[key.strip()] = value.strip()
        
        # Detect generalized-Buchi acceptance
        if "acc-name:" in line.lower() and "generalized-buchi" in line.lower():
            is_gen_buchi = True
        
        # Extract max priority from acceptance condition
        # e.g., "Acceptance: 3 Inf(2) | (Fin(1) & Inf(0))" -> max priority = 2
        if line.startswith("Acceptance:"):
            # Find all Inf(n) and Fin(n) terms
            for match in re.findall(r"(Inf|Fin)\((\d+)\)", line):
                pri = int(match[1])
                if max_priority is None or pri > max_priority:
                    max_priority = pri
    
    return max_priority, header, is_gen_buchi


def _parse_hoa_body(lines: Iterable[str], is_gen_buchi: bool = False) -> Dict[int, Node]:
    """Parse HOA body (states and transitions).
    
    Supports three acceptance modes:
    - State-based: "State: id "name" {priority}" (priority on state)
    - Transition-based: "State: id" followed by "[label] target {priority}" (priority on edge)
    - Generalized-Buchi: "State: id" followed by "[label] target {set}" (acceptance sets)
    
    For generalized-Buchi, converts to parity by using the set index as priority:
    - {0} -> priority 1, {1} -> priority 2, {0,1} -> priority 3, etc.
    """
    nodes: Dict[int, Node] = {}
    current_state: Optional[int] = None
    current_name: Optional[str] = None
    current_priority: Optional[int] = None
    current_successors: List[int] = []
    current_edge_priorities: List[int] = []  # For trans-acc format
    controllable_aps: Set[int] = set()
    trans_acc: bool = False  # Whether using transition-based acceptance
    
    # First pass: collect header info
    header_lines = list(lines)
    for line in header_lines:
        if line.startswith("controllable-AP:"):
            val = line.split(":", 1)[1].strip()
            if val.isdigit():
                controllable_aps.add(int(val))
            elif val:
                for ap in val.split(","):
                    ap = ap.strip()
                    if ap.isdigit():
                        controllable_aps.add(int(ap))
        if "trans-acc" in line:
            trans_acc = True
    
    # Helper to convert acceptance set to priority
    def set_to_priority(acc_set: str) -> int:
        """Convert acceptance set like '{0}' or '{0 1}' to parity priority."""
        if not acc_set:
            return 0
        # Parse set indices: "{0 1}" -> [0, 1] -> priority = max + 1
        indices = []
        for part in acc_set.strip('{}').split():
            part = part.strip()
            if part.isdigit():
                indices.append(int(part))
        if not indices:
            return 0
        # Use max index + 1 as priority (parity convention)
        return max(indices) + 1
    
    # Second pass: parse body
    in_body = False
    for raw_line in header_lines:
        line = raw_line.strip()
        
        if line == "--BODY--":
            in_body = True
            continue
        if line == "--END--":
            break
        if not in_body:
            continue
        
        # State declaration: "State: id" or "State: id "name" {priority}" or "State: id {priority}"
        state_match = re.match(r'^State:\s+(\d+)(?:\s+"([^"]+)")?(?:\s*\{(\d+)\})?', line)
        if state_match:
            # Save previous state if any
            if current_state is not None:
                if trans_acc and current_edge_priorities:
                    # Use max priority from edges for trans-acc format
                    max_edge_pri = max(current_edge_priorities) if current_edge_priorities else 0
                    player = _determine_player(max_edge_pri, controllable_aps)
                    nodes[current_state] = Node(
                        node_id=current_state,
                        priority=max_edge_pri,
                        player=player,
                        successors=tuple(current_successors),
                        name=current_name or f"v{current_state}",
                    )
                elif is_gen_buchi and current_edge_priorities:
                    # Use max priority from edges for generalized-Buchi
                    max_edge_pri = max(current_edge_priorities) if current_edge_priorities else 0
                    player = _determine_player(max_edge_pri, controllable_aps)
                    nodes[current_state] = Node(
                        node_id=current_state,
                        priority=max_edge_pri,
                        player=player,
                        successors=tuple(current_successors),
                        name=current_name or f"v{current_state}",
                    )
                elif current_priority is not None:
                    player = _determine_player(current_priority, controllable_aps)
                    nodes[current_state] = Node(
                        node_id=current_state,
                        priority=current_priority,
                        player=player,
                        successors=tuple(current_successors),
                        name=current_name or f"v{current_state}",
                    )
            
            current_state = int(state_match.group(1))
            current_name = state_match.group(2)
            current_priority = int(state_match.group(3)) if state_match.group(3) else None
            current_successors = []
            current_edge_priorities = []
            continue
        
        # Transition with acceptance set: "[label] target {set}" (generalized-Buchi format)
        # Handle whitespace: "[0 & !1]  0 {0 1}" or "[0&!1] 0 {0}"
        trans_match = re.match(r'^\[([^\]]+)\]\s+(\d+)\s*\{(\d+(?:\s+\d+)*)\}', line)
        if trans_match and current_state is not None:
            target = int(trans_match.group(2))
            acc_set = trans_match.group(3)
            priority = set_to_priority(acc_set)
            current_successors.append(target)
            current_edge_priorities.append(priority)
            continue
        
        # Transition with priority: "[label] target {priority}" (trans-acc format)
        trans_match = re.match(r'^\[([^\]]+)\]\s+(\d+)\s*\{(\d+)\}', line)
        if trans_match and current_state is not None:
            target = int(trans_match.group(2))
            priority = int(trans_match.group(3))
            current_successors.append(target)
            current_edge_priorities.append(priority)
            continue
        
        # Transition without priority or set: "[label] target" or "t target"
        # Handle optional whitespace between label and target
        trans_match = re.match(r'^\[([^\]]+)\]\s+(\d+)', line)
        if trans_match and current_state is not None:
            target = int(trans_match.group(2))
            current_successors.append(target)
            # No priority on this edge
            continue
        
        # Simple transition: "t target" or just "target"
        trans_match = re.match(r'^(?:t\s+)?(\d+)', line)
        if trans_match and current_state is not None:
            target = int(trans_match.group(1))
            current_successors.append(target)
            continue
    
    # Save last state
    if current_state is not None:
        if trans_acc and current_edge_priorities:
            max_edge_pri = max(current_edge_priorities) if current_edge_priorities else 0
            player = _determine_player(max_edge_pri, controllable_aps)
            nodes[current_state] = Node(
                node_id=current_state,
                priority=max_edge_pri,
                player=player,
                successors=tuple(current_successors),
                name=current_name or f"v{current_state}",
            )
        elif is_gen_buchi and current_edge_priorities:
            max_edge_pri = max(current_edge_priorities) if current_edge_priorities else 0
            player = _determine_player(max_edge_pri, controllable_aps)
            nodes[current_state] = Node(
                node_id=current_state,
                priority=max_edge_pri,
                player=player,
                successors=tuple(current_successors),
                name=current_name or f"v{current_state}",
            )
        elif current_priority is not None:
            player = _determine_player(current_priority, controllable_aps)
            nodes[current_state] = Node(
                node_id=current_state,
                priority=current_priority,
                player=player,
                successors=tuple(current_successors),
                name=current_name or f"v{current_state}",
            )
    
    return nodes


def _determine_player(priority: int, controllable_aps: Set[int]) -> int:
    """
    Determine player from priority and controllable-AP setting.
    
    In parity games:
    - Player 0 owns even priorities (default)
    - Player 1 owns odd priorities
    
    The controllable-AP field indicates which atomic propositions are
    controllable by Player 0. When not specified, we use the standard
    parity game convention: even priorities -> Player 0, odd -> Player 1.
    """
    return 0 if priority % 2 == 0 else 1


def parse_hoa(lines: Iterable[str]) -> Tuple[int, Dict[int, Node]]:
    """Parse complete HOA format parity game."""
    all_lines = list(lines)
    
    max_priority, header, is_gen_buchi = _parse_hoa_header(all_lines)
    nodes = _parse_hoa_body(all_lines, is_gen_buchi)
    
    if not nodes:
        raise ValueError("No states found in HOA body")
    
    # Determine max priority from nodes if not from header
    if max_priority is None:
        max_priority = max(n.priority for n in nodes.values())
    
    return max_priority, nodes


def _escape_dot_string(s: str) -> str:
    """Escape string for DOT format."""
    return s.replace("\\", r"\\").replace('"', r'\"')


def to_dot(nodes: Dict[int, Node]) -> str:
    """Convert parsed nodes to DOT format."""
    out: List[str] = []
    out.append("digraph ParityGame {")
    for node_id in sorted(nodes):
        n = nodes[node_id]
        out.append(
            f"    {n.node_id} [name=\"{_escape_dot_string(n.name)}\", player={n.player}, priority={n.priority}];"
        )

    out.append("")
    # Deduplicate edges: use set to remove duplicates
    seen_edges: Set[Tuple[int, int]] = set()
    for node_id in sorted(nodes):
        n = nodes[node_id]
        for succ in n.successors:
            edge = (n.node_id, succ)
            if edge not in seen_edges:
                seen_edges.add(edge)
                out.append(f"    {n.node_id} -> {succ};")
    out.append("}")
    out.append("")
    return "\n".join(out)


def _parse_dot_vertices_and_edges(dot_text: str) -> Tuple[Dict[int, Tuple[str, int, int]], List[Tuple[int, int]]]:
    """Parse DOT format to extract vertices and edges."""
    vertex_re = re.compile(
        r'^\s*(\d+)\s*\[\s*name="((?:[^"\\]|\\.)*)"\s*,\s*player=([01])\s*,\s*priority=(\d+)\s*\]\s*;\s*$'
    )
    edge_re = re.compile(r"^\s*(\d+)\s*->\s*(\d+)\s*;\s*$")
    vertices: Dict[int, Tuple[str, int, int]] = {}
    edges: List[Tuple[int, int]] = []

    for raw in dot_text.splitlines():
        line = raw.strip()
        if not line or line.startswith("digraph") or line == "{" or line == "}":
            continue
        vm = vertex_re.match(line)
        if vm:
            vid = int(vm.group(1))
            name = bytes(vm.group(2), "utf-8").decode("unicode_escape")
            player = int(vm.group(3))
            priority = int(vm.group(4))
            vertices[vid] = (name, player, priority)
            continue
        em = edge_re.match(line)
        if em:
            edges.append((int(em.group(1)), int(em.group(2))))
            continue

    return vertices, edges


def verify_semantics(nodes: Dict[int, Node], dot_text: str) -> None:
    """Verify that generated DOT preserves vertices, attributes, and edges."""
    vertices, edges = _parse_dot_vertices_and_edges(dot_text)

    if set(vertices.keys()) != set(nodes.keys()):
        raise AssertionError("Vertex id set mismatch after conversion")

    for node_id, node in nodes.items():
        name, player, priority = vertices[node_id]
        if name != node.name:
            raise AssertionError(f"Name mismatch for node {node_id}: {name!r} != {node.name!r}")
        if player != node.player:
            raise AssertionError(f"Player mismatch for node {node_id}: {player} != {node.player}")
        if priority != node.priority:
            raise AssertionError(
                f"Priority mismatch for node {node_id}: {priority} != {node.priority}"
            )

    expected_edges = sorted((src, dst) for src, n in nodes.items() for dst in n.successors)
    if sorted(edges) != expected_edges:
        raise AssertionError("Edge set mismatch after conversion")


def run_self_test() -> None:
    """Run internal converter self-test."""
    sample = """HOA: v1
States: 3
Start: 0
acc-name: parity max even 3
Acceptance: 3 Inf(2) | (Fin(1) & Inf(0))
AP: 2 "pl0_0" "pl1_0"
controllable-AP: 0
properties: deterministic complete colored
--BODY--
State: 0 "vertex0" {2}
[t] 1
State: 1 "vertex1" {2}
[t] 2
State: 2 "vertex2" {1}
[t] 2
--END--
"""
    _, nodes = parse_hoa(sample.splitlines())
    dot = to_dot(nodes)
    verify_semantics(nodes, dot)
    print(dot)


def _parse_extensions(exts_csv: str) -> set[str]:
    """Parse comma-separated file extensions."""
    exts = set()
    for raw in exts_csv.split(","):
        item = raw.strip().lower()
        if not item:
            continue
        if not item.startswith("."):
            item = f".{item}"
        exts.add(item)
    return exts


def _convert_one_file(input_path: Path, output_path: Path, verify: bool) -> None:
    """Convert a single HOA file to DOT."""
    src_lines = input_path.read_text(encoding="utf-8").splitlines()
    _, nodes = parse_hoa(src_lines)
    dot = to_dot(nodes)
    if verify:
        verify_semantics(nodes, dot)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(dot, encoding="utf-8")


def main(argv: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Convert parity games from HOA format to DOT format."
    )
    parser.add_argument(
        "input", nargs="?", default="-", help="Input file path or '-' for stdin"
    )
    parser.add_argument(
        "-o", "--output", default="-", help="Output file path or '-' for stdout"
    )
    parser.add_argument(
        "--verify",
        action="store_true",
        help="Verify that generated DOT preserves vertices, attributes, and edges",
    )
    parser.add_argument(
        "--recursive",
        action="store_true",
        help="When input is a directory, traverse it recursively",
    )
    parser.add_argument(
        "--input-exts",
        default=".ehoa,.hoa,.aut",
        help="Comma-separated file extensions to process in directory mode",
    )
    parser.add_argument(
        "--self-test", action="store_true", help="Run internal converter self-test and exit"
    )

    args = parser.parse_args(argv[1:])

    if args.self_test:
        run_self_test()
        print("self-test: ok", file=sys.stderr)
        return 0

    input_path = Path(args.input)
    output_path = Path(args.output)
    exts = _parse_extensions(args.input_exts)

    # Single file or stdin mode
    if input_path.suffix.lower() in exts or args.input == "-":
        if args.input == "-":
            src_lines = sys.stdin.read().splitlines()
        else:
            src_lines = input_path.read_text(encoding="utf-8").splitlines()
        
        _, nodes = parse_hoa(src_lines)
        dot = to_dot(nodes)
        
        if args.verify:
            verify_semantics(nodes, dot)
        
        if output_path == Path("-"):
            print(dot)
        else:
            output_path.parent.mkdir(parents=True, exist_ok=True)
            output_path.write_text(dot, encoding="utf-8")
            print(f"Converted: {input_path} -> {output_path}", file=sys.stderr)
        
        return 0

    # Directory mode
    if not input_path.is_dir():
        print(f"Error: {input_path} is not a file or directory", file=sys.stderr)
        return 1

    if output_path == Path("-"):
        print("Error: output directory required when input is a directory", file=sys.stderr)
        return 1

    output_path.mkdir(parents=True, exist_ok=True)

    pattern = "**/*" if args.recursive else "*"
    converted = 0
    errors = 0

    for src_file in input_path.glob(pattern):
        if src_file.is_file() and src_file.suffix.lower() in exts:
            # Preserve relative structure in output
            rel_path = src_file.relative_to(input_path)
            out_file = output_path / rel_path.with_suffix(".dot")
            
            try:
                _convert_one_file(src_file, out_file, args.verify)
                converted += 1
                print(f"Converted: {src_file} -> {out_file}", file=sys.stderr)
            except Exception as e:
                errors += 1
                print(f"Error converting {src_file}: {e}", file=sys.stderr)

    print(f"Conversion complete: {converted} files, {errors} errors", file=sys.stderr)
    return 1 if errors > 0 else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
