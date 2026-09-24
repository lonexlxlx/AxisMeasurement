#pragma once

#include <QString>
#include <QVector>

struct GraphicalProgramContract {
    bool supported = false;
    bool requiresImage = false;
    bool requiresCalibration = false;
    bool requiresSecondRoi = false;
    bool requiresTemplate = false;
    bool threeSectionScan = false;
    bool requiresTwoReferences = false;
    int cameraIndex = -1;
    QVector<int> axes;
    QString legacyWorksheet;
};

namespace GraphicalProgramGeneration {

inline GraphicalProgramContract contractForType(const QString& type, bool crossFrameLength = false,
    bool singleRoiAngle = false)
{
    GraphicalProgramContract contract;
    contract.supported = true;
    if (type == QStringLiteral("直径")) {
        contract.axes = { 5 };
        contract.legacyWorksheet = QStringLiteral("zhijing");
    }
    else if (type == QStringLiteral("孔径")) {
        contract.requiresImage = true;
        contract.requiresCalibration = true;
        contract.cameraIndex = 1;
        contract.axes = { 2, 5 };
        contract.legacyWorksheet = QStringLiteral("kongjing");
    }
    else if (type == QStringLiteral("圆柱度")) {
        contract.axes = { 5 };
        contract.threeSectionScan = true;
        contract.legacyWorksheet = QStringLiteral("yuanzhudu");
    }
    else if (type == QStringLiteral("跳动")) {
        contract.axes = { 5 };
        contract.threeSectionScan = true;
        contract.requiresTwoReferences = true;
        contract.legacyWorksheet = QStringLiteral("tiaodong");
    }
    else if (type == QStringLiteral("长度")) {
        contract.requiresImage = true;
        contract.requiresCalibration = true;
        contract.requiresSecondRoi = crossFrameLength;
        contract.requiresTemplate = !crossFrameLength;
        contract.cameraIndex = 0;
        contract.axes = { 5 };
        contract.legacyWorksheet = QStringLiteral("yuanxin");
    }
    else if (type == QStringLiteral("角度")) {
        contract.requiresImage = true;
        contract.requiresSecondRoi = !singleRoiAngle;
        contract.cameraIndex = 0;
        contract.axes = { 5 };
        contract.legacyWorksheet = QStringLiteral("yuanxin");
    }
    else if (type == QStringLiteral("圆弧半径")) {
        contract.requiresImage = true;
        contract.requiresCalibration = true;
        contract.cameraIndex = 0;
        contract.axes = { 5 };
        contract.legacyWorksheet = QStringLiteral("yuanxin");
    }
    else {
        contract.supported = false;
    }
    return contract;
}

inline bool isReservedProgramNumber(int programNumber)
{
    return programNumber >= 0 && programNumber <= 60;
}

} // namespace GraphicalProgramGeneration
