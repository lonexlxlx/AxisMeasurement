# AxisMeasurement 界面优化方案

日期：2026-09-09 ｜ 范围：登录窗口 → 主窗口 → sdk_assist → 图形化二次开发编辑器

## 一、现状问题清单（按发现顺序）

| # | 问题 | 位置 | 影响 |
|---|---|---|---|
| 1 | 字体混用：默认宋体 116 处、`Agency FB` 11 处（英文展示字体，中文会回退）、微软雅黑仅 1 处 | 三个 .ui 文件 | 整体观感廉价、中文发虚 |
| 2 | 荧光色状态样式：`rgb(85,255,255)` 青、`rgb(255,255,127)` 黄、`rgb(170,255,0)` 黄绿、`rgb(255,0,0)` 纯红 | AxisMeasurement.ui 内 21 处 styleSheet | 刺眼、无语义规律，现场久看疲劳 |
| 3 | 分组框默认灰描边，12 个 QGroupBox 挤在一屏（1907×1018） | 主窗口 | 密度大、层次不清 |
| 4 | 34 个按钮全部默认样式，无主次、无图标 | 主窗口 | 关键操作（自动测量/急停）不突出，急停必须是全场最醒目的红色 |
| 5 | 表格默认样式，无斑马纹、表头原始 | 主窗口结果表、编辑器测量流程表 | 数据密集时难以逐行阅读 |
| 6 | 登录窗口 234 行 ui、无任何样式，logo 拉伸 | logIn.ui | 第一印象差 |
| 7 | 图形化编辑器工具栏纯文字按钮，无图标无分隔样式 | graphical_program_editor.cpp | 专业感不足（这是对外展示最多的窗口） |
| 8 | 窗口 1907×1018 写死，小于此分辨率的屏幕会出滚动条/裁切 | AxisMeasurement.ui geometry | 换屏幕适配性差 |

## 二、已完成（本次可直接生效）

### 1. 全局主题 `config/theme.qss`

新建 `G:\AxisMeasurement\config\theme.qss`（源文件）并已部署到运行目录 `x64\Release\config\theme.qss`。内容覆盖：统一微软雅黑字体、分组框卡片化（白底+圆角+蓝色标题）、按钮 hover/pressed/disabled 三态、输入框聚焦蓝框、表格斑马纹+浅蓝选中、进度条圆角、Tab 页下划线、细滚动条、工具按钮选中态。文件内还预留了两段注释掉的选择器：

- `QPushButton#primaryButton`（主操作蓝底白字）
- `QPushButton#dangerButton`（危险操作红底白字）

在代码里 `btn->setObjectName("dangerButton")` 即可启用，无需改样式表。

### 2. `main.cpp` 加载逻辑（已改）

```cpp
QApplication a(argc, argv);
//全局界面主题：运行目录 config/theme.qss，文件缺失时保持 Qt 默认样式（即回滚开关）
{
    QFile themeFile(QCoreApplication::applicationDirPath() + "/config/theme.qss");
    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text))
        a.setStyleSheet(QString::fromUtf8(themeFile.readAll()));
}
```

**生效方式**：Release|x64 重新编译 → F5。因为 `QApplication::setStyleSheet` 是全局的，登录窗口、主窗口、sdk_assist、图形化编辑器**全部自动换肤**，一处改动全局生效。

**回滚方式**：删除 `x64\Release\config\theme.qss` 即恢复原样，不用改代码。

**已知边界**：.ui 里 21 处控件级 styleSheet 优先级高于全局样式，会继续显示荧光色——这正好作为第三节的 P1 逐个替换对象。

## 三、建议改动清单（按优先级）

### P0｜半小时内，立竿见影 ✅ 已完成（2026-09-09）

> 实际改动记录：
> - **AxisMeasurement.ui**：删除 12 处 QGroupBox 灰框样式（让全局卡片样式生效）+ 8 处荧光色按钮样式（apexMoveUp/apexMoveDown 黄绿、partRotate_clockwise/anticlockwise 黄、lsMoveUp/lsMoveDown 青、urgentStop/urgrentStopMearsure 红）+ 11 处 Agency FB 字体换成 Microsoft YaHei UI。
> - **sdk_assist.ui**：删除 7 处 QGroupBox 灰框样式。
> - **AxisMeasurement.cpp**：7 个轴状态指示灯 rgb(0,255,0) → #16A34A（圆角3px）；changeLedColor 的红/绿指示灯 → #DC2626 / #16A34A；measureTable 表头/选中色改为与主题一致（#F3F4F6 / #DBEAFE + 斑马纹）；表格字体 song → Microsoft YaHei UI。
> - **config/theme.qss**：按控件实名新增语义按钮样式——`#startAutoMearsurement`（开始测量，蓝底白字主操作）、`#urgentStop`/`#urgrentStopMearsure`（紧急停止，红底白字）、6 个运动控制按钮（橙色描边警示）；`#primaryButton`/`#dangerButton` 通用类保留备用。
> - 验证：两个 .ui XML 解析合法、荧光色与 Agency FB 零残留、property 标签平衡。生效方式：Release|x64 重新编译。

