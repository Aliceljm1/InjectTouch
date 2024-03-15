import os
import shutil
import sys

from _copy_dll import copy_dll

out_dir = r"./out/"
ini_dir = r"../ini/"
points_set_dir = r"../points_set/"
exe_path = r'../build/Release/InjectTouch.exe'

if __name__ == '__main__':
    if os.path.exists(out_dir):
        shutil.rmtree(out_dir)

    os.makedirs(out_dir)
    shutil.copytree(ini_dir, out_dir + 'ini')
    shutil.copy2(exe_path, out_dir)
    shutil.copytree(points_set_dir, out_dir + 'points_set')

    copy_dll(out_dir + os.path.basename(exe_path))

    shutil.make_archive('InjectTouch', 'zip', out_dir)
