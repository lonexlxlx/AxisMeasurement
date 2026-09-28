#pragma once

#include "graphical_program_contract.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <limits>

struct GraphicalProgramDescriptor {
    int programNumber = 0;
    QString packagePath;
    QString definitionPath;
    QString assetPath;
    QString partNumber;
    QString partName;
    QString processNumber;
    int recordCount = 0;
};

struct GraphicalProgramStep {
    int sequence = 0;
    QString featureNumber;
    QString type;
    GraphicalProgramContract contract;
    QJsonObject definition;
};

struct GraphicalProgramExecutionPlan {
    GraphicalProgramDescriptor descriptor;
    QVector<GraphicalProgramStep> steps;
};

namespace GraphicalProgramRegistry {

inline bool isJsonInteger(const QJsonValue& value)
{
    if (!value.isDouble()) return false;
    const double number = value.toDouble();
    return std::isfinite(number) && std::floor(number) == number
        && number >= std::numeric_limits<int>::min()
        && number <= std::numeric_limits<int>::max();
}

inline bool contractMatches(const QJsonObject& manifestRecord,
    const QJsonObject& definitionRecord, QString& error)
{
    if (!isJsonInteger(manifestRecord.value(QStringLiteral("sequence")))
        || !manifestRecord.value(QStringLiteral("featureNumber")).isString()
        || !manifestRecord.value(QStringLiteral("type")).isString()
        || !definitionRecord.value(QStringLiteral("featureNumber")).isString()
        || !definitionRecord.value(QStringLiteral("type")).isString()) {
        error = QStringLiteral("程序包记录标识字段无效。");
        return false;
    }
    const int sequence = manifestRecord.value(QStringLiteral("sequence")).toInt();
    const QString featureNumber = manifestRecord.value(QStringLiteral("featureNumber")).toString();
    const QString type = manifestRecord.value(QStringLiteral("type")).toString();
    if (featureNumber.isEmpty()
        || definitionRecord.value(QStringLiteral("sequence")).toInt(-1) != sequence
        || definitionRecord.value(QStringLiteral("featureNumber")).toString() != featureNumber
        || definitionRecord.value(QStringLiteral("type")).toString() != type) {
        error = QStringLiteral("程序包记录%1与测量定义不一致。").arg(sequence);
        return false;
    }
    const bool crossFrameLength = definitionRecord.value(QStringLiteral("crossFrameLength")).toBool(false);
    const bool singleRoiAngle = definitionRecord.value(QStringLiteral("singleRoiAngle")).toBool(false);
    const GraphicalProgramContract contract = GraphicalProgramGeneration::contractForType(
        type, crossFrameLength, singleRoiAngle);
    if (!contract.supported
        || manifestRecord.value(QStringLiteral("legacyWorksheet")).toString() != contract.legacyWorksheet
        || !isJsonInteger(manifestRecord.value(QStringLiteral("cameraIndex")))
        || manifestRecord.value(QStringLiteral("cameraIndex")).toInt() != contract.cameraIndex) {
        error = QStringLiteral("程序包记录%1的测量类型或设备契约无效。").arg(sequence);
        return false;
    }
    const struct { const char* name; bool expected; } flags[] = {
        { "requiresImage", contract.requiresImage },
        { "requiresCalibration", contract.requiresCalibration },
        { "requiresSecondRoi", contract.requiresSecondRoi },
        { "requiresTemplate", contract.requiresTemplate },
        { "threeSectionScan", contract.threeSectionScan },
        { "requiresTwoReferences", contract.requiresTwoReferences }
    };
    for (const auto& flag : flags) {
        const QJsonValue value = manifestRecord.value(QString::fromLatin1(flag.name));
        if (!value.isBool() || value.toBool() != flag.expected) {
            error = QStringLiteral("程序包记录%1的生成契约字段%2无效。")
                .arg(sequence).arg(QString::fromLatin1(flag.name));
            return false;
        }
    }
    const QJsonValue axesValue = manifestRecord.value(QStringLiteral("axes"));
    if (!axesValue.isArray()) {
        error = QStringLiteral("程序包记录%1的运动轴契约无效。").arg(sequence);
        return false;
    }
    QVector<int> axes;
    for (const QJsonValue& value : axesValue.toArray()) {
        if (!isJsonInteger(value)) {
            error = QStringLiteral("程序包记录%1的运动轴契约无效。").arg(sequence);
            return false;
        }
        axes.append(value.toInt());
    }
    if (axes != contract.axes) {
        error = QStringLiteral("程序包记录%1的运动轴契约与测量类型不一致。").arg(sequence);
        return false;
    }
    return true;
}

inline bool finiteNumber(const QJsonValue& value, double* result = nullptr)
{
    if (!value.isDouble() || !std::isfinite(value.toDouble())) return false;
    if (result) *result = value.toDouble();
    return true;
}

inline bool validateCollectedPosition(const QJsonObject& position,
    const GraphicalProgramContract& contract, int sequence, const QString& label, QString& error)
{
    if (position.value(QStringLiteral("status")).toString() != QStringLiteral("collected")
        || position.value(QStringLiteral("source")).toString() != QStringLiteral("hardware")
        || position.value(QStringLiteral("unit")).toString() != QStringLiteral("pulse")
        || !position.value(QStringLiteral("axes")).isArray()
        || !isJsonInteger(position.value(QStringLiteral("cameraIndex")))
        || !isJsonInteger(position.value(QStringLiteral("exposure")))) {
        error = QStringLiteral("记录%1的%2未包含完整硬件点位。").arg(sequence).arg(label);
        return false;
    }
    QVector<int> axes;
    for (const QJsonValue& value : position.value(QStringLiteral("axes")).toArray()) {
        if (!value.isObject()) {
            error = QStringLiteral("记录%1的%2轴点位无效。").arg(sequence).arg(label);
            return false;
        }
        const QJsonObject axis = value.toObject();
        if (!isJsonInteger(axis.value(QStringLiteral("axis")))
            || !finiteNumber(axis.value(QStringLiteral("planned")))
            || !finiteNumber(axis.value(QStringLiteral("encoder")))) {
            error = QStringLiteral("记录%1的%2轴点位无效。").arg(sequence).arg(label);
            return false;
        }
        axes.append(axis.value(QStringLiteral("axis")).toInt());
    }
    const int cameraIndex = position.value(QStringLiteral("cameraIndex")).toInt();
    const int exposure = position.value(QStringLiteral("exposure")).toInt();
    if (axes != contract.axes || cameraIndex != contract.cameraIndex
        || (cameraIndex < 0 ? exposure != -1 : (exposure < 0 || exposure > 30000))) {
        error = QStringLiteral("记录%1的%2与设备契约不一致。").arg(sequence).arg(label);
        return false;
    }
    return true;
}

inline bool validateRuntimeFeature(const QJsonObject& feature, int frameId, QSet<int>& ids, QString& error)
{
    if (!isJsonInteger(feature.value(QStringLiteral("id")))
        || !feature.value(QStringLiteral("type")).isString()
        || !finiteNumber(feature.value(QStringLiteral("width")))
        || !finiteNumber(feature.value(QStringLiteral("height")))
        || !feature.value(QStringLiteral("points")).isArray()) {
        error = QStringLiteral("执行计划帧%1包含无效ROI几何。").arg(frameId);
        return false;
    }
    const int id = feature.value(QStringLiteral("id")).toInt();
    if (id <= 0 || feature.value(QStringLiteral("type")).toString().trimmed().isEmpty()
        || feature.value(QStringLiteral("width")).toDouble() < 0
        || feature.value(QStringLiteral("height")).toDouble() < 0
        || ids.contains(id)) {
        error = QStringLiteral("执行计划帧%1包含重复或无效ROI。").arg(frameId);
        return false;
    }
    const QJsonArray points = feature.value(QStringLiteral("points")).toArray();
    if (points.isEmpty()) {
        error = QStringLiteral("执行计划帧%1的ROI%2缺少坐标。").arg(frameId).arg(id);
        return false;
    }
    for (const QJsonValue& pointValue : points) {
        if (!pointValue.isArray()) {
            error = QStringLiteral("执行计划帧%1的ROI%2坐标无效。").arg(frameId).arg(id);
            return false;
        }
        const QJsonArray point = pointValue.toArray();
        if (point.size() != 2 || !finiteNumber(point.at(0)) || !finiteNumber(point.at(1))) {
            error = QStringLiteral("执行计划帧%1的ROI%2坐标无效。").arg(frameId).arg(id);
            return false;
        }
    }
    ids.insert(id);
    return true;
}

inline bool buildFrameFeatureIndex(const QJsonObject& definition,
    QHash<int, QSet<int>>& featuresByFrame, QString& error)
{
    featuresByFrame.clear();
    if (!definition.value(QStringLiteral("frames")).isArray()) {
        error = QStringLiteral("测量定义缺少运行帧列表。");
        return false;
    }
    const QJsonArray frames = definition.value(QStringLiteral("frames")).toArray();
    for (const QJsonValue& value : frames) {
        if (!value.isObject()) {
            error = QStringLiteral("测量定义包含无效运行帧。");
            return false;
        }
        const QJsonObject frame = value.toObject();
        if (!isJsonInteger(frame.value(QStringLiteral("id")))
            || !finiteNumber(frame.value(QStringLiteral("width")))
            || !finiteNumber(frame.value(QStringLiteral("height")))
            || !frame.value(QStringLiteral("features")).isArray()) {
            error = QStringLiteral("测量定义运行帧字段无效。");
            return false;
        }
        const int frameId = frame.value(QStringLiteral("id")).toInt();
        if (frameId <= 0 || frame.value(QStringLiteral("width")).toDouble() <= 0
            || frame.value(QStringLiteral("height")).toDouble() <= 0
            || featuresByFrame.contains(frameId)) {
            error = QStringLiteral("测量定义运行帧重复或尺寸无效。");
            return false;
        }
        QSet<int> featureIds;
        for (const QJsonValue& featureValue : frame.value(QStringLiteral("features")).toArray()) {
            if (!featureValue.isObject()
                || !validateRuntimeFeature(featureValue.toObject(), frameId, featureIds, error)) {
                return false;
            }
        }
        featuresByFrame.insert(frameId, featureIds);
    }
    return true;
}

inline bool validateRuntimeRoiReference(const QJsonObject& record,
    const QHash<int, QSet<int>>& featuresByFrame, int sequence,
    const QString& frameField, const QString& geometryField,
    const QString& label, QString& error)
{
    if (!isJsonInteger(record.value(frameField)) || !isJsonInteger(record.value(geometryField))) {
        error = QStringLiteral("记录%1的%2运行ROI字段无效。").arg(sequence).arg(label);
        return false;
    }
    const int frameId = record.value(frameField).toInt();
    const int geometryId = record.value(geometryField).toInt();
    if (frameId <= 0 || geometryId <= 0
        || !featuresByFrame.contains(frameId)
        || !featuresByFrame.value(frameId).contains(geometryId)) {
        error = QStringLiteral("记录%1的%2运行ROI不存在或已失效。").arg(sequence).arg(label);
        return false;
    }
    return true;
}

inline bool buildExecutionStep(const QJsonObject& record, GraphicalProgramStep& step,
    QString& error, const QHash<int, QSet<int>>* featuresByFrame = nullptr)
{
    error.clear();
    if (!isJsonInteger(record.value(QStringLiteral("sequence")))
        || !record.value(QStringLiteral("featureNumber")).isString()
        || !record.value(QStringLiteral("type")).isString()) {
        error = QStringLiteral("执行记录标识字段无效。");
        return false;
    }
    step = GraphicalProgramStep();
    step.sequence = record.value(QStringLiteral("sequence")).toInt();
    step.featureNumber = record.value(QStringLiteral("featureNumber")).toString();
    step.type = record.value(QStringLiteral("type")).toString();
    const bool crossFrameLength = record.value(QStringLiteral("crossFrameLength")).toBool(false);
    const bool singleRoiAngle = record.value(QStringLiteral("singleRoiAngle")).toBool(false);
    step.contract = GraphicalProgramGeneration::contractForType(
        step.type, crossFrameLength, singleRoiAngle);
    if (step.sequence <= 0 || step.featureNumber.isEmpty() || !step.contract.supported) {
        error = QStringLiteral("执行记录序号、特征号或类型无效。");
        return false;
    }
    if (!record.value(QStringLiteral("hasTolerance")).isBool()
        || !finiteNumber(record.value(QStringLiteral("nominal")))
        || !finiteNumber(record.value(QStringLiteral("lower")))
        || !finiteNumber(record.value(QStringLiteral("upper")))) {
        error = QStringLiteral("记录%1的公差字段无效。").arg(step.sequence);
        return false;
    }
    if (record.value(QStringLiteral("hasTolerance")).toBool()
        && record.value(QStringLiteral("lower")).toDouble()
            > record.value(QStringLiteral("upper")).toDouble()) {
        error = QStringLiteral("记录%1的下偏差大于上偏差。").arg(step.sequence);
        return false;
    }
    if (step.type == QStringLiteral("孔径")) {
        double calibration = 0;
        if (!isJsonInteger(record.value(QStringLiteral("holeUniformCount")))
            || record.value(QStringLiteral("holeUniformCount")).toInt() <= 0
            || !finiteNumber(record.value(QStringLiteral("holeCalibrationMmPerPixel")), &calibration)
            || calibration <= 0) {
            error = QStringLiteral("记录%1的孔径参数无效。").arg(step.sequence);
            return false;
        }
    }
    if (step.type == QStringLiteral("长度") || step.type == QStringLiteral("圆弧半径")) {
        double calibration = 0;
        if (!finiteNumber(record.value(QStringLiteral("lengthCalibrationMmPerPixel")), &calibration)
            || calibration <= 0) {
            error = QStringLiteral("记录%1的远心标定无效。").arg(step.sequence);
            return false;
        }
    }
    if (step.contract.threeSectionScan) {
        double lowerOffset = 0, upperOffset = 0;
        if (!finiteNumber(record.value(QStringLiteral("lowerAxialOffsetPulse")), &lowerOffset)
            || !finiteNumber(record.value(QStringLiteral("upperAxialOffsetPulse")), &upperOffset)
            || lowerOffset <= 0 || upperOffset <= 0) {
            error = QStringLiteral("记录%1的三截面偏移无效。").arg(step.sequence);
            return false;
        }
    }
    if (step.contract.requiresTwoReferences) {
        const QString first = record.value(QStringLiteral("roundoutReference1")).toString().trimmed();
        const QString second = record.value(QStringLiteral("roundoutReference2")).toString().trimmed();
        if (first.isEmpty() || second.isEmpty() || first.compare(second, Qt::CaseInsensitive) == 0) {
            error = QStringLiteral("记录%1的两个跳动基准无效。").arg(step.sequence);
            return false;
        }
    }
    if (step.contract.requiresImage && featuresByFrame) {
        if (!validateRuntimeRoiReference(record, *featuresByFrame, step.sequence,
                QStringLiteral("frameId"), QStringLiteral("geometryId"),
                QStringLiteral("主"), error)) return false;
        if (!record.value(QStringLiteral("runtimeRoi")).isObject()
            && step.type != QStringLiteral("角度")) {
            error = QStringLiteral("记录%1缺少主ROI运行几何数据。").arg(step.sequence);
            return false;
        }
        if (step.contract.requiresSecondRoi
            && !validateRuntimeRoiReference(record, *featuresByFrame, step.sequence,
                QStringLiteral("secondaryFrameId"), QStringLiteral("secondaryGeometryId"),
                QStringLiteral("第二"), error)) return false;
        if (step.contract.requiresSecondRoi
            && !record.value(QStringLiteral("runtimeSecondaryRoi")).isObject()) {
            error = QStringLiteral("记录%1缺少第二ROI运行几何数据。").arg(step.sequence);
            return false;
        }
    }
    if (crossFrameLength) {
        if (!validateCollectedPosition(record.value(QStringLiteral("lengthStartPosition")).toObject(),
                step.contract, step.sequence, QStringLiteral("起点"), error)
            || !validateCollectedPosition(record.value(QStringLiteral("lengthEndPosition")).toObject(),
                step.contract, step.sequence, QStringLiteral("终点"), error)) return false;
    }
    else {
        if (!record.value(QStringLiteral("devicePosition")).isObject()
            || !validateCollectedPosition(record.value(QStringLiteral("devicePosition")).toObject(),
                step.contract, step.sequence, QStringLiteral("设备点位"), error)) return false;
        if (step.contract.requiresTemplate) {
            const QJsonObject model = record.value(QStringLiteral("lengthTemplate")).toObject();
            if (model.value(QStringLiteral("status")).toString() != QStringLiteral("trained")
                || model.value(QStringLiteral("encoding")).toString()
                    != QStringLiteral("halcon-shape-model-base64")
                || model.value(QStringLiteral("data")).toString().isEmpty()) {
                error = QStringLiteral("记录%1缺少长度模板。").arg(step.sequence);
                return false;
            }
        }
    }
    step.definition = record;
    return true;
}

inline QByteArray sha256(const QString& filePath, QString& error)
{
    QFile input(filePath);
    if (!input.open(QIODevice::ReadOnly)) {
        error = input.errorString();
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!input.atEnd()) {
        const QByteArray block = input.read(1024 * 1024);
        if (block.isEmpty() && input.error() != QFile::NoError) {
            error = input.errorString();
            return {};
        }
        hash.addData(block);
    }
    return hash.result().toHex();
}

inline bool loadPackage(const QString& packageDirectory,
    GraphicalProgramDescriptor& descriptor, QString& error)
{
    descriptor = GraphicalProgramDescriptor();
    error.clear();
    const QFileInfo directoryInfo(packageDirectory);
    if (!directoryInfo.exists() || !directoryInfo.isDir()) {
        error = QStringLiteral("程序包目录不存在：%1").arg(packageDirectory);
        return false;
    }
    const QRegularExpressionMatch directoryMatch = QRegularExpression(
        QStringLiteral("^program_([0-9]+)$")).match(directoryInfo.fileName());
    if (!directoryMatch.hasMatch()) {
        error = QStringLiteral("程序包目录名必须为program_N：%1").arg(directoryInfo.fileName());
        return false;
    }
    bool numberOk = false;
    const int directoryNumber = directoryMatch.captured(1).toInt(&numberOk);
    if (!numberOk || directoryNumber < 61 || directoryNumber > 999) {
        error = QStringLiteral("图形化程序号须为61–999：%1").arg(directoryInfo.fileName());
        return false;
    }

    const QDir directory(directoryInfo.absoluteFilePath());
    QFile manifestFile(directory.filePath(QStringLiteral("generation_manifest.json")));
    if (!manifestFile.open(QIODevice::ReadOnly) || manifestFile.size() > 1024 * 1024) {
        error = manifestFile.isOpen() ? QStringLiteral("生成清单超过1 MB限制。")
            : QStringLiteral("无法打开生成清单：%1").arg(manifestFile.errorString());
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument manifestDocument = QJsonDocument::fromJson(manifestFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !manifestDocument.isObject()) {
        error = QStringLiteral("生成清单JSON无效：%1").arg(parseError.errorString());
        return false;
    }
    const QJsonObject manifest = manifestDocument.object();
    const int programNumber = manifest.value(QStringLiteral("programNumber")).toInt(-1);
    const QString definitionName = manifest.value(QStringLiteral("definition")).toString();
    const QString expectedHash = manifest.value(QStringLiteral("definitionSha256")).toString().toLower();
    if (manifest.value(QStringLiteral("format")).toString()
            != QStringLiteral("AxisMeasurement.GraphicalProgramPackage")
        || manifest.value(QStringLiteral("version")).toInt(-1) != 1
        || programNumber != directoryNumber
        || QFileInfo(definitionName).fileName() != definitionName
        || definitionName.isEmpty()
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(expectedHash).hasMatch()
        || !manifest.value(QStringLiteral("records")).isArray()
        || manifest.value(QStringLiteral("records")).toArray().isEmpty()) {
        error = QStringLiteral("程序包清单字段无效或与目录程序号不一致。");
        return false;
    }

    const QString definitionPath = directory.filePath(definitionName);
    QString hashError;
    const QByteArray actualHash = sha256(definitionPath, hashError);
    if (actualHash.isEmpty() || QString::fromLatin1(actualHash) != expectedHash) {
        error = actualHash.isEmpty()
            ? QStringLiteral("无法校验测量定义：%1").arg(hashError)
            : QStringLiteral("测量定义SHA-256与生成清单不一致。");
        return false;
    }

    QFile definitionFile(definitionPath);
    if (!definitionFile.open(QIODevice::ReadOnly) || definitionFile.size() > 20 * 1024 * 1024) {
        error = definitionFile.isOpen() ? QStringLiteral("测量定义超过20 MB限制。")
            : QStringLiteral("无法打开测量定义：%1").arg(definitionFile.errorString());
        return false;
    }
    const QJsonDocument definitionDocument = QJsonDocument::fromJson(definitionFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !definitionDocument.isObject()) {
        error = QStringLiteral("测量定义JSON无效：%1").arg(parseError.errorString());
        return false;
    }
    const QJsonObject definition = definitionDocument.object();
    const QJsonObject recipe = definition.value(QStringLiteral("recipe")).toObject();
    const QJsonArray manifestRecords = manifest.value(QStringLiteral("records")).toArray();
    const QJsonArray definitionRecords = definition.value(QStringLiteral("records")).toArray();
    if (definition.value(QStringLiteral("format")).toString()
            != QStringLiteral("AxisMeasurement.GraphicalProject")
        || recipe.value(QStringLiteral("programNumber")).toInt(-1) != programNumber
        || !definition.value(QStringLiteral("records")).isArray()
        || definitionRecords.size() != manifestRecords.size()) {
        error = QStringLiteral("测量定义与生成清单不一致。");
        return false;
    }

    QHash<int, QJsonObject> definitionsBySequence;
    QSet<QString> featureNumbers;
    for (const QJsonValue& value : definitionRecords) {
        if (!value.isObject()) {
            error = QStringLiteral("测量定义包含无效记录。");
            return false;
        }
        const QJsonObject record = value.toObject();
        if (!isJsonInteger(record.value(QStringLiteral("sequence")))) {
            error = QStringLiteral("测量定义包含无效记录序号。");
            return false;
        }
        const int sequence = record.value(QStringLiteral("sequence")).toInt();
        const QString featureNumber = record.value(QStringLiteral("featureNumber")).toString();
        const QString featureKey = featureNumber.toCaseFolded();
        if (sequence <= 0 || featureNumber.isEmpty() || definitionsBySequence.contains(sequence)
            || featureNumbers.contains(featureKey)) {
            error = QStringLiteral("测量定义的记录序号或特征号重复或无效。");
            return false;
        }
        definitionsBySequence.insert(sequence, record);
        featureNumbers.insert(featureKey);
    }
    QSet<int> manifestSequences;
    for (const QJsonValue& value : manifestRecords) {
        if (!value.isObject()) {
            error = QStringLiteral("程序包清单包含无效记录。");
            return false;
        }
        const QJsonObject record = value.toObject();
        if (!isJsonInteger(record.value(QStringLiteral("sequence")))) {
            error = QStringLiteral("程序包清单包含无效记录序号。");
            return false;
        }
        const int sequence = record.value(QStringLiteral("sequence")).toInt();
        if (manifestSequences.contains(sequence) || !definitionsBySequence.contains(sequence)
            || !contractMatches(record, definitionsBySequence.value(sequence), error)) {
            if (error.isEmpty()) error = QStringLiteral("程序包清单记录重复或无法匹配测量定义。");
            return false;
        }
        manifestSequences.insert(sequence);
    }

    QString assetPath;
    const QJsonValue assetValue = manifest.value(QStringLiteral("assetDirectory"));
    if (!assetValue.isNull()) {
        const QString assetName = assetValue.toString();
        if (assetName.isEmpty() || QFileInfo(assetName).fileName() != assetName
            || !QFileInfo(directory.filePath(assetName)).isDir()) {
            error = QStringLiteral("程序包图像资源目录无效。");
            return false;
        }
        assetPath = directory.filePath(assetName);
    }

    const QJsonObject metadata = manifest.value(QStringLiteral("metadata")).toObject();
    const QStringList metadataFields = { QStringLiteral("partNumber"), QStringLiteral("partName"),
        QStringLiteral("processNumber"), QStringLiteral("note") };
    for (const QString& field : metadataFields) {
        if (!metadata.value(field).isString() || !recipe.value(field).isString()
            || metadata.value(field).toString() != recipe.value(field).toString()) {
            error = QStringLiteral("程序包元数据与测量定义不一致：%1。").arg(field);
            return false;
        }
    }
    descriptor.programNumber = programNumber;
    descriptor.packagePath = directoryInfo.absoluteFilePath();
    descriptor.definitionPath = QFileInfo(definitionPath).absoluteFilePath();
    descriptor.assetPath = assetPath;
    descriptor.partNumber = metadata.value(QStringLiteral("partNumber")).toString();
    descriptor.partName = metadata.value(QStringLiteral("partName")).toString();
    descriptor.processNumber = metadata.value(QStringLiteral("processNumber")).toString();
    descriptor.recordCount = manifest.value(QStringLiteral("records")).toArray().size();
    return true;
}

inline bool scan(const QString& rootPath, QVector<GraphicalProgramDescriptor>& programs, QString& error)
{
    programs.clear();
    error.clear();
    const QDir root(rootPath);
    if (!root.exists()) return true;
    QSet<int> numbers;
    const QFileInfoList directories = root.entryInfoList(
        { QStringLiteral("program_*") }, QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& directory : directories) {
        GraphicalProgramDescriptor descriptor;
        if (!loadPackage(directory.absoluteFilePath(), descriptor, error)) return false;
        if (numbers.contains(descriptor.programNumber)) {
            error = QStringLiteral("发现重复图形化程序号：%1").arg(descriptor.programNumber);
            return false;
        }
        numbers.insert(descriptor.programNumber);
        programs.append(descriptor);
    }
    std::sort(programs.begin(), programs.end(), [](const auto& left, const auto& right) {
        return left.programNumber < right.programNumber;
    });
    return true;
}

inline bool loadExecutionPlan(const QString& packageDirectory,
    GraphicalProgramExecutionPlan& plan, QString& error)
{
    plan = GraphicalProgramExecutionPlan();
    if (!loadPackage(packageDirectory, plan.descriptor, error)) return false;
    QFile definitionFile(plan.descriptor.definitionPath);
    if (!definitionFile.open(QIODevice::ReadOnly) || definitionFile.size() > 20 * 1024 * 1024) {
        error = definitionFile.isOpen() ? QStringLiteral("测量定义超过20 MB限制。")
            : QStringLiteral("无法打开测量定义：%1").arg(definitionFile.errorString());
        plan = GraphicalProgramExecutionPlan();
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(definitionFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("测量定义JSON无效：%1").arg(parseError.errorString());
        plan = GraphicalProgramExecutionPlan();
        return false;
    }
    QHash<int, QSet<int>> featuresByFrame;
    if (!buildFrameFeatureIndex(document.object(), featuresByFrame, error)) {
        plan = GraphicalProgramExecutionPlan();
        return false;
    }
    const QJsonArray records = document.object().value(QStringLiteral("records")).toArray();
    for (const QJsonValue& value : records) {
        if (!value.isObject()) {
            error = QStringLiteral("执行计划包含无效记录。");
            plan = GraphicalProgramExecutionPlan();
            return false;
        }
        GraphicalProgramStep step;
        if (!buildExecutionStep(value.toObject(), step, error, &featuresByFrame)) {
            plan = GraphicalProgramExecutionPlan();
            return false;
        }
        plan.steps.append(step);
    }
    std::sort(plan.steps.begin(), plan.steps.end(), [](const auto& left, const auto& right) {
        return left.sequence < right.sequence;
    });
    if (plan.steps.size() != plan.descriptor.recordCount) {
        error = QStringLiteral("执行计划记录数量与程序包不一致。");
        plan = GraphicalProgramExecutionPlan();
        return false;
    }
    return true;
}

} // namespace GraphicalProgramRegistry
