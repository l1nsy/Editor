#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include <QDialog>

namespace Ui {
class ReplaceDialog;
}

// 替换对话框：同样只负责收集条件，查找与替换动作由主窗口执行。
class ReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplaceDialog(QWidget *parent = nullptr);
    ~ReplaceDialog();

    QString findText() const;
    QString replaceText() const;
    void setFindText(const QString &text);

signals:
    void findRequested(const QString &text, bool caseSensitive, bool forward);
    void replaceRequested(const QString &text, const QString &replacement, bool caseSensitive);
    void replaceAllRequested(const QString &text, const QString &replacement, bool caseSensitive);

private slots:
    void onFindClicked();
    void onReplaceClicked();
    void onReplaceAllClicked();

private:
    // 统一检查查找内容是否为空，为空时把焦点放回输入框
    bool checkFindTextNotEmpty();

    Ui::ReplaceDialog *ui;
};

#endif // REPLACEDIALOG_H