1. **删除 .ui 里的荧光色 styleSheet**，改成语义色（配合全局主题的按钮已带边框，只需删掉背景色）：
   - 运行/OK 类 → `color:#16A34A; font-weight:bold;`（或设 objectName 用主题类）
   - 警告/暂停类 → `color:#D97706; font-weight:bold;`
   - NG/急停类 → `color:#DC2626; font-weight:bold;`
   - 急停按钮 → 代码里 `setObjectName("dangerButton")`，红底白字
2. **替换 Agency FB 字体**：在 Qt Designer 里全局查找 `Agency FB`（11 处），统一改为 `Microsoft YaHei UI`；字号层级建议 10pt 正文 / 12pt 分组标题 / 14pt 关键数值。
3. **主操作按钮突出**：`自动测量` 按钮 `setObjectName("primaryButton")`。

### P1｜半天级，专业感明显提升

4. **按钮加图标**：运动控制类（顶尖上/下移、光幕上/下移、转台）用 12~16px 单色箭头图标。图标加入 `AxisMeasurement.qrc`，Designer 里给按钮同时设 `icon` + `text`；没有美术资源可先用免费图标库（如 iconfont 下载 PNG，统一 #4B5563 灰色）。
5. **测量结果表增加判定着色**：程序里写入"判定"列时，OK 项 `setForeground(QColor("#16A34A"))`、NG 项 `setForeground(QColor("#DC2626"))` 且 `setBackground(QColor("#FEF2F2"))`，NG 一眼可见。
6. **登录窗口美化**（logIn.ui）：
   - 整窗 `border-radius` 圆角卡片居中、去掉系统标题栏或加自定义标题（`setWindowFlags(Qt::FramelessWindowHint)`）
   - 已有 `login_backgroud.jpg` 做背景，输入框改白底半透明、登录按钮用 primaryButton
   - 密码框 `setEchoMode(QLineEdit::Password)`
7. **图形化编辑器工具栏**（graphical_program_editor.cpp buildInterface）：
   - 工具按钮加图标 + `setToolTip`（如"选择 (V)"）
   - `toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon)` 图标在上文字在下
   - 画布空态（未开图）时中间显示"拖入或点击『打开图像』开始"的浅灰提示
   - 画布背景建议深灰 `m_canvas->setBackgroundBrush(QColor("#2B2B2B"))`，图像边界更清晰
8. **状态栏分区**：主窗口状态栏用两个固定 Label（设备状态 | 提示信息），设备在线绿点/离线红点。

### P2｜一至两天，布局级重构

9. **控制面板收纳**：主窗口 12 个分组框按使用频率分层——
   - 常驻：自动测量、测量结果、程序选择、轴监控
   - 收进 `QTabWidget` 或可折叠 `QGroupBox`（点击标题展开）：上顶尖控制/旋转控制/光幕位置控制/trapControl/jogControl/axisControl/cameraControl/LS9000
   - 预计一屏元素从 12 组减到 4~5 组，视觉密度减半
10. **窗口自适应**：去掉写死的 1907×1018，改 `setMinimumSize(1440, 900)` + 中央区用 `QSplitter`，左右面板可拖动收放。
11. **数值显示大字号**：光幕实时数值、直径等关键测量值用 20pt+ 等宽字体（如 `Consolas`）+ 语义色，形成"仪表盘"感。
12. **视觉反馈**：测量进行时进度条加 `QPropertyAnimation` 平滑；急停触发时全窗口边框闪红（边缘 QFrame + 动画）。

## 四、落地顺序建议

```
第1步  重新编译 Release，确认 theme.qss 生效、无样式冲突   （10分钟）
第2步  P0 三项：删荧光色 / 统一字体 / 主按钮高亮          （30分钟）
第3步  观察现场使用反馈，挑 P1 中最痛的两三项做             （半天）
第4步  P2 布局重构放在图形化二次开发功能定型之后，避免返工
```

> P2 建议最后做的理由：图形化编辑器还在快速迭代（双ROI角度、长度算法、JSON 工程保存都排着队），布局定型过早会和功能改动互相牵制。P0/P1 是纯皮肤层，不动布局和逻辑，随时做都安全。

## 五、配套文件

| 文件 | 作用 |
|---|---|
| `config/theme.qss`（源）+ `x64\Release\config\theme.qss`（运行） | 全局主题，改这个文件即时生效（重启程序），不用重新编译 |
| `myCode\sourceCode\main.cpp` | 已加入主题加载逻辑 |
| `docs\UI优化预览.html` | 改动前后对照预览 |
