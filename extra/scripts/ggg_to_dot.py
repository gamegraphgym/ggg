#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
import argparse
from typing import Dict, List, Tuple, Optional

# ---------------------------------------------------------------------------
# Configuration constants
# ---------------------------------------------------------------------------
ATTRIBUTE_COLORS: List[str] = [
    "blue", "red", "green", "brown", "purple",
    "orange", "cyan", "magenta", "goldenrod", "gray"
]

# Map vertex property -> node attribute name -> spec
# Spec can be either:
#  - dict of value -> attribute_value, with optional '*' default
#  - template string using placeholders {k},{v},{raw_k},{raw_v}

INTERPRET_VERTEX_NODEATTRS: Dict[str, Dict[str, object]] = {
    'player': {
        'shape': {'0': 'diamond', '1': 'square', '*': 'circle'},
        'label': ''
    }
}

"""
How to derive edge attributes from per-edge properties. Use the same structure as
INTERPRET_VERTEX_NODEATTRS: property -> edge-attribute -> spec (dict mapping or template).
To influence the edge label text, use the special attribute name 'label' to contribute
fragments to the label string.
"""
INTERPRET_EDGE_ATTRS: Dict[str, Dict[str, object]] = {
    # show only the value of existing 'label' property
    'label': {'label': '{v}'},
}

"""
How to derive graph-level attributes from graph properties. Same structure as above.
Attribute values are quoted (no HTML-like graph labels are used here).
"""
INTERPRET_GRAPH_ATTRS_MAP: Dict[str, Dict[str, object]] = {
    # 'label': {'label': 'Graph: {v}'},
}


# Regex patterns (simple, line-based)
VERTEX_LINE_RE = re.compile(r"^\s*([A-Za-z0-9_]+)\s*\[(.*?)\];\s*$")
EDGE_LINE_RE = re.compile(
    r"^\s*([A-Za-z0-9_]+)\s*->\s*([A-Za-z0-9_]+)\s*\[(.*?)\];\s*$")

# Graph attribute line: graph [key=value, ...];
GRAPH_ATTR_LINE_RE = re.compile(r"^\s*graph\s*\[(.*?)\];\s*$", re.IGNORECASE)

# Attribute token regex: split on commas not inside quotes (basic). For simplicity,
# we split by comma and strip; values cannot contain commas unquoted under current usage.


def parse_attribute_list(attr_text: str) -> List[Tuple[str, str]]:
    if not attr_text.strip():
        return []
    parts = [p.strip() for p in attr_text.split(',') if p.strip()]
    kv_pairs: List[Tuple[str, str]] = []
    for part in parts:
        if '=' in part:
            k, v = part.split('=', 1)
            kv_pairs.append((k.strip(), v.strip()))
        else:
            # attribute without explicit value (treat as flag with value "true")
            kv_pairs.append((part, 'true'))
    return kv_pairs


# Maintain a global mapping from attribute name to color index for stable coloring.
_attribute_color_map: Dict[str, str] = {}


def color_for_attribute(attr_name: str) -> str:
    if attr_name not in _attribute_color_map:
        idx = len(_attribute_color_map)
        color = ATTRIBUTE_COLORS[idx % len(ATTRIBUTE_COLORS)]
        _attribute_color_map[attr_name] = color
    return _attribute_color_map[attr_name]


def _escape_html_text(s: str) -> str:
    """Escape characters significant to Graphviz HTML-like labels."""
    return (
        s.replace("&", "&amp;")
         .replace("<", "&lt;")
         .replace(">", "&gt;")
         .replace('"', '&quot;')
    )


def _escape_for_quoted(s: str) -> str:
    """Escape for inclusion inside a double-quoted DOT string."""
    return s.replace('\\', r'\\').replace('"', r'\"')


def _format_with_template(tmpl: str, k: str, v: str, *, context: str) -> str:
    """Apply a template with placeholders. Context: 'vertex'|'edge'|'graph'."""
    if context == 'vertex':
        ek = _escape_html_text(k)
        ev = _escape_html_text(v)
    elif context == 'edge':
        ek = _escape_for_quoted(k)
        ev = _escape_for_quoted(v)
    else:  # graph
        ek = k
        ev = v
    return tmpl.replace('{k}', ek).replace('{v}', ev).replace('{raw_k}', k).replace('{raw_v}', v)


