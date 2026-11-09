#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>

namespace Ui {
class FindDialog;
}

// 查找对话框：只负责收集用户输入的查找条件，
// 真正的查找动作由主窗口完成（通过 findRequested 信号通知）。
class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QWidget *parent = nullptr);
    ~FindDialog();

    QString findText() const;
    void setFindText(const QString &text);

signals:
    // forward 为 true 表示向下查找，false 表示向上查找
    void findRequested(const QString &text, bool caseSensitive, bool forward);

private slots:
    void onFindClicked();

private:
    Ui::FindDialog *ui;
};

#endif // FINDDIALOG_H
