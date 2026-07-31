import argparse
import subprocess
from pathlib import Path
from typing import Final, Literal
import os
import sys

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

        self._env['x64'] = self._get_dev_env("amd64")
        self._env['x86'] = self._get_dev_env("x86")

    def _get_dev_env(self, arch: Literal['amd64', 'x86']) -> dict[str, str]:
        cmd: str = (
            rf'"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" '
            rf'-arch={arch} && set'
        )

        result = subprocess.run(['cmd', '/c', cmd], capture_output=True, text=True, check=True)

        env = os.environ.copy()

        for line in result.stdout.splitlines():
            if '=' in line:
                k, v = line.split('=', 1)
                env[k] = v

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
                         configuration: Literal['debug', 'release']) -> list[str]:
        
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
                      obj_dir: Path,
                      output: Path,
                      configuration: Literal['debug', 'release']) -> list[str]:

        obj_files = list(obj_dir.glob("*.obj"))

        if not obj_files:
            raise FileNotFoundError(f"No object files found in {obj_dir}")

        if configuration == 'debug':
            cmd = ["link", "/DEBUG", f"/PDB:{output.with_suffix('.pdb')}", f"/OUT:{output}"]
        else:
            cmd = ["link", "/lib", f"/out:{output}",]

            cmd += [str(x) for x in obj_files]

        return cmd

    def _validate_entry_source(self, project: Path, sources: list[Path], configuration: Literal['debug', 'release']):
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
              configuration: Literal['debug', 'release'],
              platform: Literal['x64', 'x86']) -> str | None:

        project: Path
        build_root: Path = Path("build")
        output_root: Path
        object_dir: Path
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
        object_dir = output_root / platform

        build_root.mkdir(parents=True, exist_ok=True)
        output_root.mkdir(parents=True, exist_ok=True)
        object_dir.mkdir(parents=True, exist_ok=True)

        includes.append(project)

        compile_cmd = self._get_compile_cmd(sources, includes, object_dir, configuration)

        # build
        try:
            subprocess.run(
                compile_cmd, 
                # cwd=proj,
                capture_output=True,
                check=True,
                text=True,
                env=self._env[platform]
            )
        except subprocess.CalledProcessError as err:
            return str(err.output)

        try:
            link_cmd = self._get_link_cmd(object_dir, build_root / configuration / platform, configuration)
        except FileNotFoundError as err:
            return str(err)

        # link
        try:
            subprocess.run(
                link_cmd, 
                # cwd=proj,
                capture_output=True,
                check=True,
                text=True,
                env=self._env[platform]
            )
        except subprocess.CalledProcessError as err:
            return str(err.output)



        

# def get_dev_env(self, arch: Literal['amd64', 'x86'] = 'amd64') -> dict[str, str]:
#         cmd: str = (
#             rf'"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" '
#             rf'-arch={arch} && set'
#         )

#         result = subprocess.run(['cmd', '/c', cmd], capture_output=True, text=True, check=True)

#         env = os.environ.copy()

#         for line in result.stdout.splitlines():
#             if '=' in line:
#                 k, v = line.split('=', 1)
#                 env[k] = v

#         return env

# def get_bof_path(bof_name: str) -> Path:
#     bof_name = bof_name.casefold()

#     for entry in BOFS_DIR.iterdir():
#         if entry.is_dir() and entry.name.casefold() == bof_name:
#             return entry

#     raise FileNotFoundError(f"'{bof_name}' not found in '{BOFS_DIR}'")

# def get_source_files(bof_dir: Path) -> list[Path]:
#     sources: list[Path] = []

#     for file in bof_dir.iterdir():
#         if not file.is_file():
#             continue

#         if file.suffix.lower() in (".c", ".cpp"):
#             sources.append(file)

#     if not sources:
#         raise FileNotFoundError(f"No C/C++ sources found in {bof_dir}")

#     return sources

# def get_compile_cmd(bof_dir:Path, arch:Literal['x64', 'x86'], debug:bool) -> list[str]:

#     sources = get_source_files(bof_dir).copy()
#     compile_sources = sources.copy()

#     output_root: Path = BUILD_DIR / (DEBUG if debug else RELEASE) / arch
#     obj_dir: Path = output_root / bof_dir.name

