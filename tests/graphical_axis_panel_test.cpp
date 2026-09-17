// Offline UI checks. The backend below never calls any device SDK.
// Link editor/canvas objects with /DELAYLOAD:halconcpp.dll; algorithms are not executed.
#include "graphical_program_editor.h"
#include <QApplication>
#include <QPushButton>
#include <QComboBox>
#include <QToolBar>
#include <QtTest/QTest>
#include <QEvent>
#include <QFile>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTabWidget>
#include <QLabel>
#include <limits>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

static void testCornerGeometry()
{
    require(std::abs(cornerPointSegmentDistance(QPointF(5, 2), QPointF(0, 0), QPointF(10, 0)) - 2) < 1e-9,
        "point-to-segment interior distance");
    require(std::abs(cornerPointSegmentDistance(QPointF(12, 0), QPointF(0, 0), QPointF(10, 0)) - 2) < 1e-9,
        "point-to-segment endpoint distance");
    QVector<double> mostlyStraight(9, 1.0);
    mostlyStraight.append(100.0);
    require(cornerDeviationPercentile(mostlyStraight) == 1.0,
        "P90 deviation must ignore one outlier in ten points");
    mostlyStraight[8] = 100.0;
    require(cornerDeviationPercentile(mostlyStraight) == 100.0,
        "P90 deviation must reject two outliers in ten points");
    require(!std::isfinite(cornerDeviationPercentile({ 1.0, std::numeric_limits<double>::quiet_NaN() })),
        "nonfinite deviations must be rejected");
    GraphicalCornerEdge first, second;
    first.first = QPointF(10, 10); first.second = QPointF(50, 10);
    second.first = QPointF(10, 10); second.second = QPointF(30, 30);
    GraphicalCornerPair pair;
    require(cornerPairGeometry(first, second, 0, pair), "adjacent corner must be accepted");
    require(std::abs(pair.smallerAngle - 45) < 1e-9, "known 45 degree corner");
    require(!cornerPairOverlay(first, second, pair, false).isEmpty(), "corner overlay required");
    require(!cornerPairOverlay(first, second, pair, true).isEmpty(), "supplement overlay required");
    std::swap(second.first, second.second);
    require(cornerPairGeometry(first, second, 0, pair) && std::abs(pair.smallerAngle - 45) < 1e-9, "endpoint reversal must preserve angle");
    second.first = QPointF(10, 10); second.second = QPointF(10, 50);
    require(cornerPairGeometry(first, second, 0, pair) && std::abs(pair.smallerAngle - 90) < 1e-9, "right angle");
    second.first = QPointF(10, 12); second.second = QPointF(10, 50);
    require(!cornerPairGeometry(first, second, 1, pair) && cornerPairGeometry(first, second, 2, pair), "gap threshold must be enforced");
    second.first = QPointF(10, 20); second.second = QPointF(50, 20);
    require(!cornerPairGeometry(first, second, 100, pair), "parallel lines must be rejected");
    second.first = second.second;
    require(!cornerPairGeometry(first, second, 100, pair), "zero length must be rejected");
    second.first.setX(std::numeric_limits<double>::quiet_NaN());
    require(!cornerPairGeometry(first, second, 100, pair), "nonfinite edges must be rejected");
    std::cout << "PASS: robust P90 deviation, corner angle, endpoint reversal, right angle, gap limits, parallel/degenerate/nonfinite rejection, overlays\n";
}

