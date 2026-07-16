#!/usr/bin/env python3
"""
extract_api.py

Extracts Godot's built-in Variant types, enums, and classes from the
GDExtension API definition (extension_api.json) into smaller, focused JSON
files that are easier to consume at runtime (e.g. for editor autocompletion).

This is a build-time step. The source of truth is Godot's own API definition
(extension_api.json), which is exactly what registers Vector3's members and
methods. Nothing here is hardcoded: every entry comes from that file.

Outputs (written next to this script, under ../src/luauscript/generated/):
  - variants.json : built-in Variant types (Vector3, Color, Transform3D, ...)
  - enums.json    : all enums (global + per-class)
  - classes.json  : Object-derived classes (Node, CharacterBody3D, ...)

Each member captures at least: name, type, default_value (where applicable).
"""

import json
import os
import sys

# Resolve paths relative to this script.
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)
SRC_API = os.path.join(REPO_ROOT, "extern", "godot-cpp", "gdextension", "extension_api.json")
OUT_DIR = os.path.join(REPO_ROOT, "src", "luauscript", "generated")


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def clean_arguments(args):
    """Normalize a method's argument list into {name, type, default_value}."""
    out = []
    for a in args or []:
        out.append({
            "name": a.get("name", ""),
            "type": a.get("type", ""),
            "default_value": a.get("default_value", ""),
        })
    return out


def extract_variant(cls):
    """Build a compact record for a built-in Variant type."""
    properties = []
    for m in cls.get("members", []):
        # Members without a return_type are plain properties (x, y, z, ...).
        if "return_type" not in m:
            properties.append({
                "name": m.get("name", ""),
                "type": m.get("type", ""),
            })

    methods = []
    for m in cls.get("methods", []):
        methods.append({
            "name": m.get("name", ""),
            "return_type": m.get("return_type", ""),
            "is_static": m.get("is_static", False),
            "is_const": m.get("is_const", False),
            "arguments": clean_arguments(m.get("arguments")),
        })

    constants = []
    for c in cls.get("constants", []):
        constants.append({
            "name": c.get("name", ""),
            "type": c.get("type", ""),
            "value": c.get("value", ""),
        })

    operators = []
    for o in cls.get("operators", []):
        operators.append({
            "name": o.get("name", ""),
            "right_type": o.get("right_type", ""),
            "return_type": o.get("return_type", ""),
        })

    enums = []
    for e in cls.get("enums", []):
        enums.append({
            "name": e.get("name", ""),
            "is_bitfield": e.get("is_bitfield", False),
            "values": [{"name": v.get("name", ""), "value": v.get("value", "")} for v in e.get("values", [])],
        })

    return {
        "name": cls.get("name", ""),
        "indexing_return_type": cls.get("indexing_return_type", ""),
        "properties": properties,
        "methods": methods,
        "constants": constants,
        "operators": operators,
        "enums": enums,
    }


def extract_class(cls):
    """Build a compact record for an Object-derived class."""
    properties = []
    for p in cls.get("properties", []):
        properties.append({
            "name": p.get("name", ""),
            "type": p.get("type", ""),
            "default_value": p.get("default_value", ""),
        })

    methods = []
    for m in cls.get("methods", []):
        methods.append({
            "name": m.get("name", ""),
            "return_type": m.get("return_type", ""),
            "is_static": m.get("is_static", False),
            "is_const": m.get("is_const", False),
            "arguments": clean_arguments(m.get("arguments")),
        })

    constants = []
    for c in cls.get("constants", []):
        constants.append({
            "name": c.get("name", ""),
            "type": c.get("type", ""),
            "value": c.get("value", ""),
        })

    enums = []
    for e in cls.get("enums", []):
        enums.append({
            "name": e.get("name", ""),
            "is_bitfield": e.get("is_bitfield", False),
            "values": [{"name": v.get("name", ""), "value": v.get("value", "")} for v in e.get("values", [])],
        })

    return {
        "name": cls.get("name", ""),
        "inherits": cls.get("inherits", ""),
        "properties": properties,
        "methods": methods,
        "constants": constants,
        "enums": enums,
    }


def extract_enum(enum, owner):
    return {
        "name": enum.get("name", ""),
        "owner": owner,
        "is_bitfield": enum.get("is_bitfield", False),
        "values": [{"name": v.get("name", ""), "value": v.get("value", "")} for v in enum.get("values", [])],
    }


def main():
    if not os.path.isfile(SRC_API):
        print(f"[extract_api] ERROR: source API not found: {SRC_API}", file=sys.stderr)
        sys.exit(1)

    print(f"[extract_api] Reading {SRC_API}")
    with open(SRC_API, "r", encoding="utf-8") as f:
        api = json.load(f)

    ensure_dir(OUT_DIR)

    classes = api.get("classes", [])

    # Built-in Variant types live under `builtin_classes` (Vector3, Color,
    # Transform3D, ...). Object-derived classes live under `classes`.
    builtin_classes = api.get("builtin_classes", [])

    variants = []
    for cls in builtin_classes:
        variants.append(extract_variant(cls))

    class_records = []
    for cls in classes:
        class_records.append(extract_class(cls))

    # Enums: global ones + per-builtin-class + per-object-class ones.
    enums = []
    for e in api.get("global_enums", []):
        enums.append(extract_enum(e, "global"))
    for cls in builtin_classes:
        owner = cls.get("name", "")
        for e in cls.get("enums", []):
            enums.append(extract_enum(e, owner))
    for cls in classes:
        owner = cls.get("name", "")
        for e in cls.get("enums", []):
            enums.append(extract_enum(e, owner))

    variants_path = os.path.join(OUT_DIR, "variants.json")
    enums_path = os.path.join(OUT_DIR, "enums.json")
    classes_path = os.path.join(OUT_DIR, "classes.json")

    with open(variants_path, "w", encoding="utf-8") as f:
        json.dump({"variants": variants}, f, indent=2, ensure_ascii=False)
    with open(enums_path, "w", encoding="utf-8") as f:
        json.dump({"enums": enums}, f, indent=2, ensure_ascii=False)
    with open(classes_path, "w", encoding="utf-8") as f:
        json.dump({"classes": class_records}, f, indent=2, ensure_ascii=False)

    print(f"[extract_api] Wrote {variants_path} ({len(variants)} variants)")
    print(f"[extract_api] Wrote {enums_path} ({len(enums)} enums)")
    print(f"[extract_api] Wrote {classes_path} ({len(class_records)} classes)")


if __name__ == "__main__":
    main()
