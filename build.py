import argparse
import shutil
import subprocess
from pathlib import Path
from typing import Final, Literal, TypeAlias
import os
import sys

Arch: TypeAlias   = Literal['x64', 'x86']
Config: TypeAlias = Literal['debug', 'release']

BUILD_DIR: Path = Path("build")
PROJ_ROOT: Path = Path("bofs")

BUILD_ERRS: dict[str, str] = {}

# Debug build command x64/x86
# cl /nologo /c /GS- /EHsc /std:c++20 /D_DEBUG /Zi /MTd /I$(COMMON) /Fo$(DEBUG_OUT)\ /Fd$(DEBUG_OUT)\ kit\_Example\bof.cpp $(MOCK)
# link /DEBUG /PDB:$(DEBUG_OUT)\example.pdb /OUT:$@ $(DEBUG_OUT)\*.obj
# release build command x64/x86
# CL_CMD: Final[list[str]] = [
#     "cl",
#     "/nologo",
#     "/c",
#     "/Od",
#     "/W0",
#     "/GS-",
#     "/Tc"
# ]

# BOFS_DIR: Path = Path("bofs")

# SCRIPTS_DIR: Path = Path("scripts")

# BUILD_DIR: Path = Path("build")
# COMMON_DIR: Path = Path("common")

# DEBUG: Final[str] = "debug"
# RELEASE: Final[str] = "release"

class BuildSystem:
    # _DEBUG: Final[str] = "debug"
    # _RELEASE: Final[str] = "release"

    def __init__(self,
                #  arch: Literal['x64', 'x86'],
                 root_dir: Path,
                 build_dir: Path,
                 include_dirs: list[Path]):
        
        # self._arch = arch
        self._env: dict[str, dict[str, str]] = {}
        self._root_dir: Path = root_dir
        self._build_dir: Path = build_dir
        self._include_dirs: list[Path] = include_dirs

        # self._project_root: Path
        # self._project_sources: list[Path] = []
        # self._compile_cmd: list[str] = []

        # self._get_dev_env()

        # TODO create build/.cache/ 
        # store env_x64.json and 86 and mock.obj there
        # only build those once locally
        # gitignore build or .cache
        # maybe only store?:
        # PATH
        # INCLUDE
        # LIB
        # LIBPATH
        self._env['x64'] = self._get_dev_env("x64")
        # self._env['x86'] = self._get_dev_env("x86")

    def _get_dev_env(self, arch: Arch) -> dict[str, str]:
        print("Generating build environment...")

        launch = r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1"

        cmd = (
            f"& '{launch}' -Arch {'amd64' if arch == 'x64' else arch}; "
            "Get-ChildItem Env: | ForEach-Object { \"$($_.Name)=$($_.Value)\" }" 
        )

        result = subprocess.run(
            ["powershell.exe", "-NoProfile", "-Command", cmd],
            capture_output=True,
            text=True,
            check=True
        )

        env = {} # os.environ.copy()

        for line in result.stdout.splitlines():
            if '=' in line:
                k, v = line.split('=', 1)
                env[k.upper()] = v

        return env

    def _get_project_path(self, proj_name: str) -> Path:
        proj_name = proj_name.casefold()

        for entry in self._root_dir.iterdir():
            if entry.is_dir() and entry.name.casefold() == proj_name:
                return entry

        raise FileNotFoundError(f"'{proj_name}' not found in '{self._root_dir}'")

    def _get_source_files(self, project: Path) -> list[Path]:
        sources: list[Path] = []
        
        for file in project.iterdir():
            if not file.is_file():
                continue
    
            if file.suffix.lower() in (".c", ".cpp"):
                sources.append(file)
    
        if not sources:
            raise FileNotFoundError(f"No C/C++ sources found in {project}")
    
        return sources

    def _get_compile_cmd(self,
                         sources: list[Path],
                         includes: list[Path],
                         output_dir: Path,
                         configuration: Config) -> list[str]:
        
        cmd: list[str] = ["cl", "/nologo", "/c", "/GS-", "/EHsc", "/std:c++20"]
        cmd += [f"/I{str(x)}" for x in includes]
        cmd += [f"/Fo{output_dir}\\", f"/Fd{output_dir}\\"]
        
        if configuration == "debug":
            cmd += ["/D_DEBUG", "/Zi", "/MTd"]
            sources.append(Path(r"common\base\mock.cpp"))
        else:
            cmd += ["/Od", "/W0", "/MT"]

        cmd += [str(x) for x in sources]

        return cmd

    def _get_link_cmd(self,
                      file_name: str,
                      obj_dir: Path,
                      output: Path,
                      configuration: Config) -> list[str]:

        obj_files = list(obj_dir.glob("*.obj"))

        if not obj_files:
            raise FileNotFoundError(f"No object files found in {obj_dir}")

        if configuration == 'debug':
            cmd = ["link", "/DEBUG", f"/PDB:{output}\\{file_name}.pdb", f"/OUT:{output}\\{file_name}.exe"]
        else:
            cmd = ["link", "/lib", f"/out:{output}",]

        cmd += [str(x) for x in obj_files]

        return cmd

    def _validate_entry_source(self, project: Path, sources: list[Path], configuration: Config):
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
            if configuration == 'debug':
                raise FileNotFoundError(
                    f"Compiling in debug requires {project.name}.cpp but only {project.name}.c source was found."
                )

            return

        raise FileNotFoundError(f"{project.name}.cpp or {project.name}.c not found in {project}")

    def build(self,
              proj_name: str,
              configuration: Config,
              architecture: Arch) -> str | None:

        project: Path
        build_root: Path = Path("build")
        output_root: Path
        object_dir: Path
        debug_build_path: Path
        sources: list[Path] = []
        includes: list[Path] = self._include_dirs
        compile_cmd: list[str] = []
        link_cmd: list[str] = []

        try:
            project = self._get_project_path(proj_name)
        except FileNotFoundError as err:
            # TODO
            return str(err)
        
        try:
            sources = self._get_source_files(project)
        except FileNotFoundError as err:
            # TODO
            return str(err)

        try:
            self._validate_entry_source(project, sources, configuration)
        except FileNotFoundError as err:
            # TODO
            return str(err)

        
        output_root = build_root / configuration / project.name
        object_dir = output_root / architecture
        debug_build_path = build_root / configuration / architecture

        build_root.mkdir(parents=True, exist_ok=True)
        output_root.mkdir(parents=True, exist_ok=True)
        object_dir.mkdir(parents=True, exist_ok=True)
        # TODO only make relevent dirs?
        debug_build_path.mkdir(parents=True, exist_ok=True)

        includes.append(project)

        compile_cmd = self._get_compile_cmd(sources, includes, object_dir, configuration)
        # build
        try:
            subprocess.run(
                ["powershell.exe", "-NoProfile", "-Command", ' '.join(compile_cmd)], 
                # cwd=proj,
                capture_output=True,
                check=True,
                text=True,
                env=self._env[architecture]
            )
        except subprocess.CalledProcessError as err:
            return str(err.output)
        except FileNotFoundError as err:
            return str(err)

        try:
            link_cmd = self._get_link_cmd(
                proj_name.casefold(),
                object_dir,
                debug_build_path,
                configuration
            )
        except FileNotFoundError as err:
            return str(err)

        # link
        try:
            subprocess.run(
                ["powershell.exe", "-NoProfile", "-Command", ' '.join(link_cmd)], 
                # cwd=proj,
                capture_output=True,
                check=True,
                text=True,
                env=self._env[architecture]
            )
        except subprocess.CalledProcessError as err:
            return str(err.output)


