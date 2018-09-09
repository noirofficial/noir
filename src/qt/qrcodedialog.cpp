#include "qrcodedialog.h"
#include "ui_qrcodedialog.h"

#include "bitcoinunits.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"

#include <QPixmap>
#if QT_VERSION < 0x050000
#include <QUrl>
#endif

#include <qrencode.h>

QRCodeDialog::QRCodeDialog(const QString &addr,  const QString &label, bool enableReq, QWidget *parent,  const QString &priv, bool paperWallet) :
    QDialog(parent),
    ui(new Ui::QRCodeDialog),
    model(0),
    priv(priv),
    address(addr)
{
    ui->setupUi(this);

    address.replace( " ", "" );
    //setWindowTitle(QString("%1").arg(address));
    lblQRCode_pub = new QLabel(this);
    lblQRCode_pub->setGeometry(273,45,225,225);
    lblQRCode_pub->setStyleSheet("background: rgb(0,0,0,0);");
    lblQRCode_pub->show();

    lblQRCode_priv = new QLabel(this);
    lblQRCode_priv->setGeometry(627,45,225,225);
    lblQRCode_priv->setStyleSheet("background: rgb(0,0,0,0);");
    lblQRCode_priv->show();

    QFont med("ZoinMedium" , 14);

#ifdef __linux__
    med.setPointSize(12);
#elif _WIN32
    med.setPointSize(12);
#else

#endif

    outUri_pub = new QLabel(this);
    outUri_pub->setGeometry(180,300,788,30);
#ifdef __linux__
    outUri_pub->setGeometry(185,300,788,30);
#elif _WIN32
    outUri_pub->setGeometry(185,300,788,30);
#else

#endif
    outUri_pub->setAlignment(Qt::AlignBottom | Qt::AlignCenter);
    outUri_pub->setFont(med);
    outUri_pub->setStyleSheet("background: rgb(0,0,0,0);");
    outUri_pub->show();

    outUri_priv = new QLabel(this);
    outUri_priv->setGeometry(280,345,788,30);
#ifdef __linux__
    outUri_priv->setGeometry(300,345,788,30);
#elif _WIN32
    outUri_priv->setGeometry(300,345,788,30);
#else

#endif
    outUri_priv->setAlignment(Qt::AlignBottom | Qt::AlignCenter);
    outUri_priv->setFont(med);
    outUri_priv->setStyleSheet("background: rgb(0,0,0,0);");
    outUri_priv->show();

    btnSaveAs = new QPushButton(this);
    btnSaveAs->setGeometry(920,380,94,32);
    btnSaveAs->setStyleSheet("background: rgb(0,0,0,0);");
    btnSaveAs->setText("Save");
    btnSaveAs->show();


    setWindowTitle(QString("Zoin Paper Wallet"));
    //ui->chkReqPayment->setVisible(enableReq);
    //ui->lblAmount->setVisible(enableReq);
    //ui->lnReqAmount->setVisible(enableReq);

    //ui->lnLabel->setText(label);

    this->resize(1021,420);
    this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    btnSaveAs->setEnabled(true);

    connect(btnSaveAs, SIGNAL(pressed()), this, SLOT(on_btnSaveAs_clicked()));

    if(paperWallet){
        //ui->title->setText(label + " Paper Wallet");
        genCodePriv();
    }
    genCodePub();
}

QRCodeDialog::~QRCodeDialog()
{
    delete ui;
    delete btnSaveAs;
    delete outUri_priv;
    delete outUri_pub;
    delete lblQRCode_priv;
    delete lblQRCode_pub;

}

void QRCodeDialog::setModel(OptionsModel *model)
{
    this->model = model;

    if (model)
        connect(model, SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void QRCodeDialog::genCodePub()
{
    QString uri = getURIPub();

    if (uri != "")
    {
        lblQRCode_pub->setText("");

        QRcode *code = QRcode_encodeString(uri.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        if (!code)
        {
            lblQRCode_pub->setText(tr("Error encoding URI into QR Code."));
            return;
        }
        myImage = QImage(code->width +8, code->width + 8, QImage::Format_RGB32);
        myImage.fill(0xffffff);
        unsigned char *p = code->data;
        for (int y = 0; y < code->width; y++)
        {
            for (int x = 0; x < code->width; x++)
            {
                myImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
                p++;
            }
        }
        QRcode_free(code);

        lblQRCode_pub->setPixmap(QPixmap::fromImage(myImage).scaled(225, 225));

        outUri_pub->setText(uri);
    }
}

QString QRCodeDialog::getURIPub()
{
    QString ret = QString("zoin:%1").arg(address);
    int paramCount = 0;

    outUri_pub->clear();


    // limit URI length to prevent a DoS against the QR-Code dialog
    if (ret.length() > MAX_URI_LENGTH)
    {
        btnSaveAs->setEnabled(false);
        lblQRCode_pub->setText(tr("Resulting URI too long, try to reduce the text for label / message."));
        return QString("");
    }

    btnSaveAs->setEnabled(true);
    return ret;
}


void QRCodeDialog::genCodePriv()
{
    QString uri = getURIPriv();

    if (uri != "")
    {
        lblQRCode_priv->setText("");

        QRcode *code = QRcode_encodeString(uri.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        if (!code)
        {
            lblQRCode_priv->setText(tr("Error encoding URI into QR Code."));
            return;
        }
        myImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
        myImage.fill(0xffffff);
        unsigned char *p = code->data;
        for (int y = 0; y < code->width; y++)
        {
            for (int x = 0; x < code->width; x++)
            {
                myImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
                p++;
            }
        }
        QRcode_free(code);

        lblQRCode_priv->setPixmap(QPixmap::fromImage(myImage).scaled(225, 225));

        outUri_priv->setText(uri);
    }
}

QString QRCodeDialog::getURIPriv()
{
    QString ret = QString("zoin:%1").arg(priv);
    int paramCount = 0;

    outUri_priv->clear();


    // limit URI length to prevent a DoS against the QR-Code dialog
    if (ret.length() > MAX_URI_LENGTH)
    {
        btnSaveAs->setEnabled(false);
        lblQRCode_priv->setText(tr("Resulting URI too long, try to reduce the text for label / message."));
        return QString("");
    }

    btnSaveAs->setEnabled(true);
    return ret;
}

void QRCodeDialog::on_lnReqAmount_textChanged()
{
    genCodePub();
}

void QRCodeDialog::on_lnLabel_textChanged()
{
    genCodePub();
}

void QRCodeDialog::on_lnMessage_textChanged()
{
    genCodePub();
}

void QRCodeDialog::on_btnSaveAs_clicked()
{

    btnSaveAs->hide();
    QString fn = GUIUtil::getSaveFileName(this, tr("Save QR Code"), QString(), tr("PNG Images (*.png)"), NULL);
    if (!fn.isEmpty())
        //myImage.scaled(QR_IMAGE_SIZE, QR_IMAGE_SIZE).save(fn);
        this->grab().save(fn);
    btnSaveAs->show();
}

void QRCodeDialog::on_chkReqPayment_toggled(bool fChecked)
{
    genCodePub();
}

void QRCodeDialog::updateDisplayUnit()
{

}
