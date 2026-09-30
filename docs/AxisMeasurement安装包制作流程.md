# AxisMeasurement 安装包制作流程

本文档用于把 `G:\AxisMeasurement\_github_upload` 中的协同开发版本整理成普通 Windows 安装包：

```text
AxisMeasurement_Setup_1.0.exe
```

目标效果是：用户只需要下载一个安装程序，双击安装后即可通过桌面或开始菜单快捷方式启动软件。

注意：`.gitignore` 只控制 GitHub 协同仓库中哪些文件不上传，不等于发布规则。发布包必须包含程序运行所需的 DLL、配置、模板、模型和硬件配置文件。

## 1. 总体流程

完整流程分三步：

```text
1. Visual Studio 编译 Release | x64
2. 整理干净的 publish 运行目录
3. 使用 Inno Setup 把 publish 目录打包成安装程序
```

不要直接打包整个 `_github_upload` 目录，也不要直接打包整个 `x64\Release` 目录。

## 2. 准备环境

需要具备：

```text
Visual Studio 2019
Qt 5.14.2 msvc2017_64
Qt Visual Studio Tools
Inno Setup
HALCON 18.11 运行环境/许可
OpenCV 运行 DLL
相机、运动控制卡、LS9000 等硬件 SDK 运行 DLL
```

项目目录：

```text
G:\AxisMeasurement\_github_upload
```

建议发布目录：

```text
G:\AxisMeasurement\_github_upload\publish\AxisMeasurement_1.0
```

建议安装包输出目录：

```text
G:\AxisMeasurement\_github_upload\installer\output
```

## 3. 编译 Release 程序

1. 打开：

```text
G:\AxisMeasurement\_github_upload\AxisMeasurement.sln
```

2. Visual Studio 顶部选择：

```text
Release | x64
```

3. 执行：

```text
生成 -> 清理解决方案
生成 -> 重新生成解决方案
```

4. 确认生成：

```text
G:\AxisMeasurement\_github_upload\x64\Release\AxisMeasurement.exe
```

如果没有生成 exe，说明编译未成功，先解决编译错误，不能继续打包。

## 4. 创建干净 publish 目录

以下命令在 Windows CMD 中执行。

打开 CMD：

```text
Win + R -> 输入 cmd -> 回车
```

进入项目目录并重建发布目录：

```bat
cd /d G:\AxisMeasurement\_github_upload

rmdir /S /Q publish\AxisMeasurement_1.0
mkdir publish\AxisMeasurement_1.0
```

如果 `rmdir` 提示目录不存在，可以忽略。

## 5. 复制程序和运行文件

先复制主程序：

```bat
copy /Y x64\Release\AxisMeasurement.exe publish\AxisMeasurement_1.0\
```

推荐从 `x64\Release` 复制当前能运行的整套 DLL，避免手动混入错误 Qt 版本：

```bat
copy /Y x64\Release\*.dll publish\AxisMeasurement_1.0\
```

复制 Qt 插件目录：

```bat
xcopy /E /I /Y x64\Release\platforms publish\AxisMeasurement_1.0\platforms
xcopy /E /I /Y x64\Release\imageformats publish\AxisMeasurement_1.0\imageformats
xcopy /E /I /Y x64\Release\iconengines publish\AxisMeasurement_1.0\iconengines
xcopy /E /I /Y x64\Release\styles publish\AxisMeasurement_1.0\styles
xcopy /E /I /Y x64\Release\sqldrivers publish\AxisMeasurement_1.0\sqldrivers
xcopy /E /I /Y x64\Release\translations publish\AxisMeasurement_1.0\translations
```

复制配置、模板和硬件配置：

```bat
xcopy /E /I /Y config publish\AxisMeasurement_1.0\config
xcopy /E /I /Y programParmeter publish\AxisMeasurement_1.0\programParmeter

copy /Y RingNetDmaCfg.rndma publish\AxisMeasurement_1.0\
copy /Y RingNetMapSUB6.rnmap publish\AxisMeasurement_1.0\
copy /Y test1.cfg publish\AxisMeasurement_1.0\
```

说明：

- `config` 必须包含主题、图标、登录背景等运行资源。
- `programParmeter` 包含 `.sbm`、`.hdev`、模板、粗糙度模型等，不要随意删除。
- 如果现场需要粗糙度功能，必须保留：

```text
programParmeter\roughness\*.h5
```

这些 `.h5` 模型虽然被 `.gitignore` 排除了，但运行发布包可能需要。

## 6. 可选：使用 windeployqt 补齐 Qt 文件