static void testDetectionRecords()
{
    GraphicalDetectionParameters parameters;
    require(parameters.validationError().isEmpty(), "legacy defaults must be valid");
    require(parameters.cornerMaxDeviation == 2.5 && parameters.cornerMaxGap == 20,
        "single ROI defaults must tolerate a small rounded transition");
    parameters.lowThreshold = parameters.highThreshold;
    require(!parameters.validationError().isEmpty(), "equal thresholds must be rejected");
    parameters = GraphicalDetectionParameters();
    parameters.maxLength = parameters.minLength - 1;
    require(!parameters.validationError().isEmpty(), "reversed length range must be rejected");
    parameters.maxLength = 0;
    parameters.smoothing = std::numeric_limits<double>::quiet_NaN();
    require(!parameters.validationError().isEmpty(), "nonfinite parameter must be rejected");

    GraphicalProgramEditor editor;
    editor.setAttribute(Qt::WA_DontShowOnScreen);
    editor.show();
    QTest::qWait(250);
    auto button = [&](const QString& name) {
        for (auto* value : editor.findChildren<QPushButton*>()) if (value->text() == name) return value;
        throw std::runtime_error("detection button missing");
    };
    auto* type = editor.findChild<QComboBox*>(QStringLiteral("measurementType"));
    auto* feature = editor.findChild<QLineEdit*>(QStringLiteral("measurementFeatureNumber"));
    auto* minimum = editor.findChild<QDoubleSpinBox*>(QStringLiteral("detectionMinLength"));
    auto* low = editor.findChild<QDoubleSpinBox*>(QStringLiteral("detectionLow"));
    auto* table = editor.findChild<QTableWidget*>();
    auto* diagnostic = editor.findChild<QLabel*>(QStringLiteral("detectionDiagnostic"));
    require(type && feature && minimum && low && table && diagnostic, "detection UI missing");
    type->setCurrentText(QStringLiteral("圆弧半径"));
    feature->setText(QStringLiteral("F1"));
    minimum->setValue(7);
    button(QStringLiteral("新增测量记录"))->click();
    feature->setText(QStringLiteral("F2"));
    minimum->setValue(13);
    button(QStringLiteral("新增测量记录"))->click();
    require(table->rowCount() == 2, "two measurement records required");
    table->setCurrentCell(0, 0);
    require(minimum->value() == 7, "first record must retain its own parameters");
    minimum->setValue(9); // Unsubmitted draft must not change the record.
    table->setCurrentCell(1, 0);
    require(minimum->value() == 13, "second record must retain its own parameters");
    table->setCurrentCell(0, 0);
    require(minimum->value() == 7, "unsubmitted draft must not alter saved parameters");
    low->setValue(100);
    button(QStringLiteral("应用检测参数"))->click();
    require(diagnostic->text().contains(QStringLiteral("低阈值")), "invalid threshold must explain rejection");
    table->setCurrentCell(1, 0);
    table->setCurrentCell(0, 0);
    require(low->value() == 20, "invalid parameter must not replace committed value");
    feature->setText(QStringLiteral("UNSUBMITTED"));
    minimum->setValue(8);
    button(QStringLiteral("应用检测参数"))->click();
    require(table->item(0, 1)->text() == QStringLiteral("F1"), "parameter-only apply must preserve feature number");
    require(table->item(0, 11)->text().contains(QStringLiteral("旧结果失效")), "parameter apply must invalidate execution state");
    button(QStringLiteral("恢复默认（未提交）"))->click();
    require(minimum->value() == 20, "reset must display legacy defaults");
    table->setCurrentCell(1, 0);
    table->setCurrentCell(0, 0);
    require(minimum->value() == 8, "reset draft must not overwrite committed parameters");
    for (auto* tabs : editor.findChildren<QTabWidget*>())
        for (int index = 0; index < tabs->count(); ++index)
            if (tabs->tabText(index) == QStringLiteral("检测参数")) tabs->setCurrentIndex(index);
    require(editor.grab().save("x64/detection_parameters.png"), "detection UI screenshot failed");
    auto* inputMode = editor.findChild<QComboBox*>(QStringLiteral("angleInputMode"));
    auto* candidate = editor.findChild<QComboBox*>(QStringLiteral("cornerCandidate"));
    auto* gap = editor.findChild<QDoubleSpinBox*>(QStringLiteral("cornerGap"));
    require(inputMode && candidate && gap, "single ROI UI missing");
    type->setCurrentText(QStringLiteral("角度"));
    inputMode->setCurrentIndex(1);
    feature->setText(QStringLiteral("CORNER"));
    gap->setValue(3);
    button(QStringLiteral("新增测量记录"))->click();
    require(table->item(2, 3)->text().startsWith(QStringLiteral("单ROI:")), "single ROI association required");
    require(!button(QStringLiteral("角度：选择/重选 ROI 2（直线2）"))->isEnabled(), "single ROI must disable second ROI");
    require(!candidate->isEnabled(), "no computed candidates must not be selectable");
    table->setCurrentCell(0, 0);
    table->setCurrentCell(2, 0);
    require(inputMode->currentIndex() == 1 && gap->value() == 3, "single ROI mode and parameters must survive record switching");
    inputMode->setCurrentIndex(0);
    table->setCurrentCell(0, 0); table->setCurrentCell(2, 0);
    require(inputMode->currentIndex() == 1, "unsubmitted mode must not change record");
    for (auto* tabs : editor.findChildren<QTabWidget*>())
        for (int index = 0; index < tabs->count(); ++index)
            if (tabs->tabText(index) == QStringLiteral("测量配置")) tabs->setCurrentIndex(index);
    QTest::qWait(100);
    require(editor.grab().save("x64/single_roi_angle.png"), "single ROI screenshot failed");
    inputMode->setCurrentIndex(0);
    button(QStringLiteral("更新选中记录"))->click();
    require(table->item(2, 3)->text().contains(QStringLiteral("ROI2:")), "committed mode must switch association");
    require(button(QStringLiteral("角度：选择/重选 ROI 2（直线2）"))->isEnabled(), "double ROI must enable second ROI");
    std::cout << "PASS: detection defaults, invalid thresholds/ranges/nonfinite input, record isolation, draft discard, parameter-only apply, invalidation state, reset semantics\n";
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QFile theme(QStringLiteral("config/theme.qss"));
    if (theme.open(QIODevice::ReadOnly)) app.setStyleSheet(QString::fromUtf8(theme.readAll()));
    try {
        testCornerGeometry();
        testDetectionRecords();
        GraphicalProgramEditor editor;
        editor.setAttribute(Qt::WA_DontShowOnScreen);
        using Command = GraphicalProgramEditor::AxisCommand;
        bool connected = false, moving = false, failStop = false, failStartAck = false;
        int starts = 0, stops = 0, lastAxis = -1;
        editor.setAxisBackend([&](int) {
            GraphicalProgramEditor::AxisSnapshot state;
            state.connected = state.available = state.valid = connected;
            state.status = 0x200 | (moving ? 0x400 : 0);
            state.message = connected ? QStringLiteral("测试后端（非硬件）") : QStringLiteral("未连接");
            return state;
        }, [&](int axis, Command command, double, long) -> GraphicalProgramEditor::AxisCommandResult {
            lastAxis = axis;
            if (command == Command::Stop || command == Command::EmergencyStop) {
                ++stops;
                if (failStop) return QStringLiteral("测试：停止通讯失败");
                moving = false;
            } else if (command == Command::JogNegative || command == Command::JogPositive || command == Command::MoveAbsolute) {
                ++starts; moving = true;
                if (failStartAck) return { QStringLiteral("测试：启动响应丢失"), true };
            }
            return QString();
        });
        editor.show();
        QTest::qWait(250);
        auto button = [&](const QString& name) {
            for (auto* value : editor.findChildren<QPushButton*>()) if (value->text() == name) return value;
            throw std::runtime_error("button missing");
        };
        auto* positive = button(QStringLiteral("正向 +（按住）"));
        auto* negative = button(QStringLiteral("负向 −（按住）"));
        auto* stop = button(QStringLiteral("停止当前轴"));
        auto* emergency = button(QStringLiteral("全部轴急停"));
        require(!positive->isEnabled() && !stop->isEnabled(), "offline controls must be disabled");
        QComboBox* previewMode = nullptr;
        for (auto* combo : editor.findChildren<QComboBox*>())
            if (combo->findText(QStringLiteral("绝对点位")) >= 0) previewMode = combo;
        require(previewMode && !previewMode->isEnabled(), "offline mode selector must be disabled after preview ends");
        require(starts == 0, "offline state must not issue motion");
        require(editor.windowModality() == Qt::ApplicationModal, "editor must exclude competing windows");
        require(editor.grab().save("x64/graphical_axis_offline.png"), "offline screenshot failed");
        connected = true;
        QTest::qWait(250);
        require(previewMode->isEnabled(), "ready mode selector must be enabled");
        previewMode->setCurrentIndex(1);
        require(!positive->isVisible() && button(QStringLiteral("移动至目标位置"))->isVisible(), "absolute mode must change visible controls");
        require(button(QStringLiteral("移动至目标位置"))->isEnabled() && starts == 0, "mode switch alone must not issue motion");
        previewMode->setCurrentIndex(0);
        require(positive->isVisible() && !button(QStringLiteral("移动至目标位置"))->isVisible(), "Jog mode must restore Jog controls");
        require(positive->isEnabled() && emergency->isEnabled(), "ready controls must be enabled");
        QTest::mousePress(positive, Qt::LeftButton);
        QTest::qWait(250);
        require(starts == 1 && moving && lastAxis == 1, "Jog must address selected axis");
        require(positive->isEnabled() && !negative->isEnabled(), "pressed Jog must retain release delivery");
        QTest::mouseRelease(positive, Qt::LeftButton);
        QTest::qWait(250);
        require(!moving && stops > 0 && lastAxis == 1, "Jog release must stop initiating axis");
        QComboBox* selector = nullptr;
        QComboBox* mode = nullptr;
        for (auto* combo : editor.findChildren<QComboBox*>()) {
            if (combo->count() == 5 && combo->itemData(4).toInt() == 7) selector = combo;
            if (combo->findText(QStringLiteral("绝对点位")) >= 0) mode = combo;
        }
        require(selector && mode, "axis selectors missing");
        selector->setCurrentIndex(4);
        QTest::mousePress(positive, Qt::LeftButton);
        QEvent deactivate(QEvent::WindowDeactivate);
        QApplication::sendEvent(&editor, &deactivate);
        require(!moving && lastAxis == 7, "deactivation must stop axis 7, not legacy current axis");
        QTest::mouseRelease(positive, Qt::LeftButton);
        QTest::qWait(250);
        mode->setCurrentIndex(1);
        QTest::qWait(250);
        QTest::mouseClick(button(QStringLiteral("移动至目标位置")), Qt::LeftButton);
        require(moving && lastAxis == 7, "absolute motion missing");
        QTest::mouseClick(stop, Qt::LeftButton);
        QTest::qWait(250);
        failStartAck = true;
        QTest::mouseClick(button(QStringLiteral("移动至目标位置")), Qt::LeftButton);
        require(moving, "uncertain start acknowledgement scenario missing");
        failStop = true;
        editor.close();
        require(editor.isVisible() && moving, "failed stop must block close");
        failStop = false;
        QTest::mouseClick(emergency, Qt::LeftButton);
        require(!moving, "emergency command missing");
        QTest::qWait(250);
        editor.centralWidget()->setEnabled(false);
        require(stop->isEnabled() && emergency->isEnabled(), "stop controls must survive disabled editor content");
        editor.centralWidget()->setEnabled(true);
        editor.close();
        require(!editor.isVisible(), "stationary editor must close");
        std::cout << "PASS: offline state, modality, Jog press/release, fixed-axis stop, deactivation, absolute move, uncertain start acknowledgement, failed-stop close guard, emergency, independent stop bar\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
