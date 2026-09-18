#include "sdk_assist.h"

#include "graphical_program_editor.h"

#include <QScrollArea>
#include <QResizeEvent>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QSizePolicy>
#include <QPushButton>
#include <QColor>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QStyle>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QLineEdit>
#include <QComboBox>
#include <QList>
#include <QMessageBox>
#include <QValidator>
#include <initializer_list>
#include <algorithm>

QString runtimePath(const QString& relativePath);

namespace {
constexpr int kLabelWidth = 104;
constexpr int kControlMinWidth = 170;
constexpr int kControlHeight = 25;

void prepareField(QWidget* widget)
{
    if (!widget)
        return;
    widget->setMinimumWidth(kControlMinWidth);
    widget->setMinimumHeight(kControlHeight);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void prepareLabel(QLabel* label)
{
    if (!label)
        return;
    label->setFixedWidth(kLabelWidth);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void markButton(QPushButton* button, const char* role)
{
    if (!button)
        return;
    button->setProperty("buttonRole", role);
    button->setMinimumHeight(27);
    button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void setNumericValidator(QLineEdit* field, bool integerOnly, int minimum = -1000000, int maximum = 1000000)
{
    if (!field)
        return;

    field->setProperty("inputState", "normal");
    field->setClearButtonEnabled(true);
    if (integerOnly) {
        field->setValidator(new QIntValidator(minimum, maximum, field));
    }
    else {
        field->setValidator(new QDoubleValidator(-1000000.0, 1000000.0, 4, field));
    }
}

void setOptionalPlaceholder(QLineEdit* field, const QString& placeholder)
{
    if (!field)
        return;
    field->setPlaceholderText(placeholder);
}

bool containsOrder(const vector<int>& orders, int order)
{
    return std::find(orders.begin(), orders.end(), order) != orders.end();
}

void updateRecordButtonState(QPushButton* button, bool recorded)
{
    if (!button)
        return;
    button->setProperty("recordState", recorded ? "recorded" : "ready");
    button->style()->unpolish(button);
    button->style()->polish(button);
}

bool hasInvalidRequiredField(std::initializer_list<QLineEdit*> fields)
{
    for (QLineEdit* field : fields) {
        if (!field)
            continue;
        const bool empty = field->text().trimmed().isEmpty();
        const bool invalid = field->validator() && !field->hasAcceptableInput();
        if (empty || invalid) {
            field->setProperty("inputState", "error");
            field->style()->unpolish(field);
            field->style()->polish(field);
            field->setFocus();
            return true;
        }
    }
    return false;
}

void clearInputErrorState(std::initializer_list<QLineEdit*> fields)
{
    for (QLineEdit* field : fields) {
        if (!field)
            continue;
        field->setProperty("inputState", "normal");
        field->style()->unpolish(field);
        field->style()->polish(field);
    }
}

void addRow(QGridLayout* layout, int row, QLabel* label, QWidget* field)
{
    prepareLabel(label);
    prepareField(field);
    layout->addWidget(label, row, 0);
    layout->addWidget(field, row, 1);
}

struct ModuleCardParts
{
    QGroupBox* card = nullptr;
    QGridLayout* formLayout = nullptr;
    QHBoxLayout* actionLayout = nullptr;
};

ModuleCardParts createInputModuleCard(
    const QString& number,
    const QString& title,
    const QString& device)
{
    ModuleCardParts parts;
    parts.card = new QGroupBox();
    parts.card->setObjectName(QStringLiteral("sdkAssistCard"));
    parts.card->setProperty("cardRole", "inputModule");
    parts.card->setMinimumHeight(330);
    parts.card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* shadow = new QGraphicsDropShadowEffect(parts.card);
    shadow->setBlurRadius(8);
    shadow->setOffset(0, 1);
    shadow->setColor(QColor(15, 23, 42, 10));
    parts.card->setGraphicsEffect(shadow);

    auto* cardLayout = new QVBoxLayout(parts.card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    auto* header = new QFrame(parts.card);
    header->setObjectName(QStringLiteral("sdkAssistCardHeader"));
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 8, 12, 8);
    headerLayout->setSpacing(8);

    auto* numberLabel = new QLabel(number, header);
    numberLabel->setObjectName(QStringLiteral("sdkAssistModuleNumber"));
    auto* titleLabel = new QLabel(title, header);
    titleLabel->setObjectName(QStringLiteral("sdkAssistCardTitle"));
    auto* deviceLabel = new QLabel(device, header);
    deviceLabel->setObjectName(QStringLiteral("sdkAssistDeviceBadge"));

    headerLayout->addWidget(numberLabel);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(deviceLabel);

    auto* body = new QWidget(parts.card);
    body->setObjectName(QStringLiteral("sdkAssistCardBody"));
    parts.formLayout = new QGridLayout(body);
    parts.formLayout->setContentsMargins(12, 10, 12, 10);
    parts.formLayout->setHorizontalSpacing(10);
    parts.formLayout->setVerticalSpacing(8);
    parts.formLayout->setColumnStretch(0, 0);
    parts.formLayout->setColumnStretch(1, 1);

    auto* footer = new QFrame(parts.card);
    footer->setObjectName(QStringLiteral("sdkAssistCardFooter"));
    parts.actionLayout = new QHBoxLayout(footer);
    parts.actionLayout->setContentsMargins(12, 8, 12, 10);
    parts.actionLayout->setSpacing(8);

    cardLayout->addWidget(header);
    cardLayout->addWidget(body);
    cardLayout->addStretch();
    cardLayout->addWidget(footer);
    return parts;
}

void addModuleActions(
    ModuleCardParts& parts,
    QPushButton* primary,
    QPushButton* danger)
{
    markButton(primary, "primary");
    markButton(danger, "dangerOutline");
    primary->setText(QStringLiteral("记录点位"));
    danger->setText(QStringLiteral("清除记录"));
    parts.actionLayout->addWidget(primary, 1);
    parts.actionLayout->addWidget(danger);
}

QGroupBox* createDiameterModule(Ui::sdk_assist& ui)
{
    ModuleCardParts parts = createInputModuleCard(
        QStringLiteral("01"), QStringLiteral("直径点位记录"), QStringLiteral("光幕"));
    addRow(parts.formLayout, 0, ui.label_3, ui.diameterSequence);
    addRow(parts.formLayout, 1, ui.label_5, ui.diameterFeatureNb);
    addRow(parts.formLayout, 2, ui.label_6, ui.diameterNominalValue);
    addRow(parts.formLayout, 3, ui.label_7, ui.diameterUpperOffset);
    addRow(parts.formLayout, 4, ui.label_8, ui.diameterBottomOffset);
    addRow(parts.formLayout, 5, ui.label_9, ui.diameterPostionNote);
    addModuleActions(parts, ui.diameterPostionRecord, ui.diameterPostionClear);
    return parts.card;
}

QGroupBox* createRoughnessModule(Ui::sdk_assist& ui)
{
    ModuleCardParts parts = createInputModuleCard(
        QStringLiteral("02"), QStringLiteral("粗糙度点位记录"),
        QStringLiteral("粗糙度相机 · 光幕"));
    addRow(parts.formLayout, 0, ui.label_14, ui.roughnessSequence);
    addRow(parts.formLayout, 1, ui.label_10, ui.roughnessFeatureNb);
    addRow(parts.formLayout, 2, ui.label_11, ui.roughnessNominalValue);
    addRow(parts.formLayout, 3, ui.label_4, ui.roughnessExposeTime);
    addRow(parts.formLayout, 4, ui.label_31, ui.roughnessReferenceD);
    addRow(parts.formLayout, 5, ui.label_28, ui.roughnessPostionNote);

    markButton(ui.roughnessPostionRecord, "primary");
    markButton(ui.roughnessReferenceDRecord, "secondary");
    markButton(ui.roughnessPostionClear, "dangerOutline");
    ui.roughnessPostionRecord->setText(QStringLiteral("记录粗糙度"));
    ui.roughnessReferenceDRecord->setText(QStringLiteral("记录光幕补偿"));
    ui.roughnessPostionClear->setText(QStringLiteral("清除记录"));
    parts.actionLayout->addWidget(ui.roughnessPostionRecord, 1);
    parts.actionLayout->addWidget(ui.roughnessReferenceDRecord, 1);
    parts.actionLayout->addWidget(ui.roughnessPostionClear);
    return parts.card;
}

QGroupBox* createHoleModule(Ui::sdk_assist& ui)
{
    ModuleCardParts parts = createInputModuleCard(
        QStringLiteral("03"), QStringLiteral("孔径点位记录"), QStringLiteral("孔径相机"));
    addRow(parts.formLayout, 0, ui.label_38, ui.holeSequence);
    addRow(parts.formLayout, 1, ui.label_34, ui.holeFeatureNb);
    addRow(parts.formLayout, 2, ui.label_35, ui.holeNominalValue);
    addRow(parts.formLayout, 3, ui.label_37, ui.holeUpperOffset);
    addRow(parts.formLayout, 4, ui.label_39, ui.holeBottomOffset);
    addRow(parts.formLayout, 5, ui.label_41, ui.holeNumber);
    addRow(parts.formLayout, 6, ui.label_40, ui.holeExposeTime);
    addRow(parts.formLayout, 7, ui.label_49, ui.holePostionNote);
    addModuleActions(parts, ui.holePostionRecord, ui.holePostionClear);
    return parts.card;
}

QGroupBox* createCylindricityModule(Ui::sdk_assist& ui)
{
    ModuleCardParts parts = createInputModuleCard(
        QStringLiteral("04"), QStringLiteral("圆柱度点位记录"), QStringLiteral("光幕"));
    addRow(parts.formLayout, 0, ui.label_13, ui.cylindricitySequence);
    addRow(parts.formLayout, 1, ui.label_16, ui.cylindricityFeatureNb);
    addRow(parts.formLayout, 2, ui.label_17, ui.cylindricityNominalValue);
    addRow(parts.formLayout, 3, ui.label_18, ui.cylindricityUpperRelativeLocation);
    addRow(parts.formLayout, 4, ui.label_19, ui.cylindricityBottomRelativeLocation);
    addRow(parts.formLayout, 5, ui.label_20, ui.cylindricityPostionNote);
    addModuleActions(parts, ui.cylindricityPostionRecord, ui.cylindricityPostionClear);
    return parts.card;
}

QGroupBox* createRoundoutModule(Ui::sdk_assist& ui)
{
    ModuleCardParts parts = createInputModuleCard(
        QStringLiteral("05"), QStringLiteral("跳动点位记录"), QStringLiteral("光幕"));
    addRow(parts.formLayout, 0, ui.label_24, ui.roundoutSequence);
    addRow(parts.formLayout, 1, ui.label_25, ui.roundoutFeatureNb);
    addRow(parts.formLayout, 2, ui.label_26, ui.roundoutNominalValue);
    addRow(parts.formLayout, 3, ui.label_22, ui.roundoutUpperRelativeLocation);
    addRow(parts.formLayout, 4, ui.label_23, ui.roundoutBottomRelativeLocation);
    addRow(parts.formLayout, 5, ui.label_27, ui.roundoutPostionNote1);
    addRow(parts.formLayout, 6, ui.label_33, ui.roundoutPostionNote2);
    addModuleActions(parts, ui.roundoutPostionRecord, ui.roundoutPostionClear);
    return parts.card;
}

QGroupBox* createTelecentricModule(Ui::sdk_assist& ui)
{
    ModuleCardParts parts = createInputModuleCard(
        QStringLiteral("06"), QStringLiteral("长度 / 角度 / 圆弧半径"),
        QStringLiteral("远心相机"));
    addRow(parts.formLayout, 0, ui.label_30, ui.telecentricSequence);
    addRow(parts.formLayout, 1, ui.label_32, ui.telecentricExposeTime);
    addRow(parts.formLayout, 2, ui.label_36, ui.telecentricPostionNote);

    auto* hint = new QLabel(
        QStringLiteral("用于远心相机的长度、角度和圆弧半径测量点位。"), parts.card);
    hint->setObjectName(QStringLiteral("sdkAssistModuleHint"));
    hint->setWordWrap(true);
    parts.formLayout->addWidget(hint, 3, 0, 1, 2);

    addModuleActions(parts, ui.telecentricPostionRecord, ui.telecentricPostionClear);
    return parts.card;
}

QFrame* createCountItem(QLabel* nameLabel, QLabel* valueLabel, QWidget* parent)
{
    auto* item = new QFrame(parent);
    item->setObjectName(QStringLiteral("sdkAssistCountItem"));
    auto* layout = new QHBoxLayout(item);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(8);

    nameLabel->setParent(item);
    nameLabel->setObjectName(QStringLiteral("sdkAssistCountName"));
    nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    nameLabel->setFixedWidth(0);
    nameLabel->setMinimumWidth(0);
    nameLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    valueLabel->setParent(item);
    valueLabel->setObjectName(QStringLiteral("sdkAssistCountValue"));
    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    layout->addWidget(nameLabel);
    layout->addStretch();
    layout->addWidget(valueLabel);
    return item;
}

QGroupBox* createOutputModule(
    Ui::sdk_assist& ui,
    const std::function<void()>& openOutputDirectory,
    QLabel*& outputStatusLabel)
{
    auto* card = new QGroupBox();
    card->setObjectName(QStringLiteral("sdkAssistOutputCard"));
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    auto* header = new QFrame(card);
    header->setObjectName(QStringLiteral("sdkAssistOutputHeader"));
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(14, 9, 14, 9);
    headerLayout->setSpacing(8);

    auto* numberLabel = new QLabel(QStringLiteral("07"), header);
    numberLabel->setObjectName(QStringLiteral("sdkAssistModuleNumber"));
    auto* titleLabel = new QLabel(QStringLiteral("测量信息与点位输出"), header);
    titleLabel->setObjectName(QStringLiteral("sdkAssistOutputTitle"));
    auto* descriptionLabel = new QLabel(QStringLiteral("汇总点位、填写零件信息并生成 Excel 文件"), header);
    descriptionLabel->setObjectName(QStringLiteral("sdkAssistOutputDescription"));

    headerLayout->addWidget(numberLabel);
    headerLayout->addWidget(titleLabel);
    headerLayout->addSpacing(6);
    headerLayout->addWidget(descriptionLabel);
    headerLayout->addStretch();

    auto* body = new QWidget(card);
    body->setObjectName(QStringLiteral("sdkAssistOutputBody"));
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(14, 12, 14, 12);
    bodyLayout->setSpacing(10);

    auto* countTitle = new QLabel(QStringLiteral("已记录点位"), body);
    countTitle->setObjectName(QStringLiteral("sdkAssistSectionTitle"));
    auto* countGrid = new QGridLayout();
    countGrid->setContentsMargins(0, 0, 0, 0);
    countGrid->setHorizontalSpacing(8);
    countGrid->setVerticalSpacing(8);
    countGrid->addWidget(createCountItem(ui.label_76, ui.recordDiameterNb, body), 0, 0);
    countGrid->addWidget(createCountItem(ui.label_79, ui.recordRoughnessNb, body), 0, 1);
    countGrid->addWidget(createCountItem(ui.label_74, ui.recordHoleNb, body), 0, 2);
    countGrid->addWidget(createCountItem(ui.label_81, ui.recordCylindricityNb, body), 1, 0);
    countGrid->addWidget(createCountItem(ui.label_80, ui.recordRoundoutNb, body), 1, 1);
    countGrid->addWidget(createCountItem(ui.label_72, ui.recordTelecentricNb, body), 1, 2);
    countGrid->setColumnStretch(0, 1);
    countGrid->setColumnStretch(1, 1);
    countGrid->setColumnStretch(2, 1);

    auto* infoTitle = new QLabel(QStringLiteral("零件信息"), body);
    infoTitle->setObjectName(QStringLiteral("sdkAssistSectionTitle"));
    auto* infoGrid = new QGridLayout();
    infoGrid->setContentsMargins(0, 0, 0, 0);
    infoGrid->setHorizontalSpacing(10);
    infoGrid->setVerticalSpacing(8);

    prepareLabel(ui.label_86);
    prepareLabel(ui.label_85);
    prepareLabel(ui.label_84);
    prepareLabel(ui.label_87);
    prepareField(ui.recordpartNb);
    prepareField(ui.recordpartName);
    prepareField(ui.recordpartProcessingNb);
    prepareField(ui.recordpartNote);

    infoGrid->addWidget(ui.label_86, 0, 0);
    infoGrid->addWidget(ui.recordpartNb, 0, 1);
    infoGrid->addWidget(ui.label_85, 0, 2);
    infoGrid->addWidget(ui.recordpartName, 0, 3);
    infoGrid->addWidget(ui.label_84, 0, 4);
    infoGrid->addWidget(ui.recordpartProcessingNb, 0, 5);
    infoGrid->addWidget(ui.label_87, 1, 0);
    infoGrid->addWidget(ui.recordpartNote, 1, 1, 1, 5);
    infoGrid->setColumnStretch(1, 1);
    infoGrid->setColumnStretch(3, 1);
    infoGrid->setColumnStretch(5, 1);

    auto* outputTitle = new QLabel(QStringLiteral("文件输出"), body);
    outputTitle->setObjectName(QStringLiteral("sdkAssistSectionTitle"));
    auto* outputRow = new QHBoxLayout();
    outputRow->setContentsMargins(0, 0, 0, 0);
    outputRow->setSpacing(8);

    auto* pathLabel = new QLabel(QStringLiteral("输出目录"), body);
    pathLabel->setObjectName(QStringLiteral("sdkAssistOutputPathLabel"));
    auto* pathEdit = new QLineEdit(runtimePath("SDKpostion"), body);
    pathEdit->setObjectName(QStringLiteral("sdkAssistOutputPath"));
    pathEdit->setReadOnly(true);
    pathEdit->setCursorPosition(0);

    auto* openDirectoryButton = new QPushButton(QStringLiteral("打开目录"), body);
    openDirectoryButton->setObjectName(QStringLiteral("sdkAssistOpenDirectoryButton"));
    markButton(openDirectoryButton, "secondary");
    QObject::connect(openDirectoryButton, &QPushButton::clicked,
        card, [openOutputDirectory]() { openOutputDirectory(); });

    markButton(ui.PostionRecordOut, "primary");
    markButton(ui.PostionClearOut, "dangerOutline");
    ui.PostionRecordOut->setText(QStringLiteral("生成点位文件"));
    ui.PostionClearOut->setText(QStringLiteral("清空全部"));

    outputRow->addWidget(pathLabel);
    outputRow->addWidget(pathEdit, 1);
    outputRow->addWidget(openDirectoryButton);
    outputRow->addSpacing(8);
    outputRow->addWidget(ui.PostionClearOut);
    outputRow->addWidget(ui.PostionRecordOut);

    outputStatusLabel = new QLabel(QStringLiteral("尚未生成点位文件"), body);
    outputStatusLabel->setObjectName(QStringLiteral("sdkAssistOutputStatus"));
    outputStatusLabel->setProperty("status", "idle");
    outputStatusLabel->setWordWrap(true);

    bodyLayout->addWidget(countTitle);
    bodyLayout->addLayout(countGrid);
    bodyLayout->addWidget(infoTitle);
    bodyLayout->addLayout(infoGrid);
    bodyLayout->addWidget(outputTitle);
    bodyLayout->addLayout(outputRow);
    bodyLayout->addWidget(outputStatusLabel);

    cardLayout->addWidget(header);
    cardLayout->addWidget(body);
    return card;
}
}
sdk_assist::sdk_assist(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	this->setWindowIcon(QIcon(runtimePath("config/logo.ico")));
	this->setWindowTitle("sdkAssist");
	    //====== sdkAssist UI 重构：滚动容器 + 卡片式模块 + 布局管理器 ======
    // 保留 Designer 中的控件实例和 objectName，让 Qt 自动连接槽函数继续生效；只重排父子关系和布局。
    ui.label_82->hide();
    ui.label_75->hide();
    ui.label_2->hide();
    ui.label_12->hide();
    ui.label_15->hide();
    ui.label_21->hide();
    ui.label_29->hide();
    ui.label_42->hide();
    ui.label_43->hide();
    ui.label_56->hide();
    ui.label_57->hide();
    ui.groupBox->hide();
    ui.groupBox_2->hide();
    ui.groupBox_3->hide();
    ui.groupBox_4->hide();
    ui.groupBox_5->hide();
    ui.groupBox_6->hide();
    ui.groupBox_7->hide();

    m_responsivePage = new QWidget(this);
    m_responsivePage->setObjectName(QStringLiteral("sdkAssistPage"));
    m_responsivePage->setMinimumWidth(0);
    m_responsivePage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* page = m_responsivePage;
    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(8, 6, 8, 8);
    pageLayout->setSpacing(8);
    pageLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    // 阶段1：把原来的独立工具栏和居中标题合并为一条紧凑、清晰的页面标题栏。
    // 沿用 Designer 中的 ui.label 作为主标题，避免创建重复控件。
    auto* pageHeader = new QFrame(page);
    pageHeader->setObjectName(QStringLiteral("sdkAssistHeader"));
    auto* pageHeaderLayout = new QHBoxLayout(pageHeader);
    pageHeaderLayout->setContentsMargins(16, 10, 16, 10);
    pageHeaderLayout->setSpacing(12);

    auto* headerTextLayout = new QVBoxLayout();
    headerTextLayout->setContentsMargins(0, 0, 0, 0);
    headerTextLayout->setSpacing(1);

    ui.label->setParent(pageHeader);
    ui.label->setText(QStringLiteral("二次开发辅助"));
    ui.label->setObjectName(QStringLiteral("sdkAssistHeaderTitle"));
    ui.label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui.label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto* headerSubtitle = new QLabel(QStringLiteral("点位参数配置与输出"), pageHeader);
    headerSubtitle->setObjectName(QStringLiteral("sdkAssistHeaderSubtitle"));

    headerTextLayout->addWidget(ui.label);
    headerTextLayout->addWidget(headerSubtitle);

    auto* openGraphicalEditorButton = new QPushButton(QStringLiteral("打开图形化编辑"), pageHeader);
    openGraphicalEditorButton->setObjectName(QStringLiteral("openGraphicalEditorButton"));
    openGraphicalEditorButton->setCursor(Qt::PointingHandCursor);
    openGraphicalEditorButton->setMinimumHeight(32);
    openGraphicalEditorButton->setToolTip(QStringLiteral("打开图形化测量程序编辑器"));
    connect(openGraphicalEditorButton, &QPushButton::clicked,
        this, &sdk_assist::openGraphicalProgramEditor);

    pageHeaderLayout->addLayout(headerTextLayout);
    pageHeaderLayout->addStretch();
    pageHeaderLayout->addWidget(openGraphicalEditorButton, 0, Qt::AlignVCenter);
    pageLayout->addWidget(pageHeader);

    m_moduleGrid = new QGridLayout();
    m_moduleGrid->setContentsMargins(0, 0, 0, 0);
    m_moduleGrid->setHorizontalSpacing(10);
    m_moduleGrid->setVerticalSpacing(8);

    m_moduleCards = {
        createDiameterModule(ui),
        createRoughnessModule(ui),
        createHoleModule(ui),
        createCylindricityModule(ui),
        createRoundoutModule(ui),
        createTelecentricModule(ui)
    };
    pageLayout->addLayout(m_moduleGrid);
    pageLayout->addWidget(createOutputModule(
        ui,
        [this]() { openOutputDirectory(); },
        m_outputStatusLabel), 0);

    m_centralScrollArea = new QScrollArea(this);
    m_centralScrollArea->setObjectName(QStringLiteral("sdkAssistScrollArea"));
    m_centralScrollArea->setWidget(page);
    m_centralScrollArea->setWidgetResizable(true);
    m_centralScrollArea->setFrameShape(QFrame::NoFrame);
    m_centralScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_centralScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_centralScrollArea->viewport()->setMinimumWidth(0);
    setCentralWidget(m_centralScrollArea);
    setMinimumSize(720, 640);

    // 阶段4：统一数值输入约束，避免空值、字母和超范围数据进入测量结构。
    setNumericValidator(ui.diameterFeatureNb, true, 0, 999999);
    setNumericValidator(ui.diameterNominalValue, false);
    setNumericValidator(ui.diameterUpperOffset, false);
    setNumericValidator(ui.diameterBottomOffset, false);
    setNumericValidator(ui.roughnessFeatureNb, true, 0, 999999);
    setNumericValidator(ui.roughnessNominalValue, false);
    setNumericValidator(ui.roughnessExposeTime, true, 1, 100000);
    setNumericValidator(ui.roughnessReferenceD, false);
    setNumericValidator(ui.holeFeatureNb, true, 0, 999999);
    setNumericValidator(ui.holeNominalValue, false);
    setNumericValidator(ui.holeUpperOffset, false);
    setNumericValidator(ui.holeBottomOffset, false);
    setNumericValidator(ui.holeNumber, true, 1, 999999);
    setNumericValidator(ui.holeExposeTime, true, 1, 100000);
    setNumericValidator(ui.cylindricityFeatureNb, true, 0, 999999);
    setNumericValidator(ui.cylindricityNominalValue, false);
    setNumericValidator(ui.cylindricityUpperRelativeLocation, true, 0, 100000000);
    setNumericValidator(ui.cylindricityBottomRelativeLocation, true, 0, 100000000);
    setNumericValidator(ui.roundoutFeatureNb, true, 0, 999999);
    setNumericValidator(ui.roundoutNominalValue, false);
    setNumericValidator(ui.roundoutUpperRelativeLocation, true, 0, 100000000);
    setNumericValidator(ui.roundoutBottomRelativeLocation, true, 0, 100000000);
    setNumericValidator(ui.telecentricExposeTime, true, 1, 100000);

    setOptionalPlaceholder(ui.diameterPostionNote, QStringLiteral("可选：输入备注信息"));
    setOptionalPlaceholder(ui.roughnessPostionNote, QStringLiteral("可选：输入备注信息"));
    setOptionalPlaceholder(ui.cylindricityPostionNote, QStringLiteral("可选：输入备注信息"));
    setOptionalPlaceholder(ui.roundoutPostionNote1, QStringLiteral("可选：输入备注信息"));
    setOptionalPlaceholder(ui.roundoutPostionNote2, QStringLiteral("可选：输入备注信息"));
    setOptionalPlaceholder(ui.holePostionNote, QStringLiteral("可选：输入备注信息"));
    setOptionalPlaceholder(ui.telecentricPostionNote, QStringLiteral("可选：输入备注信息"));
    setOptionalPlaceholder(ui.recordpartNb, QStringLiteral("请输入零件图号"));
    setOptionalPlaceholder(ui.recordpartName, QStringLiteral("请输入零件名称"));
    setOptionalPlaceholder(ui.recordpartProcessingNb, QStringLiteral("请输入工序号"));
    setOptionalPlaceholder(ui.recordpartNote, QStringLiteral("可选：输入装夹方式或备注"));

    const QList<QLineEdit*> validatedFields = {
        ui.diameterFeatureNb, ui.diameterNominalValue, ui.diameterUpperOffset,
        ui.diameterBottomOffset, ui.roughnessFeatureNb, ui.roughnessNominalValue,
        ui.roughnessExposeTime, ui.roughnessReferenceD, ui.holeFeatureNb,
        ui.holeNominalValue, ui.holeUpperOffset, ui.holeBottomOffset,
        ui.holeNumber, ui.holeExposeTime, ui.cylindricityFeatureNb,
        ui.cylindricityNominalValue, ui.cylindricityUpperRelativeLocation,
        ui.cylindricityBottomRelativeLocation, ui.roundoutFeatureNb,
        ui.roundoutNominalValue, ui.roundoutUpperRelativeLocation,
        ui.roundoutBottomRelativeLocation, ui.telecentricExposeTime
    };
    for (QLineEdit* field : validatedFields) {
        connect(field, &QLineEdit::textEdited, this, [field]() {
            if (field->property("inputState").toString() == QStringLiteral("error")) {
                field->setProperty("inputState", "normal");
                field->style()->unpolish(field);
                field->style()->polish(field);
            }
        });
    }

    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    if (primaryScreen) {
        const int availableHeight = primaryScreen->availableGeometry().height();
        resize(1180, qMin(940, availableHeight - 60));
    }
    else {
        resize(1180, 900);
    }

    QTimer::singleShot(0, this, [this]() {
        if (m_centralScrollArea)
            reflowModuleGrid(m_centralScrollArea->viewport()->width());
    });

	/*
	for (int i = 0; i < 99; i++)
	{
		m_diameterPositionInf[i] = new diameterPositionInf();
	}
	*/
	//this->setWindowIcon(QIcon("./config/logo.ico"));
	//ui.setupUi(this);
};
sdk_assist::~sdk_assist()
{
};

void sdk_assist::reflowModuleGrid(int availableWidth)
{
    if (!m_moduleGrid || m_moduleCards.isEmpty())
        return;

    int columnCount = 3;
    if (availableWidth < 760)
        columnCount = 1;
    else if (availableWidth < 1100)
        columnCount = 2;

    if (columnCount == m_currentColumnCount)
        return;

    while (QLayoutItem* item = m_moduleGrid->takeAt(0))
        delete item;

    for (int column = 0; column < 3; ++column)
        m_moduleGrid->setColumnStretch(column, column < columnCount ? 1 : 0);

    for (int index = 0; index < m_moduleCards.size(); ++index) {
        QWidget* card = m_moduleCards.at(index);
        card->setMinimumWidth(columnCount == 1 ? 0 : 320);
        card->setMaximumWidth(QWIDGETSIZE_MAX);
        const int row = index / columnCount;
        const int column = index % columnCount;
        m_moduleGrid->addWidget(card, row, column);
    }

    m_currentColumnCount = columnCount;
    m_moduleGrid->invalidate();
    if (m_responsivePage)
        m_responsivePage->updateGeometry();
}

void sdk_assist::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (!m_centralScrollArea)
        return;

    reflowModuleGrid(m_centralScrollArea->viewport()->width());
    QTimer::singleShot(0, this, [this]() {
        if (m_centralScrollArea)
            reflowModuleGrid(m_centralScrollArea->viewport()->width());
    });
}

void sdk_assist::openGraphicalProgramEditor()
{
    const QString error = graphicalEntryError ? graphicalEntryError() : QString();
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("暂不能打开图形化编程"), error);
        return;
    }

