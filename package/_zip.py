import os
import shutil
import sys
from enum import Enum
from _copy_dll import copy_dll


class MODE(Enum):
    RELEASE = 'Release'
    DEBUG = 'Debug'


mode = MODE.RELEASE
out_dir = r"./out"
ini_dir = r"../ini/"
points_set_dir = r"../points_set/"
exe_path = r'../build/?/InjectTouch.exe'

if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == MODE.DEBUG.value:
        mode = MODE.DEBUG

    exe_path = exe_path.replace("?", mode.value)
    if os.path.exists(exe_path):
        out_dir = out_dir + '_' + mode.value + '/'
        if os.path.exists(out_dir):
            shutil.rmtree(out_dir)

        os.makedirs(out_dir)
        shutil.copytree(ini_dir, out_dir + 'ini')
        shutil.copy2(exe_path, out_dir)
        shutil.copytree(points_set_dir, out_dir + 'points_set')

        copy_dll(out_dir + os.path.basename(exe_path))

        shutil.make_archive('InjectTouch_' + mode.value + '_x64', 'zip', out_dir)
    else:
        print('文件不存在 ' + exe_path)
