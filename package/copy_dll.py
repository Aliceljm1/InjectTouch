import os
import shutil
import subprocess

# 定义命令和参数
cmd = 'C:\\Users\\62453\\Documents\\Tools\\depends\\Dependencies_x64_Release\\Dependencies.exe'
arg1 = '-modules'
arg2 = 'C:\\Users\\62453\\Desktop\\InjectTouch\\x64\\Release\\InjectTouch.exe'
arg3 = '-depth=1'

# 使用subprocess.run执行命令
result = subprocess.run([cmd, arg1, arg2, arg3], capture_output=True, text=True)

# 获取命令的输出
output = result.stdout

# 按行分割输出
lines = output.split('\n')

# 创建一个set来存储路径
paths = set()

# 对于每一行，找到第一个 : 字符的位置，并提取出它后面的dll路径
for line in lines:
    pos = line.find(':')
    if pos != -1:
        dll_path = line[pos+1:].strip()
        if dll_path.endswith(".dll"):
            paths.add(dll_path)

# 获取arg2文件所在的目录
dest_dir = os.path.dirname(arg2)

# 遍历set并复制所有的dll文件到arg2文件所在的目录
for path in paths:
    if os.path.isfile(path):
        dest_path = os.path.join(dest_dir, os.path.basename(path))
        if path != dest_path:
            shutil.copy2(path, dest_path)