#!/usr/bin/env python3
"""
pgsolver_to_ggg.py

Quick usage:
  1) Single file -> stdout
      ./pgsolver_to_ggg.py game.pg > game.dot

  2) Single file -> file
      ./pgsolver_to_ggg.py game.pg -o game.dot --verify

  3) Directory batch conversion
      ./pgsolver_to_ggg.py ./pg_games -o ./dot_games --recursive --verify

Notes:
  - Default directory extensions: .pg,.pgsolver,.gm
  - Use --input-exts to override, e.g. --input-exts .pg,.txt
"""

from __future__ import annotations

import argparse
import dataclasses
from pathlib import Path
import re
import sys
from typing import Dict, Iterable, List, Sequence, Tuple


_HEADER_RE = re.compile(r"^\s*parity\s+(\d+)\s*;\s*$", re.IGNORECASE)
_NODE_RE = re.compile(
    r"^\s*(\d+)\s+(\d+)\s+([01])\s+([^\s;]+)(?:\s+\"((?:[^\"\\]|\\.)*)\")?\s*;\s*$"
)


@dataclasses.dataclass(frozen=True)
class Node:
    node_id: int
    priority: int
    player: int
    successors: Tuple[int, ...]
    name: str


def _strip_comment(line: str) -> str:
    """Strip trailing # comments while respecting quoted strings."""
    in_quotes = False
    escaped = False
    for i, ch in enumerate(line):
        if escaped:
            escaped = False
            continue
        if ch == "\\":
            escaped = True
            continue
        if ch == '"':
            in_quotes = not in_quotes
            continue
        if ch == "#" and not in_quotes:
            return line[:i]
    return line


def _parse_successors(token: str, line_no: int) -> Tuple[int, ...]:
    parts = [p.strip() for p in token.split(",") if p.strip()]
    if not parts:
        raise ValueError(f"Line {line_no}: each node must have at least one successor")
    out: List[int] = []
    for p in parts:
        if not p.isdigit():
            raise ValueError(f"Line {line_no}: invalid successor id '{p}'")
        out.append(int(p))
    return tuple(out)


def _unescape_pgsolver_string(s: str) -> str:
    """Unescape only the pgsolver string escapes we intentionally support."""
    out: List[str] = []
    i = 0
    while i < len(s):
        ch = s[i]
        if ch == "\\" and i + 1 < len(s):
            nxt = s[i + 1]
            if nxt == "\\" or nxt == '"':
                out.append(nxt)
                i += 2
                continue
        out.append(ch)
        i += 1
    return "".join(out)


def parse_pgsolver(lines: Iterable[str]) -> Tuple[int, Dict[int, Node]]:
    max_index: int | None = None
    nodes: Dict[int, Node] = {}

    for line_no, raw in enumerate(lines, start=1):
        line = _strip_comment(raw).strip()
        if not line:
            continue

        header_match = _HEADER_RE.match(line)
        if header_match:
            if max_index is not None:
                raise ValueError(f"Line {line_no}: duplicate parity header")
            max_index = int(header_match.group(1))
            continue

        if max_index is None:
            raise ValueError(f"Line {line_no}: expected 'parity N;' header first")

        node_match = _NODE_RE.match(line)
        if not node_match:
            raise ValueError(f"Line {line_no}: malformed node line: {line}")

        node_id = int(node_match.group(1))
        if node_id in nodes:
            raise ValueError(f"Line {line_no}: duplicate node id {node_id}")

        priority = int(node_match.group(2))
        player = int(node_match.group(3))
        successors = _parse_successors(node_match.group(4), line_no)
        raw_name = node_match.group(5)
        name = _unescape_pgsolver_string(raw_name) if raw_name is not None else f"v{node_id}"

        nodes[node_id] = Node(
            node_id=node_id,
            priority=priority,
            player=player,
            successors=successors,
            name=name,
        )

    if max_index is None:
        raise ValueError("Missing header 'parity N;'")

    expected_nodes = max_index + 1
    if len(nodes) != expected_nodes:
        raise ValueError(
            f"Header says {expected_nodes} nodes (0..{max_index}) but got {len(nodes)}"
        )

    for i in range(expected_nodes):
        if i not in nodes:
            raise ValueError(f"Missing node id {i} (expected contiguous ids 0..{max_index})")

    for node in nodes.values():
        for succ in node.successors:
            if succ not in nodes:
                raise ValueError(
                    f"Node {node.node_id} has successor {succ}, which is not declared"
                )

    return max_index, nodes