#     obj_dir.mkdir(parents=True, exist_ok=True)

#     cmd: list[str] = [
#         "cl",
#         "/nologo",
#         "/c",
#         "/GS-",
#         "/EHsc",
#         "/std:c++20",
#         f"/I{COMMON_DIR}",
#         f"/I{BOFS_DIR / bof_dir.name}",
#         f"/Fo{obj_dir}\\",
#         f"/Fd{obj_dir}\\"
#     ]

#     if debug:
#         cmd += [
#             "/D_DEBUG",
#             "/Zi",
#             "/MTd"
#         ]
#         compile_sources.append(Path(r"common\base\mock.cpp"))
#     else:
#         cmd += [
#             "/Od",
#             "/W0",
#             "/MT"
#         ]

#     cmd += [str(x) for x in compile_sources]

#     return cmd

# def get_link_cmd(obj_dir: Path, output: Path, debug: bool) -> list[str]:

#     obj_files = list(obj_dir.glob("*.obj"))

#     if not obj_files:
#         raise FileNotFoundError(f"No object files found in {obj_dir}")

#     if debug:
#         cmd = [
#             "link",
#             "/DEBUG",
#             f"/PDB:{output.with_suffix('.pdb')}",
#             f"/OUT:{output}"
#         ]

#         cmd += [str(x) for x in obj_files]
#     else:
#         cmd = [
#             "link",
#             "/lib",
#             f"/out:{output}",
#         ]

#         cmd += [str(x) for x in obj_files]

#     return cmd

# def find_entry_source(directory: Path) -> tuple[Path, bool]:
#     stem: str = directory.name
#     dir_name: str = directory.name.casefold()

#     cpp_src: Path = directory / f"{stem}.cpp"
#     c_src: Path   = directory / f"{stem}.c"

#     if cpp_src.exists():
#         return cpp_src, True

#     if c_src.exists():
#         return c_src, False

#     # Case-insensitive search
#     c_match: Path | None = None

#     for file in directory.iterdir():
#         if (file.is_file() and file.stem.casefold() == dir_name):
#             ext: str  = file.suffix.casefold()

#             if ext == ".cpp":
#                 return file, True
            
#             if ext == ".c":
#                 c_match = file

#     if c_match is not None:
#         return c_src, False

#     raise FileNotFoundError(f"Expected '{stem}.cpp' or '{stem}.c' in '{directory}'")

# def build_bof(bof_name: str, env_vars: dict[str, str]):
#     bof_path: Path
#     src_file: Path
#     debugable: bool

#     try:
#         bof_path = get_bof_path(bof_name)
#     except FileNotFoundError as e:
#         # TODO print error?
#         return

#     try:
#         src_file, debugable = find_entry_source(bof_path)
#     except FileNotFoundError as e:
#         # TODO print
#         return


#     try:
#         subprocess.run(
#             CL_CMD + [source_file.name, f"/Fo{output_file}"], 
#             cwd=proj,
#             capture_output=True,
#             check=True,
#             text=True,
#             env=env_vars)
#     except subprocess.CalledProcessError as e:
#         print(f"\033[91mFAILED\033[0m")
#         return str(e.output)





# def build_project(proj: Path, script_only: bool = False) -> str:
#     global CNA_SCRIPT_CONTENTS

#     state: Literal['FAIL', 'SUCCESS', 'SKIPPED'] = 'SUCCESS'

#     source_file: Path = proj / f"{proj.name.lower()}.c"
#     script_file: Path = proj / f"{proj.name.lower()}.cna"
#     output_file: Path = (BUILD_DIR / f"{proj.name.lower()}.o").resolve()

#     missing_files: list[str] = []

#     script_contents: str = ""

#     err_output: str = ""

#     print(f"Building {proj.name:<30}", end='')

#     if not source_file.is_file():
#         err_output += f"Cannot find 'c' source file: {source_file.resolve()}\n"
#         missing_files.append(source_file.name)
#         state = 'SKIPPED'

#     if not script_file.is_file():
#         err_output += f"Cannot find 'cna' script file: {script_file.resolve()}\n"
#         missing_files.append(script_file.name)
#         state = 'SKIPPED'

