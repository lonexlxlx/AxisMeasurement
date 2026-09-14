#include "sdk_assist.h"

#include "graphical_program_editor.h"

#include <QAction>
#include <QToolBar>
#include <QScrollArea>
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

void addRow(QGridLayout* layout, int row, QLabel* label, QWidget* field)
{
    prepareLabel(label);
    prepareField(field);
    layout->addWidget(label, row, 0);
    layout->addWidget(field, row, 1);
}

void addButtonRow(QGridLayout* layout, int row, QPushButton* primary, QPushButton* danger)
{
    markButton(primary, "primary");
    markButton(danger, "danger");
    layout->addWidget(primary, row, 0);
    layout->addWidget(danger, row, 1);
}

QGroupBox* createCard(const QString& title)
{
    QGroupBox* card = new QGroupBox();
    card->setObjectName(QStringLiteral("sdkAssistCard"));
    card->setProperty("cardTitleText", title);
    card->setMinimumHeight(330);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(10);
    shadow->setOffset(0, 1);
    shadow->setColor(QColor(15, 23, 42, 14));
    card->setGraphicsEffect(shadow);
    return card;
}

QGridLayout* createCardLayout(QGroupBox* card)
{
    auto* wrapper = new QVBoxLayout(card);
    wrapper->setContentsMargins(10, 8, 10, 10);
    wrapper->setSpacing(6);
    wrapper->setAlignment(Qt::AlignTop);

    auto* title = new QLabel(card->property("cardTitleText").toString(), card);
    title->setObjectName(QStringLiteral("sdkAssistCardTitle"));
    title->setStyleSheet(QStringLiteral("color: #2563EB; font-size: 11pt; font-weight: 700; background: transparent;"));
    title->setAlignment(Qt::AlignCenter);
    title->setWordWrap(true);
    title->setFixedHeight(34);
    title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    wrapper->addWidget(title);

    auto* layout = new QGridLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(8);
    layout->setVerticalSpacing(7);
    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 1);
    wrapper->addLayout(layout);
    return layout;
}

QGroupBox* createDiameterModule(Ui::sdk_assist& ui)
{
    QGroupBox* card = createCard(QStringLiteral("1. 直径点位记录模块（光幕）"));
    QGridLayout* layout = createCardLayout(card);
    layout->setVerticalSpacing(11);
    addRow(layout, 0, ui.label_3, ui.diameterSequence);
    addRow(layout, 1, ui.label_5, ui.diameterFeatureNb);
    addRow(layout, 2, ui.label_6, ui.diameterNominalValue);
    addRow(layout, 3, ui.label_7, ui.diameterUpperOffset);
    addRow(layout, 4, ui.label_8, ui.diameterBottomOffset);
    addRow(layout, 5, ui.label_9, ui.diameterPostionNote);
    addButtonRow(layout, 6, ui.diameterPostionRecord, ui.diameterPostionClear);
    return card;
}

QGroupBox* createRoughnessModule(Ui::sdk_assist& ui)
{
    QGroupBox* card = createCard(QStringLiteral("2. 粗糙度点位记录模块（粗糙度相机 + 光幕）"));
    QGridLayout* layout = createCardLayout(card);
    layout->setVerticalSpacing(10);
    addRow(layout, 0, ui.label_14, ui.roughnessSequence);
    addRow(layout, 1, ui.label_10, ui.roughnessFeatureNb);
    addRow(layout, 2, ui.label_11, ui.roughnessNominalValue);
    addRow(layout, 3, ui.label_4, ui.roughnessExposeTime);
    addRow(layout, 4, ui.label_31, ui.roughnessReferenceD);
    addRow(layout, 5, ui.label_28, ui.roughnessPostionNote);
    markButton(ui.roughnessPostionRecord, "primary");
    markButton(ui.roughnessReferenceDRecord, "primary");
    markButton(ui.roughnessPostionClear, "danger");
    layout->addWidget(ui.roughnessPostionRecord, 6, 0);
    layout->addWidget(ui.roughnessReferenceDRecord, 6, 1);
    layout->addWidget(ui.roughnessPostionClear, 7, 0, 1, 2);
    return card;
}