def _escape_dot_string(s: str) -> str:
    return s.replace("\\", r"\\").replace('"', r'\"')


def to_ggg_dot(nodes: Dict[int, Node]) -> str:
    out: List[str] = []
    out.append("digraph ParityGame {")
    for node_id in sorted(nodes):
        n = nodes[node_id]
        out.append(
            f"    {n.node_id} [name=\"{_escape_dot_string(n.name)}\", player={n.player}, priority={n.priority}];"
        )

    out.append("")
    for node_id in sorted(nodes):
        n = nodes[node_id]
        for succ in n.successors:
            out.append(f"    {n.node_id} -> {succ};")
    out.append("}")
    out.append("")
    return "\n".join(out)


def _parse_ggg_dot_vertices_and_edges(dot_text: str) -> Tuple[Dict[int, Tuple[str, int, int]], List[Tuple[int, int]]]:
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
    vertices, edges = _parse_ggg_dot_vertices_and_edges(dot_text)

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
    sample = """\
parity 3;
0 1 0 1,2 "alpha";
1 2 1 0;
2 3 0 3 "node\\\"2";
3 0 1 0,2;
"""
    _, nodes = parse_pgsolver(sample.splitlines())
    dot = to_ggg_dot(nodes)
    verify_semantics(nodes, dot)


def _parse_extensions(exts_csv: str) -> set[str]:
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
    src_lines = input_path.read_text(encoding="utf-8").splitlines()
    _, nodes = parse_pgsolver(src_lines)
    dot = to_ggg_dot(nodes)
    if verify:
        verify_semantics(nodes, dot)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(dot, encoding="utf-8")


def main(argv: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Convert parity games from pgsolver format to GGG DOT format."
    )
    # input: '-' (stdin), single file, or directory
    parser.add_argument("input", nargs="?", default="-", help="Input file path or '-' for stdin")
    # output: '-' (stdout) for single-file/stdin mode, or directory in batch mode
    parser.add_argument("-o", "--output", default="-", help="Output file path or '-' for stdout")
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
        default=".pg,.pgsolver,.gm",
        help="Comma-separated file extensions to process in directory mode",
    )
    parser.add_argument("--self-test", action="store_true", help="Run internal converter self-test and exit")

    args = parser.parse_args(argv[1:])

    if args.self_test:
        run_self_test()
        print("self-test: ok", file=sys.stderr)
        return 0

    if args.input == "-":
        # stdin -> single DOT output
        src_lines = sys.stdin.read().splitlines()
        _, nodes = parse_pgsolver(src_lines)
        dot = to_ggg_dot(nodes)
        if args.verify:
            verify_semantics(nodes, dot)
        if args.output == "-":
            sys.stdout.write(dot)
        else:
            output_path = Path(args.output)
            output_path.parent.mkdir(parents=True, exist_ok=True)
            output_path.write_text(dot, encoding="utf-8")
        return 0

    input_path = Path(args.input)
    if not input_path.exists():
        raise FileNotFoundError(f"Input path does not exist: {input_path}")

    if input_path.is_file():
        # single file -> single DOT output
        src_lines = input_path.read_text(encoding="utf-8").splitlines()
        _, nodes = parse_pgsolver(src_lines)
        dot = to_ggg_dot(nodes)
        if args.verify:
            verify_semantics(nodes, dot)
        if args.output == "-":
            sys.stdout.write(dot)
        else:
            output_path = Path(args.output)
            output_path.parent.mkdir(parents=True, exist_ok=True)
            output_path.write_text(dot, encoding="utf-8")
        return 0

    # Directory mode: convert each matching input file to a .dot file.
    # The output keeps the same relative folder layout as the input directory.
    allowed_exts = _parse_extensions(args.input_exts)
    if args.output == "-":
        output_dir = input_path.parent / f"{input_path.name}_ggg"
    else:
        output_dir = Path(args.output)

    output_dir.mkdir(parents=True, exist_ok=True)
    it = input_path.rglob("*") if args.recursive else input_path.glob("*")
    candidates = [p for p in it if p.is_file() and p.suffix.lower() in allowed_exts]

    if not candidates:
        print("No matching input files found in directory mode.", file=sys.stderr)
        return 0

    converted = 0
    for src in sorted(candidates):
        rel = src.relative_to(input_path)
        dst = output_dir / rel.with_suffix(".dot")
        _convert_one_file(src, dst, args.verify)
        converted += 1

    print(f"Converted {converted} file(s) to {output_dir}", file=sys.stderr)

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