如果发布目录缺 Qt 插件，可以使用：

```bat
D:\Qt\Qt5.14.2\5.14.2\msvc2017_64\bin\windeployqt.exe G:\AxisMeasurement\_github_upload\publish\AxisMeasurement_1.0\AxisMeasurement.exe
```

注意必须使用项目实际编译对应的 Qt：

```text
Qt 5.14.2 msvc2017_64
```

不要使用 MinGW 版 Qt，也不要使用其它 Qt 版本。

## 7. publish 目录最终结构

最终目录大致如下：

```text
publish\AxisMeasurement_1.0\
  AxisMeasurement.exe

  Qt5Core.dll
  Qt5Gui.dll
  Qt5Widgets.dll
  Qt5Sql.dll
  Qt5Svg.dll

  opencv_world470.dll

  halcon.dll
  halconxl.dll

  GxIAPI.dll
  GxIAPICPPEx.dll
  GCBase_MD_VC120_v3_0.dll
  GenApi_MD_VC120_v3_0.dll
  Log_MD_VC120_v3_0.dll
  MathParser_MD_VC120_v3_0.dll
  NodeMapData_MD_VC120_v3_0.dll
  XmlParser_MD_VC120_v3_0.dll

  gts.dll
  gt_rn.dll
  LS9_IF.dll

  D3Dcompiler_47.dll
  libEGL.dll
  libGLESv2.dll
  opengl32sw.dll

  RingNetDmaCfg.rndma
  RingNetMapSUB6.rnmap
  test1.cfg

  config\
    theme.qss
    logo.ico
    login_backgroud.jpg

  programParmeter\
    ...
    roughness\
      331roughness.py
      model_best_weights.h5
      model_best_weights2.h5
      mymodel.h5

  platforms\
    qwindows.dll

  imageformats\
  iconengines\
  styles\
  sqldrivers\
  translations\
```

## 8. 不要放进 publish 的内容

以下内容不应进入发布包：

```text
.git\
.vs\
.agents\
.codex-tmp-generated\
.codex-tmp-spreadsheet\
.workbuddy\
myCode\
inc\
lib\
docs\
tests\
0418\
备用\
*_backup_*\
*backup*\
x64\Release\*.obj
x64\Release\*.pdb
x64\Release\*.ilk
x64\Release\*.tlog
x64\Release\*.log
x64\Release\moc\
x64\Release\uic\
x64\Release\rcc\
x64\Release\qt\
AxisMeasurement.sln
AxisMeasurement.vcxproj
AxisMeasurement.vcxproj.filters
*.cpp
*.h
*.ui
*.qrc
```

## 9. 测试 publish 目录

在制作安装包之前，必须先测试：

```text
G:\AxisMeasurement\_github_upload\publish\AxisMeasurement_1.0\AxisMeasurement.exe
```

检查：

```text
1. 能启动
2. 登录界面正常
3. UI 主题正常
4. 图片、图标正常
5. 不接硬件时不直接崩溃
6. 接硬件时相机、运动控制卡、光幕能打开
7. programParmeter 模板能找到
8. HALCON 许可正常
9. 粗糙度功能能找到模型
```

如果 publish 目录不能直接运行，安装包也不会正常运行。

## 10. 使用 Inno Setup 制作安装包

打开 Inno Setup Script Wizard 后，各页面按下面选择。

### 10.1 Application Information

建议：

```text
Application name: AxisMeasurement
Application version: 1.0
Application publisher: AxisMeasurement
Application website: 可留空
```

### 10.2 Application Folder

建议不要安装到 `Program Files`，因为程序可能需要写入测量数据、日志、报告、配置等。

推荐最终脚本使用：

```ini
DefaultDirName={localappdata}\AxisMeasurement
```

向导里如果不好选 `{localappdata}`，先继续生成脚本，之后手动修改 `.iss` 文件。

设置建议：

```text
Application folder name: AxisMeasurement
Allow user to change the application folder: 勾选
The application doesn't need a folder: 不勾选
```

### 10.3 Application Files

主程序选择：

```text
G:\AxisMeasurement\_github_upload\publish\AxisMeasurement_1.0\AxisMeasurement.exe
```

勾选：

```text
Allow user to start the application after Setup has finished
```

不要勾选：

```text
The application doesn't have a main executable file
```

Other application files：

```text
Add folder...
```

选择：

```text
G:\AxisMeasurement\_github_upload\publish\AxisMeasurement_1.0
```

如果询问是否包含子目录，选择 `Yes`。

### 10.4 Application File Association

当前软件不需要注册专用文件类型。

设置：

