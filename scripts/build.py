#!/usr/bin/env python3
"""Compile the library and API regression sketch; never upload to a device."""

import argparse
import datetime
import hashlib
import json
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
CONFIG = json.loads((ROOT / "scripts/ci-targets.json").read_text())


def timestamp():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def git_output(*args):
    return subprocess.check_output(
        ["git", "-C", str(ROOT), *args], text=True
    ).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", choices=CONFIG["targets"], required=True)
    parser.add_argument("--arduino-cli", default="arduino-cli")
    parser.add_argument("--config-file", type=Path,
                        help="Reuse a specific Arduino CLI configuration.")
    parser.add_argument("--skip-install", action="store_true",
                        help="Use already installed dependencies; still verify versions.")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "build-ci")
    args = parser.parse_args()
    output = args.output_dir.resolve() / args.target
    fqbn = CONFIG["targets"][args.target]
    core_id = ":".join(fqbn.split(":")[:2])
    core = CONFIG["cores"][core_id]
    source_paths = [*ROOT.glob("*.cpp"), *ROOT.glob("*.h"),
                    ROOT / "library.properties", ROOT / "scripts/build.py",
                    ROOT / "scripts/ci-targets.json"]
    for sketch in CONFIG["sketches"]:
        source_paths.extend((ROOT / sketch).rglob("*.ino"))
    record = {
        "schema_version": 1,
        "started_at": timestamp(),
        "source_revision": git_output("rev-parse", "HEAD"),
        "source_dirty": bool(git_output("status", "--porcelain")),
        "source_sha256": {
            str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(source_paths)
        },
        "host": platform.platform(),
        "target": args.target,
        "fqbn": fqbn,
        "requested_versions": {
            "arduino_cli": CONFIG["arduino_cli"]["version"],
            "core": f"{core_id}@{core['version']}",
            "libraries": CONFIG["libraries"],
        },
        "validation_scope": "Compilation and linking only; no upload or hardware test.",
        "board_assumption": (
            "Generic ESP32-S3 compile target; does not establish LilyGo/Waveshare pinout, memory, or flash compatibility."
            if args.target == "esp32s3" else "Upstream CI board definition."
        ),
        "commands": [], "sketches": [], "artifacts": [], "status": "failed",
    }
    output.mkdir(parents=True, exist_ok=True)
    # Never report old binaries as outputs of a failed new build.
    binaries = output / "binaries"
    if binaries.exists():
        shutil.rmtree(binaries)
    config_file = args.config_file
    if config_file is None:
        state = args.output_dir.resolve() / ".arduino-state"
        state.mkdir(parents=True, exist_ok=True)
        config_file = state / f"{args.target}.yaml"
        # JSON is also valid YAML, so no third-party parser is required.
        config_file.write_text(json.dumps({
            "board_manager": {"additional_urls": core["urls"]},
            "directories": {
                "data": str(state / "data"),
                "downloads": str(state / "downloads"),
                "user": str(state / "user"),
            },
        }, indent=2) + "\n")
    cli = [args.arduino_cli, "--config-file", str(config_file.resolve())]
    exit_code = 1
    with (output / "build.log").open("w", encoding="utf-8") as log:
        def run(arguments):
            command = [*cli, *arguments]
            entry = {"argv": command, "started_at": timestamp()}
            record["commands"].append(entry)
            heading = "$ " + shlex.join(command)
            print(heading, flush=True)
            log.write(heading + "\n")
            lines = []
            process = subprocess.Popen(command, cwd=ROOT, stdout=subprocess.PIPE,
                                       stderr=subprocess.STDOUT, text=True,
                                       encoding="utf-8", errors="replace")
            for line in process.stdout:
                print(line, end="", flush=True)
                log.write(line)
                log.flush()
                lines.append(line)
            entry["exit_code"] = process.wait()
            entry["finished_at"] = timestamp()
            if process.returncode:
                raise subprocess.CalledProcessError(process.returncode, command)
            return "".join(lines)

        try:
            version = run(["version"])
            record["cli_version_output"] = version.strip()
            expected = CONFIG["arduino_cli"]["version"]
            if not re.search(r"Version:\s*" + re.escape(expected) + r"(?:\s|$)", version):
                raise RuntimeError(f"Arduino CLI {expected} is required.")
            if not args.skip_install:
                urls = ["--additional-urls", ",".join(core["urls"])] if core["urls"] else []
                run(["core", "update-index", *urls])
                run(["core", "install", f"{core_id}@{core['version']}", *urls])
                run(["lib", "update-index"])
                run(["lib", "install", *CONFIG["libraries"]])
            core_list = json.loads(run(["core", "list", "--format", "json"]))
            library_list = json.loads(run(["lib", "list", "--format", "json"]))
            record["installed_cores"] = core_list
            record["installed_libraries"] = library_list
            if not any(p.get("id") == core_id and p.get("installed_version") == core["version"]
                       for p in core_list.get("platforms", [])):
                raise RuntimeError(f"Required core {core_id}@{core['version']} is not installed.")
            for specification in CONFIG["libraries"]:
                name, required_version = specification.rsplit("@", 1)
                if not any(item.get("library", {}).get("name") == name
                           and item.get("library", {}).get("version") == required_version
                           for item in library_list.get("installed_libraries", [])):
                    raise RuntimeError(f"Required library {specification} is not installed.")
            for sketch in CONFIG["sketches"]:
                destination = binaries / Path(sketch).name
                # Arduino's default cache is keyed by sketch path. Keep each
                # target separate so parallel local builds cannot mix objects.
                work = (args.output_dir.resolve() / ".arduino-build" /
                        args.target / Path(sketch).name)
                run(["compile", "--fqbn", fqbn, "--warnings", "all",
                     "--library", str(ROOT), "--build-path", str(work),
                     "--output-dir", str(destination),
                     str(ROOT / sketch)])
                record["sketches"].append({"path": sketch, "exit_code": 0})
            record["status"] = "compiled"
            exit_code = 0
        except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
            record["error"] = str(error)
            print(f"Build failed: {error}", file=sys.stderr)
            log.write(f"Build failed: {error}\n")
        finally:
            for artifact in sorted(binaries.rglob("*")) if binaries.exists() else []:
                if artifact.is_file():
                    record["artifacts"].append({
                        "path": str(artifact.relative_to(output)),
                        "bytes": artifact.stat().st_size,
                        "sha256": hashlib.sha256(artifact.read_bytes()).hexdigest(),
                    })
            record["finished_at"] = timestamp()
            record["exit_code"] = exit_code
            log.flush()
            record["build_log_sha256"] = hashlib.sha256(
                (output / "build.log").read_bytes()).hexdigest()
            (output / "build-record.json").write_text(json.dumps(record, indent=2) + "\n")
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
