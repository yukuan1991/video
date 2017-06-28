#include "verification_dlg.h"
#include "ui_verification_dlg.h"

verification_dlg::verification_dlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::verification_dlg)
{
    ui->setupUi(this);
}

verification_dlg::~verification_dlg()
{
    delete ui;
}

QString verification_dlg::get_email()
{
    return ui->edit_email->text ();
}
