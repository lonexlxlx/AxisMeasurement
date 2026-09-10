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

### P1｜半天级，专业感明显提升 ✅ 已完成（2026-09-10）

> 实际改动记录（回退备份：`ui_p1_backup_20260910\`，含 9 个界面文件 + 62 个 program cpp）：
> - **按钮图标**：生成 16 个 32px 灰底透明 PNG（`config\icons\`，Temp\gen_icons.py 可重新生成），全部注册进 `AxisMeasurement.qrc`；6 个运动控制按钮在 .ui 挂上 icon（顶尖/光幕=上下箭头，转台慢转/快转=单/双旋转箭头）。
> - **判定着色**：62 个 program_N.cpp 共 496 处批量替换（Temp\p1_verdict_color.py）——NG：红字 #DC2626+浅红底 #FEF2F2；OK：绿字 #16A34A；临界：橙字 #B45309+浅橙底 #FFF7ED。
> - **登录窗重写**（logIn.ui + logIn.cpp/h）：无边框 + 背景图 + 白色圆角卡片 + 投影 + 鼠标拖动 + 右上角关闭按钮；密码框 Password 掩码 + placeholder + 一键清空；登录按钮蓝底主样式 + 回车默认触发。
> - **编辑器工具栏**（graphical_program_editor.cpp）：12 个按钮全部加图标+tooltip，图标在上文字在下；V/P/L/R/C/A/F 快捷键（WidgetWithChildrenShortcut 挂画布，不与输入框冲突）；画布深灰 #2B2B2B；空态居中提示（开图后隐藏）。
> - **状态栏分区**（AxisMeasurement.cpp/h）：右侧永久两个 Label——最近提示信息（同步 showDeviceInf）+ 设备状态灯（● 绿=已打开/● 红=未打开，开关设备时自动切换）。
> - 验证：三个 XML 文件解析合法；.ui 6 个按钮图标挂载确认；判定着色零残留。
> - **回退方法**：把 `ui_p1_backup_20260910\` 里的文件拷回原位置（program_sourceCode\ 里的 62 个 cpp 拷回 myCode\sourceCode\），删除 `config\icons\`，重新编译即可。

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

### P2｜一至两天，布局级重构 ✅ 已完成（2026-09-10，_github_upload 目录）

9. **控制面板收纳**：主窗口 12 个分组框按使用频率分层——
   - 常驻：自动测量、测量结果、程序选择、轴监控
   - 收进 `QTabWidget` 或可折叠 `QGroupBox`（点击标题展开）：上顶尖控制/旋转控制/光幕位置控制/trapControl/jogControl/axisControl/cameraControl/LS9000
   - 预计一屏元素从 12 组减到 4~5 组，视觉密度减半
   > ✅ 实现方式：`AxisMeasurement::restructureMainLayout()`（构造函数内、setupUi 之后立即调用）。自动页常驻=设备控制+程序选择+光幕实时显示+图像区，顶尖/旋转/光幕位置收进 1 个页签；手动页常驻=轴监控+相机图像，5 组收进页签；右侧测量统计+测量结果独立成栏。所有控件只 reparent 不改名，ui.xxx 引用和信号槽全部有效。
10. **窗口自适应**：去掉写死的 1907×1018，改 `setMinimumSize(1440, 900)` + 中央区用 `QSplitter`，左右面板可拖动收放。
    > ✅ 初始尺寸改为 1768×997（.ui），最小 1440×900（代码）；三层 QSplitter：中央区[页面区|统计结果栏]、自动页[左控制栏|图像区]、手动页[轴监控|相机图像]，分隔条可拖动（悬停变蓝），不可拖没。
11. **数值显示大字号**：光幕实时数值、直径等关键测量值用 20pt+ 等宽字体（如 `Consolas`）+ 语义色，形成"仪表盘"感。
    > ✅ 光幕实时值 24pt Consolas 绿、直径 22pt 蓝、光幕 4 通道 20pt 绿、补偿前 20pt 琥珀，均带浅色底板；测量结果表 12pt Consolas 加粗（表宽所限，单值显示为 20pt+）。
12. **视觉反馈**：测量进行时进度条加 `QPropertyAnimation` 平滑；急停触发时全窗口边框闪红（边缘 QFrame + 动画）。
    > ✅ `setProgramProgressSmooth()`：OutCubic 400ms 平滑过渡（0/100 边界值直接设置）；`flashEmergencyBorder()`：6px 红色边框层 + 透明度动画闪 3 次（1.2 秒），鼠标穿透不影响操作，两个急停按钮均已接入。

#### P2 回退方法

P2 改动集中在 4 个文件，回退 = 把 `G:\AxisMeasurement\ui_p2_backup_20260910\` 里的文件拷回 `_github_upload` 对应位置后重新编译：
```
AxisMeasurement.ui                    → _github_upload\AxisMeasurement.ui
myCode\sourceCode\AxisMeasurement.cpp → _github_upload\myCode\sourceCode\
myCode\head\AxisMeasurement.h         → _github_upload\myCode\head\
config\theme.qss                      → _github_upload\config\ 和 _github_upload\x64\Release\config\（两处）
```
P0/P1 的改动都在这些文件里同步保留，回退 P2 会一并退掉——如需只退 P2 保留 P0/P1，用 git：P2 未提交前 `git checkout -- <文件>` 即可回到 P1 状态。

#### 布局修复（2026-09-10 第二轮，P2  bug 修正）

P2 首次上线后出现：分组框被压到只剩标题、内容裁切、面板互相遮挡、sdkAssist 窗口底部显示不全。
根因：`.ui` 里各分组框内部都是**绝对定位**，没有尺寸提示，进布局后被任意压缩。

修复方式（仍在 `restructureMainLayout()` 内，`_github_upload`）：
1. **锁原始尺寸**：布局接管前读取各面板的 .ui 设计尺寸设为 `minimumSize`（groupBox 321×211、groupBox_2 321×160、autoMoveAdjust 321×381、autoDeviceControl 高 109、groupBox_6 高 111 等）→ 内容永不被裁。
2. **滚动兜底**：autoMoveAdjust、trapControl、jogControl、cameraControl、LS9000、axisControl、axisMonitor 包进 `QScrollArea`（`widgetResizable=true`）→ 窗口太小时出滚动条，而不是裁切或遮挡。
3. **手动页还原原始设计**：axisControl（运动轴设置，窄高 161×541）从页签挪回顶部，与轴状态显示、相机图像三区并列；页签只留 4 个底部组（点位/Jog/相机/光幕）。
4. **仪表盘适配容器**：光幕实时值 24→20pt、直径 22→18pt；groupBox_2 内部布局区加高 91→110、组框加高 141→160；lsCurrentValue_2 标签加高 26→36。
5. **sdkAssist 窗口**：内容设计高度 1199 超出窗口 892（原有问题），中央控件改由 `QScrollArea` 承载并锁定 1142×1199 → 底部"输出表单/表单生成路径"完整可见，窗口可自由缩放。

回退本次修复（保留 P2 其余部分）：把 `G:\AxisMeasurement\ui_layout_fix_backup_20260910\` 里的 3 个文件（AxisMeasurement.cpp、AxisMeasurement.h、sdk_assist.cpp）拷回 `_github_upload` 对应位置后重新编译。

#### sdkAssist 布局整理（2026-09-10 第三轮，用户截图反馈后）

针对 sdkAssist 窗口：底部信息初始不可见、模块混乱、小标题不居中、下拉框箭头显示不全、下拉框顶出卡片边界。

1. **隐藏孤儿标签**：`label_82`(跳动点数)/`label_75`(孔径点数) 坐标在 y1090/1148 的真内容之外，与 groupBox_7 内统计重复且代码零引用——它们把内容高度虚撑到 1199。隐藏后真实内容仅 ~870。
2. **垂直重排**（压缩行间 300px 浪费）：总标题 y4 → 行1 标题 y36/组 y68 → 行2 标题 y407/组 y439（模块6 双行标题、组 y459）→ 模块7 标题 y698/组 y730 → 路径条 y907，内容总高 ~950。窗口初始高度按屏幕可用区自适应（`resize(1160, min(1010, 屏高-60))`）→ **底部"输出表单/表单生成路径"一屏初始可见**，小屏滚动兜底。
3. **小标题统一居中**：7 个模块标题（独立 QLabel）`AlignHCenter` 并与对应组左右对齐（x/宽=组的 x/宽）；模块7 标题和路径条通栏居中。
4. **下拉框修复**：各组内部 layoutWidget 右缘内收到 `组宽-24`（原距卡片边框仅 3~9px，视觉上顶出边界）；theme.qss 删掉过小的 CSS 三角箭头（渲染成小灰块像显示不全），恢复样式表风格内置箭头 + drop-down 细分隔线。

回退本轮修改：把 `G:\AxisMeasurement\ui_sdkassist_fix_backup_20260910\` 里的 2 个文件（sdk_assist.cpp、theme.qss）拷回 `_github_upload` 对应位置（theme.qss 两处：config\ 和 x64\Release\config\）后重新编译。

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
