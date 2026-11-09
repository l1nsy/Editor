#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdialog.h"
#include "codeeditor.h"
#include "finddialog.h"
#include "replacedialog.h"

#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QGuiApplication>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QStyle>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif

namespace {

// 取工具栏图标：优先使用系统图标主题，取不到时退回 Qt 自带的标准图标
QIcon themeIcon(const QString &name, QStyle::StandardPixmap fallback)
{
    QIcon icon = QIcon::fromTheme(name);
    if (icon.isNull())
        icon = qApp->style()->standardIcon(fallback);
    return icon;
}

// 按当前 Qt 版本设置文本流的编码，保证中文内容读写正常
void setStreamUtf8(QTextStream &stream)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_editor(nullptr)
    , m_findDialog(nullptr)
    , m_replaceDialog(nullptr)
    , m_autoSaveTimer(nullptr)
    , m_positionLabel(nullptr)
    , m_charCountLabel(nullptr)
    , m_stateLabel(nullptr)
{
    ui->setupUi(this);
    m_editor = ui->textEdit;

    // 让文本与边框之间留出一点空白，阅读时不至于太拥挤
    m_editor->document()->setDocumentMargin(8);
    m_editor->setLineWrapMode(QPlainTextEdit::WidgetWidth);

    createActions();
    createStatusBar();

    // 文本内容与光标位置变化时刷新状态栏
    connect(m_editor, &QPlainTextEdit::textChanged, this, &MainWindow::updateStatusBar);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatusBar);
    connect(m_editor->document(), &QTextDocument::modificationChanged,
            this, [this](bool) {
                updateWindowTitle();
                updateStatusBar();
                updateActions();
            });

    // 撤销、重做、剪切、复制、粘贴的可用状态跟随编辑状态变化
    connect(m_editor, &QPlainTextEdit::undoAvailable, this, &MainWindow::updateActions);
    connect(m_editor, &QPlainTextEdit::redoAvailable, this, &MainWindow::updateActions);
    connect(m_editor, &QPlainTextEdit::copyAvailable, this, &MainWindow::updateActions);
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged,
            this, &MainWindow::updateActions);

    // 定时自动保存：默认关闭，由“视图”菜单中的动作控制
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(60 * 1000);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSave);

    // 设置界面属性时文档会被标记为“已修改”，这里恢复为初始的未修改状态
    m_editor->document()->setModified(false);

    updateWindowTitle();
    updateStatusBar();
    updateActions();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createActions()
{
    // 为工具栏准备图标，使按钮比纯文字更直观
    ui->actionNew->setIcon(themeIcon("document-new", QStyle::SP_FileIcon));
    ui->actionOpen->setIcon(themeIcon("document-open", QStyle::SP_DirOpenIcon));
    ui->actionSave->setIcon(themeIcon("document-save", QStyle::SP_DialogSaveButton));
    ui->actionSaveAs->setIcon(themeIcon("document-save-as", QStyle::SP_DialogSaveButton));
    ui->actionUndo->setIcon(themeIcon("edit-undo", QStyle::SP_ArrowBack));
    ui->actionRedo->setIcon(themeIcon("edit-redo", QStyle::SP_ArrowForward));
    ui->actionCut->setIcon(themeIcon("edit-cut", QStyle::SP_FileDialogDetailedView));
    ui->actionCopy->setIcon(themeIcon("edit-copy", QStyle::SP_FileDialogListView));
    ui->actionPaste->setIcon(themeIcon("edit-paste", QStyle::SP_FileDialogContentsView));
    ui->actionFind->setIcon(themeIcon("edit-find", QStyle::SP_FileDialogStart));
    ui->actionReplace->setIcon(themeIcon("edit-find", QStyle::SP_FileDialogStart));
    ui->actionFont->setIcon(themeIcon("format-text-bold", QStyle::SP_FileDialogInfoView));
    ui->actionTextColor->setIcon(themeIcon("format-text-color", QStyle::SP_DesktopIcon));
    ui->actionBackground->setIcon(themeIcon("format-fill-color", QStyle::SP_DialogResetButton));
    ui->actionWrap->setIcon(themeIcon("format-justify-fill", QStyle::SP_FileDialogContentsView));
    ui->actionLineNumber->setIcon(themeIcon("format-list-ordered", QStyle::SP_FileDialogListView));
    ui->actionAutoSave->setIcon(themeIcon("document-save", QStyle::SP_DialogSaveButton));

    // 文件操作
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::save);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveAs);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);

    // 编辑操作，直接复用 QPlainTextEdit 自带的槽函数
    connect(ui->actionUndo, &QAction::triggered, m_editor, &QPlainTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered, m_editor, &QPlainTextEdit::redo);
    connect(ui->actionCut, &QAction::triggered, m_editor, &QPlainTextEdit::cut);
    connect(ui->actionCopy, &QAction::triggered, m_editor, &QPlainTextEdit::copy);
    connect(ui->actionPaste, &QAction::triggered, m_editor, &QPlainTextEdit::paste);
    connect(ui->actionSelectAll, &QAction::triggered, m_editor, &QPlainTextEdit::selectAll);
    connect(ui->actionFind, &QAction::triggered, this, &MainWindow::showFindDialog);
    connect(ui->actionReplace, &QAction::triggered, this, &MainWindow::showReplaceDialog);

    // 格式设置
    connect(ui->actionFont, &QAction::triggered, this, &MainWindow::changeFont);
    connect(ui->actionTextColor, &QAction::triggered, this, &MainWindow::changeTextColor);
    connect(ui->actionBackground, &QAction::triggered, this, &MainWindow::changeBackgroundColor);
    connect(ui->actionWrap, &QAction::toggled, this, &MainWindow::setWordWrapEnabled);

    // 视图设置
    connect(ui->actionLineNumber, &QAction::toggled, this, &MainWindow::setLineNumberVisible);
    connect(ui->actionAutoSave, &QAction::toggled, this, &MainWindow::setAutoSaveEnabled);
    connect(ui->actionToolBar, &QAction::toggled, this, [this](bool visible) {
        ui->mainToolBar->setVisible(visible);
        ui->formatToolBar->setVisible(visible);
    });
    connect(ui->actionStatusBar, &QAction::toggled, ui->statusbar, &QStatusBar::setVisible);

    // 帮助
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAboutDialog);
}

