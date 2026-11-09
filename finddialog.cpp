#include "finddialog.h"
#include "ui_finddialog.h"

FindDialog::FindDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FindDialog)
{
    ui->setupUi(this);
    // 使用非模态对话框，查找时仍可看到编辑区中的匹配结果
    setModal(false);
    ui->lineEditFind->setFocus();

    connect(ui->btnFind, &QPushButton::clicked, this, &FindDialog::onFindClicked);
    connect(ui->btnClose, &QPushButton::clicked, this, &QDialog::close);
    // 在输入框中直接回车等同于点击“查找下一个”
    connect(ui->lineEditFind, &QLineEdit::returnPressed, this, &FindDialog::onFindClicked);
}

FindDialog::~FindDialog()
{
    delete ui;
}

QString FindDialog::findText() const
{
    return ui->lineEditFind->text();
}

void FindDialog::setFindText(const QString &text)
{
    ui->lineEditFind->setText(text);
    ui->lineEditFind->selectAll();
}

void FindDialog::onFindClicked()
{
    const QString text = ui->lineEditFind->text();
    if (text.isEmpty()) {
        ui->lineEditFind->setFocus();
        return;
    }
    emit findRequested(text, ui->checkCase->isChecked(), ui->radioDown->isChecked());
}