    if (!m_graphicalProgramEditor) {
        m_graphicalProgramEditor = new GraphicalProgramEditor(this);
        emit graphicalEditorCreated(m_graphicalProgramEditor);
    }

    m_graphicalProgramEditor->show();
    m_graphicalProgramEditor->raise();
    m_graphicalProgramEditor->activateWindow();
}

void sdk_assist::openOutputDirectory()
{
    const QString outputDirectory = runtimePath("SDKpostion");
    QDir().mkpath(outputDirectory);
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(outputDirectory))) {
        setOutputStatus(QStringLiteral("无法打开输出目录：%1").arg(outputDirectory), "error");
    }
}

void sdk_assist::setOutputStatus(const QString& text, const char* status)
{
    if (!m_outputStatusLabel)
        return;

    m_outputStatusLabel->setText(text);
    m_outputStatusLabel->setProperty("status", status);
    m_outputStatusLabel->style()->unpolish(m_outputStatusLabel);
    m_outputStatusLabel->style()->polish(m_outputStatusLabel);
}

//直径槽函数*******************************************************************************************************************************************************************************************
void sdk_assist::on_diameterSequence_currentIndexChanged(int nIndex)
{
	currentDiameterOrder = nIndex;
	updateDiameterPostionInf(currentDiameterOrder);
    updateRecordButtonState(ui.diameterPostionRecord,
                            containsOrder(recordDiameterList, currentDiameterOrder));
};
void sdk_assist::on_diameterPostionRecord_clicked()
{
    if (hasInvalidRequiredField({ui.diameterFeatureNb, ui.diameterNominalValue,
                               ui.diameterUpperOffset, ui.diameterBottomOffset})) {
        emit tips(QStringLiteral("直径模块存在未填写的必填数值，请补充后再记录。"));
        return;
    }
    clearInputErrorState({ui.diameterFeatureNb, ui.diameterNominalValue,
                          ui.diameterUpperOffset, ui.diameterBottomOffset});
	emit diameterPostionRecord();
	_sleep(150);
	m_diameterPositionInf[currentDiameterOrder].diameterFeatureNb = ui.diameterFeatureNb->text().toInt();
	m_diameterPositionInf[currentDiameterOrder].diameterNominalValue = ui.diameterNominalValue->text().toFloat();;
	m_diameterPositionInf[currentDiameterOrder].diameterUpperOffset = ui.diameterUpperOffset->text().toFloat();
	m_diameterPositionInf[currentDiameterOrder].diameterBottomOffset = ui.diameterBottomOffset->text().toFloat();;
	m_diameterPositionInf[currentDiameterOrder].diameterPostionNote = ui.diameterPostionNote->text();
	if (!containsOrder(recordDiameterList, currentDiameterOrder)) {
		recordDiameterList.push_back(currentDiameterOrder);
	}
	recordDiameterNb_all = static_cast<int>(recordDiameterList.size());
	updateRecordButtonState(ui.diameterPostionRecord, true);
	updatePostionInfOut();


};
void sdk_assist::on_diameterPostionClear_clicked()
{
	if (!containsOrder(recordDiameterList, currentDiameterOrder))
	{
		emit tips("当前顺序号尚未记录直径点位，请检查！");
		return;
	}
	clearSingleDiameter(currentDiameterOrder);
	for (vector<int>::iterator iter = recordDiameterList.begin(); iter != recordDiameterList.end();)
	{
		if (*iter == currentDiameterOrder)
		{
			iter = recordDiameterList.erase(iter);
			break;
		}
	}
    recordDiameterNb_all = static_cast<int>(recordDiameterList.size());
	updateDiameterPostionInf(currentDiameterOrder);
    updateRecordButtonState(ui.diameterPostionRecord, false);
	updatePostionInfOut();
	std::cout << "on_diameterPostionClear_clickes()" << endl;
};
void sdk_assist::clearSingleDiameter(int index)
{
	m_diameterPositionInf[index].diameterFeatureNb = 0;
	m_diameterPositionInf[index].diameterNominalValue = 0;
	m_diameterPositionInf[index].diameterUpperOffset = 0;
	m_diameterPositionInf[index].diameterBottomOffset = 0;
	m_diameterPositionInf[index].diameterPostionNote = "-";
	m_diameterPositionInf[index].axisGuangMuEncodePostion = 0;
	m_diameterPositionInf[index].axisGuangMuRealPostion = 0;
};
void  sdk_assist::updateDiameterPostionInf(int selectedPostion)
{
	ui.diameterFeatureNb->setText(QString::number(m_diameterPositionInf[selectedPostion].diameterFeatureNb));
	ui.diameterNominalValue->setText(QString::number(m_diameterPositionInf[selectedPostion].diameterNominalValue));
	ui.diameterUpperOffset->setText(QString::number(m_diameterPositionInf[selectedPostion].diameterUpperOffset));
	ui.diameterBottomOffset->setText(QString::number(m_diameterPositionInf[selectedPostion].diameterBottomOffset));
	ui.diameterPostionNote->setText(m_diameterPositionInf[selectedPostion].diameterPostionNote);
};

