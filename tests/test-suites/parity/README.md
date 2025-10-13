# Parity Game Test Suite

This directory contains 224 parity games converted from the Oink verification benchmark suite. These games were originally from the Oink repository's test collection and have been converted to the GGG DOT format.

## Format

Each `.dot` file contains a parity game in Graphviz DOT format with the following vertex attributes:
- `name`: Human-readable vertex identifier (e.g., "v0", "v1", etc.)
- `player`: Owning player (0 or 1)
- `priority`: Priority value for the parity condition

## Files

- `vb001.dot` through `vb225.dot`: Verification benchmark games from the Oink test suite
- Note: `vb181.dot` is missing from the original Oink collection

## Usage

These games can be used for:
- Testing parity game solvers
- Benchmarking solver performance
- Validating solver correctness
- Educational purposes

## Source

Original games from: [Oink - A modern parity game solver](https://github.com/trolando/oink)
Converted using the Oink `dotty` tool and adapted to GGG format specifications.
