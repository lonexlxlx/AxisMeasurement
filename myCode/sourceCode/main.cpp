#include "AxisMeasurement.h"
#include <QtWidgets/QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QIODevice>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    SetConsoleOutputCP(65001);//修改控制台为utf-8编码，便于中文显示
    QApplication a(argc, argv);
    a.setApplicationName(QStringLiteral("AxisMeasurement"));
    //全局界面主题：运行目录 config/theme.qss，文件缺失时保持 Qt 默认样式（即回滚开关）
    {
        QFile themeFile(QCoreApplication::applicationDirPath() + "/config/theme.qss");
        if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text))
            a.setStyleSheet(QString::fromUtf8(themeFile.readAll()));
    }
    AxisMeasurement w;
    return a.exec();
}
