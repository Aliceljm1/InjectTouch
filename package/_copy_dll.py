import os
import shutil
import subprocess

dependencies_path = '.\\dependencies\\Dependencies.exe'


def execute_cmd(exe_path):
    cmd = dependencies_path
    arg1 = exe_path
    arg2 = '-modules'
    arg3 = '-depth=1'
    result = subprocess.run([cmd, arg1, arg2, arg3], capture_output=True, text=True)
    return result


def copy_dll(exe_path):
    output = execute_cmd(exe_path).stdout
    lines = output.split('\n')
    paths = set()
    for line in lines:
        pos = line.find(':')
        if pos != -1:
            dll_path = line[pos + 1:].strip()
            if dll_path.endswith(".dll"):
                paths.add(dll_path)

    dest_dir = os.path.dirname(exe_path)

    for path in paths:
        if os.path.isfile(path):
            dest_path = os.path.join(dest_dir, os.path.basename(path))
            if path != dest_path:
                shutil.copy2(path, dest_path)
