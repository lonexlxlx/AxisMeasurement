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
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QFile theme(QStringLiteral("config/theme.qss"));
    if (theme.open(QIODevice::ReadOnly)) app.setStyleSheet(QString::fromUtf8(theme.readAll()));
    try {
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