QGroupBox* createHoleModule(Ui::sdk_assist& ui)
{
    QGroupBox* card = createCard(QStringLiteral("3. 孔径点位记录模块（孔径相机）"));
    QGridLayout* layout = createCardLayout(card);
    addRow(layout, 0, ui.label_38, ui.holeSequence);
    addRow(layout, 1, ui.label_34, ui.holeFeatureNb);
    addRow(layout, 2, ui.label_35, ui.holeNominalValue);
    addRow(layout, 3, ui.label_37, ui.holeUpperOffset);
    addRow(layout, 4, ui.label_39, ui.holeBottomOffset);
    addRow(layout, 5, ui.label_41, ui.holeNumber);
    addRow(layout, 6, ui.label_40, ui.holeExposeTime);
    addRow(layout, 7, ui.label_49, ui.holePostionNote);
    layout->setRowStretch(8, 1);
    addButtonRow(layout, 9, ui.holePostionRecord, ui.holePostionClear);
    return card;
}

QGroupBox* createCylindricityModule(Ui::sdk_assist& ui)
{
    QGroupBox* card = createCard(QStringLiteral("4. 圆柱度点位记录模块（光幕）"));
    QGridLayout* layout = createCardLayout(card);
    layout->setVerticalSpacing(11);
    addRow(layout, 0, ui.label_13, ui.cylindricitySequence);
    addRow(layout, 1, ui.label_16, ui.cylindricityFeatureNb);
    addRow(layout, 2, ui.label_17, ui.cylindricityNominalValue);
    addRow(layout, 3, ui.label_18, ui.cylindricityUpperRelativeLocation);
    addRow(layout, 4, ui.label_19, ui.cylindricityBottomRelativeLocation);
    addRow(layout, 5, ui.label_20, ui.cylindricityPostionNote);
    addButtonRow(layout, 6, ui.cylindricityPostionRecord, ui.cylindricityPostionClear);
    return card;
}

QGroupBox* createRoundoutModule(Ui::sdk_assist& ui)
{
    QGroupBox* card = createCard(QStringLiteral("5. 跳动点位记录模块（光幕）"));
    QGridLayout* layout = createCardLayout(card);
    addRow(layout, 0, ui.label_24, ui.roundoutSequence);
    addRow(layout, 1, ui.label_25, ui.roundoutFeatureNb);
    addRow(layout, 2, ui.label_26, ui.roundoutNominalValue);
    addRow(layout, 3, ui.label_22, ui.roundoutUpperRelativeLocation);
    addRow(layout, 4, ui.label_23, ui.roundoutBottomRelativeLocation);
    addRow(layout, 5, ui.label_27, ui.roundoutPostionNote1);
    addRow(layout, 6, ui.label_33, ui.roundoutPostionNote2);
    layout->setRowStretch(7, 1);
    addButtonRow(layout, 8, ui.roundoutPostionRecord, ui.roundoutPostionClear);
    return card;
}

QGroupBox* createTelecentricModule(Ui::sdk_assist& ui)
{
    QGroupBox* card = createCard(QStringLiteral("6. 长度/角度/圆弧半径点位记录模块（远心相机）"));
    QGridLayout* layout = createCardLayout(card);
    layout->setVerticalSpacing(14);
    layout->setContentsMargins(0, 28, 0, 0);
    addRow(layout, 0, ui.label_30, ui.telecentricSequence);
    layout->setRowStretch(1, 1);
    addRow(layout, 2, ui.label_32, ui.telecentricExposeTime);
    layout->setRowStretch(3, 1);
    addRow(layout, 4, ui.label_36, ui.telecentricPostionNote);
    layout->setRowStretch(5, 2);
    addButtonRow(layout, 6, ui.telecentricPostionRecord, ui.telecentricPostionClear);
    return card;
}

