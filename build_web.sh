#!/usr/bin/env bash
# Provision and build on a temporary Google Compute Engine Ubuntu VM.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
exec python3 - "$root" "$@" <<'PY'
import argparse
import filecmp
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import time

parser = argparse.ArgumentParser(prog="build_web.sh", description="Build with Emscripten on a temporary Google Cloud VM.")
parser.add_argument("--vm", default="opende-build", help="Compute Engine instance name")
parser.add_argument("--project", help="Google Cloud project (defaults to active gcloud project)")
parser.add_argument("--zone", default="us-west1-a")
parser.add_argument("--create", action="store_true", help="Create a new VM; fails if the name already exists")
parser.add_argument("--machine-type", default="n2-standard-16", help="Machine type for --create (16 vCPUs, 64 GB by default)")
parser.add_argument("--max-run-duration", default="24h", help="Automatic deletion deadline for --create")
parser.add_argument("--prepare-only", action="store_true", help="Upload source and install the toolchain without building")
parser.add_argument("--delete-vm", action="store_true", help="Delete the named OpenDE build VM and its boot disk, then exit")
parser.add_argument("--output", default="build-web-local", help="Local bundle output directory")
parser.add_argument("--game-data", help="Local game data directory to upload (defaults to demo-data or game-data)")
parser.add_argument("--remote-game-data", help="Absolute game data directory already on the build VM")
parser.add_argument("--jobs", type=int, default=16, help="Parallel compiler jobs")
args = parser.parse_args(sys.argv[2:])
if args.jobs < 1:
    parser.error("--jobs must be positive")
if args.remote_game_data and (args.game_data or not args.remote_game_data.startswith("/")):
    parser.error("--remote-game-data must be absolute and cannot be combined with --game-data")
if args.delete_vm and (args.create or args.prepare_only):
    parser.error("--delete-vm cannot be combined with --create or --prepare-only")
root = Path(sys.argv[1])
if not shutil.which("gcloud"):
    parser.error("Install the Google Cloud CLI and run gcloud auth login first")
if not args.project:
    args.project = subprocess.check_output(["gcloud", "config", "get-value", "project"], text=True).strip()
if not args.project or args.project == "(unset)":
    parser.error("Set --project or configure a gcloud project")
cloud = ["gcloud", "--project=" + args.project, "--quiet", "compute"]
zone = "--zone=" + args.zone

def compute(*command, **kwargs):
    return subprocess.run(cloud + list(command), check=True, **kwargs)

def guest(code, capture=False):
    result = compute("ssh", args.vm, zone, "--command=bash -se", "--", "-T", "-o", "ConnectTimeout=15",
                     input="set -euo pipefail\n" + code, text=True,
                     stdout=subprocess.PIPE if capture else None)
    return result.stdout.strip() if capture else None

if args.delete_vm:
    result = compute("instances", "describe", args.vm, zone, "--format=json", capture_output=True, text=True)
    info = json.loads(result.stdout)
    if info.get("labels", {}).get("purpose") != "opende-build":
        parser.error("Refusing to delete an instance without the purpose=opende-build label")
    compute("instances", "delete", args.vm, zone)
    sys.exit(0)

output = Path(args.output).expanduser()
if not output.is_absolute():
    output = root / output
bundle_files = {"index.html", "index.js", "index.wasm", "index.data", "SHA256SUMS"}
if not args.prepare_only and output.exists():
    if output.is_symlink() or not output.is_dir() or any(
        item.is_symlink() or item.name not in bundle_files for item in output.iterdir()
    ):
        parser.error(f"Output contains other files: {output}. Choose a dedicated --output directory.")
data = Path(args.game_data).expanduser().resolve() if args.game_data else next(
    (root / name for name in ("demo-data", "game-data") if (root / name).is_dir()), None)
packed_data = data is None and all((root / "static" / name).is_file() for name in ("deusex.js", "deusex.data"))
if not args.prepare_only and not args.remote_game_data and not packed_data and (data is None or not all((data / name).is_dir() for name in ("System", "Maps", "Textures", "Sounds", "Music"))):
    parser.error("Provide --game-data with System, Maps, Textures, Sounds and Music, or provide static/deusex.js and static/deusex.data, or use --prepare-only")