//粗糙度输出槽函数*****************************************************************************************************************************************************************************************
void sdk_assist::on_roughnessSequence_currentIndexChanged(int nIndex)
{
	currentRoughnessOrder = nIndex;
	updateRoughnessPostionInf(currentRoughnessOrder);
    updateRecordButtonState(ui.roughnessPostionRecord,
                            containsOrder(recordRoughnessList, currentRoughnessOrder));
	std::cout << "on_roughnessSequence_currentIndexChanged(int nIndex)" << endl;
};
void sdk_assist::on_roughnessPostionRecord_clicked()
{
    if (hasInvalidRequiredField({ui.roughnessFeatureNb, ui.roughnessNominalValue,
                               ui.roughnessExposeTime})) {
        emit tips(QStringLiteral("粗糙度模块存在未填写的必填数值，请补充后再记录。"));
        return;
    }
    clearInputErrorState({ui.roughnessFeatureNb, ui.roughnessNominalValue,
                          ui.roughnessExposeTime});
	emit roughnessPostionRecord();
	_sleep(150);
	m_roughnessPositionInf[currentRoughnessOrder].roughnessFeatureNb = ui.roughnessFeatureNb->text().toInt();
	m_roughnessPositionInf[currentRoughnessOrder].roughnessNominalValue = ui.roughnessNominalValue->text().toFloat();
    //m_roughnessPositionInf[currentRoughnessOrder].roughnessReferenceD = ui.roughnessReferenceD->text().toFloat();改动
	m_roughnessPositionInf[currentRoughnessOrder].roughnessPostionNote = ui.roughnessPostionNote->text();
    if (!containsOrder(recordRoughnessList, currentRoughnessOrder)) {
        recordRoughnessList.push_back(currentRoughnessOrder);
    }
    recordRoughnessNb_all = static_cast<int>(recordRoughnessList.size());
    updateRecordButtonState(ui.roughnessPostionRecord, true);
	updateRoughnessPostionInf(currentRoughnessOrder);
	updatePostionInfOut();
	std::cout << "on_roughnessPostionRecord_clicked()" << endl;
};
void sdk_assist::on_roughnessPostionClear_clicked()
{
	if (!containsOrder(recordRoughnessList, currentRoughnessOrder))
	{
		emit tips("当前顺序号尚未记录粗糙度点位，请检查！");
		return;
	}
	clearSingleRoughness(currentRoughnessOrder);
	for (vector<int>::iterator iter = recordRoughnessList.begin(); iter != recordRoughnessList.end();)
	{
		if (*iter == currentRoughnessOrder)
		{
			iter = recordRoughnessList.erase(iter);
			break;
		}
	}
    recordRoughnessNb_all = static_cast<int>(recordRoughnessList.size());
	updateRoughnessPostionInf(currentRoughnessOrder);
    updateRecordButtonState(ui.roughnessPostionRecord, false);
	updatePostionInfOut();
	std::cout << "on_roughnessPostionClear_clickes()" << endl;
};
void sdk_assist::on_roughnessReferenceDRecord_clicked()//改动（该函数为新加内容）
{
	emit roughnessReferenceDRecord();
	_sleep(550);
	ui.roughnessReferenceD->setText(QString::number(m_roughnessPositionInf[currentRoughnessOrder].roughnessReferenceD));
	//updateRoughnessPostionInf(currentRoughnessOrder);
};
void sdk_assist::clearSingleRoughness(int index)
{
	m_roughnessPositionInf[index].roughnessFeatureNb = 0;
	m_roughnessPositionInf[index].roughnessNominalValue = 0;
	m_roughnessPositionInf[index].roughnessExposeTime = 550;
	m_roughnessPositionInf[index].roughnessReferenceD = 0;
	m_roughnessPositionInf[index].roughnessReferenceDEncodePostion = 0;//改动
	m_roughnessPositionInf[index].roughnessReferenceDRealPostion = 0;//改动
	m_roughnessPositionInf[index].roughnessPostionNote = "-";
	m_roughnessPositionInf[index].axisGuangMuEncodePostion = 0;
	m_roughnessPositionInf[index].axisGuangMuRealPostion = 0;
	m_roughnessPositionInf[index].axisRoughnessEncodePostion = 0;
	m_roughnessPositionInf[index].axisRoughnessRealPostion = 0;
}
void  sdk_assist::updateRoughnessPostionInf(int selectedPostion)
{
	ui.roughnessFeatureNb->setText(QString::number(m_roughnessPositionInf[selectedPostion].roughnessFeatureNb));
	ui.roughnessNominalValue->setText(QString::number(m_roughnessPositionInf[selectedPostion].roughnessNominalValue));
	ui.roughnessExposeTime->setText(QString::number(m_roughnessPositionInf[selectedPostion].roughnessExposeTime));
	ui.roughnessReferenceD->setText(QString::number(m_roughnessPositionInf[selectedPostion].roughnessReferenceD));
	ui.roughnessPostionNote->setText(m_roughnessPositionInf[selectedPostion].roughnessPostionNote);
};