```text
Associate a file type to the main executable: 不勾选
```

### 10.5 Application Shortcuts

建议：

```text
Create a shortcut to the main executable in the Start Menu Programs folder: 勾选
Application Start Menu folder name: AxisMeasurement
Allow user to create a desktop shortcut: 勾选
```

其它选项保持不勾选即可。

### 10.6 Application Documentation

如果没有许可证或安装说明文件，全部留空，直接下一步。

后续正式交付时可以增加说明：

```text
HALCON 许可要求
硬件驱动要求
运动安全检查
```

### 10.7 Setup Install Mode

建议选择：

```text
Non administrative install mode (install for current user only)
```

不要勾选：

```text
Allow user to override the install mode via the command line
Ask the user to choose the install mode at startup
```

### 10.8 Registry Keys And Values

不导入 `.reg` 文件：

```text
Windows registry file (.reg) to import: 留空
```

保持：

```text
Delete keys which are empty on uninstall: 勾选
Delete values on uninstall: 勾选
```

不要勾选：

```text
Also delete keys which are not empty
Create only if Windows' version is at least
```

### 10.9 Compiler Settings

建议：

```text
Custom compiler output folder:
G:\AxisMeasurement\_github_upload\installer\output

Compiler output base file name:
AxisMeasurement_Setup_1.0

Custom Setup icon file:
G:\AxisMeasurement\_github_upload\config\logo.ico

Setup password:
留空
```

最终输出：

```text
G:\AxisMeasurement\_github_upload\installer\output\AxisMeasurement_Setup_1.0.exe
```

### 10.10 Wizard Style

保持默认：

```text
modern
dynamic
default
```

### 10.11 Inno Setup Preprocessor

保持勾选：

```text
Yes, use #define compiler directives
```

这样生成的 `.iss` 脚本更好维护。

## 11. 推荐检查生成的 .iss 脚本

向导完成后，建议检查 `.iss` 中这些关键项。

安装目录建议是：

```ini
DefaultDirName={localappdata}\AxisMeasurement
PrivilegesRequired=lowest
```

文件复制应包含整个 publish 目录：

```ini
[Files]
Source: "G:\AxisMeasurement\_github_upload\publish\AxisMeasurement_1.0\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
```

快捷方式建议带工作目录：

```ini
[Icons]
Name: "{group}\AxisMeasurement"; Filename: "{app}\AxisMeasurement.exe"; WorkingDir: "{app}"
Name: "{commondesktop}\AxisMeasurement"; Filename: "{app}\AxisMeasurement.exe"; WorkingDir: "{app}"; Tasks: desktopicon
```

安装后启动：

```ini
[Run]
Filename: "{app}\AxisMeasurement.exe"; Description: "启动 AxisMeasurement"; Flags: nowait postinstall skipifsilent
```

## 12. 编译安装包

在 Inno Setup Compiler 中：

```text
Build -> Compile
```

成功后得到：

```text
G:\AxisMeasurement\_github_upload\installer\output\AxisMeasurement_Setup_1.0.exe
```

用户最终只需要这个安装包。

## 13. 安装包测试

建议找一台尽量干净的电脑测试。

测试顺序：

```text
1. 双击 AxisMeasurement_Setup_1.0.exe
2. 安装到默认目录
3. 勾选桌面快捷方式
4. 安装完成后启动
5. 检查登录界面、UI 主题、图片图标
6. 检查 programParmeter 是否存在
7. 检查 HALCON 许可是否正常
8. 检查相机、运动控制卡、光幕设备
9. 尝试一个低风险流程
```

安装后默认路径类似：

```text
C:\Users\用户名\AppData\Local\AxisMeasurement
```

## 14. 常见问题与解决方式

### 14.1 找不到 `halconcpp.dll`

现象：

```text
系统找不到指定的文件
copy /Y x64\Release\halconcpp.dll ...
```

原因：

当前项目目录里可能没有 `halconcpp.dll`，实际存在的是：

```text
x64\Release\halcon.dll
x64\Release\halconxl.dll
```

处理：

跳过 `halconcpp.dll`，复制：

```bat
copy /Y x64\Release\halcon.dll publish\AxisMeasurement_1.0\
copy /Y x64\Release\halconxl.dll publish\AxisMeasurement_1.0\
```

如果目标电脑运行时明确提示缺 `halconcpp.dll`，再从 HALCON 安装目录中查找与版本匹配的 DLL。

### 14.2 启动时报 `无法定位程序输入点 QAbstractItemView::eventFilter`

现象：

```text
无法定位程序输入点 ... QAbstractItemView::eventFilter ...
```

