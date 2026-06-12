#!/usr/bin/env python3
"""Validate Kubernetes manifest structure without requiring a cluster."""

from __future__ import annotations

import sys
from pathlib import Path

REQUIRED_PREFIXES = ("apiVersion:", "kind:", "metadata:")


def iter_yaml_files(root: Path) -> list[Path]:
    return sorted(path for path in root.rglob("*.yaml") if path.is_file())


def split_documents(text: str) -> list[str]:
    docs = []
    current = []
    for line in text.splitlines():
        if line.strip() == "---":
            if current:
                docs.append("\n".join(current))
                current = []
            continue
        current.append(line)
    if current:
        docs.append("\n".join(current))
    return docs


def validate_file(path: Path) -> list[str]:
    errors = []
    documents = [doc for doc in split_documents(path.read_text()) if doc.strip()]
    if not documents:
        return [f"{path}: no YAML documents found"]

    for index, document in enumerate(documents, start=1):
        stripped_lines = [
            line.strip()
            for line in document.splitlines()
            if line.strip() and not line.startswith("#")
        ]
        for prefix in REQUIRED_PREFIXES:
            if not any(line.startswith(prefix) for line in stripped_lines):
                errors.append(f"{path}:{index}: missing required field {prefix[:-1]!r}")
        if "metadata:" in stripped_lines:
            metadata_index = stripped_lines.index("metadata:")
            following = stripped_lines[metadata_index + 1 : metadata_index + 8]
            if not any(line.startswith("name:") for line in following):
                errors.append(f"{path}:{index}: metadata.name is required")
    return errors


def main() -> int:
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("deploy/k8s")
    files = iter_yaml_files(root)
    if not files:
        print(f"No Kubernetes manifests found under {root}", file=sys.stderr)
        return 1

    errors = []
    for path in files:
        errors.extend(validate_file(path))

    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1

    print(f"Validated {len(files)} Kubernetes manifest files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