def clean() -> None:
    if BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)

def build_projects(projects: list[str], arch: Arch, config: Config):
    
    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    builder: BuildSystem = BuildSystem(PROJ_ROOT, BUILD_DIR, includes)

    print("Building BOFs")

    for p in projects_list:
        print(f"{p} ... ", end='', flush=True)

        if res := builder.build(p, config, arch):
            BUILD_ERRS[p] = res
            print("FAIL")
        else:
            print("SUCCESS")

def compile_scripts():
    pass

def print_error_report() -> None:
    print("Build errors")

    for k, v in BUILD_ERRS.items():
        print(f"\n{k}\n{'-'*20}\n{v}")

if __name__ == "__main__":
    projects_list: list[str] = []
    scripts_only: bool = False

    includes: list[Path] = [
        Path("common"),
    ]

    parser = argparse.ArgumentParser()

    project_choices = sorted(
        entry.name
        for entry in PROJ_ROOT.iterdir()
        if entry.is_dir()
    )

    project_lookup = {p.casefold(): p  for p in project_choices}

    parser.add_argument("projects", nargs="*",
        help="Projects to build. If omitted, all projects are built."
    )

    parser.add_argument("-s", "--scripts-only", action="store_true",
                        help="Skip compiling C projects and only combine CNA scripts.")

    parser.add_argument("-a", "--arch", choices=['x64', 'x86'], required=False)

    parser.add_argument("-c", "--config", choices=['clean', 'release', 'debug'], required=True)

    args = parser.parse_args()

    invalid = [p for p in args.projects if p.casefold() not in project_lookup]

    if invalid:
        parser.error(
            f"Unknown project(s): {', '.join(invalid)}. "
            f"Valid projects: {', '.join(project_choices)}"
        )

    if args.config in ("debug", "release") and args.arch is None:
        parser.error("--arch is required for debug and release builds.")
    

    # TODO -s overrides all building
    # TODO no scripts version?
    # TODO delete the build folder for the debug objs when done?

    if args.projects:
        projects_list = args.projects
    else:
        projects_list = project_choices.copy()

    if args.config == 'clean':
        clean()
        sys.exit(0)

    if not args.scripts_only:
        build_projects(projects_list, args.arch, args.config)

    # TODO: compile scripts
        
    if BUILD_ERRS:
       print_error_report()
       sys.exit(1)

    sys.exit(0)
