#ifndef VERIFICATION_DLG_H
#define VERIFICATION_DLG_H

#include <QDialog>

namespace Ui {
class verification_dlg;
}

class verification_dlg : public QDialog
{
    Q_OBJECT

public:
    explicit verification_dlg(QWidget *parent = 0);
    ~verification_dlg();
    QString get_email ();

private:
    Ui::verification_dlg *ui;
};

#endif // VERIFICATION_DLG_H