if args.create:
    compute("instances", "create", args.vm, zone, "--machine-type=" + args.machine_type,
            "--image-family=ubuntu-2404-lts-amd64", "--image-project=ubuntu-os-cloud",
            "--boot-disk-size=100GB", "--boot-disk-type=pd-balanced", "--boot-disk-auto-delete",
            "--no-service-account", "--no-scopes", "--labels=purpose=opende-build,lifecycle=temporary",
            "--max-run-duration=" + args.max_run_duration, "--instance-termination-action=DELETE")
# Retry only SSH readiness; installation/build failures should surface immediately.
for attempt in range(18):
    try:
        guest("true")
        break
    except subprocess.CalledProcessError:
        if attempt == 17:
            raise
        time.sleep(10)
print("Preparing Ubuntu build dependencies...", flush=True)
guest("sudo env DEBIAN_FRONTEND=noninteractive apt-get -o DPkg::Lock::Timeout=180 update -qq\n"
      "sudo env DEBIAN_FRONTEND=noninteractive apt-get -o DPkg::Lock::Timeout=180 install -y -qq build-essential cmake ninja-build ccache python3 python3-lz4 git curl xz-utils ca-certificates\n")
work = guest('mkdir -p "$HOME/build/OpenDE-Web"\nmktemp -d "$HOME/build/OpenDE-Web/run.XXXXXX"', capture=True)
guest("mkdir -p " + shlex.quote(work + "/source"))
print(f"Uploading this checkout to {args.vm}:{work}/source ...", flush=True)
environment = dict(os.environ, COPYFILE_DISABLE="1")
with tempfile.TemporaryDirectory(prefix="opende-upload-") as temporary:
    archive = Path(temporary) / "source.tar.gz"
    tar_args = ["tar", "--no-xattrs", "-czf", str(archive), "--exclude=./.git", "--exclude=./.cache",
                "--exclude=./.aws", "--exclude=./.env", "--exclude=./.env.*", "--exclude=./.codex",
                "--exclude=./.agents", "--exclude=./build", "--exclude=./build-*",
                "--exclude=./game-data", "--exclude=./demo-data"]
    if output.is_relative_to(root):
        tar_args.append("--exclude=./" + str(output.relative_to(root)))
    subprocess.run(tar_args + ["-C", str(root), "."], env=environment, check=True)
    compute("scp", zone, str(archive), args.vm + ":" + work + "/source.tar.gz")
    guest("tar -xzf " + shlex.quote(work + "/source.tar.gz") + " -C " + shlex.quote(work + "/source") + " --no-same-owner\nrm " + shlex.quote(work + "/source.tar.gz"))
    if data is not None and not args.remote_game_data:
        archive = Path(temporary) / "game-data.tar.gz"
        subprocess.run(["tar", "--no-xattrs", "-czf", str(archive), "-C", str(data), "System", "Maps", "Textures", "Sounds", "Music"], env=environment, check=True)
        compute("scp", zone, str(archive), args.vm + ":" + work + "/game-data.tar.gz")
        guest("mkdir -p " + shlex.quote(work + "/game-data") + "\ntar -xzf " + shlex.quote(work + "/game-data.tar.gz") + " -C " + shlex.quote(work + "/game-data") + " --no-same-owner\nrm " + shlex.quote(work + "/game-data.tar.gz"))
