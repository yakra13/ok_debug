import argparse
from enum import Enum
import itertools
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Final, Iterable, Literal, TypeAlias

Arch: TypeAlias   = Literal['x64', 'x86']
Config: TypeAlias = Literal['debug', 'release']

class BuildResult(Enum):
    FAIL = "FAIL"
    SKIP = "SKIPPED"
    SUCCESS = "SUCCESS"
    UP_TO_DATE = "UP TO DATE"
    

S_CLEAN: Final[str]   = 'clean'
S_DEBUG: Final[str]   = 'debug'
S_RELEASE: Final[str] = 'release'
S_SCRIPT_ONLY: Final[str]  = 'script-only'

PROJECT_ROOT: Path = Path(__file__).resolve().parent

BOFS_DIR: Path    = Path("kit")
BUILD_DIR: Path   = Path("build")
DEBUG_DIR: Path   = BUILD_DIR / S_DEBUG
RELEASE_DIR: Path = BUILD_DIR / S_RELEASE

CACHE_DIR: Path = BUILD_DIR / ".cache"

CLEAN_KEEPS: set[str] = {".cache"}

ENV_FILES: dict [Arch, Path] = {
    'x64': CACHE_DIR / "env_x64.json",
    'x86': CACHE_DIR / "env_x86.json"
}

SCRIPTS_DIR: Path          = Path("scripts")
CNA_LIB_DIR: Path          = SCRIPTS_DIR / "lib"
MAIN_SCRIPT_FILE: Path     = CNA_LIB_DIR / "main.cna"
COMPILED_SCRIPT_FILE: Path = RELEASE_DIR / "OperatorsKit.cna"

BUILD_ERRS: dict[str, str] = {}
INCLUDES: list[Path] = [
    Path("common"),
]

MOCK_PATH: Path = Path(r"common\base\mock.cpp")

CL_BASE_CMD: list[str]      = ["cl", "/nologo", "/c", "/GS-", "/EHsc", "/std:c++20"]
CL_DEBUG_FLAGS: list[str]   = ["/D_DEBUG", "/Zi", "/MTd"]
CL_RELEASE_FLAGS: list[str] = ["/Od", "/W0", "/MT"]

POWERSHELL_CMD: list[str] = ["powershell.exe", "-NoProfile", "-Command"]

DEV_SHELL_PATH: Final[str] = r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1"

C_RED: Final[str]    = "\033[91m"
C_GREEN: Final[str]  = "\033[92m"
C_YELLOW: Final[str] = "\033[93m"
C_RESET: Final[str]  = "\033[0m"