void MainWindow::createStatusBar()
{
    m_positionLabel = new QLabel(tr("行 1，列 1"), this);
    m_positionLabel->setMinimumWidth(120);
    m_charCountLabel = new QLabel(tr("字符数：0"), this);
    m_charCountLabel->setMinimumWidth(100);
    m_stateLabel = new QLabel(tr("未修改"), this);
    m_stateLabel->setMinimumWidth(80);

    statusBar()->addWidget(m_positionLabel);
    statusBar()->addPermanentWidget(m_charCountLabel);
    statusBar()->addPermanentWidget(m_stateLabel);
}

void MainWindow::newFile()
{
    if (!maybeSave())
        return;

    m_editor->clear();
    setCurrentFile(QString());
    m_editor->document()->setModified(false);
    updateWindowTitle();
    updateStatusBar();
    statusBar()->showMessage(tr("已新建空白文档"), 2000);
}

void MainWindow::openFile()
{
    if (!maybeSave())
        return;

    const QString startDir = m_currentFile.isEmpty()
                                 ? QDir::homePath()
                                 : QFileInfo(m_currentFile).absolutePath();
    const QString fileName = QFileDialog::getOpenFileName(
        this, tr("打开文件"), startDir,
        tr("文本文件 (*.txt *.md *.log *.cpp *.h);;所有文件 (*.*)"));
    if (fileName.isEmpty())
        return;

    loadFile(fileName);
}

