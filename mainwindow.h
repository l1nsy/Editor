#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class CodeEditor;
class FindDialog;
class ReplaceDialog;
class QLabel;
class QTimer;

// 主窗口：负责组织界面、维护当前文件状态，并实现文件、编辑、
// 格式、视图四组功能的业务逻辑。
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 供命令行参数调用：直接打开指定路径的文件
    bool loadFile(const QString &fileName);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // 文件操作
    void newFile();
    void openFile();
    bool save();
    bool saveAs();
    // 格式设置
    void changeFont();
    void changeTextColor();
    void changeBackgroundColor();
    void setWordWrapEnabled(bool enabled);
    // 查找与替换
    void showFindDialog();
    void showReplaceDialog();
    void findNext(const QString &text, bool caseSensitive, bool forward);
    void replaceNext(const QString &text, const QString &replacement, bool caseSensitive);
    void replaceAll(const QString &text, const QString &replacement, bool caseSensitive);
    // 视图
    void setLineNumberVisible(bool visible);
    void setAutoSaveEnabled(bool enabled);
    void showAboutDialog();
    // 状态栏、标题栏与动作状态
    void updateStatusBar();
    void updateActions();
    void updateWindowTitle();
    void autoSave();

private:
    void createActions();
    void createStatusBar();
    bool maybeSave();
    // 统一的提示框，按钮文字使用中文，避免出现英文按钮
    void showInformation(const QString &title, const QString &text);
    void showWarning(const QString &title, const QString &text);
    bool askContinue(const QString &title, const QString &text);
    bool saveToFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    QString shownFileName() const;

    Ui::MainWindow *ui;
    CodeEditor *m_editor;           // 中央编辑区（由 QPlainTextEdit 提升而来）
    FindDialog *m_findDialog;       // 查找对话框，第一次使用时创建
    ReplaceDialog *m_replaceDialog; // 替换对话框
    QTimer *m_autoSaveTimer;        // 定时自动保存计时器
    QLabel *m_positionLabel;        // 状态栏：光标行列
    QLabel *m_charCountLabel;       // 状态栏：字符数
    QLabel *m_stateLabel;           // 状态栏：修改状态
    QString m_currentFile;          // 当前文件的完整路径，空串表示尚未保存
};

#endif // MAINWINDOW_H
