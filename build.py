import argparse
import subprocess
from pathlib import Path
from typing import Final, Literal
import os
import sys

CL_CMD: Final[list[str]] = [
    "cl",
    "/nologo",
    "/c",
    "/Od",
    "/W0",
    "/GS-",
    "/Tc"
]

ARCH: Final[str | None] = os.environ.get("VSCMD_ARG_TGT_ARCH")

if ARCH not in ("x86", "x64"):
    raise RuntimeError(f"Unsupported architecture: {ARCH}")

ROOT_DIR: Path = Path("KIT")
BUILD_DIR: Path = Path("Build") / ARCH
OUTPUT_SCRIPT_PATH: Path = Path("Build") / "OperatorsKit.cna"
CNA_SCRIPT_CONTENTS: str = ""


def build_project(proj: Path, script_only: bool = False) -> str:
    global CNA_SCRIPT_CONTENTS

    state: Literal['FAIL', 'SUCCESS', 'SKIPPED'] = 'SUCCESS'

    source_file: Path = proj / f"{proj.name.lower()}.c"
    script_file: Path = proj / f"{proj.name.lower()}.cna"
    output_file: Path = (BUILD_DIR / f"{proj.name.lower()}.o").resolve()

    missing_files: list[str] = []

    script_contents: str = ""

    err_output: str = ""

    print(f"Building {proj.name:<30}", end='')

    if not source_file.is_file():
        err_output += f"Cannot find 'c' source file: {source_file.resolve()}\n"
        missing_files.append(source_file.name)
        state = 'SKIPPED'

    if not script_file.is_file():
        err_output += f"Cannot find 'cna' script file: {script_file.resolve()}\n"
        missing_files.append(script_file.name)
        state = 'SKIPPED'

    if state == 'SKIPPED':
        print(f"\033[93m{state} \033[90mFiles not found: {missing_files}\033[0m")
        return err_output

    if script_only == False:
        # Build the project with cl.exe
        try:
            subprocess.run(
                CL_CMD + [source_file.name, f"/Fo{output_file}"], 
                cwd=proj,
                capture_output=True,
                check=True,
                text=True)
        except subprocess.CalledProcessError as e:
            print(f"\033[91mFAILED\033[0m")
            return str(e.output)


    # Get the contents of the script file
    script_contents = script_file.read_text(encoding='utf-8')

    CNA_SCRIPT_CONTENTS += script_contents + "\n\n"

    print("\033[92mSUCCESS\033[0m" + (" - script only" if script_only else ""))

    return err_output

def build_all(scripts_only: bool = False) -> dict[str, str]:
    build_errs: dict[str, str] = {}

    for proj in ROOT_DIR.iterdir():
        if not proj.is_dir():
            continue

        err = build_project(proj, scripts_only)

        if err:
            build_errs[proj.name] = err

    return build_errs


if __name__ == "__main__":
    build_errs: dict[str, str] = {}

    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    CNA_SCRIPT_CONTENTS = Path("base.cna").read_text(encoding='utf-8')

    parser = argparse.ArgumentParser()

    parser.add_argument(
        "projects",
        nargs="*",
        help="Projects to build. If omitted, all projects are built."
    )

    parser.add_argument(
        "-s",
        "--scripts-only",
        action="store_true",
        help="Skip compiling C projects and only combine CNA scripts."
    )

    args = parser.parse_args()

    if args.projects and args.scripts_only == False:
        projects = [ROOT_DIR / p for p in args.projects]
        
        for project in projects:
            if not project.exists() or not project.is_dir():
                print(f"Building {project.name:<30}"
                      f"\033[33mSKIPPED\033[0m \033[90m "
                      f"Directory not found: {project.resolve()}\033[0m")
                continue

            result = build_project(project)

            if result:
                build_errs[project.name] = result
    else:
        build_errs = build_all(args.scripts_only)

    if build_errs:
        print("**********************************************************************\n"
              "*                           Build Errors                             *\n"
              "**********************************************************************")
        for key, value in build_errs.items():
            print(f"> {key}\n{value}")
        sys.exit(1)

    OUTPUT_SCRIPT_PATH.write_text(CNA_SCRIPT_CONTENTS, encoding='utf-8')

    sys.exit(0)
