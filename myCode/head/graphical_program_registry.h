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

} // namespace GraphicalProgramRegistry