//圆柱度输出槽函数*****************************************************************************************************************************************************************************************
void sdk_assist::on_cylindricitySequence_currentIndexChanged(int nIndex)
{
	currentCylindricityOrder = nIndex;
	updateCylindricityPostionInf(currentCylindricityOrder);
    updateRecordButtonState(ui.cylindricityPostionRecord,
                            containsOrder(recordCylindricityList, currentCylindricityOrder));
	std::cout << "on_cylindricitySequence_currentIndexChanged(int nIndex)" << endl;
};
void sdk_assist::on_cylindricityPostionRecord_clicked()
{
    if (hasInvalidRequiredField({ui.cylindricityFeatureNb, ui.cylindricityNominalValue,
                               ui.cylindricityUpperRelativeLocation,
                               ui.cylindricityBottomRelativeLocation})) {
        emit tips(QStringLiteral("圆柱度模块存在未填写的必填数值，请补充后再记录。"));
        return;
    }
    clearInputErrorState({ui.cylindricityFeatureNb, ui.cylindricityNominalValue,
                          ui.cylindricityUpperRelativeLocation,
                          ui.cylindricityBottomRelativeLocation});
	cylindricityBottomRelativeLocation_current=ui.cylindricityBottomRelativeLocation->text().toInt();
	cylindricityUpperRelativeLocation_current = ui.cylindricityUpperRelativeLocation->text().toInt();
	if (cylindricityUpperRelativeLocation_current >= 0 && cylindricityBottomRelativeLocation_current >= 0) {
		emit cylindricityPostionRecord();
		_sleep(150);
		m_cylindricityPositionInf[currentCylindricityOrder].cylindricityFeatureNb = ui.cylindricityFeatureNb->text().toInt();
		m_cylindricityPositionInf[currentCylindricityOrder].cylindricityNominalValue = ui.cylindricityNominalValue->text().toFloat();
		m_cylindricityPositionInf[currentCylindricityOrder].cylindricityPostionNote = ui.cylindricityPostionNote->text();
		m_cylindricityPositionInf[currentCylindricityOrder].cylindricityPostionNote = ui.cylindricityPostionNote->text();
        if (!containsOrder(recordCylindricityList, currentCylindricityOrder)) {
            recordCylindricityList.push_back(currentCylindricityOrder);
        }
        recordCylindricityNb_all = static_cast<int>(recordCylindricityList.size());
        updateRecordButtonState(ui.cylindricityPostionRecord, true);
		updateCylindricityPostionInf(currentCylindricityOrder);
		updatePostionInfOut();
	}
	else {
		emit tips("上下偏移脉冲数量应为非负数，请修改后重新保存！");
	}
	
	std::cout << "on_cylindricityPostionRecord_clicked()" << endl;
};
void sdk_assist::on_cylindricityPostionClear_clicked()
{
	if (!containsOrder(recordCylindricityList, currentCylindricityOrder))
	{
		emit tips("当前顺序号尚未记录圆柱度点位，请检查！");
		return;
	}
	clearSingleCylindricity(currentCylindricityOrder);
	for (vector<int>::iterator iter = recordCylindricityList.begin(); iter != recordCylindricityList.end();)
	{
		if (*iter == currentCylindricityOrder)
		{
			iter = recordCylindricityList.erase(iter);
			break;
		}
	}
    recordCylindricityNb_all = static_cast<int>(recordCylindricityList.size());
	updateCylindricityPostionInf(currentCylindricityOrder);
    updateRecordButtonState(ui.cylindricityPostionRecord, false);
	updatePostionInfOut();
	std::cout << "on_cylindricityPostionClear_clickes()" << endl;
};
void sdk_assist::clearSingleCylindricity(int index)
{
	cylindricityBottomRelativeLocation_current = 0;
	cylindricityUpperRelativeLocation_current = 0;
	m_cylindricityPositionInf[index].cylindricityFeatureNb = 0;
	m_cylindricityPositionInf[index].cylindricityNominalValue = 0;
	m_cylindricityPositionInf[index].cylindricityPostionNote = "-";
	m_cylindricityPositionInf[index].axisGuangMuEncodePostion_bottom = 0;
	m_cylindricityPositionInf[index].axisGuangMuRealPostion_bottom = 0;
	m_cylindricityPositionInf[index].axisGuangMuEncodePostion_middle = 0;
	m_cylindricityPositionInf[index].axisGuangMuRealPostion_middle = 0;
	m_cylindricityPositionInf[index].axisGuangMuEncodePostion_upper = 0;
	m_cylindricityPositionInf[index].axisGuangMuRealPostion_upper = 0;
}
void  sdk_assist::updateCylindricityPostionInf(int selectedPostion)
{
	ui.cylindricityFeatureNb->setText(QString::number(m_cylindricityPositionInf[selectedPostion].cylindricityFeatureNb));
	ui.cylindricityNominalValue->setText(QString::number(m_cylindricityPositionInf[selectedPostion].cylindricityNominalValue));
	ui.cylindricityUpperRelativeLocation->setText(QString::number(m_cylindricityPositionInf[selectedPostion].axisGuangMuEncodePostion_upper- m_cylindricityPositionInf[selectedPostion].axisGuangMuEncodePostion_middle));
	ui.cylindricityBottomRelativeLocation->setText(QString::number(m_cylindricityPositionInf[selectedPostion].axisGuangMuEncodePostion_middle - m_cylindricityPositionInf[selectedPostion].axisGuangMuEncodePostion_bottom));
	ui.cylindricityPostionNote->setText(m_cylindricityPositionInf[selectedPostion].cylindricityPostionNote);
	std::cout << "updateCylindricityPostionInf(int selectedPostion)" << endl;
};

//跳动输出槽函数*****************************************************************************************************************************************************************************************
void sdk_assist::on_roundoutSequence_currentIndexChanged(int nIndex)
{
	currentRoundoutOrder = nIndex;
	updateRoundoutPostionInf(currentRoundoutOrder);
    updateRecordButtonState(ui.roundoutPostionRecord,
                            containsOrder(recordRoundoutList, currentRoundoutOrder));
	std::cout << "on_roundoutSequence_currentIndexChanged(int nIndex)" << endl;
};
void sdk_assist::on_roundoutPostionRecord_clicked()
{
    if (hasInvalidRequiredField({ui.roundoutFeatureNb, ui.roundoutNominalValue,
                               ui.roundoutUpperRelativeLocation,
                               ui.roundoutBottomRelativeLocation})) {
        emit tips(QStringLiteral("跳动模块存在未填写的必填数值，请补充后再记录。"));
        return;
    }
    clearInputErrorState({ui.roundoutFeatureNb, ui.roundoutNominalValue,
                          ui.roundoutUpperRelativeLocation,
                          ui.roundoutBottomRelativeLocation});
	roundoutBottomRelativeLocation_current = ui.roundoutUpperRelativeLocation->text().toInt();
	roundoutUpperRelativeLocation_current = ui.roundoutBottomRelativeLocation->text().toInt();
	if (roundoutBottomRelativeLocation_current >= 0 && roundoutUpperRelativeLocation_current >= 0) {
		emit roundoutPostionRecord();
		_sleep(150);
		m_roundoutPositionInf[currentRoundoutOrder].roundoutFeatureNb = ui.roundoutFeatureNb->text().toInt();
		m_roundoutPositionInf[currentRoundoutOrder].roundoutNominalValue = ui.roundoutNominalValue->text().toFloat();
		m_roundoutPositionInf[currentRoundoutOrder].roundoutPostionNote1 = ui.roundoutPostionNote1->text();//改动
		m_roundoutPositionInf[currentRoundoutOrder].roundoutPostionNote2 = ui.roundoutPostionNote2->text();//改动
        if (!containsOrder(recordRoundoutList, currentRoundoutOrder)) {
            recordRoundoutList.push_back(currentRoundoutOrder);
        }
        recordRoundoutNb_all = static_cast<int>(recordRoundoutList.size());
        updateRecordButtonState(ui.roundoutPostionRecord, true);
		updateRoundoutPostionInf(currentRoundoutOrder);
		updatePostionInfOut();
	}
	else {
		emit tips("上下偏移脉冲数量应为非负数，请修改后重新保存！");
	}
	
	std::cout << "on_roundoutPostionRecord_clicked()" << endl;
};
void sdk_assist::on_roundoutPostionClear_clicked()
{
	if (!containsOrder(recordRoundoutList, currentRoundoutOrder))
	{
		emit tips("当前顺序号尚未记录跳动点位，请检查！");
		return;
	}
	clearSingleRoundout(currentRoundoutOrder);
	for (vector<int>::iterator iter = recordRoundoutList.begin(); iter != recordRoundoutList.end();)
	{
		if (*iter == currentRoundoutOrder)
		{
			iter = recordRoundoutList.erase(iter);
			break;
		}
	}
    recordRoundoutNb_all = static_cast<int>(recordRoundoutList.size());
	updateRoundoutPostionInf(currentRoundoutOrder);
    updateRecordButtonState(ui.roundoutPostionRecord, false);
	updatePostionInfOut();
	std::cout << "on_roundoutPostionClear_clickes()" << endl;
};
void sdk_assist::clearSingleRoundout(int index)
{
	roundoutBottomRelativeLocation_current = 0;
	roundoutUpperRelativeLocation_current = 0;
	m_roundoutPositionInf[index].roundoutFeatureNb = 0;
	m_roundoutPositionInf[index].roundoutNominalValue = 0;
	m_roundoutPositionInf[index].roundoutPostionNote1 = "-";//改动
	m_roundoutPositionInf[index].roundoutPostionNote2 = "-";//改动
	m_roundoutPositionInf[index].axisGuangMuEncodePostion_bottom = 0;
	m_roundoutPositionInf[index].axisGuangMuRealPostion_bottom = 0;
	m_roundoutPositionInf[index].axisGuangMuEncodePostion_middle = 0;
	m_roundoutPositionInf[index].axisGuangMuRealPostion_middle = 0;
	m_roundoutPositionInf[index].axisGuangMuEncodePostion_upper = 0;
	m_roundoutPositionInf[index].axisGuangMuRealPostion_upper = 0;
};
void  sdk_assist::updateRoundoutPostionInf(int selectedPostion)
{
	ui.roundoutFeatureNb->setText(QString::number(m_roundoutPositionInf[selectedPostion].roundoutFeatureNb));
	ui.roundoutNominalValue->setText(QString::number(m_roundoutPositionInf[selectedPostion].roundoutNominalValue));
	ui.roundoutUpperRelativeLocation->setText(QString::number(m_roundoutPositionInf[selectedPostion].axisGuangMuEncodePostion_upper- m_roundoutPositionInf[selectedPostion].axisGuangMuEncodePostion_middle));
	ui.roundoutBottomRelativeLocation->setText(QString::number(m_roundoutPositionInf[selectedPostion].axisGuangMuEncodePostion_middle - m_roundoutPositionInf[selectedPostion].axisGuangMuEncodePostion_bottom));
	ui.roundoutPostionNote1->setText(m_roundoutPositionInf[selectedPostion].roundoutPostionNote1);
	ui.roundoutPostionNote2->setText(m_roundoutPositionInf[selectedPostion].roundoutPostionNote2);
	std::cout << "updateRoundoutPostionInf(int selectedPostion)" << endl;
};

