#include "mainwindow.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QStringList>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("简易文本编辑器"));
    QApplication::setApplicationVersion(QStringLiteral("1.0"));

    // 加载 Qt 自带的中文翻译，使标准对话框（字体、颜色等）显示为中文。
    // 先找程序目录下的 translations，再找 Qt 安装目录，便于程序拷贝到其他机器后仍能正常显示。
    QTranslator qtTranslator;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QString qtTranslations = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#else
    const QString qtTranslations = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
    const QStringList searchDirs = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/translations"),
        qtTranslations
    };
    for (const QString &dir : searchDirs) {
        if (qtTranslator.load(QStringLiteral("qtbase_zh_CN"), dir)) {
            app.installTranslator(&qtTranslator);
            break;
        }
    }

    MainWindow window;
    window.show();

    // 支持带参数启动，例如在命令行中执行 TextEditor.exe D:\demo.txt
    if (argc > 1)
        window.loadFile(QString::fromLocal8Bit(argv[1]));

    return app.exec();
}
