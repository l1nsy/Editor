#include "replacedialog.h"
#include "ui_replacedialog.h"

ReplaceDialog::ReplaceDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ReplaceDialog)
{
    ui->setupUi(this);
    setModal(false);
    ui->lineEditFind->setFocus();

    connect(ui->btnFind, &QPushButton::clicked, this, &ReplaceDialog::onFindClicked);
    connect(ui->btnReplace, &QPushButton::clicked, this, &ReplaceDialog::onReplaceClicked);
    connect(ui->btnReplaceAll, &QPushButton::clicked, this, &ReplaceDialog::onReplaceAllClicked);
    connect(ui->btnClose, &QPushButton::clicked, this, &QDialog::close);
    connect(ui->lineEditFind, &QLineEdit::returnPressed, this, &ReplaceDialog::onFindClicked);
}

ReplaceDialog::~ReplaceDialog()
{
    delete ui;
}

QString ReplaceDialog::findText() const
{
    return ui->lineEditFind->text();
}

QString ReplaceDialog::replaceText() const
{
    return ui->lineEditReplace->text();
}

void ReplaceDialog::setFindText(const QString &text)
{
    ui->lineEditFind->setText(text);
    ui->lineEditFind->selectAll();
}

bool ReplaceDialog::checkFindTextNotEmpty()
{
    if (ui->lineEditFind->text().isEmpty()) {
        ui->lineEditFind->setFocus();
        return false;
    }
    return true;
}

void ReplaceDialog::onFindClicked()
{
    if (!checkFindTextNotEmpty())
        return;
    emit findRequested(ui->lineEditFind->text(),
                       ui->checkCase->isChecked(),
                       ui->radioDown->isChecked());
}

void ReplaceDialog::onReplaceClicked()
{
    if (!checkFindTextNotEmpty())
        return;
    emit replaceRequested(ui->lineEditFind->text(),
                          ui->lineEditReplace->text(),
                          ui->checkCase->isChecked());
}

void ReplaceDialog::onReplaceAllClicked()
{
    if (!checkFindTextNotEmpty())
        return;
    emit replaceAllRequested(ui->lineEditFind->text(),
                             ui->lineEditReplace->text(),
                             ui->checkCase->isChecked());
}