//孔径输出槽函数*****************************************************************************************************************************************************************************************
void sdk_assist::on_holeSequence_currentIndexChanged(int nIndex)
{
	currentHoleOrder = nIndex;
	updateHolePostionInf(currentHoleOrder);
    updateRecordButtonState(ui.holePostionRecord,
                            containsOrder(recordHoleList, currentHoleOrder));
	std::cout << "on_holeSequence_currentIndexChanged(int nIndex)" << endl;
};
void sdk_assist::on_holePostionRecord_clicked()
{
    if (hasInvalidRequiredField({ui.holeFeatureNb, ui.holeNominalValue,
                               ui.holeUpperOffset, ui.holeBottomOffset,
                               ui.holeNumber, ui.holeExposeTime})) {
        emit tips(QStringLiteral("孔径模块存在未填写的必填数值，请补充后再记录。"));
        return;
    }
    clearInputErrorState({ui.holeFeatureNb, ui.holeNominalValue,
                          ui.holeUpperOffset, ui.holeBottomOffset,
                          ui.holeNumber, ui.holeExposeTime});
	emit holePostionRecord();
	_sleep(150);
	m_holePositionInf[currentHoleOrder].holeFeatureNb = ui.holeFeatureNb->text().toInt();
	m_holePositionInf[currentHoleOrder].holeNominalValue = ui.holeNominalValue->text().toFloat();
	m_holePositionInf[currentHoleOrder].holeUpperOffset = ui.holeUpperOffset->text().toFloat();
	m_holePositionInf[currentHoleOrder].holeBottomOffset = ui.holeBottomOffset->text().toFloat();
	m_holePositionInf[currentHoleOrder].holeNumber = ui.holeNumber->text().toInt();
	m_holePositionInf[currentHoleOrder].holePostionNote = ui.holePostionNote->text();
    if (!containsOrder(recordHoleList, currentHoleOrder)) {
        recordHoleList.push_back(currentHoleOrder);
    }
    recordHoleNb_all = static_cast<int>(recordHoleList.size());
    updateRecordButtonState(ui.holePostionRecord, true);
	updateHolePostionInf(currentHoleOrder);
	updatePostionInfOut();
	std::cout << "on_holePostionRecord_clicked()" << endl;
};
void sdk_assist::on_holePostionClear_clicked()
{
	if (!containsOrder(recordHoleList, currentHoleOrder))
	{
		emit tips("当前顺序号尚未记录孔径点位，请检查！");
		return;
	}
	clearSingleHole(currentHoleOrder);
	for (vector<int>::iterator iter = recordHoleList.begin(); iter != recordHoleList.end();)
	{
		if (*iter == currentHoleOrder)
		{
			iter = recordHoleList.erase(iter);
			break;
		}
	}
    recordHoleNb_all = static_cast<int>(recordHoleList.size());
	updateHolePostionInf(currentHoleOrder);
    updateRecordButtonState(ui.holePostionRecord, false);
	updatePostionInfOut();
	std::cout << "on_holePostionClear_clickes()" << endl;
};
void sdk_assist::clearSingleHole(int index)
{
	m_holePositionInf[index].holeFeatureNb = 0;
	m_holePositionInf[index].holeNominalValue = 0;
	m_holePositionInf[index].holeUpperOffset = 0;
	m_holePositionInf[index].holeBottomOffset = 0;
	m_holePositionInf[index].holeNumber = 0;
	m_holePositionInf[index].holeExposeTime = 550;
	m_holePositionInf[index].holePostionNote = "-";
	m_holePositionInf[index].axisGuangMuEncodePostion = 0;
	m_holePositionInf[index].axisGuangMuRealPostion = 0;
	m_holePositionInf[index].axisHoleEncodePostion = 0;
	m_holePositionInf[index].axisHoleRealPostion = 0;
};
void  sdk_assist::updateHolePostionInf(int selectedPostion)
{
	ui.holeFeatureNb->setText(QString::number(m_holePositionInf[selectedPostion].holeFeatureNb));
	ui.holeNominalValue->setText(QString::number(m_holePositionInf[selectedPostion].holeNominalValue));
	ui.holeUpperOffset->setText(QString::number(m_holePositionInf[selectedPostion].holeUpperOffset));
	ui.holeBottomOffset->setText(QString::number(m_holePositionInf[selectedPostion].holeBottomOffset));
	ui.holeNumber->setText(QString::number(m_holePositionInf[selectedPostion].holeNumber));
	ui.holeExposeTime->setText(QString::number(m_holePositionInf[selectedPostion].holeExposeTime));
	ui.holePostionNote->setText(m_holePositionInf[selectedPostion].holePostionNote);
};


//远心输出槽函数*****************************************************************************************************************************************************************************************
void sdk_assist::on_telecentricSequence_currentIndexChanged(int nIndex)
{
	currentTelecentricOrder = nIndex;
	updateTelecentricPostionInf(currentTelecentricOrder);
    updateRecordButtonState(ui.telecentricPostionRecord,
                            containsOrder(recordTelecentricList, currentTelecentricOrder));
	std::cout << "on_telecentricSequence_currentIndexChanged(int nIndex)" << endl;
};
void sdk_assist::on_telecentricPostionRecord_clicked()
{
    if (hasInvalidRequiredField({ui.telecentricExposeTime})) {
        emit tips(QStringLiteral("远心模块的曝光值不能为空，请补充后再记录。"));
        return;
    }
    clearInputErrorState({ui.telecentricExposeTime});
	emit telecentricPostionRecord();
	_sleep(150);
	m_telecentricPositionInf[currentTelecentricOrder].telecentricPostionNote = ui.telecentricPostionNote->text();
    if (!containsOrder(recordTelecentricList, currentTelecentricOrder)) {
        recordTelecentricList.push_back(currentTelecentricOrder);
    }
    recordTelecentricNb_all = static_cast<int>(recordTelecentricList.size());
    updateRecordButtonState(ui.telecentricPostionRecord, true);
	updateTelecentricPostionInf(currentTelecentricOrder);
	updatePostionInfOut();
	std::cout << "on_telecentricPostionRecord_clicked()" << endl;
};
void sdk_assist::on_telecentricPostionClear_clicked()
{
    if (!containsOrder(recordTelecentricList, currentTelecentricOrder)) {
        emit tips(QStringLiteral("当前顺序号尚未记录远心点位，请检查！"));
        return;
    }
	clearSingleTelecentric(currentTelecentricOrder);
	for (vector<int>::iterator iter = recordTelecentricList.begin(); iter != recordTelecentricList.end();)
	{
		if (*iter == currentTelecentricOrder)
		{
			iter = recordTelecentricList.erase(iter);
			break;
		}
	}
    recordTelecentricNb_all = static_cast<int>(recordTelecentricList.size());
	updateTelecentricPostionInf(currentTelecentricOrder);
    updateRecordButtonState(ui.telecentricPostionRecord, false);
	updatePostionInfOut();
	std::cout << "on_telecentricPostionClear_clickes()" << endl;
};
void sdk_assist::clearSingleTelecentric(int index)
{
	m_telecentricPositionInf[index].telecentricExposeTime = 550;
	m_telecentricPositionInf[index].telecentricPostionNote = "-";
	m_telecentricPositionInf[index].axisGuangMuEncodePostion = 0;
	m_telecentricPositionInf[index].axisGuangMuRealPostion = 0;
};
void  sdk_assist::updateTelecentricPostionInf(int selectedPostion)
{
	ui.telecentricExposeTime->setText(QString::number(m_telecentricPositionInf[selectedPostion].telecentricExposeTime));
	ui.telecentricPostionNote->setText(m_telecentricPositionInf[selectedPostion].telecentricPostionNote);
};