bool MainWindow::loadFile(const QString &fileName)
{
    const QFileInfo info(fileName);
    if (!info.exists() || !info.isFile()) {
        showWarning(tr("打开文件"),
                    tr("文件不存在或不是一个普通文件：\n%1")
                        .arg(QDir::toNativeSeparators(fileName)));
        return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        showWarning(tr("打开文件"),
                    tr("无法打开文件：\n%1\n\n原因：%2")
                        .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    setStreamUtf8(stream);
    const QString content = stream.readAll();
    file.close();

    // 出现替换字符说明文件不是合法的 UTF-8 文本，多半是二进制文件
    if (content.contains(QChar::ReplacementCharacter)) {
        if (!askContinue(tr("打开文件"),
                         tr("文件“%1”可能不是 UTF-8 编码的文本文件，继续打开可能出现乱码。\n"
                            "是否仍要继续？")
                             .arg(info.fileName())))
            return false;
    }

    m_editor->setPlainText(content);
    setCurrentFile(fileName);
    m_editor->document()->setModified(false);
    updateWindowTitle();
    updateStatusBar();
    statusBar()->showMessage(tr("已打开“%1”").arg(info.fileName()), 3000);
    return true;
}

bool MainWindow::save()
{
    // 尚未命名的新文档，等同于“另存为”
    if (m_currentFile.isEmpty())
        return saveAs();
    return saveToFile(m_currentFile);
}

bool MainWindow::saveAs()
{
    const QString startPath = m_currentFile.isEmpty()
                                  ? QDir::homePath() + QDir::separator() + tr("无标题.txt")
                                  : m_currentFile;
    QString fileName = QFileDialog::getSaveFileName(
        this, tr("另存为"), startPath,
        tr("文本文件 (*.txt);;所有文件 (*.*)"));
    if (fileName.isEmpty())
        return false;

    // 用户没有输入扩展名时，默认按 .txt 保存
    if (QFileInfo(fileName).suffix().isEmpty())
        fileName += QStringLiteral(".txt");

    return saveToFile(fileName);
}

bool MainWindow::saveToFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        showWarning(tr("保存文件"),
                    tr("无法保存文件：\n%1\n\n原因：%2")
                        .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    setStreamUtf8(stream);
    stream << m_editor->toPlainText();
    stream.flush();

    if (stream.status() != QTextStream::Ok) {
        showWarning(tr("保存文件"),
                    tr("写入文件时发生错误：\n%1")
                        .arg(QDir::toNativeSeparators(fileName)));
        file.close();
        return false;
    }
    file.close();

    setCurrentFile(fileName);
    m_editor->document()->setModified(false);
    updateWindowTitle();
    statusBar()->showMessage(tr("已保存到“%1”").arg(QFileInfo(fileName).fileName()), 3000);
    return true;
}

bool MainWindow::maybeSave()
{
    if (!m_editor->document()->isModified())
        return true;

    QMessageBox box(QMessageBox::Warning, tr("简易文本编辑器"),
                    tr("文档“%1”已被修改，是否保存修改？").arg(shownFileName()),
                    QMessageBox::NoButton, this);
    QPushButton *saveButton = box.addButton(tr("保存"), QMessageBox::AcceptRole);
    box.addButton(tr("不保存"), QMessageBox::DestructiveRole);
    QPushButton *cancelButton = box.addButton(tr("取消"), QMessageBox::RejectRole);
    box.setDefaultButton(saveButton);
    box.exec();

    if (box.clickedButton() == saveButton)
        return save();
    if (box.clickedButton() == cancelButton)
        return false;
    return true;
}

void MainWindow::showInformation(const QString &title, const QString &text)
{
    QMessageBox box(QMessageBox::Information, title, text, QMessageBox::NoButton, this);
    box.addButton(tr("确定"), QMessageBox::AcceptRole);
    box.exec();
}

void MainWindow::showWarning(const QString &title, const QString &text)
{
    QMessageBox box(QMessageBox::Warning, title, text, QMessageBox::NoButton, this);
    box.addButton(tr("确定"), QMessageBox::AcceptRole);
    box.exec();
}

bool MainWindow::askContinue(const QString &title, const QString &text)
{
    QMessageBox box(QMessageBox::Question, title, text, QMessageBox::NoButton, this);
    QPushButton *continueButton = box.addButton(tr("继续打开"), QMessageBox::AcceptRole);
    box.addButton(tr("取消"), QMessageBox::RejectRole);
    box.setDefaultButton(continueButton);
    box.exec();
    return box.clickedButton() == continueButton;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    m_currentFile = fileName.isEmpty() ? QString() : QFileInfo(fileName).absoluteFilePath();
    updateWindowTitle();
}

QString MainWindow::shownFileName() const
{
    return m_currentFile.isEmpty() ? tr("无标题") : QFileInfo(m_currentFile).fileName();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}

void MainWindow::changeFont()
{
    bool ok = false;
    const QFont font = QFontDialog::getFont(&ok, m_editor->font(), this, tr("设置字体"));
    if (!ok)
        return;

    m_editor->setFont(font);
    m_editor->refreshLineNumberArea();
    statusBar()->showMessage(tr("字体已设置为 %1 %2 磅")
                                 .arg(font.family())
                                 .arg(font.pointSize()),
                             3000);
}

void MainWindow::changeTextColor()
{
    // QPlainTextEdit 没有 textColor()，从当前字符格式中取前景色作为默认值
    QColor currentColor = m_editor->textCursor().charFormat().foreground().color();
    if (!currentColor.isValid())
        currentColor = m_editor->palette().color(QPalette::Text);

    const QColor color = QColorDialog::getColor(currentColor, this, tr("设置字体颜色"));
    if (!color.isValid())
        return;

    // 有选中内容时只改变选中部分，否则改变整篇文档
    QTextCursor cursor = m_editor->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::Document);

    QTextCharFormat format;
    format.setForeground(color);
    cursor.mergeCharFormat(format);

    statusBar()->showMessage(tr("字体颜色已设置为 %1").arg(color.name()), 3000);
}

void MainWindow::changeBackgroundColor()
{
    const QColor color = QColorDialog::getColor(m_editor->palette().color(QPalette::Base),
                                                this, tr("设置背景色"));
    if (!color.isValid())
        return;

    // QPlainTextEdit 的底色由调色板的 Base 角色决定
    QPalette palette = m_editor->palette();
    palette.setColor(QPalette::Base, color);
    m_editor->setPalette(palette);
    m_editor->refreshCurrentLineHighlight();

    statusBar()->showMessage(tr("背景色已设置为 %1").arg(color.name()), 3000);
}

void MainWindow::setWordWrapEnabled(bool enabled)
{
    m_editor->setLineWrapMode(enabled ? QPlainTextEdit::WidgetWidth
                                      : QPlainTextEdit::NoWrap);
    statusBar()->showMessage(enabled ? tr("已开启自动换行") : tr("已关闭自动换行"), 2000);
}

void MainWindow::showFindDialog()
{
    if (!m_findDialog) {
        m_findDialog = new FindDialog(this);
        connect(m_findDialog, &FindDialog::findRequested, this, &MainWindow::findNext);
    }

    // 编辑区中有选中的单行文本时，直接带入查找框，省去重复输入
    const QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection() && !cursor.selectedText().contains(QChar::ParagraphSeparator))
        m_findDialog->setFindText(cursor.selectedText());

    m_findDialog->show();
    m_findDialog->raise();
    m_findDialog->activateWindow();
}