class BuildSystem:
    def __init__(self):
        self._env: dict[str, dict[str, str]] = {}

    def _load_env_from_cache(self, arch: Arch):
        if ENV_FILES[arch].is_file():
            with ENV_FILES[arch].open('r', encoding='utf-8') as f:
                self._env[arch] = json.load(f)
        else:
            print(f"Generating {arch} build environment...")
            
            self._env[arch] = self._get_dev_env(arch)

            ENV_FILES[arch].parent.mkdir(parents=True, exist_ok=True)

            with ENV_FILES[arch].open('w', encoding='utf-8') as f:
                json.dump(self._env[arch], f, indent=2)

    def _get_dev_env(self, arch: Arch) -> dict[str, str]:
        cmd = (
            f"& '{DEV_SHELL_PATH}' -Arch {'amd64' if arch == 'x64' else arch}; "
            "Get-ChildItem Env: | ForEach-Object { \"$($_.Name)=$($_.Value)\" }" 
        )

        result = subprocess.run(
            ["powershell.exe", "-NoProfile", "-Command", cmd],
            capture_output=True,
            text=True,
            check=True
        )

        env = os.environ.copy() # {}

        for line in result.stdout.splitlines():
            if '=' in line:
                k, v = line.split('=', 1)
                env[k.upper()] = v

        return env

    def _get_project_path(self, proj_name: str) -> Path:
        proj_name = proj_name.casefold()

        for entry in BOFS_DIR.iterdir():
            if entry.is_dir() and entry.name.casefold() == proj_name:
                return entry

        raise FileNotFoundError(f"'{proj_name}' not found in '{BOFS_DIR}'")

    def _get_built_files(self, proj_name: str, cfg: Config, arch: Arch) -> list[Path]:
        files: list[Path] = []

        build_path: Path = RELEASE_DIR if cfg == 'release' else DEBUG_DIR

        build_path = build_path / arch

        if not build_path.exists():
            return []

        for file in build_path.iterdir():
            if not file.is_file():
                continue

            if file.stem.casefold() == proj_name.casefold():
                files.append(file)

        return files    
        
    def _get_source_file(self, project: Path, proj_name: str) -> Path:
        # sources: list[Path] = []
        
        for file in project.iterdir():
            if not file.is_file():
                continue
    
            if file.suffix.lower() in (".c", ".cpp"):
                if file.stem.casefold() == proj_name.casefold():
                    return file
    
        raise FileNotFoundError(f"No C/C++ source found in {project}")
        # if not sources:
    
        # return sources

    def _get_compile_cmd(self, src: list[Path], out_dir: Path, cfg: Config) -> list[str]:
        cmd: list[str] = CL_BASE_CMD.copy()
        cmd += [f"/I{str(x)}" for x in INCLUDES]
        cmd += [f"/Fo{out_dir}\\", f"/Fd{out_dir}\\"]
        
        if cfg == S_DEBUG:
            cmd += CL_DEBUG_FLAGS
            # Append mock.cpp required for debug builds
            src.append(MOCK_PATH)
        else:
            cmd += CL_RELEASE_FLAGS

        cmd += [str(x) for x in src]

        return cmd

    def _get_link_cmd(self, file_name: str, obj_dir: Path, out_dir: Path) -> list[str]:
        obj_files: list[Path] = list(obj_dir.glob("*.obj"))

        full_path: Path = out_dir / file_name

        pdb: Path = full_path.with_suffix(".pdb")
        exe: Path = full_path.with_suffix(".exe")
        obj: Path = full_path.with_suffix(".o")

        if not obj_files:
            raise FileNotFoundError(f"No object files found in {obj_dir}")

        # if cfg == S_DEBUG:
        cmd = ["link", "/DEBUG", f"/PDB:{pdb}", f"/OUT:{exe}"]
        # else:
        #     cmd = ["link", "/lib", f"/out:{obj}",]

        cmd += [str(x) for x in obj_files]

        return cmd

    def _validate_entry_source(self, project: Path, sources: list[Path], cfg: Config):
        name: str = project.name.casefold()
        c_entry: bool = False

        for file in sources:
            if file.stem.casefold() != name:
                continue

            ext = file.suffix.casefold()

            if ext == '.cpp':
                return

            if ext == '.c':
                c_entry = True

        if  c_entry:
            if cfg == S_DEBUG:
                raise FileNotFoundError(
                    f"Compiling in debug requires {project.name}.cpp but only {project.name}.c source was found."
                )

            return

        raise FileNotFoundError(f"{project.name}.cpp or {project.name}.c not found in {project}")

    def _run_with_spinner(self, cmd: list[str], env: dict[str, str]) -> tuple[int, str, str]:
        spinner = itertools.cycle("⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏")

        try:
            proc = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                env=env,
            )

            # Hide cursor during load animation
            sys.stdout.write("\033[?25l")
            sys.stdout.flush()

            while proc.poll() is None:
                print(f"{next(spinner)}", end="", flush=True)
                time.sleep(0.08)
                print("\b", end="", flush=True)

        finally:
            # Show cursor when done or on error
            sys.stdout.write("\033[?25h")
            sys.stdout.flush()

        stdout, stderr = proc.communicate()

        return proc.returncode, stdout, stderr

    def build(self, proj_name: str, cfg: Config, arch: Arch) -> tuple[BuildResult, str | None]:
        project: Path
        output_root: Path
        object_dir: Path
        debug_out: Path

        # sources: list[Path]    = []
        source: Path
        compile_cmd: list[str] = []
        link_cmd: list[str]    = []

        self._load_env_from_cache(arch)

        try:
            project = self._get_project_path(proj_name)
        except FileNotFoundError as err:
            return BuildResult.FAIL, str(err)
        
        try:
            source = self._get_source_file(project, proj_name)
        except FileNotFoundError as err:
            return BuildResult.FAIL, str(err)

        # try:
        #     self._validate_entry_source(project, source, cfg)
        # except FileNotFoundError as err:
        #     return BuildResult.FAIL, str(err)

        # if debug build source must be .cpp
        if cfg == S_DEBUG and source.suffix != '.cpp':
            return BuildResult.FAIL, "Source must be cpp file for debug builds"

        #--------------------------------------
        #      Setup / create directories
        #--------------------------------------        

        output_root = BUILD_DIR / cfg / project.name
        debug_out   = BUILD_DIR / cfg / arch
        object_dir  = output_root / arch

        

        # Check if built file(s) already exists
        built_files = self._get_built_files(proj_name, cfg, arch)

        if built_files:
            has_exe: bool = True
            has_pdb: bool = True

            oldest_source: float = get_oldest_modified(project, [".c", ".cpp", ".h"])
            oldest_built: float = min(file.stat().st_mtime for file in built_files)

            if cfg == S_DEBUG:
                has_exe = any(file.suffix.lower() == '.exe' for file in built_files)
                has_pdb = any(file.suffix.lower() == '.pdb' for file in built_files)

            # If the existing exe is newer than the source
            # we can skip
            if oldest_built > oldest_source:
                # Even if the built is newer it must have both files
                if has_exe and has_pdb:
                    return BuildResult.UP_TO_DATE, None



        # Append this projects directory to the include directories
        INCLUDES.append(project)

        #--------------------------------------
        #           Build the project
        #--------------------------------------
        
        object_dir.mkdir(parents=True, exist_ok=True)

        compile_cmd = self._get_compile_cmd([source], object_dir, cfg)

        ret, stdout, stderr = self._run_with_spinner(
            POWERSHELL_CMD + [' '.join(compile_cmd)],
            self._env[arch]
        )

        if ret != 0:
            return BuildResult.FAIL, stderr or stdout

        #--------------------------------------
        #     Link the project (Debug only)
        #--------------------------------------
        output_root.mkdir(parents=True, exist_ok=True)
        debug_out.mkdir(parents=True, exist_ok=True)

        if cfg == S_DEBUG:
            try:
                link_cmd = self._get_link_cmd(
                    proj_name.casefold(),
                    object_dir,
                    debug_out
                )
            except FileNotFoundError as err:
                return BuildResult.FAIL, str(err)
            
            ret, stdout, stderr = self._run_with_spinner(
                POWERSHELL_CMD +  [' '.join(link_cmd)],
                self._env[arch]
            )

            if ret != 0:
                return BuildResult.FAIL, stderr or stdout
        else:
            # we need to move the obj if its not linked since that handles placing the file
            for file in object_dir.iterdir():
                if file.is_file():
                    name = file.with_suffix(".o").name if file.suffix.lower() == ".obj" else file.name
                    shutil.move(file, RELEASE_DIR / arch / name)

        #--------------------------------------
        #         Cleanup build files
        #--------------------------------------

        if output_root.exists():
            shutil.rmtree(output_root)

        return BuildResult.SUCCESS, None

