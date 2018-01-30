#ifndef QRCODEDIALOG_H
#define QRCODEDIALOG_H

#include <QDialog>
#include <QImage>
#include <QLabel>
#include <QPushButton>

namespace Ui {
    class QRCodeDialog;
}
class OptionsModel;

class QRCodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QRCodeDialog(const QString &addr, const QString &label, bool enableReq, QWidget *parent = 0, const QString &priv = "",bool paperWallet = false);
    ~QRCodeDialog();

    void setModel(OptionsModel *model);

private Q_SLOTS:
    void on_lnReqAmount_textChanged();
    void on_lnLabel_textChanged();
    void on_lnMessage_textChanged();
    void on_btnSaveAs_clicked();
    void on_chkReqPayment_toggled(bool fChecked);

    void updateDisplayUnit();

private:
    Ui::QRCodeDialog *ui;
    OptionsModel *model;
    QString address;
    QString priv;
    QImage myImage;
    QLabel* lblQRCode_pub;
    QLabel* lblQRCode_priv;
    QLabel* outUri_pub;
    QLabel *outUri_priv;
    QPushButton* btnSaveAs;



    void genCodePub();
    QString getURIPub();
    void genCodePriv();
    QString getURIPriv();
};

#endif // QRCODEDIALOG_H