QGroupBox* createOutputModule(Ui::sdk_assist& ui)
{
    QGroupBox* card = createCard(QStringLiteral("7. 测量信息与点位输出"));
    card->setMinimumHeight(0);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QGridLayout* layout = createCardLayout(card);
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(3, 1);
    layout->setColumnStretch(5, 1);

    prepareLabel(ui.label_76);
    prepareLabel(ui.label_81);
    prepareLabel(ui.label_79);
    prepareLabel(ui.label_72);
    prepareLabel(ui.label_74);
    prepareLabel(ui.label_80);
    ui.recordDiameterNb->setObjectName(QStringLiteral("countValueLabel"));
    ui.recordCylindricityNb->setObjectName(QStringLiteral("countValueLabel"));
    ui.recordRoughnessNb->setObjectName(QStringLiteral("countValueLabel"));
    ui.recordTelecentricNb->setObjectName(QStringLiteral("countValueLabel"));
    ui.recordHoleNb->setObjectName(QStringLiteral("countValueLabel"));
    ui.recordRoundoutNb->setObjectName(QStringLiteral("countValueLabel"));

    layout->addWidget(ui.label_76, 0, 0);
    layout->addWidget(ui.recordDiameterNb, 0, 1);
    layout->addWidget(ui.label_81, 0, 2);
    layout->addWidget(ui.recordCylindricityNb, 0, 3);
    layout->addWidget(ui.label_79, 0, 4);
    layout->addWidget(ui.recordRoughnessNb, 0, 5);
    layout->addWidget(ui.label_72, 1, 0);
    layout->addWidget(ui.recordTelecentricNb, 1, 1);
    layout->addWidget(ui.label_74, 1, 2);
    layout->addWidget(ui.recordHoleNb, 1, 3);
    layout->addWidget(ui.label_80, 1, 4);
    layout->addWidget(ui.recordRoundoutNb, 1, 5);

    prepareLabel(ui.label_86);
    prepareLabel(ui.label_85);
    prepareLabel(ui.label_84);
    prepareField(ui.recordpartNb);
    prepareField(ui.recordpartName);
    prepareField(ui.recordpartProcessingNb);
    layout->addWidget(ui.label_86, 2, 0);
    layout->addWidget(ui.recordpartNb, 2, 1);
    layout->addWidget(ui.label_85, 2, 2);
    layout->addWidget(ui.recordpartName, 2, 3);
    layout->addWidget(ui.label_84, 2, 4);
    layout->addWidget(ui.recordpartProcessingNb, 2, 5);

    prepareLabel(ui.label_87);
    prepareField(ui.recordpartNote);
    markButton(ui.PostionRecordOut, "primary");
    markButton(ui.PostionClearOut, "danger");
    layout->addWidget(ui.label_87, 3, 0);
    layout->addWidget(ui.recordpartNote, 3, 1, 1, 3);
    layout->addWidget(ui.PostionRecordOut, 3, 4);
    layout->addWidget(ui.PostionClearOut, 3, 5);
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

    auto* page = new QWidget(this);
    page->setObjectName(QStringLiteral("sdkAssistPage"));
    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(8, 6, 8, 8);
    pageLayout->setSpacing(8);
    pageLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    ui.label->setParent(page);
    ui.label->setObjectName(QStringLiteral("sdkAssistTitle"));
    ui.label->setAlignment(Qt::AlignCenter);
    ui.label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    pageLayout->addWidget(ui.label);

    auto* moduleGrid = new QGridLayout();
    moduleGrid->setContentsMargins(0, 0, 0, 0);
    moduleGrid->setHorizontalSpacing(10);
    moduleGrid->setVerticalSpacing(8);
    moduleGrid->setColumnStretch(0, 1);
    moduleGrid->setColumnStretch(1, 1);
    moduleGrid->setColumnStretch(2, 1);
    moduleGrid->setRowStretch(0, 1);
    moduleGrid->setRowStretch(1, 1);
    moduleGrid->addWidget(createDiameterModule(ui), 0, 0);
    moduleGrid->addWidget(createRoughnessModule(ui), 0, 1);
    moduleGrid->addWidget(createHoleModule(ui), 0, 2);
    moduleGrid->addWidget(createCylindricityModule(ui), 1, 0);
    moduleGrid->addWidget(createRoundoutModule(ui), 1, 1);
    moduleGrid->addWidget(createTelecentricModule(ui), 1, 2);
    pageLayout->addLayout(moduleGrid, 1);
    pageLayout->addWidget(createOutputModule(ui), 0);

<<<<<<< HEAD
    auto* pathHint = new QLabel(QStringLiteral("表单生成路径：%1").arg(runtimePath("SDKpostion")), page);
    pathHint->setObjectName(QStringLiteral("sdkAssistPathHint"));
    pathHint->setWordWrap(true);
    pageLayout->addWidget(pathHint);

    auto* centralScrollArea = new QScrollArea(this);
    centralScrollArea->setObjectName(QStringLiteral("sdkAssistScrollArea"));
    centralScrollArea->setWidget(page);
    centralScrollArea->setWidgetResizable(true);
    centralScrollArea->setFrameShape(QFrame::NoFrame);
    setCentralWidget(centralScrollArea);
    setMinimumSize(1024, 768);

    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    if (primaryScreen) {
        const int availableHeight = primaryScreen->availableGeometry().height();
        resize(1180, qMin(940, availableHeight - 60));
    }
    else {
        resize(1180, 900);
    }

    QToolBar* graphicalToolBar = addToolBar(QStringLiteral("图形化编程"));
    graphicalToolBar->setObjectName(QStringLiteral("graphicalProgrammingEntryToolBar"));
    graphicalToolBar->setMovable(false);
    QAction* graphicalProgrammingAction = graphicalToolBar->addAction(QStringLiteral("打开图形化编程"));
    connect(graphicalProgrammingAction, &QAction::triggered, this, [this]() {
        if (!m_graphicalProgramEditor)
            m_graphicalProgramEditor = new GraphicalProgramEditor(this);
        m_graphicalProgramEditor->show();
        m_graphicalProgramEditor->raise();
        m_graphicalProgramEditor->activateWindow();
    });
/*
=======
	//5) 滚动区承载 + 窗口初始高度适配屏幕：重排后内容高约 950，一屏尽览；小屏滚动兜底
	ui.centralwidget->setMinimumSize(1142, 950);
	QScrollArea* centralScrollArea = new QScrollArea(this);
	centralScrollArea->setWidget(ui.centralwidget);
	centralScrollArea->setWidgetResizable(true);
	centralScrollArea->setFrameShape(QFrame::NoFrame);
	setCentralWidget(centralScrollArea);
	QScreen* primaryScreen = QGuiApplication::primaryScreen();
	if (primaryScreen) {
		const int availableHeight = primaryScreen->availableGeometry().height();
		resize(1160, qMin(1010, availableHeight - 60));
	}
	QToolBar* graphicalToolBar = addToolBar(QStringLiteral("图形化编程"));
	graphicalToolBar->setObjectName(QStringLiteral("graphicalProgrammingEntryToolBar"));
	graphicalToolBar->setMovable(false);
	QAction* graphicalProgrammingAction = graphicalToolBar->addAction(QStringLiteral("打开图形化编程"));
	connect(graphicalProgrammingAction, &QAction::triggered, this, [this]() {
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
	});
	/*
>>>>>>> 18f4e14 (增加运动控制部分)
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
//直径槽函数*******************************************************************************************************************************************************************************************
void sdk_assist::on_diameterSequence_currentIndexChanged(int nIndex)
{
	currentDiameterOrder = nIndex;
	updateDiameterPostionInf(currentDiameterOrder);
};
void sdk_assist::on_diameterPostionRecord_clicked()
{
	emit diameterPostionRecord();
	_sleep(150);
	m_diameterPositionInf[currentDiameterOrder].diameterFeatureNb = ui.diameterFeatureNb->text().toInt();
	m_diameterPositionInf[currentDiameterOrder].diameterNominalValue = ui.diameterNominalValue->text().toFloat();;
	m_diameterPositionInf[currentDiameterOrder].diameterUpperOffset = ui.diameterUpperOffset->text().toFloat();
	m_diameterPositionInf[currentDiameterOrder].diameterBottomOffset = ui.diameterBottomOffset->text().toFloat();;
	m_diameterPositionInf[currentDiameterOrder].diameterPostionNote = ui.diameterPostionNote->text();
	recordDiameterNb_all++;
	recordDiameterList.push_back(currentDiameterOrder);
	updatePostionInfOut();


};
void sdk_assist::on_diameterPostionClear_clicked()
{
	if (recordDiameterNb_all == 0)
	{
		emit tips("未记录任何直径点位，请检查！");
		return;
	}
	clearSingleDiameter(currentDiameterOrder);
	recordDiameterNb_all--;
	for (vector<int>::iterator iter = recordDiameterList.begin(); iter != recordDiameterList.end();)
	{
		if (*iter == currentDiameterOrder)
		{
			iter = recordDiameterList.erase(iter);
			break;
		}
	}
	updateDiameterPostionInf(currentDiameterOrder);
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
	std::cout << "on_roughnessSequence_currentIndexChanged(int nIndex)" << endl;
};
void sdk_assist::on_roughnessPostionRecord_clicked()
{
	emit roughnessPostionRecord();
	_sleep(150);
	m_roughnessPositionInf[currentRoughnessOrder].roughnessFeatureNb = ui.roughnessFeatureNb->text().toInt();
	m_roughnessPositionInf[currentRoughnessOrder].roughnessNominalValue = ui.roughnessNominalValue->text().toFloat();
    //m_roughnessPositionInf[currentRoughnessOrder].roughnessReferenceD = ui.roughnessReferenceD->text().toFloat();改动
	m_roughnessPositionInf[currentRoughnessOrder].roughnessPostionNote = ui.roughnessPostionNote->text();
	recordRoughnessNb_all++;
	recordRoughnessList.push_back(currentRoughnessOrder);
	updateRoughnessPostionInf(currentRoughnessOrder);
	updatePostionInfOut();
	std::cout << "on_roughnessPostionRecord_clicked()" << endl;
};
void sdk_assist::on_roughnessPostionClear_clicked()
{
	if (recordRoughnessNb_all == 0)
	{
		emit tips("未记录任何粗糙度点位，请检查！");
		return;
	}
	clearSingleRoughness(currentRoughnessOrder);
	recordRoughnessNb_all--;
	for (vector<int>::iterator iter = recordRoughnessList.begin(); iter != recordRoughnessList.end();)
	{
		if (*iter == currentRoughnessOrder)
		{
			iter = recordRoughnessList.erase(iter);
			break;
		}
	}
	updateRoughnessPostionInf(currentRoughnessOrder);
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
	std::cout << "on_cylindricitySequence_currentIndexChanged(int nIndex)" << endl;
};
void sdk_assist::on_cylindricityPostionRecord_clicked()
{
	cylindricityBottomRelativeLocation_current=ui.cylindricityBottomRelativeLocation->text().toInt();
	cylindricityUpperRelativeLocation_current = ui.cylindricityUpperRelativeLocation->text().toInt();
	if (cylindricityUpperRelativeLocation_current >= 0 && cylindricityBottomRelativeLocation_current >= 0) {
		emit cylindricityPostionRecord();
		_sleep(150);
		m_cylindricityPositionInf[currentCylindricityOrder].cylindricityFeatureNb = ui.cylindricityFeatureNb->text().toInt();
		m_cylindricityPositionInf[currentCylindricityOrder].cylindricityNominalValue = ui.cylindricityNominalValue->text().toFloat();
		m_cylindricityPositionInf[currentCylindricityOrder].cylindricityPostionNote = ui.cylindricityPostionNote->text();
		m_cylindricityPositionInf[currentCylindricityOrder].cylindricityPostionNote = ui.cylindricityPostionNote->text();
		recordCylindricityNb_all++;
		recordCylindricityList.push_back(currentCylindricityOrder);
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
	if (recordCylindricityNb_all == 0)
	{
		emit tips("未记录任何圆柱度点位，请检查！");
		return;
	}
	clearSingleCylindricity(currentCylindricityOrder);
	recordCylindricityNb_all--;
	for (vector<int>::iterator iter = recordCylindricityList.begin(); iter != recordCylindricityList.end();)
	{
		if (*iter == currentCylindricityOrder)
		{
			iter = recordCylindricityList.erase(iter);
			break;
		}
	}
	updateCylindricityPostionInf(currentCylindricityOrder);
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
	std::cout << "on_roundoutSequence_currentIndexChanged(int nIndex)" << endl;
};
void sdk_assist::on_roundoutPostionRecord_clicked()
{
	roundoutBottomRelativeLocation_current = ui.roundoutUpperRelativeLocation->text().toInt();
	roundoutUpperRelativeLocation_current = ui.roundoutBottomRelativeLocation->text().toInt();
	if (roundoutBottomRelativeLocation_current >= 0 && roundoutUpperRelativeLocation_current >= 0) {
		emit roundoutPostionRecord();
		_sleep(150);
		m_roundoutPositionInf[currentRoundoutOrder].roundoutFeatureNb = ui.roundoutFeatureNb->text().toInt();
		m_roundoutPositionInf[currentRoundoutOrder].roundoutNominalValue = ui.roundoutNominalValue->text().toFloat();
		m_roundoutPositionInf[currentRoundoutOrder].roundoutPostionNote1 = ui.roundoutPostionNote1->text();//改动
		m_roundoutPositionInf[currentRoundoutOrder].roundoutPostionNote2 = ui.roundoutPostionNote2->text();//改动
		recordRoundoutNb_all++;
		recordRoundoutList.push_back(currentRoundoutOrder);
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
	if (recordRoundoutNb_all == 0)
	{
		emit tips("未记录任何跳动点位，请检查！");
		return;
	}
	clearSingleRoundout(currentRoundoutOrder);
	recordRoundoutNb_all--;
	for (vector<int>::iterator iter = recordRoundoutList.begin(); iter != recordRoundoutList.end();)
	{
		if (*iter == currentRoundoutOrder)
		{
			iter = recordRoundoutList.erase(iter);
			break;
		}
	}
	updateRoundoutPostionInf(currentRoundoutOrder);
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
	std::cout << "on_holeSequence_currentIndexChanged(int nIndex)" << endl;
};
void sdk_assist::on_holePostionRecord_clicked()
{
	emit holePostionRecord();
	_sleep(150);
	m_holePositionInf[currentHoleOrder].holeFeatureNb = ui.holeFeatureNb->text().toInt();
	m_holePositionInf[currentHoleOrder].holeNominalValue = ui.holeNominalValue->text().toFloat();
	m_holePositionInf[currentHoleOrder].holeUpperOffset = ui.holeUpperOffset->text().toFloat();
	m_holePositionInf[currentHoleOrder].holeBottomOffset = ui.holeBottomOffset->text().toFloat();
	m_holePositionInf[currentHoleOrder].holeNumber = ui.holeNumber->text().toInt();
	m_holePositionInf[currentHoleOrder].holePostionNote = ui.holePostionNote->text();
	recordHoleNb_all++;
	recordHoleList.push_back(currentHoleOrder);
	updateHolePostionInf(currentHoleOrder);
	updatePostionInfOut();
	std::cout << "on_holePostionRecord_clicked()" << endl;
};
void sdk_assist::on_holePostionClear_clicked()
{
	if (recordHoleNb_all == 0)
	{
		emit tips("未记录任何孔径点位，请检查！");
		return;
	}
	clearSingleHole(currentHoleOrder);
	recordHoleNb_all--;
	for (vector<int>::iterator iter = recordHoleList.begin(); iter != recordHoleList.end();)
	{
		if (*iter == currentHoleOrder)
		{
			iter = recordHoleList.erase(iter);
			break;
		}
	}
	updateHolePostionInf(currentHoleOrder);
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
	m_holePositionInf[index].holeExposeTime = 0;
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
	std::cout << "on_telecentricSequence_currentIndexChanged(int nIndex)" << endl;
};
void sdk_assist::on_telecentricPostionRecord_clicked()
{
	emit telecentricPostionRecord();
	_sleep(150);
	m_telecentricPositionInf[currentTelecentricOrder].telecentricPostionNote = ui.telecentricPostionNote->text();
	recordTelecentricNb_all++;
	recordTelecentricList.push_back(currentTelecentricOrder);
	updateTelecentricPostionInf(currentTelecentricOrder);
	updatePostionInfOut();
	std::cout << "on_telecentricPostionRecord_clicked()" << endl;
};
void sdk_assist::on_telecentricPostionClear_clicked()
{
	clearSingleTelecentric(currentTelecentricOrder);
	recordTelecentricNb_all--;
	for (vector<int>::iterator iter = recordTelecentricList.begin(); iter != recordTelecentricList.end();)
	{
		if (*iter == currentTelecentricOrder)
		{
			iter = recordTelecentricList.erase(iter);
			break;
		}
	}
	updateTelecentricPostionInf(currentTelecentricOrder);
	updatePostionInfOut();
	std::cout << "on_telecentricPostionClear_clickes()" << endl;
};
void sdk_assist::clearSingleTelecentric(int index)
{
	m_telecentricPositionInf[index].telecentricExposeTime = 14;
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
	if ( recordTelecentricNb_all ==0 && recordCylindricityNb_all == 0 && recordTelecentricNb_all == 0 && recordRoughnessNb_all ==0&& recordRoundoutNb_all == 0&&recordHoleNb_all == 0&& recordDiameterNb_all == 0)
	{
		return;
	}
	else 
	{
		saveAsExcel();
	}
	
};
void sdk_assist::on_PostionClearOut_clicked()
{
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
		emit tips("点位保存完成");
	}
};
