#pragma once

#include "graphical_program_editor.h"
#include "moveControl.h"
#include <cmath>
#include <set>

// Uses the existing connection, explicit axis addresses and local SDK return values.
// The supplied readiness predicate is evaluated on every command, not just on opening.
inline void attachGraphicalAxisBackend(GraphicalProgramEditor* editor, moveControl* card,
    std::function<bool()> ready)
{
    using Command = GraphicalProgramEditor::AxisCommand;
    auto validAxis = [](int axis) { return axis == 1 || axis == 2 || axis == 5 || axis == 6 || axis == 7; };
    auto check = [](short result, const char* operation) -> QString {
        return result ? QStringLiteral("%1 失败（SDK %2）").arg(QString::fromLatin1(operation)).arg(result) : QString();
    };
    auto read = [card, ready, validAxis](int axis) {
        GraphicalProgramEditor::AxisSnapshot state;
        state.connected = card && card->openControllerFlag;
        if (!state.connected) { state.message = QStringLiteral("控制卡未连接；请关闭编辑器后在主窗口打开设备。"); return state; }
        if (!validAxis(axis)) { state.message = QStringLiteral("轴号无效"); return state; }
        state.available = ready();
        if (!state.available) { state.message = QStringLiteral("设备未全部就绪，或自动测量/回零占用控制。"); return state; }
        const short core = card->axisCore[axis - 1];
        short result = GTN_GetSts(core, axis, &state.status);
        if (!result) result = GTN_GetPrfPos(core, axis, &state.planned);
        if (!result) result = GTN_GetAxisEncPos(core, axis, &state.encoder);
        state.valid = !result && std::isfinite(state.planned) && std::isfinite(state.encoder);
        state.message = state.valid ? QStringLiteral("轴 %1 · 控制卡已连接").arg(axis)
            : QStringLiteral("轴状态读取失败（SDK %1），位置不可用").arg(result);
        return state;
    };
    editor->setAxisBackend(read, [editor, card, read, validAxis, check](int axis, Command command, double speed, long target) -> GraphicalProgramEditor::AxisCommandResult {
        if (!card || !card->openControllerFlag)
            return QStringLiteral("控制卡未连接，无法发送命令；必要时使用物理急停。");
        if (!validAxis(axis)) return QStringLiteral("拒绝无效轴号");
        const short core = card->axisCore[axis - 1];
        const long mask = 1L << (axis - 1);
        if (command == Command::Stop) return check(GTN_Stop(core, mask, 0), "GTN_Stop");
        if (command == Command::EmergencyStop) {
            std::set<short> cores;
            for (short value : card->axisCore) cores.insert(value);
            QStringList errors;
            for (short value : cores) {
                const QString error = check(GTN_Stop(value, 0xffff, 0xffff), "GTN_Stop(urgent)");
                if (!error.isEmpty()) errors << error;
            }
            return errors.join(QStringLiteral("；"));
        }
        if (!editor->isVisible() || !editor->isActiveWindow()) return QStringLiteral("图形化窗口未处于活动状态");
        const auto state = read(axis);
        if (!state.available || !state.valid) return state.message;
        for (short other = 1; other <= 8; ++other) {
            long status = 0;
            const QString error = check(GTN_GetSts(card->axisCore[other - 1], other, &status), "GTN_GetSts");
            if (!error.isEmpty()) return error;
            if (status & 0x400) return QStringLiteral("轴 %1 正在运动，请先停止。").arg(other);
        }
        if (command == Command::Disable) return check(GTN_AxisOff(core, axis), "GTN_AxisOff");
        if (state.status & 0x192) return QStringLiteral("轴报警或停止输入有效，请在主窗口检查处理。");
        if (command == Command::Enable) return check(GTN_AxisOn(core, axis), "GTN_AxisOn");
        if (!(state.status & 0x200)) return QStringLiteral("轴未使能");
        if (!std::isfinite(speed) || speed <= 0 || speed > 1000) return QStringLiteral("速度输入无效");
        const bool jog = command == Command::JogNegative || command == Command::JogPositive;
        const bool positive = jog ? command == Command::JogPositive : target > state.planned;
        if ((positive && (state.status & 0x20)) || (!positive && (state.status & 0x40)))
            return QStringLiteral("目标方向限位已触发，拒绝继续向限位移动。");
        QString error;
        if (jog) {
            auto parameters = card->jog[axis - 1];
            if (!std::isfinite(parameters.acc) || !std::isfinite(parameters.dec) || parameters.acc <= 0 || parameters.dec <= 0)
                return QStringLiteral("原轴Jog加减速参数无效");
            error = check(GTN_PrfJog(core, axis), "GTN_PrfJog");
            if (error.isEmpty()) error = check(GTN_SetJogPrm(core, axis, &parameters), "GTN_SetJogPrm");
            if (error.isEmpty()) error = check(GTN_SetVel(core, axis, positive ? speed : -speed), "GTN_SetVel");
        } else if (command == Command::MoveAbsolute) {
            auto parameters = card->trapManual[axis - 1];
            if (!std::isfinite(parameters.acc) || !std::isfinite(parameters.dec) || parameters.acc <= 0 || parameters.dec <= 0)
                return QStringLiteral("原轴点位加减速参数无效");
            error = check(GTN_PrfTrap(core, axis), "GTN_PrfTrap");
            if (error.isEmpty()) error = check(GTN_SetTrapPrm(core, axis, &parameters), "GTN_SetTrapPrm");
            if (error.isEmpty()) error = check(GTN_SetPos(core, axis, target), "GTN_SetPos");
            if (error.isEmpty()) error = check(GTN_SetVel(core, axis, speed), "GTN_SetVel");
        } else return QStringLiteral("不支持的轴命令");
        if (!error.isEmpty()) return error;
        error = check(GTN_Update(core, mask), "GTN_Update");
        if (!error.isEmpty()) {
            const QString stopError = check(GTN_Stop(core, mask, 0xffff), "GTN_Stop");
            if (!stopError.isEmpty()) error += QStringLiteral("；") + stopError;
        }
        return { error, true };
    });
}
