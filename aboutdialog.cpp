#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QPushButton>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    // 关于对话框用模态显示，弹出后需先关闭才能继续操作主窗口
    setModal(true);
    setFixedSize(sizeHint());

    connect(ui->btnOk, &QPushButton::clicked, this, &QDialog::accept);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