def get_oldest_modified(dir: Path, extensions: Iterable[str]) -> float:
    extensions = {ext.lower().lstrip(".") for ext in extensions}

    files = (
        file
        for file in dir.rglob("*")
        if file.is_file() and file.suffix.lower().lstrip(".") in extensions
    )

    return min(
        (file.stat().st_mtime for file in files),
        default=0.0
    )

def clean() -> None:
    if not BUILD_DIR.is_dir():
        return

    for path in BUILD_DIR.iterdir():
        if path.name in CLEAN_KEEPS:
            continue

        if path.is_dir():
            # Remove directory
            shutil.rmtree(path)
        else:
            # Remove file
            path.unlink()

def build_projects(projects: list[str], arch: Arch, config: Config) -> dict[str, str]:
    result: BuildResult
    output: str | None
    errors: dict[str, str] = {}

    # Normalize names
    projects = [p.casefold() for p in projects]

    print("Building BOFs")

    builder: BuildSystem = BuildSystem()

    for project in BOFS_DIR.iterdir():
        if project.name.startswith('_'):
            continue

        if project.name.casefold() not in projects:
            continue

        # build returns a string on failure; None on success
        print(f"  {project.name:<30}", end='', flush=True)

        result, output = builder.build(project.name, config, arch)

        if result == BuildResult.FAIL:
            errors[project.name] = output or "No output received"
            print(C_RED + result.name + C_RESET)
        elif result == BuildResult.SUCCESS:
            print(C_GREEN + result.value + C_RESET)
        else:
            print(C_YELLOW + result.value + C_RESET)

    return errors