def _vertex_label_fragment(k: str, v: str) -> str:
    """Compute one fragment for the vertex label using INTERPRET_VERTEX_NODEATTRS's 'label' mapping.
    Falls back to 'k=v'. Returns HTML-safe text (no tags) suitable to place inside FONT.
    """
    raw_v = v.strip('"')
    spec = INTERPRET_VERTEX_NODEATTRS.get(k)
    if spec and 'label' in spec:
        templ = spec['label']
        frag = None
        if isinstance(templ, dict):
            frag = templ.get(raw_v, templ.get('*'))
        else:
            frag = _format_with_template(
                str(templ), k, raw_v, context='vertex')
        if frag is not None:
            return _escape_html_text(frag)
    return f"{_escape_html_text(k)}={_escape_html_text(raw_v)}"


def _edge_label_fragment(k: str, v: str) -> str:
    """Compute one fragment for the edge HTML-like label using INTERPRET_EDGE_ATTRS 'label' mapping.
    Falls back to 'k=v'. Returns HTML-safe text (no tags).
    """
    raw_v = v.strip('"')
    spec = INTERPRET_EDGE_ATTRS.get(k)
    if spec and 'label' in spec:
        templ = spec['label']
        frag = None
        if isinstance(templ, dict):
            frag = templ.get(raw_v, templ.get('*'))
        else:
            frag = _format_with_template(str(templ), k, raw_v, context='edge')
        if frag is not None:
            return _escape_html_text(frag)
    return f"{_escape_html_text(k)}={_escape_html_text(raw_v)}"


def build_vertex_label(kv_pairs: List[Tuple[str, str]]) -> str:
    """Build HTML-like composite label from attribute key/value pairs."""
    if not kv_pairs:
        return '<>'
    fragments = []
    for k, v in kv_pairs:
        disp = _vertex_label_fragment(k, v)
        # Skip empty/suppressed fragments so they do not produce empty FONT tags
        if not disp:
            continue
        color = color_for_attribute(k)
        fragments.append(f"<FONT COLOR=\"{color}\">{disp}</FONT>")
    inner = ', '.join(fragments)
    # HTML-like labels are enclosed in angle brackets and are NOT quoted
    return f"<{inner}>"


def _compute_vertex_node_attrs(kv_pairs: List[Tuple[str, str]]) -> Dict[str, str]:
    attrs: Dict[str, str] = {}
    for k, v in kv_pairs:
        raw_v = v.strip('"')
        if k not in INTERPRET_VERTEX_NODEATTRS:
            continue
        spec = INTERPRET_VERTEX_NODEATTRS[k]
        for attr_name, attr_spec in spec.items():
            if isinstance(attr_spec, dict):
                val = attr_spec.get(raw_v, attr_spec.get('*'))
                if val is None:
                    continue
                attrs[attr_name] = str(val)
            else:
                # template string
                formatted = _format_with_template(
                    str(attr_spec), k, raw_v, context='graph')
                attrs[attr_name] = formatted
    return attrs


def transform_vertex_line(line: str) -> str:
    m = VERTEX_LINE_RE.match(line)
    if not m:
        return line
    node, attr_text = m.groups()
    kv_pairs = parse_attribute_list(attr_text)
    if not kv_pairs:
        return line  # nothing to rewrite
    node_attrs = _compute_vertex_node_attrs(kv_pairs)

    label = build_vertex_label(kv_pairs)

    # Start with computed label; allow node_attrs['label'] to override it.
    attrs_out = [f"label={label}"]
    for attr_name, attr_val in node_attrs.items():
        if attr_name.lower() == 'label':
            val_strip = attr_val.strip()
            # empty mapping means "suppress/keep computed label" -> skip
            if val_strip == "":
                continue
            # override computed label with provided HTML-like form
            if val_strip.startswith('<') and val_strip.endswith('>'):
                attrs_out[0] = f"label={val_strip}"
            else:
                attrs_out[0] = f"label=\"{_escape_for_quoted(attr_val)}\""
            continue
        attrs_out.append(f"{attr_name}=\"{_escape_for_quoted(attr_val)}\"")
    # Optionally we could add 'style=filled' etc.; keep minimal for now.

    return f"{node} [{', '.join(attrs_out)}];"