#     if state == 'SKIPPED':
#         print(f"\033[93m{state} \033[90mFiles not found: {missing_files}\033[0m")
#         return err_output

#     if script_only == False:
#         # Build the project with cl.exe
#         try:
#             vs_env = get_dev_env()
            
#             subprocess.run(
#                 CL_CMD + [source_file.name, f"/Fo{output_file}"], 
#                 cwd=proj,
#                 capture_output=True,
#                 check=True,
#                 text=True,
#                 env=vs_env)
#         except subprocess.CalledProcessError as err:
#             print(f"\033[91mFAILED\033[0m")
#             return str(err.output)


#     # Get the contents of the script file
#     script_contents = script_file.read_text(encoding='utf-8')

#     CNA_SCRIPT_CONTENTS += script_contents + "\n\n"

#     print("\033[92mSUCCESS\033[0m" + (" - script only" if script_only else ""))

#     return err_output

# def build_all(scripts_only: bool = False) -> dict[str, str]:
#     build_errs: dict[str, str] = {}

#     for proj in ROOT_DIR.iterdir():
#         if not proj.is_dir():
#             continue

#         err = build_project(proj, scripts_only)

#         if err:
#             build_errs[proj.name] = err

#     return build_errs


if __name__ == "__main__":

    build_dir: Path = Path("build")
    project_root: Path = Path("bofs")
    build_errs: dict[str, str] = {}

    projects_list: list[str] = []
    scripts_only: bool = False

    includes: list[Path] = [
        Path("common"),
    ]

    build_dir.mkdir(parents=True, exist_ok=True)

    builder = BuildSystem(project_root, build_dir, includes)

    parser = argparse.ArgumentParser()

    project_choices = sorted(
        entry.name
        for entry in project_root.iterdir()
        if entry.is_dir()
    )

    parser.add_argument("projects", nargs="*", choices=project_choices,
        help="Projects to build. If omitted, all projects are built."
    )

    parser.add_argument("-s", "--scripts-only", action="store_true",
                        help="Skip compiling C projects and only combine CNA scripts.")

    parser.add_argument("-a", "--arch", choices=['x64', 'x86'], required=True)

    parser.add_argument("-c", "--config", choices=['release', 'debug'], required=True)

    args = parser.parse_args()

    # TODO -s overrides all building
    # TODO no scripts version?

    if args.projects:
        projects_list = args.projects
    else:
        projects_list = project_choices.copy()


    for p in projects_list:
        # TODO print building p ... result SUCCESS SKIP FAIL
        # how to determine skip? maybe not necessary since argparse gathering the directories
        if res := builder.build(p, args.config, args.arch):
            build_errs[p] = res
            # TODO print FAIL
        else:
            # TODO print SUCCESS
            pass

    # TODO: compile scripts
        
    # TODO: print build errors

    # BUILD_DIR.mkdir(parents=True, exist_ok=True)

    # CNA_SCRIPT_CONTENTS = Path("base.cna").read_text(encoding='utf-8')


    # parser.add_argument(
    #     "projects",
    #     nargs="*",
    #     help="Projects to build. If omitted, all projects are built."
    # )

    # parser.add_argument(
    #     "-s",
    #     "--scripts-only",
    #     action="store_true",
    #     help="Skip compiling C projects and only combine CNA scripts."
    # )

    # args = parser.parse_args()

    # if args.projects and args.scripts_only == False:
    #     projects = [ROOT_DIR / p for p in args.projects]
        
    #     for project in projects:
    #         if not project.exists() or not project.is_dir():
    #             print(f"Building {project.name:<30}"
    #                   f"\033[33mSKIPPED\033[0m \033[90m "
    #                   f"Directory not found: {project.resolve()}\033[0m")
    #             continue

    #         result = build_project(project)

    #         if result:
    #             build_errs[project.name] = result
    # else:
    #     build_errs = build_all(args.scripts_only)

    # if build_errs:
    #     print("**********************************************************************\n"
    #           "*                           Build Errors                             *\n"
    #           "**********************************************************************")
    #     for key, value in build_errs.items():
    #         print(f"> {key}\n{value}")
    #     sys.exit(1)

    # OUTPUT_SCRIPT_PATH.write_text(CNA_SCRIPT_CONTENTS, encoding='utf-8')

    sys.exit(0)