void MainWindow::showReplaceDialog()
{
    if (!m_replaceDialog) {
        m_replaceDialog = new ReplaceDialog(this);
        connect(m_replaceDialog, &ReplaceDialog::findRequested, this, &MainWindow::findNext);
        connect(m_replaceDialog, &ReplaceDialog::replaceRequested, this, &MainWindow::replaceNext);
        connect(m_replaceDialog, &ReplaceDialog::replaceAllRequested, this, &MainWindow::replaceAll);
    }

    const QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection() && !cursor.selectedText().contains(QChar::ParagraphSeparator))
        m_replaceDialog->setFindText(cursor.selectedText());

    m_replaceDialog->show();
    m_replaceDialog->raise();
    m_replaceDialog->activateWindow();
}

void MainWindow::findNext(const QString &text, bool caseSensitive, bool forward)
{
    if (text.isEmpty())
        return;

    QTextDocument::FindFlags flags;
    if (caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;
    if (!forward)
        flags |= QTextDocument::FindBackward;

    if (m_editor->find(text, flags)) {
        statusBar()->showMessage(tr("已找到“%1”").arg(text), 2000);
        return;
    }

    // 查找到文档末尾（或开头）后回到另一端再找一次，实现循环查找
    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(forward ? QTextCursor::Start : QTextCursor::End);
    m_editor->setTextCursor(cursor);
    if (m_editor->find(text, flags)) {
        statusBar()->showMessage(forward ? tr("已到达文档末尾，从开头继续查找")
                                         : tr("已到达文档开头，从末尾继续查找"),
                                 3000);
        return;
    }

    showInformation(tr("查找"), tr("未找到“%1”。").arg(text));
}

void MainWindow::replaceNext(const QString &text, const QString &replacement, bool caseSensitive)
{
    const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QTextCursor cursor = m_editor->textCursor();

    // 只有当前选中的内容与查找内容一致时才替换，否则先跳到下一处
    if (cursor.hasSelection() && cursor.selectedText().compare(text, cs) == 0) {
        cursor.insertText(replacement);
        m_editor->setTextCursor(cursor);
    }

    findNext(text, caseSensitive, true);
}

void MainWindow::replaceAll(const QString &text, const QString &replacement, bool caseSensitive)
{
    if (text.isEmpty())
        return;

    QTextDocument::FindFlags flags;
    if (caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;

    QTextCursor cursor(m_editor->document());
    int count = 0;

    // 整批替换放在一个编辑块中，撤销时可以作为一步整体回退
    cursor.beginEditBlock();
    forever {
        const QTextCursor found = m_editor->document()->find(text, cursor, flags);
        if (found.isNull())
            break;
        QTextCursor target = found;
        target.insertText(replacement);
        cursor = target;
        ++count;
    }
    cursor.endEditBlock();

    if (count == 0) {
        showInformation(tr("替换"), tr("未找到“%1”。").arg(text));
        return;
    }

    statusBar()->showMessage(tr("已替换 %1 处").arg(count), 3000);
    showInformation(tr("替换"), tr("共替换 %1 处。").arg(count));
}

void MainWindow::setLineNumberVisible(bool visible)
{
    m_editor->setLineNumberVisible(visible);
    statusBar()->showMessage(visible ? tr("已显示行号") : tr("已隐藏行号"), 2000);
}

void MainWindow::setAutoSaveEnabled(bool enabled)
{
    if (enabled) {
        m_autoSaveTimer->start();
        statusBar()->showMessage(tr("已开启定时自动保存，间隔 60 秒"), 3000);
    } else {
        m_autoSaveTimer->stop();
        statusBar()->showMessage(tr("已关闭定时自动保存"), 2000);
    }
}

void MainWindow::autoSave()
{
    if (!m_editor->document()->isModified())
        return;

    // 已经保存过的文件直接写回原文件
    if (!m_currentFile.isEmpty()) {
        if (saveToFile(m_currentFile))
            statusBar()->showMessage(tr("已自动保存“%1”").arg(shownFileName()), 3000);
        return;
    }

    // 未命名文档备份到系统临时目录，避免内容丢失
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                        + QStringLiteral("/TextEditorAutosave");
    QDir().mkpath(dir);
    const QString backup = dir + QStringLiteral("/autosave_%1.txt")
                                     .arg(QDateTime::currentDateTime()
                                              .toString(QStringLiteral("yyyyMMdd_HHmmss")));

    QFile file(backup);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return;
    QTextStream stream(&file);
    setStreamUtf8(stream);
    stream << m_editor->toPlainText();
    file.close();

    statusBar()->showMessage(tr("文档尚未命名，已备份到 %1")
                                 .arg(QDir::toNativeSeparators(backup)),
                             5000);
}

void MainWindow::showAboutDialog()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::updateStatusBar()
{
    const QTextCursor cursor = m_editor->textCursor();
    m_positionLabel->setText(tr("行 %1，列 %2")
                                 .arg(cursor.blockNumber() + 1)
                                 .arg(cursor.columnNumber() + 1));
    m_charCountLabel->setText(tr("字符数：%1").arg(m_editor->toPlainText().length()));
    m_stateLabel->setText(m_editor->document()->isModified() ? tr("● 已修改")
                                                             : tr("未修改"));
}

void MainWindow::updateActions()
{
    const bool hasSelection = m_editor->textCursor().hasSelection();

    ui->actionCut->setEnabled(hasSelection);
    ui->actionCopy->setEnabled(hasSelection);
    ui->actionPaste->setEnabled(!QGuiApplication::clipboard()->text().isEmpty());
    ui->actionUndo->setEnabled(m_editor->document()->isUndoAvailable());
    ui->actionRedo->setEnabled(m_editor->document()->isRedoAvailable());
    ui->actionSelectAll->setEnabled(!m_editor->document()->isEmpty());
}

void MainWindow::updateWindowTitle()
{
    // 标题中的 [*] 会被替换成 *，用来提示文档尚未保存
    setWindowTitle(tr("%1[*] - 简易文本编辑器").arg(shownFileName()));
    setWindowModified(m_editor->document()->isModified());
}
