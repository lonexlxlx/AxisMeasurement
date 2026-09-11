# AxisMeasurement UI 自适应布局重构指南

## 1. 重构思路

当前项目的主要风险来自 `.ui` 文件和少量 C++ 代码中的固定坐标。固定坐标在 1080P、2K、4K 或 Windows 125%/150% 缩放下不会自动重算，因此会出现遮挡、裁剪和重叠。

推荐按页面逐步重构，而不是一次性删除所有 `setGeometry`：

1. 先选中一个独立页面，例如主界面、参数设置页或历史数据页。
2. 保留业务控件对象名，不改槽函数连接，只替换外层布局结构。
3. 最外层使用 `QScrollArea` 兜底，低分辨率或高 DPI 时允许滚动。
4. 页面主区域使用 `QSplitter` 表达左/中/右栏比例。
5. 表单区域使用 `QGridLayout` 或 `QFormLayout`，Label 固定或最小宽度，输入框 `Expanding`。
6. 按钮区使用 `QHBoxLayout`，按钮 `Fixed`，中间用 `addStretch()` 控制靠左、居中或靠右。
7. 删除普通控件的 `setGeometry()`、`move()`、`resize()`；只保留特殊覆盖层或动画遮罩一类非表单控件，并在注释中说明原因。

## 2. 通用代码模板

```cpp
QWidget* buildAdaptivePage(QWidget* parent)
{
    QWidget* page = new QWidget(parent);
    QVBoxLayout* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(10, 10, 10, 10);
    pageLayout->setSpacing(8);

    QScrollArea* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget* content = new QWidget(scroll);
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(8);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, content);
    splitter->setChildrenCollapsible(false);

    QWidget* leftPanel = new QWidget(splitter);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    QWidget* formCard = new QWidget(leftPanel);
    QGridLayout* form = new QGridLayout(formCard);
    form->setContentsMargins(12, 12, 12, 12);
    form->setHorizontalSpacing(8);
    form->setVerticalSpacing(8);
    form->setColumnStretch(0, 0);
    form->setColumnStretch(1, 1);

    QLabel* nameLabel = new QLabel(QStringLiteral("零件编号:"), formCard);
    nameLabel->setMinimumWidth(80);
    QLineEdit* nameEdit = new QLineEdit(formCard);
    nameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    form->addWidget(nameLabel, 0, 0);
    form->addWidget(nameEdit, 0, 1);

    QHBoxLayout* buttonRow = new QHBoxLayout();
    QPushButton* okButton = new QPushButton(QStringLiteral("确定"), formCard);
    okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    buttonRow->addStretch();
    buttonRow->addWidget(okButton);
    form->addLayout(buttonRow, 1, 0, 1, 2);

    leftLayout->addWidget(formCard);
    leftLayout->addStretch();

    QWidget* centerPanel = new QWidget(splitter);
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->addWidget(new QLabel(QStringLiteral("图像显示区"), centerPanel), 1);

    QWidget* rightPanel = new QWidget(splitter);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    QTableWidget* table = new QTableWidget(rightPanel);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table->horizontalHeader()->setStretchLastSection(false);
    rightLayout->addWidget(table, 1);

    splitter->addWidget(leftPanel);
    splitter->addWidget(centerPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    splitter->setStretchFactor(2, 2);
    splitter->setSizes({ 320, 780, 520 });

    contentLayout->addWidget(splitter);
    scroll->setWidget(content);
    pageLayout->addWidget(scroll);
    return page;
}
```

## 3. 高 DPI 配置

`main.cpp` 中应在创建 `QApplication` 前设置：

```cpp
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
QApplication app(argc, argv);
```

Qt6 默认启用高 DPI，但保留 rounding policy 可以减少 125%/150% 缩放下的尺寸跳变。

## 4. QSS 字号原则

字体单位优先使用 `pt`，不要使用 `px`。例如：

```css
QWidget { font-size: 9pt; }
QGroupBox { font-size: 10pt; }
```

当前 `config/theme.qss` 已经使用 `pt`，后续新增样式也应保持一致。

## 5. 常见坑位

- 不要在已经加入 Layout 的控件上继续调用 `setGeometry()`、`move()` 或 `resize()`。
- `setFixedSize()` 会破坏自适应，只在图标按钮、固定工具按钮等少数场景使用。
- `QScrollArea::setWidgetResizable(true)` 只保证内容可随视口扩展；内容本身仍需要合理的 `minimumSize` 或 Layout sizeHint。
- `QSplitter` 的 `setSizes()` 是初始比例，不是固定宽度；配合 `setStretchFactor()` 使用。
- 表格不要默认 `Stretch` 所有列，否则窄屏会压缩文字；需要用户拖拽列宽时使用 `QHeaderView::Interactive`。
- 嵌套 Layout 时统一设置 `setContentsMargins()` 和 `setSpacing()`，否则不同平台默认间距会不一致。
- 旧 `.ui` 页面如果内部还是绝对定位，可以先包 `QScrollArea` 防裁剪，再逐块把内部控件迁移到 `QGridLayout`。