原因：

`AxisMeasurement.exe` 加载到的 Qt DLL 与编译时 Qt 版本不一致，常见是 `Qt5Widgets.dll` 混用了旧版本或其它 Qt 版本。

处理：

删除发布目录，重新从 `x64\Release` 复制同一套 DLL：

```bat
rmdir /S /Q publish\AxisMeasurement_1.0
mkdir publish\AxisMeasurement_1.0
copy /Y x64\Release\AxisMeasurement.exe publish\AxisMeasurement_1.0\
copy /Y x64\Release\*.dll publish\AxisMeasurement_1.0\
```

不要从不同 Qt 安装目录手动混拷 DLL。

### 14.3 启动时报 `无法定位程序输入点 QHighDpiScaling::scaleAndOrigin`

现象：

```text
无法定位程序输入点 ... QHighDpiScaling::scaleAndOrigin ...
```

原因：

`Qt5Gui.dll` 版本不匹配，通常是发布目录中的 Qt DLL 不是 `Qt 5.14.2 msvc2017_64` 这一套。

处理：

同样重新复制 `x64\Release\*.dll`，或者用正确路径的 `windeployqt`：

```bat
D:\Qt\Qt5.14.2\5.14.2\msvc2017_64\bin\windeployqt.exe G:\AxisMeasurement\_github_upload\publish\AxisMeasurement_1.0\AxisMeasurement.exe
```

确认 `Qt5Core.dll`、`Qt5Gui.dll`、`Qt5Widgets.dll`、`platforms\qwindows.dll` 都来自同一套 Qt。

### 14.4 `AxisMeasurement.exe` 单独复制后打不开

原因：

该项目不是单文件程序，依赖 Qt、OpenCV、HALCON、硬件 SDK、配置和模板。

处理：

必须运行整个发布目录：

```text
publish\AxisMeasurement_1.0\
```

不能只发一个 `AxisMeasurement.exe`。

如果要让用户只下载一个 exe，应下载 Inno Setup 生成的：

```text
AxisMeasurement_Setup_1.0.exe
```

### 14.5 安装到 Program Files 后无法写入数据

现象：

程序无法保存测量数据、日志、报告或配置。

原因：

`C:\Program Files` 对普通用户写入受限。

处理：

Inno Setup 安装目录使用：

```ini
DefaultDirName={localappdata}\AxisMeasurement
PrivilegesRequired=lowest
```

这样默认安装到：

```text
C:\Users\用户名\AppData\Local\AxisMeasurement
```

### 14.6 缺少 `VCRUNTIME*.dll` 或 `MSVCP*.dll`

原因：

目标电脑没有安装 Visual C++ 运行库。

处理：

安装：

```text
Microsoft Visual C++ Redistributable 2015-2022 x64
```

### 14.7 主题或图片不生效

原因：

程序运行时读取 exe 同级目录：

```text
config\theme.qss
config\logo.ico
config\login_backgroud.jpg
```

处理：

确认发布目录或安装目录中存在：

```text
config\
```

并且内容是最新的。

### 14.8 算法模板找不到

原因：

缺少：

```text
programParmeter\
```

处理：

发布目录必须复制：

```bat
xcopy /E /I /Y programParmeter publish\AxisMeasurement_1.0\programParmeter
```

### 14.9 HALCON 许可错误

现象：

例如：

```text
HALCON error #2042: Feature has expired
```

原因：

目标电脑的 HALCON 许可不可用、过期或许可环境未配置。

处理：

这不是安装包文件缺失能完全解决的问题。需要确认：

```text
HALCON 18.11 许可
许可文件或加密狗
HALCON 运行环境
系统日期
```

恢复后再测试软件。

## 15. 后续更新版本流程

每次发布新版本按以下步骤：

```text
1. 确认 GitHub 代码最新
2. Visual Studio 选择 Release | x64
3. 清理解决方案
4. 重新生成解决方案
5. 重建 publish\AxisMeasurement_版本号
6. 双击 publish 中的 AxisMeasurement.exe 测试
7. 用 Inno Setup 编译 .iss
8. 生成 AxisMeasurement_Setup_版本号.exe
9. 在干净电脑安装测试
```

## 16. 核心原则

```text
GitHub 仓库 = 源码协同，尽量轻
publish 目录 = 程序运行，必须完整
Inno Setup 输出 = 给用户下载的单个安装 exe
```

不能把 `.gitignore` 当成发布规则。被 Git 忽略的文件，例如 `.h5`、`.bmp`、`x64`，如果程序运行需要，就必须进入发布目录或由安装包携带。