def transform_edge_line(line: str) -> str:
    m = EDGE_LINE_RE.match(line)
    if not m:
        return line
    src, dst, attr_text = m.groups()
    kv_pairs = parse_attribute_list(attr_text)
    if not kv_pairs:
        return line
    label_parts: List[str] = []
    extra_attrs: Dict[str, str] = {}
    for k, v in kv_pairs:
        # label fragments (HTML-like)
        label_parts.append(_edge_label_fragment(k, v))
        # other edge attributes via mapping
        spec = INTERPRET_EDGE_ATTRS.get(k)
        if spec:
            for attr_name, templ in spec.items():
                if attr_name == 'label':
                    continue
                raw_v = v.strip('"')
                if isinstance(templ, dict):
                    aval = templ.get(raw_v, templ.get('*'))
                    if aval is None:
                        continue
                    extra_attrs[attr_name] = str(aval)
                else:
                    aval = _format_with_template(
                        str(templ), k, raw_v, context='edge')
                    extra_attrs[attr_name] = aval
    # Build HTML-like label with per-attribute colors
    colored = []
    for (k, _), frag in zip(kv_pairs, label_parts):
        colored.append(
            f"<FONT COLOR=\"{color_for_attribute(k)}\">{frag}</FONT>")
    inner = ', '.join(colored)
    attrs_out = [f"label=<{inner}>"]
    for an, av in extra_attrs.items():
        attrs_out.append(f"{an}=\"{_escape_for_quoted(av)}\"")
    return f"{src}->{dst}  [{', '.join(attrs_out)}];"


def transform_graph_line(line: str) -> str:
    m = GRAPH_ATTR_LINE_RE.match(line)
    if not m:
        return line
    attr_text = m.group(1)
    kv_pairs = parse_attribute_list(attr_text)
    if not kv_pairs:
        return line
    out_parts: List[str] = []
    # Build graph attributes and label from mapping (property -> graph-attr -> templ)
    computed: Dict[str, str] = {}
    label_frags: List[str] = []
    for k, v in kv_pairs:
        raw_v = v.strip('"')
        spec = INTERPRET_GRAPH_ATTRS_MAP.get(k)
        # contribute to label fragments (HTML-like) either via mapping or default
        # Use mapping 'label' under the spec if present; else default to k=v
        if spec and 'label' in spec:
            templ = spec['label']
            if isinstance(templ, dict):
                frag = templ.get(raw_v, templ.get('*'))
            else:
                frag = _format_with_template(
                    str(templ), k, raw_v, context='graph')
            if frag is not None:
                label_frags.append((k, _escape_html_text(str(frag))))
        else:
            label_frags.append(
                (k, f"{_escape_html_text(k)}={_escape_html_text(raw_v)}"))
        # other mapped graph attributes
        if spec:
            for attr_name, templ in spec.items():
                if attr_name == 'label':
                    continue
                if isinstance(templ, dict):
                    aval = templ.get(raw_v, templ.get('*'))
                    if aval is None:
                        continue
                    computed[attr_name] = str(aval)
                else:
                    aval = _format_with_template(
                        str(templ), k, raw_v, context='graph')
                    computed[attr_name] = aval
    # Remove any existing 'label=' from original and set colored HTML-like label
    colored = [
        f"<FONT COLOR=\"{color_for_attribute(k)}\">{frag}</FONT>" for k, frag in label_frags]
    out_parts.append(f"label=<{', '.join(colored)}>")
    # Append remaining original attrs (except existing label) and mapped overrides
    for k, v in kv_pairs:
        if k.lower() == 'label':
            continue
        if k not in computed:
            out_parts.append(f"{k}={v}")
    for an, av in computed.items():
        out_parts.append(f"{an}=\"{_escape_for_quoted(av)}\"")
    return f"graph [{', '.join(out_parts)}];"


def convert(stream_in, stream_out) -> None:
    for raw_line in stream_in:
        line = raw_line.rstrip('\n')
        # Attempt vertex/edge transform; else pass through
        if GRAPH_ATTR_LINE_RE.match(line):
            stream_out.write(transform_graph_line(line) + '\n')
        elif VERTEX_LINE_RE.match(line):
            stream_out.write(transform_vertex_line(line) + '\n')
        elif EDGE_LINE_RE.match(line):
            stream_out.write(transform_edge_line(line) + '\n')
        else:
            stream_out.write(raw_line)


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Convert simplified GGG DOT into enhanced DOT with HTML labels.")
    parser.add_argument('input', nargs='?', default='-',
                        help="Input DOT file (default: stdin)")
    args = parser.parse_args(argv[1:])

    if args.input != '-' and args.input != '':
        with open(args.input, 'r') as f:
            convert(f, sys.stdout)
    else:
        convert(sys.stdin, sys.stdout)
    return 0


if __name__ == '__main__':  # pragma: no cover
    sys.exit(main(sys.argv))
