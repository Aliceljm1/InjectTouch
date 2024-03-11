# InjectTouch
## 介绍
- 模拟触控事件，支持输入触控点数据文件批量模拟。
- 白板代码生成txt文件供此项目使用
- 文本规则
  1. LeftDown 683,482 0: 左键点击，点为(683,482)，当前笔迹ID为0
  2. MoveTo 683,482 2: 2号笔迹移动到(683,482)
  3. Delay 5: 当前行延迟5毫秒

## 使用
```powershell
InjectTouch.exe [笔迹文件路径] [开启的笔迹数] [缩放方式]
InjectTouch.exe ..\zoomout.txt 3 zoomin
```
- 项目中txt文本只提供了三指数据，如果开启的笔迹数量超过3，仍然只会绘制3条
- 缩放方式为`normal`,`zoomin`,`zoomout`;`zoomin`待实现

## 参考资料：
https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-injecttouchinput
https://github.com/hecomi/uTouchInjection