sdk_setup = r'''
sdk="$HOME/.local/share/opende/emsdk-6.0.12"
if [[ ! -f "$sdk/emsdk_env.sh" ]]; then
    python3 - <<'INSTALL'
from pathlib import Path
import io, tarfile, urllib.request
destination = Path.home() / ".local/share/opende/emsdk-6.0.12"
destination.parent.mkdir(parents=True, exist_ok=True)
url = "https://codeload.github.com/emscripten-core/emsdk/tar.gz/refs/tags/6.0.12"
print("Downloading official Emscripten SDK 6.0.12...", flush=True)
with urllib.request.urlopen(url) as response:
    archive = response.read()
with tarfile.open(fileobj=io.BytesIO(archive), mode="r:gz") as package:
    package.extractall(destination.parent, filter="data")
INSTALL
fi
if [[ ! -x "$sdk/upstream/emscripten/emcc" ]]; then
    "$sdk/emsdk" install 6.0.12
    "$sdk/emsdk" activate 6.0.12
fi
source "$sdk/emsdk_env.sh"
emcc --version
'''
guest(sdk_setup)
if args.remote_game_data:
    remote_data = shlex.quote(args.remote_game_data)
    guest(f'for folder in System Maps Textures Sounds Music; do test -d {remote_data}/"$folder"; done\n'
          + "cp -a " + remote_data + " " + shlex.quote(work + "/game-data"))
elif packed_data:
    guest("python3 " + shlex.quote(work + "/source/static/scripts/unpack-web-data.py") +
          " --loader " + shlex.quote(work + "/source/static/deusex.js") +
          " --data " + shlex.quote(work + "/source/static/deusex.data") +
          " --output " + shlex.quote(work + "/game-data"))
cleanup = shlex.join(["./build_web.sh", "--project", args.project, "--zone", args.zone, "--vm", args.vm, "--delete-vm"])
print(f"VM workspace: {work}", flush=True)
print(f"Delete this temporary VM when finished: {cleanup}", flush=True)
if args.prepare_only:
    print("Source uploaded and Emscripten 6.0.12 ready. VM retained for later builds.")
    sys.exit(0)
data = shlex.quote(work + "/game-data")
build = shlex.quote(work + "/build")
source = shlex.quote(work + "/source")
guest(sdk_setup + f"""
emcmake cmake -S {source} -B {build} -G Ninja \\
    -DCMAKE_BUILD_TYPE=Release -DSKIP_QA=ON -DSKIP_UT=ON \\
    -DDEUSEX_GAME_DATA={data}
cmake --build {build} --target deusex --parallel {args.jobs}
cd {build}/bin
sha256sum index.html index.js index.wasm index.data > SHA256SUMS
""")
print("Copying the browser bundle back...", flush=True)
output.mkdir(parents=True, exist_ok=True)
for name in sorted(bundle_files):
    compute("scp", zone, args.vm + ":" + work + "/build/bin/" + name, str(output / name))
checksums = dict(line.split(maxsplit=1)[::-1] for line in (output / "SHA256SUMS").read_text().splitlines())
if set(checksums) != bundle_files - {"SHA256SUMS"}:
    raise RuntimeError("Downloaded checksum manifest does not describe the complete bundle")
for name, expected in checksums.items():
    actual = hashlib.sha256((output / name).read_bytes()).hexdigest()
    if actual != expected:
        raise RuntimeError(f"Copied artifact checksum mismatch: {name}")
static = root / "static"
static.mkdir(exist_ok=True)
# The client fetches game data from GCS; keep local data in the ignored output directory.
for extension in ("wasm", "js"):
    source = output / f"index.{extension}"
    destination = static / f"deusex.{extension}"
    if destination.is_file() and filecmp.cmp(source, destination, shallow=False):
        continue
    with tempfile.NamedTemporaryFile(dir=static, delete=False) as temporary:
        temporary_path = Path(temporary.name)
        try:
            with source.open("rb") as artifact:
                shutil.copyfileobj(artifact, temporary)
            temporary.close()
            temporary_path.chmod(0o644)
            temporary_path.replace(destination)
        finally:
            temporary_path.unlink(missing_ok=True)
print(f"\nBuild and transfer verified: {output}")
print(f"Updated browser bundle: {static}")
print(f"VM build directory: {work}")
print("To test the static page on the Mac:")
print(f"  python3 -m http.server 7332 --bind 127.0.0.1 --directory {shlex.quote(str(static))}")
print("  open http://127.0.0.1:7332/explore.html")
PY