def compile_scripts() -> bool:
    script_files: list[Path]  = []
    missing_files: list[Path] = []

    compiled_data: str = ""

    print("Compiling CNA Scripts")

    for project in BOFS_DIR.iterdir():
        # Skip projects starting with _
        if project.name.startswith("_"):
            continue

        cna_script: Path = (project / project.name.casefold()).with_suffix(".cna")

        if cna_script.is_file():
            script_files.append(cna_script)
        else:
            missing_files.append(cna_script)

    if not script_files:
        print(f"  No CNA scripts found\n  Expected:")

        for file in missing_files:
            print(C_RED + f"    {file}" + C_RESET)

        return False

    # Get the latest change of all the BOF scripts and base script
    latest_change = max(p.stat().st_mtime for p in script_files)
    latest_change = max(latest_change, MAIN_SCRIPT_FILE.stat().st_mtime)

    # If the compiled script is newer then no rebuild is needed
    if COMPILED_SCRIPT_FILE.exists() and latest_change < COMPILED_SCRIPT_FILE.stat().st_mtime:
        if missing_files:
            print(C_YELLOW + "Missing expected files:" + C_RESET)

            for file in missing_files:
                print(C_YELLOW + f"    {file}" + C_RESET)

            return False

        print(f"  {COMPILED_SCRIPT_FILE.name} already up-to-date.")

        return True

    # Add the base script first
    compiled_data += MAIN_SCRIPT_FILE.read_text().strip()

    for file in CNA_LIB_DIR.iterdir():
        if file == MAIN_SCRIPT_FILE:
            continue

        if not file.is_file():
            continue

        if file.suffix != ".cna":
            continue

        comment = file.resolve().relative_to(PROJECT_ROOT)
        content = file.read_text(encoding='utf-8').strip()
        compiled_data += f"\n\n#\n# {comment}\n#\n\n{content}"

    for project in BOFS_DIR.iterdir():
        # Skip projects starting with _
        if project.name.startswith("_"):
            continue

        for file in project.iterdir():
            if file.suffix != ".cna":
                continue

            comment = file.resolve().relative_to(PROJECT_ROOT)
            content = file.read_text(encoding='utf-8').strip()
            compiled_data += f"\n\n#\n# {comment}\n#\n\n{content}"

    if missing_files:
        print(C_YELLOW + "  Missing expected files:" + C_RESET)

        for file in missing_files:
            print(C_YELLOW + f"    {file}" + C_RESET)

        return False

    COMPILED_SCRIPT_FILE.write_text(compiled_data, encoding='utf-8')

    print(f"  {COMPILED_SCRIPT_FILE.name} created.")

    return True

def get_args() -> argparse.Namespace:
    parser: argparse.ArgumentParser = argparse.ArgumentParser()

    parser.add_argument(
        "config",
        choices=[S_CLEAN, S_RELEASE, S_DEBUG, S_SCRIPT_ONLY],
        help="Build configuration"
    )

    parser.add_argument(
        "-a",
        "--arch",
        choices=['x64', 'x86'],
        default="x64",
        help="Target architecure (default: x64)"
    )

    parser.add_argument(
        "projects",
        nargs="*",
        help="Projects to build. If omitted, all projects are built."
    )

    return parser.parse_intermixed_args()

if __name__ == "__main__":
    proj_choices: list[str]     = []
    proj_list: list[str]        = []
    proj_lookup: dict[str, str] = {}

    proj_choices = sorted(
        entry.name
        for entry in BOFS_DIR.iterdir()
        if entry.is_dir() and not entry.name.startswith('_')
    )

    proj_lookup = {p.casefold(): p  for p in proj_choices}

    args: argparse.Namespace = get_args()

    if args.config == S_CLEAN:
        clean()
        sys.exit(0)

    if args.config == S_SCRIPT_ONLY:
        sys.exit(compile_scripts())

    invalid_projects = [p for p in args.projects if p.casefold() not in proj_lookup]

    # Check provided project names exist
    if invalid_projects:
        print(
            f"Unknown project(s):\n  {', '.join(invalid_projects)}\n"
            f"Valid projects:\n  {', '.join(proj_choices)}"
        )
        sys.exit(1)

    if args.projects:
        proj_list = args.projects
    else:
        proj_list = proj_choices.copy()

    errors = build_projects(proj_list, args.arch, args.config)

    if errors:
        print("\nError Report:")
        
        for k, v in errors.items():
            print(f"\n{k}\n{'-'*20}\n{C_RED + v + C_RESET}")

        sys.exit(1)

    print("")

    if args.config == S_RELEASE:
        result = compile_scripts()

        if not result:
            sys.exit(1)

    sys.exit(0)
