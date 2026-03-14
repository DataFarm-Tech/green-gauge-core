#!/usr/bin/env python3
"""Upload firmware.bin and version.txt to a Backblaze B2 bucket."""

from __future__ import annotations

import os
import sys
import tempfile
from datetime import UTC, datetime
from pathlib import Path
from typing import Any

VERSION = "0.0.1"


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    sys.exit(1)


def require_env(name: str) -> str:
    value = os.getenv(name, "").strip()
    if not value:
        fail(f"Missing required environment variable: {name}")
    return value


def resolve_app_binary(build_dir: Path) -> Path:
    flash_app_args = build_dir / "flash_app_args"
    if not flash_app_args.is_file():
        fail(f"Missing build metadata file: {flash_app_args}")

    tokens: list[str] = []
    for raw_line in flash_app_args.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if line:
            tokens = line.split()

    if not tokens:
        fail(f"Unable to parse application binary from: {flash_app_args}")

    app_name = tokens[-1]
    app_path = build_dir / app_name
    if not app_path.is_file():
        fail(f"Application binary not found: {app_path}")

    return app_path


def try_archive_existing_file(bucket: Any, file_name: str, archive_prefix: str, temp_dir: Path) -> None:
    source_path = temp_dir / file_name
    source_path.parent.mkdir(parents=True, exist_ok=True)

    try:
        downloaded_file = bucket.download_file_by_name(file_name)
        downloaded_file.save_to(str(source_path))
    except Exception as exc:
        message = str(exc).lower()
        if "not present" in message or "not found" in message:
            print(f"No existing {file_name} to archive (skipping).")
            return
        raise

    archived_name = f"{archive_prefix}/{file_name}"
    bucket.upload_local_file(local_file=str(source_path), file_name=archived_name)
    print(f"Archived {file_name} to {archived_name}")


def main() -> None:
    try:
        from b2sdk.v2 import B2Api, InMemoryAccountInfo  # pyright: ignore[reportMissingImports]
    except ImportError as exc:
        fail(f"b2sdk is not installed: {exc}")

    b2_key_id = require_env("B2_KEY_ID")
    b2_application_key = require_env("B2_APPLICATION_KEY")
    b2_bucket = require_env("B2_BUCKET")

    repo_root = Path.cwd()
    build_dir = repo_root / "build"
    app_binary = resolve_app_binary(build_dir)

    archive_prefix = f"archive/{datetime.now(UTC).strftime('%Y%m%d-%H%M%S')}"

    version_file = repo_root / "version.txt"
    version_file.write_text(f"{VERSION}\n", encoding="utf-8")

    info = InMemoryAccountInfo()
    b2_api = B2Api(info)
    b2_api.authorize_account("production", b2_key_id, b2_application_key)
    bucket = b2_api.get_bucket_by_name(b2_bucket)

    with tempfile.TemporaryDirectory(prefix="b2-archive-") as tmpdir:
        temp_dir = Path(tmpdir)
        try_archive_existing_file(bucket, "firmware.bin", archive_prefix, temp_dir)
        try_archive_existing_file(bucket, "version.txt", archive_prefix, temp_dir)

    bucket.upload_local_file(local_file=str(app_binary), file_name="firmware.bin")
    bucket.upload_local_file(
        local_file=str(version_file),
        file_name="version.txt",
        content_type="text/plain",
    )

    print(f"Uploaded firmware.bin from {app_binary} and version.txt ({VERSION})")


if __name__ == "__main__":
    main()