//点位信息输出槽函数
void sdk_assist::updatePostionInfOut()
{
	ui.recordDiameterNb->setText(QString::number(recordDiameterNb_all));
	ui.recordTelecentricNb->setText(QString::number(recordTelecentricNb_all));
	ui.recordCylindricityNb->setText(QString::number(recordCylindricityNb_all));
	ui.recordRoughnessNb->setText(QString::number(recordRoughnessNb_all));
	ui.recordRoundoutNb->setText(QString::number(recordRoundoutNb_all));
	ui.recordHoleNb->setText(QString::number(recordHoleNb_all));
	ui.recordpartNb->setText(recordpartNb);
	ui.recordpartName->setText(recordpartName);
	ui.recordpartProcessingNb->setText(recordpartProcessingNb);
	ui.recordpartNote->setText(recordpartNote);
};
void sdk_assist::on_PostionRecordOut_clicked()
{
    const bool hasNoRecordedPosition =
        recordDiameterNb_all == 0 &&
        recordRoughnessNb_all == 0 &&
        recordCylindricityNb_all == 0 &&
        recordRoundoutNb_all == 0 &&
        recordHoleNb_all == 0 &&
        recordTelecentricNb_all == 0;

    if (hasNoRecordedPosition) {
        const QString message = QStringLiteral("当前没有已记录点位，无法生成点位文件。");
        setOutputStatus(message, "error");
        emit tips(message);
        return;
    }

    setOutputStatus(QStringLiteral("正在生成点位文件，请稍候……"), "working");
    saveAsExcel();
};
void sdk_assist::on_PostionClearOut_clicked()
{
    const auto confirmation = QMessageBox::question(
        this,
        QStringLiteral("确认清空全部点位"),
        QStringLiteral("确定要清空全部已记录点位和零件信息吗？此操作无法撤销。"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (confirmation != QMessageBox::Yes)
        return;

	recordDiameterNb_all = 0;
	recordTelecentricNb_all = 0;
	recordCylindricityNb_all = 0;
	recordRoughnessNb_all = 0;
	recordRoundoutNb_all = 0;
	recordHoleNb_all = 0;
	recordpartNb = "-";
	recordpartName = "-";
	recordpartProcessingNb = "-";
	recordpartNote = "-";
	recordDiameterList.clear();
	recordDiameterList.shrink_to_fit();

	for (int i = 0; i < 100; i++)
	{
		if (i <= 29)
		{
			clearSingleDiameter(i);
			clearSingleRoughness(i);
			clearSingleCylindricity(i);
			clearSingleRoundout(i);
			clearSingleHole(i);
			clearSingleTelecentric(i);
		}
		else
		{
			clearSingleDiameter(i);
			clearSingleCylindricity(i);
			clearSingleRoundout(i);
		}
	};
	recordDiameterList.clear();
	recordDiameterList.shrink_to_fit();
	recordRoughnessList.clear();
	recordRoughnessList.shrink_to_fit();
	recordCylindricityList.clear();
	recordCylindricityList.shrink_to_fit();
	recordRoundoutList.clear();
	recordRoundoutList.shrink_to_fit();
	recordHoleList.clear();
	recordHoleList.shrink_to_fit();
	recordTelecentricList.clear();
	recordTelecentricList.shrink_to_fit();
    updatePostionInfOut();
    updateRecordButtonState(ui.diameterPostionRecord, false);
    updateRecordButtonState(ui.roughnessPostionRecord, false);
    updateRecordButtonState(ui.cylindricityPostionRecord, false);
    updateRecordButtonState(ui.roundoutPostionRecord, false);
    updateRecordButtonState(ui.holePostionRecord, false);
    updateRecordButtonState(ui.telecentricPostionRecord, false);
    setOutputStatus(QStringLiteral("点位信息已清空，尚未生成新文件。"), "idle");
	emit tips("已清空所有点位信息！");
};
void sdk_assist::on_recordpartNb_editingFinished()
{
	recordpartNb = ui.recordpartNb->text();
};
void sdk_assist::on_recordpartName_editingFinished()
{
	recordpartName = ui.recordpartName->text();
};
void sdk_assist::on_recordpartProcessingNb_editingFinished()
{
	recordpartProcessingNb = ui.recordpartProcessingNb->text();
};
void sdk_assist::on_recordpartNote_editingFinished()
{
	recordpartNote = ui.recordpartNote->text();
};

//保存函数
bool sdk_assist::mergeCells(QString start, QString end, QString value)
{
	if (worksheet == NULL)
		return false;
	QAxObject* mergeRange = worksheet->querySubObject("range(const Qvariant&)", QVariant("=(" + start + ": " + end + ")"));
	if (mergeRange == NULL)
		return false;

	mergeRange->setProperty("MergeCells", true); // 合并单元格
	mergeRange->setProperty("Value", value);
	return true;
}

void sdk_assist::saveAsExcel()
{
	QDir().mkpath(runtimePath("SDKpostion"));
	QString excelPath = runtimePath(QString("SDKpostion/%1-%2.xlsx").arg(recordpartNb).arg(recordpartProcessingNb));
	std::cout << "excel   "<< excelPath.toStdString()<< std::endl;

	//string excelPath = "C:\\Users\\Administrator\\Desktop\\measureResult.xlsx";
	
	CoInitializeEx(NULL, COINITBASE_MULTITHREADED);
	QAxObject* excel = new QAxObject;
	QAxObject* workbooks;
	QAxObject* worksheets;
	QAxObject* range;
	QAxObject* colm;
	QAxObject* row;
	QAxObject* font;
	QAxObject* cell;
	int currentWorksheet=0;
	if (excel->setControl("Excel.Application"))
	{
		excel->dynamicCall("SetVisible (bool Visible)", false);
		excel->setProperty("DisplayAlerts", false);
		workbooks = excel->querySubObject("WorkBooks");            //获取工作簿集合
		workbooks->dynamicCall("Add");                                        //新建一个工作簿
		workbook = excel->querySubObject("ActiveWorkBook");        //获取当前工作簿
		worksheets= workbook->querySubObject("Sheets");            //获取sheets集合
		worksheets->querySubObject("Add()");
		worksheets->querySubObject("Add()");
		worksheets->querySubObject("Add()");
		worksheets->querySubObject("Add()");
		worksheets->querySubObject("Add()");
		worksheet = workbook->querySubObject("Worksheets(int)", 1);
		worksheet->setProperty("Name", "zhijing");
		worksheet = workbook->querySubObject("Worksheets(int)", 2);
		worksheet->setProperty("Name", "cucaodu");
		worksheet = workbook->querySubObject("Worksheets(int)", 3);
		worksheet->setProperty("Name", "yuanxin");
		worksheet = workbook->querySubObject("Worksheets(int)", 4);
		worksheet->setProperty("Name", "yuanzhudu");
		worksheet = workbook->querySubObject("Worksheets(int)", 5);
		worksheet->setProperty("Name", "tiaodong");
		worksheet = workbook->querySubObject("Worksheets(int)", 6);
		worksheet->setProperty("Name", "kongjing");
		sort(recordDiameterList.begin(), recordDiameterList.end());
		auto last = unique(recordDiameterList.begin(), recordDiameterList.end());
		recordDiameterList.erase(last, recordDiameterList.end());
		recordDiameterNb_all = recordDiameterList.size();
		if (recordDiameterNb_all > 0)//写入直径点位
		{
			worksheet = workbook->querySubObject("Worksheets(int)", 1);
			//添加直径Excel表头数据

			bool mergeCellsFlag;
			mergeCellsFlag = mergeCells("A1", "F1", "直径二次开发测量点位记录表");
			cell = worksheet->querySubObject("Cells(int,int)", 2, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("装夹方式：\t"));
			mergeCellsFlag = mergeCells("B2", "F2", recordpartNote);
			cell = worksheet->querySubObject("Cells(int,int)", 3, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("零件图号：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 2);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartNb));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 3);
			cell->dynamicCall("SetValue(const QString&)", QVariant("零件名称：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 4);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartName));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 5);
			cell->dynamicCall("SetValue(const QString&)", QVariant("工序号：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 6);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartProcessingNb));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 7);
			cell->dynamicCall("SetValue(const QString&)", QVariant("直径点位总数：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 8);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordDiameterNb_all));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("直径测量顺序号\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 2);
			cell->dynamicCall("SetValue(const QString&)", QVariant("直径特征号\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 3);
			cell->dynamicCall("SetValue(const QString&)", QVariant("公称值\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 4);
			cell->dynamicCall("SetValue(const QString&)", QVariant("上偏差\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 5);
			cell->dynamicCall("SetValue(const QString&)", QVariant("下偏差\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 6);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕轴点位\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 7);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕轴移动距离\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 8);
			cell->dynamicCall("SetValue(const QString&)", QVariant("备注\t"));

			font = worksheet->querySubObject("Range(const QString&)", "A1:H1")->querySubObject("Font");// 表头单元格字体设置
			font->setProperty("Bold", true);// 设置单元格字体加粗
			font->setProperty("Size", 36);// 设置单元格字体大小
			font = worksheet->querySubObject("Range(const QString&)", "A2:H3")->querySubObject("Font");// 表头单元格字体设置
			font->setProperty("Bold", true);// 设置单元格字体加粗
			font->setProperty("Size", 14);// 设置单元格字体大小
			colm = worksheet->querySubObject("Columns(const QString&)", "A");
			colm->setProperty("ColumnWidth", 15);
			colm = worksheet->querySubObject("Columns(const QString&)", "B");
			colm->setProperty("ColumnWidth", 10);
			colm = worksheet->querySubObject("Columns(const QString&)", "C:G");
			colm->setProperty("ColumnWidth", 20);
			colm = worksheet->querySubObject("Columns(const QString&)", "H");
			colm->setProperty("ColumnWidth", 40);
			row = worksheet->querySubObject("Rows(const QString&)", "1");
			row->setProperty("RowHeight", 40);
			row = worksheet->querySubObject("Rows(const QString&)", "2:4");
			row->setProperty("RowHeight", 30);
			for (int i = 0; i < recordDiameterNb_all; i++)
			{
				int currentRow = i + 5;
				int currentDsequence = recordDiameterList[i];

				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 1);
				cell->dynamicCall("SetValue(const QString&)", QVariant(currentDsequence));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 2);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_diameterPositionInf[currentDsequence].diameterFeatureNb));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 3);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_diameterPositionInf[currentDsequence].diameterNominalValue));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 4);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_diameterPositionInf[currentDsequence].diameterUpperOffset));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 5);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_diameterPositionInf[currentDsequence].diameterBottomOffset));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 6);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_diameterPositionInf[currentDsequence].axisGuangMuEncodePostion));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 7);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_diameterPositionInf[currentDsequence].axisGuangMuRealPostion));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 8);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_diameterPositionInf[currentDsequence].diameterPostionNote));
			};

			
			//全局表格设置
			range = worksheet->querySubObject("UsedRange");
			QAxObject* cells = range->querySubObject("Columns");
			//cells->dynamicCall("AutoFit");//这句代码可以使得所有单元格自适应宽度

			range->setProperty("HorizontalAlignment", -4108);//水平居中
			range->setProperty("VerticalAlignment", -4108);//垂直居中
			QAxObject* border = range->querySubObject("Borders");
			border->setProperty("Color", QColor(0, 0, 0));
			std::cout << "saveAsExcel_diamete_finished" << endl;
		}
		sort( recordRoughnessList.begin(),  recordRoughnessList.end());
		 last = unique( recordRoughnessList.begin(),  recordRoughnessList.end());
		 recordRoughnessList.erase(last,  recordRoughnessList.end());
		 recordRoughnessNb_all =  recordRoughnessList.size();
		if (recordRoughnessNb_all > 0)//写入粗糙度点位
			{
				worksheet = workbook->querySubObject("Worksheets(int)", 2);
				//添加远心Excel表头数据

				bool mergeCellsFlag;
				mergeCellsFlag = mergeCells("A1", "F1", "粗糙度二次开发测量点位记录表");
				cell = worksheet->querySubObject("Cells(int,int)", 2, 1);
				cell->dynamicCall("SetValue(const QString&)", QVariant("装夹方式：\t"));
				mergeCellsFlag = mergeCells("B2", "F2", recordpartNote);
				cell = worksheet->querySubObject("Cells(int,int)", 3, 1);
				cell->dynamicCall("SetValue(const QString&)", QVariant("零件图号：\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 3, 2);
				cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartNb));
				cell = worksheet->querySubObject("Cells(int,int)", 3, 3);
				cell->dynamicCall("SetValue(const QString&)", QVariant("零件名称：\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 3, 4);
				cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartName));
				cell = worksheet->querySubObject("Cells(int,int)", 3, 5);
				cell->dynamicCall("SetValue(const QString&)", QVariant("工序号：\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 3, 6);
				cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartProcessingNb));
				//mergeCellsFlag = mergeCells("G3", "H3", "粗糙度点位总数：\t");
				//cell = worksheet->querySubObject("Cells(int,int)", 3, 9);
				cell->dynamicCall("SetValue(const QString&)", QVariant(recordRoughnessNb_all));
				cell = worksheet->querySubObject("Cells(int,int)", 4, 1);
				cell->dynamicCall("SetValue(const QString&)", QVariant("粗糙度测量顺序号\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 4, 2);
				cell->dynamicCall("SetValue(const QString&)", QVariant("粗糙度特征号\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 4, 3);
				cell->dynamicCall("SetValue(const QString&)", QVariant("公称值\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 4, 4);
				cell->dynamicCall("SetValue(const QString&)", QVariant("粗糙度轴点位\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 4, 5);
				cell->dynamicCall("SetValue(const QString&)", QVariant("粗糙度轴移动距离\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 4, 6);
				cell->dynamicCall("SetValue(const QString&)", QVariant("光幕轴点位\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 4, 7);
				cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴移动距离\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 4, 8);
				cell->dynamicCall("SetValue(const QString&)", QVariant("粗糙度相机曝光\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 4, 9);
				cell->dynamicCall("SetValue(const QString&)", QVariant("参考直径\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 4, 10);
				cell->dynamicCall("SetValue(const QString&)", QVariant("参考直径光幕位置\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 4, 11);
				cell->dynamicCall("SetValue(const QString&)", QVariant("参考直径光幕移动距离\t"));
				cell = worksheet->querySubObject("Cells(int,int)", 4, 12);
				cell->dynamicCall("SetValue(const QString&)", QVariant("备注\t"));

				font = worksheet->querySubObject("Range(const QString&)", "A1:L1")->querySubObject("Font");// 表头单元格字体设置
				font->setProperty("Bold", true);// 设置单元格字体加粗
				font->setProperty("Size", 36);// 设置单元格字体大小
				font = worksheet->querySubObject("Range(const QString&)", "A2:L3")->querySubObject("Font");// 表头单元格字体设置
				font->setProperty("Bold", true);// 设置单元格字体加粗
				font->setProperty("Size", 14);// 设置单元格字体大小
				colm = worksheet->querySubObject("Columns(const QString&)", "A");
				colm->setProperty("ColumnWidth", 15);
				colm = worksheet->querySubObject("Columns(const QString&)", "B");
				colm->setProperty("ColumnWidth", 20);
				colm = worksheet->querySubObject("Columns(const QString&)", "C:K");
				colm->setProperty("ColumnWidth", 20);
				colm = worksheet->querySubObject("Columns(const QString&)", "L");
				colm->setProperty("ColumnWidth", 40);
				row = worksheet->querySubObject("Rows(const QString&)", "1");
				row->setProperty("RowHeight", 40);
				row = worksheet->querySubObject("Rows(const QString&)", "2:4");
				row->setProperty("RowHeight", 30);
				for (int i = 0; i < recordRoughnessNb_all; i++)
				{
					int currentRow = i + 5;
					int currentDsequence = recordRoughnessList[i];

					cell = worksheet->querySubObject("Cells(int,int)", currentRow, 1);
					cell->dynamicCall("SetValue(const QString&)", QVariant(currentDsequence));
					cell = worksheet->querySubObject("Cells(int,int)", currentRow, 2);
					cell->dynamicCall("SetValue(const QString&)", QVariant(m_roughnessPositionInf[currentDsequence].roughnessFeatureNb));
					cell = worksheet->querySubObject("Cells(int,int)", currentRow, 3);
					cell->dynamicCall("SetValue(const QString&)", QVariant(m_roughnessPositionInf[currentDsequence].roughnessNominalValue));
					cell = worksheet->querySubObject("Cells(int,int)", currentRow, 4);
					cell->dynamicCall("SetValue(const QString&)", QVariant(m_roughnessPositionInf[currentDsequence].axisRoughnessEncodePostion));
					cell = worksheet->querySubObject("Cells(int,int)", currentRow, 5);
					cell->dynamicCall("SetValue(const QString&)", QVariant(m_roughnessPositionInf[currentDsequence].axisRoughnessRealPostion));
					cell = worksheet->querySubObject("Cells(int,int)", currentRow, 6);
					cell->dynamicCall("SetValue(const QString&)", QVariant(m_roughnessPositionInf[currentDsequence].axisGuangMuEncodePostion));
					cell = worksheet->querySubObject("Cells(int,int)", currentRow, 7);
					cell->dynamicCall("SetValue(const QString&)", QVariant(m_roughnessPositionInf[currentDsequence].axisGuangMuRealPostion));
					cell = worksheet->querySubObject("Cells(int,int)", currentRow, 8);
					cell->dynamicCall("SetValue(const QString&)", QVariant(m_roughnessPositionInf[currentDsequence].roughnessExposeTime));
					cell = worksheet->querySubObject("Cells(int,int)", currentRow, 9);
					cell->dynamicCall("SetValue(const QString&)", QVariant(m_roughnessPositionInf[currentDsequence].roughnessReferenceD));
					cell = worksheet->querySubObject("Cells(int,int)", currentRow, 10);
					cell->dynamicCall("SetValue(const QString&)", QVariant(m_roughnessPositionInf[currentDsequence].roughnessReferenceDEncodePostion));
					cell = worksheet->querySubObject("Cells(int,int)", currentRow, 11);
					cell->dynamicCall("SetValue(const QString&)", QVariant(m_roughnessPositionInf[currentDsequence].roughnessReferenceDRealPostion));
					cell = worksheet->querySubObject("Cells(int,int)", currentRow, 12);
					cell->dynamicCall("SetValue(const QString&)", QVariant(m_roughnessPositionInf[currentDsequence].roughnessPostionNote));
				};
				//全局表格设置
				range = worksheet->querySubObject("UsedRange");
				QAxObject* cells = range->querySubObject("Columns");
				//cells->dynamicCall("AutoFit");//这句代码可以使得所有单元格自适应宽度

				range->setProperty("HorizontalAlignment", -4108);//水平居中
				range->setProperty("VerticalAlignment", -4108);//垂直居中
				QAxObject* border = range->querySubObject("Borders");
				border->setProperty("Color", QColor(0, 0, 0));
				std::cout << "saveAsExcel_roughness_finished" << endl;
		};
		sort( recordTelecentricList.begin(),  recordTelecentricList.end());
		last = unique( recordTelecentricList.begin(),  recordTelecentricList.end());
	    recordTelecentricList.erase(last,  recordTelecentricList.end());
		recordTelecentricNb_all =  recordTelecentricList.size();
		if (recordTelecentricNb_all > 0)//写入远心点位
		{
			worksheet = workbook->querySubObject("Worksheets(int)", 3);
			//添加远心Excel表头数据

			bool mergeCellsFlag;
			mergeCellsFlag = mergeCells("A1", "B1", "远心二次开发测量点位记录表");
			cell = worksheet->querySubObject("Cells(int,int)", 2, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("装夹方式：\t"));
			mergeCellsFlag = mergeCells("B2", "E2", recordpartNote);
			cell = worksheet->querySubObject("Cells(int,int)", 3, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("零件图号：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 2);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartNb));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 3);
			cell->dynamicCall("SetValue(const QString&)", QVariant("零件名称：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 4);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartName));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 5);
			cell->dynamicCall("SetValue(const QString&)", QVariant("工序号：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 6);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartProcessingNb));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 7);
			//cell->dynamicCall("SetValue(const QString&)", QVariant("远心拍照点位总数：\t"));
			//cell = worksheet->querySubObject("Cells(int,int)", 3, 8);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordTelecentricNb_all));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("远心拍照顺序号\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 2);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕轴点位\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 3);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴移动距离\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 4);
			cell->dynamicCall("SetValue(const QString&)", QVariant("远心相机曝光\t"));
			mergeCellsFlag = mergeCells("E4", "H4", "备注\t");

			font = worksheet->querySubObject("Range(const QString&)", "A1:B1")->querySubObject("Font");// 表头单元格字体设置
			font->setProperty("Bold", true);// 设置单元格字体加粗
			font->setProperty("Size", 14);// 设置单元格字体大小
			font = worksheet->querySubObject("Range(const QString&)", "A2:H3")->querySubObject("Font");// 表头单元格字体设置
			font->setProperty("Bold", true);// 设置单元格字体加粗
			font->setProperty("Size", 14);// 设置单元格字体大小
			colm = worksheet->querySubObject("Columns(const QString&)", "A");
			colm->setProperty("ColumnWidth", 15);
			colm = worksheet->querySubObject("Columns(const QString&)", "B");
			colm->setProperty("ColumnWidth", 20);
			colm = worksheet->querySubObject("Columns(const QString&)", "C:H");
			colm->setProperty("ColumnWidth", 25);
			row = worksheet->querySubObject("Rows(const QString&)", "1");
			row->setProperty("RowHeight", 40);
			row = worksheet->querySubObject("Rows(const QString&)", "2:4");
			row->setProperty("RowHeight", 30);
			for (int i = 0; i < recordTelecentricNb_all; i++)
			{
				int currentRow = i + 5;
				int currentDsequence = recordTelecentricList[i];

				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 1);
				cell->dynamicCall("SetValue(const QString&)", QVariant(currentDsequence));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 2);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_telecentricPositionInf[currentDsequence].axisGuangMuEncodePostion));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 3);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_telecentricPositionInf[currentDsequence].axisGuangMuRealPostion));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 4);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_telecentricPositionInf[currentDsequence].telecentricExposeTime));
				QString rangeBegin = "E" + QString::number(currentRow);
				QString rangeEnd = "H" + QString::number(currentRow);
				mergeCellsFlag = mergeCells(rangeBegin, rangeEnd, m_telecentricPositionInf[currentDsequence].telecentricPostionNote);
			
			};
			//全局表格设置
			range = worksheet->querySubObject("UsedRange");
			QAxObject* cells = range->querySubObject("Columns");
			//cells->dynamicCall("AutoFit");//这句代码可以使得所有单元格自适应宽度

			range->setProperty("HorizontalAlignment", -4108);//水平居中
			range->setProperty("VerticalAlignment", -4108);//垂直居中
			QAxObject* border = range->querySubObject("Borders");
			border->setProperty("Color", QColor(0, 0, 0));
			std::cout << "saveAsExcel_telecentric_finished" << endl;
		};
		sort(recordCylindricityList.begin(), recordCylindricityList.end());
		last = unique(recordCylindricityList.begin(), recordCylindricityList.end());
		recordCylindricityList.erase(last, recordCylindricityList.end());
		recordCylindricityNb_all = recordCylindricityList.size();
		if (recordCylindricityNb_all > 0)//写入圆柱度点位
		{
			worksheet = workbook->querySubObject("Worksheets(int)", 4);
			//添加圆柱度Excel表头数据

			bool mergeCellsFlag;
			mergeCellsFlag = mergeCells("A1", "H1", "圆柱度二次开发测量点位记录表");
			cell = worksheet->querySubObject("Cells(int,int)", 2, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("装夹方式：\t"));
			mergeCellsFlag = mergeCells("B2", "H2", recordpartNote);
			cell = worksheet->querySubObject("Cells(int,int)", 3, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("零件图号：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 2);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartNb));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 3);
			cell->dynamicCall("SetValue(const QString&)", QVariant("零件名称：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 4);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartName));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 5);
			cell->dynamicCall("SetValue(const QString&)", QVariant("工序号：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 6);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartProcessingNb));
			//mergeCellsFlag = mergeCells("G3", "I3", "圆柱度点位总数：\t");
			//cell = worksheet->querySubObject("Cells(int,int)", 3, 10);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordCylindricityNb_all));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("圆柱度顺序号\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 2);
			cell->dynamicCall("SetValue(const QString&)", QVariant("圆柱度特征号\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 3);
			cell->dynamicCall("SetValue(const QString&)", QVariant("圆柱度公称值\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 4);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴点位（下侧）\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 5);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴移动距离（下侧）\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 6);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴点位（中间）\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 7);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴移动距离（中间）\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 8);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴点位（上侧）\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 9);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴移动距离（上侧）\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 10);
			cell->dynamicCall("SetValue(const QString&)", QVariant("备注\t"));

			font = worksheet->querySubObject("Range(const QString&)", "A1:J1")->querySubObject("Font");// 表头单元格字体设置
			font->setProperty("Bold", true);// 设置单元格字体加粗
			font->setProperty("Size", 36);// 设置单元格字体大小
			font = worksheet->querySubObject("Range(const QString&)", "A2:J3")->querySubObject("Font");// 表头单元格字体设置
			font->setProperty("Bold", true);// 设置单元格字体加粗
			font->setProperty("Size", 14);// 设置单元格字体大小
			colm = worksheet->querySubObject("Columns(const QString&)", "A");
			colm->setProperty("ColumnWidth", 15);
			colm = worksheet->querySubObject("Columns(const QString&)", "B");
			colm->setProperty("ColumnWidth", 15);
			colm = worksheet->querySubObject("Columns(const QString&)", "C:J");
			colm->setProperty("ColumnWidth", 25);
			row = worksheet->querySubObject("Rows(const QString&)", "1");
			row->setProperty("RowHeight", 40);
			row = worksheet->querySubObject("Rows(const QString&)", "2:4");
			row->setProperty("RowHeight", 30);
			for (int i = 0; i < recordCylindricityNb_all; i++)
			{
				int currentRow = i + 5;
				int currentDsequence = recordCylindricityList[i];

				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 1);
				cell->dynamicCall("SetValue(const QString&)", QVariant(currentDsequence));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 2);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_cylindricityPositionInf[currentDsequence].cylindricityFeatureNb));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 3);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_cylindricityPositionInf[currentDsequence].cylindricityNominalValue));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 4);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_cylindricityPositionInf[currentDsequence].axisGuangMuEncodePostion_bottom));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 5);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_cylindricityPositionInf[currentDsequence].axisGuangMuRealPostion_bottom));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 6);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_cylindricityPositionInf[currentDsequence].axisGuangMuEncodePostion_middle));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 7);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_cylindricityPositionInf[currentDsequence].axisGuangMuRealPostion_middle));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 8);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_cylindricityPositionInf[currentDsequence].axisGuangMuEncodePostion_upper));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 9);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_cylindricityPositionInf[currentDsequence].axisGuangMuRealPostion_upper));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 10);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_cylindricityPositionInf[currentDsequence].cylindricityPostionNote));
			};
			//全局表格设置
			range = worksheet->querySubObject("UsedRange");
			QAxObject* cells = range->querySubObject("Columns");
			//cells->dynamicCall("AutoFit");//这句代码可以使得所有单元格自适应宽度

			range->setProperty("HorizontalAlignment", -4108);//水平居中
			range->setProperty("VerticalAlignment", -4108);//垂直居中
			QAxObject* border = range->querySubObject("Borders");
			border->setProperty("Color", QColor(0, 0, 0));
			std::cout << "saveAsExcel_cylindricity_finished" << endl;
		};
		sort(recordRoundoutList.begin(), recordRoundoutList.end());
		last = unique(recordRoundoutList.begin(), recordRoundoutList.end());
		recordRoundoutList.erase(last, recordRoundoutList.end());
		recordRoundoutNb_all = recordRoundoutList.size();
		if (recordRoundoutNb_all > 0)//写入跳动点位
		{
			worksheet = workbook->querySubObject("Worksheets(int)", 5);
			//添加跳动Excel表头数据

			bool mergeCellsFlag;
			mergeCellsFlag = mergeCells("A1", "G1", "跳动二次开发测量点位记录表");//改动
			cell = worksheet->querySubObject("Cells(int,int)", 2, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("装夹方式：\t"));
			mergeCellsFlag = mergeCells("B2", "G2", recordpartNote);//改动
			cell = worksheet->querySubObject("Cells(int,int)", 3, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("零件图号：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 2);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartNb));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 3);
			cell->dynamicCall("SetValue(const QString&)", QVariant("零件名称：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 4);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartName));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 5);
			cell->dynamicCall("SetValue(const QString&)", QVariant("工序号：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 6);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartProcessingNb));
			//mergeCellsFlag = mergeCells("G3", "H3", "跳动点位总数：\t");//改动
			//cell = worksheet->querySubObject("Cells(int,int)", 3, 11);//改动
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordRoundoutNb_all));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("跳动顺序号\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 2);
			cell->dynamicCall("SetValue(const QString&)", QVariant("跳动特征号\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 3);
			cell->dynamicCall("SetValue(const QString&)", QVariant("跳动公称值\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 4);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴点位（下侧）\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 5);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴移动距离（下侧）\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 6);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴点位（中间）\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 7);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴移动距离（中间）\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 8);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴点位（上侧）\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 9);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕光幕轴移动距离（上侧）\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 10);
			cell->dynamicCall("SetValue(const QString&)", QVariant("跳动基准1\t"));//改动
			cell = worksheet->querySubObject("Cells(int,int)", 4, 11);
			cell->dynamicCall("SetValue(const QString&)", QVariant("跳动基准2\t"));//改动

			font = worksheet->querySubObject("Range(const QString&)", "A1:K1")->querySubObject("Font");// 表头单元格字体设置(改动)
			font->setProperty("Bold", true);// 设置单元格字体加粗
			font->setProperty("Size", 36);// 设置单元格字体大小
			font = worksheet->querySubObject("Range(const QString&)", "A2:K3")->querySubObject("Font");// 表头单元格字体设置(改动)
			font->setProperty("Bold", true);// 设置单元格字体加粗
			font->setProperty("Size", 14);// 设置单元格字体大小
			colm = worksheet->querySubObject("Columns(const QString&)", "A");
			colm->setProperty("ColumnWidth", 15);
			colm = worksheet->querySubObject("Columns(const QString&)", "B");
			colm->setProperty("ColumnWidth", 15);
			colm = worksheet->querySubObject("Columns(const QString&)", "C:K");//改动
			colm->setProperty("ColumnWidth", 25);
			row = worksheet->querySubObject("Rows(const QString&)", "1");
			row->setProperty("RowHeight", 40);
			row = worksheet->querySubObject("Rows(const QString&)", "2:4");
			row->setProperty("RowHeight", 30);
			for (int i = 0; i < recordRoundoutNb_all; i++)
			{
				int currentRow = i + 5;
				int currentDsequence = recordRoundoutList[i];

				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 1);
				cell->dynamicCall("SetValue(const QString&)", QVariant(currentDsequence));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 2);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_roundoutPositionInf[currentDsequence].roundoutFeatureNb));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 3);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_roundoutPositionInf[currentDsequence].roundoutNominalValue));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 4);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_roundoutPositionInf[currentDsequence].axisGuangMuEncodePostion_bottom));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 5);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_roundoutPositionInf[currentDsequence].axisGuangMuRealPostion_bottom));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 6);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_roundoutPositionInf[currentDsequence].axisGuangMuEncodePostion_middle));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 7);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_roundoutPositionInf[currentDsequence].axisGuangMuRealPostion_middle));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 8);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_roundoutPositionInf[currentDsequence].axisGuangMuEncodePostion_upper));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 9);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_roundoutPositionInf[currentDsequence].axisGuangMuRealPostion_upper));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 10);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_roundoutPositionInf[currentDsequence].roundoutPostionNote1));//改动
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 11);//改动
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_roundoutPositionInf[currentDsequence].roundoutPostionNote2));//改动
			};
			//全局表格设置
			range = worksheet->querySubObject("UsedRange");
			QAxObject* cells = range->querySubObject("Columns");
			//cells->dynamicCall("AutoFit");//这句代码可以使得所有单元格自适应宽度

			range->setProperty("HorizontalAlignment", -4108);//水平居中
			range->setProperty("VerticalAlignment", -4108);//垂直居中
			QAxObject* border = range->querySubObject("Borders");
			border->setProperty("Color", QColor(0, 0, 0));
			std::cout << "saveAsExcel_roundout_finished" << endl;
		};
		sort(recordHoleList.begin(), recordHoleList.end());
		last = unique(recordHoleList.begin(), recordHoleList.end());
		recordHoleList.erase(last, recordHoleList.end());
		recordHoleNb_all = recordHoleList.size();
		if (recordHoleNb_all > 0)//写入孔径点位
		{
			worksheet = workbook->querySubObject("Worksheets(int)", 6);
			//添加远心Excel表头数据

			bool mergeCellsFlag;
			mergeCellsFlag = mergeCells("A1", "E1", "孔径二次开发测量点位记录表");
			cell = worksheet->querySubObject("Cells(int,int)", 2, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("装夹方式：\t"));
			mergeCellsFlag = mergeCells("B2", "E2", recordpartNote);
			cell = worksheet->querySubObject("Cells(int,int)", 3, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("零件图号：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 2);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartNb));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 3);
			cell->dynamicCall("SetValue(const QString&)", QVariant("零件名称：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 4);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartName));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 5);
			cell->dynamicCall("SetValue(const QString&)", QVariant("工序号：\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 3, 6);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordpartProcessingNb));
			mergeCellsFlag = mergeCells("G3", "K3", "孔径点位总数：\t");
			cell = worksheet->querySubObject("Cells(int,int)", 3, 12);
			cell->dynamicCall("SetValue(const QString&)", QVariant(recordHoleNb_all));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 1);
			cell->dynamicCall("SetValue(const QString&)", QVariant("孔径测量顺序号\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 2);
			cell->dynamicCall("SetValue(const QString&)", QVariant("孔径特征号\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 3);
			cell->dynamicCall("SetValue(const QString&)", QVariant("公称值\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 4);
			cell->dynamicCall("SetValue(const QString&)", QVariant("上偏差\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 5);
			cell->dynamicCall("SetValue(const QString&)", QVariant("下偏差\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 6);
			cell->dynamicCall("SetValue(const QString&)", QVariant("均布个数\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 7);
			cell->dynamicCall("SetValue(const QString&)", QVariant("孔径轴点位\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 8);
			cell->dynamicCall("SetValue(const QString&)", QVariant("孔径轴移动距离\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 9);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕轴点位\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 10);
			cell->dynamicCall("SetValue(const QString&)", QVariant("光幕轴移动距离\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 11);
			cell->dynamicCall("SetValue(const QString&)", QVariant("孔径相机曝光\t"));
			cell = worksheet->querySubObject("Cells(int,int)", 4, 12);
			cell->dynamicCall("SetValue(const QString&)", QVariant("备注\t"));

			font = worksheet->querySubObject("Range(const QString&)", "A1:L1")->querySubObject("Font");// 表头单元格字体设置
			font->setProperty("Bold", true);// 设置单元格字体加粗
			font->setProperty("Size", 36);// 设置单元格字体大小
			font = worksheet->querySubObject("Range(const QString&)", "A2:L3")->querySubObject("Font");// 表头单元格字体设置
			font->setProperty("Bold", true);// 设置单元格字体加粗
			font->setProperty("Size", 14);// 设置单元格字体大小
			colm = worksheet->querySubObject("Columns(const QString&)", "A");
			colm->setProperty("ColumnWidth", 15);
			colm = worksheet->querySubObject("Columns(const QString&)", "B");
			colm->setProperty("ColumnWidth", 12);
			colm = worksheet->querySubObject("Columns(const QString&)", "C:K");
			colm->setProperty("ColumnWidth", 20);
			colm = worksheet->querySubObject("Columns(const QString&)", "L");
			colm->setProperty("ColumnWidth", 40);
			row = worksheet->querySubObject("Rows(const QString&)", "1");
			row->setProperty("RowHeight", 40);
			row = worksheet->querySubObject("Rows(const QString&)", "2:4");
			row->setProperty("RowHeight", 30);
			for (int i = 0; i < recordHoleNb_all; i++)
			{
				int currentRow = i + 5;
				int currentDsequence = recordHoleList[i];

				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 1);
				cell->dynamicCall("SetValue(const QString&)", QVariant(currentDsequence));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 2);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_holePositionInf[currentDsequence].holeFeatureNb));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 3);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_holePositionInf[currentDsequence].holeNominalValue));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 4);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_holePositionInf[currentDsequence].holeUpperOffset));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 5);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_holePositionInf[currentDsequence].holeBottomOffset));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 6);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_holePositionInf[currentDsequence].holeNumber));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 7);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_holePositionInf[currentDsequence].axisHoleEncodePostion));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 8);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_holePositionInf[currentDsequence].axisHoleRealPostion));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 9);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_holePositionInf[currentDsequence].axisGuangMuEncodePostion));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 10);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_holePositionInf[currentDsequence].axisGuangMuRealPostion));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 11);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_holePositionInf[currentDsequence].holeExposeTime));
				cell = worksheet->querySubObject("Cells(int,int)", currentRow, 12);
				cell->dynamicCall("SetValue(const QString&)", QVariant(m_holePositionInf[currentDsequence].holePostionNote));
			};
			//全局表格设置
			range = worksheet->querySubObject("UsedRange");
			QAxObject* cells = range->querySubObject("Columns");
			//cells->dynamicCall("AutoFit");//这句代码可以使得所有单元格自适应宽度

			range->setProperty("HorizontalAlignment", -4108);//水平居中
			range->setProperty("VerticalAlignment", -4108);//垂直居中
			QAxObject* border = range->querySubObject("Borders");
			border->setProperty("Color", QColor(0, 0, 0));
			std::cout << "saveAsExcel_hole_finished" << endl;
		};


		QString fileName = excelPath;
		workbook->dynamicCall("SaveAs(const QString&)", QDir::toNativeSeparators(fileName)); //保存至fileName
		workbook->dynamicCall("Close()");                                                   //关闭工作簿
		excel->dynamicCall("Quit()");                                                       //关闭excel
		delete excel;
		excel = NULL;
		workbook = NULL;
		worksheet = NULL;
		const QString generatorWorkbook = runtimePath("SDKprogram/program0107.xlsm");
		ShellExecuteW(nullptr,L"open",reinterpret_cast<LPCWSTR>(generatorWorkbook.utf16()),nullptr,nullptr,SW_SHOW);
        setOutputStatus(QStringLiteral("已生成：%1").arg(QDir::toNativeSeparators(excelPath)), "success");
		emit tips("点位保存完成");
	}
};
