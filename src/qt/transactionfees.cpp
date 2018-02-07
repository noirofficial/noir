// Copyright (c) 2017 The Zoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ui_transactionfees.h"
#include "transactionfees.h"
#include "amount.h"
#include "wallet/wallet.h"
#include "coincontrol.h"
#include "coincontroldialog.h"
#include "sendcoinsdialog.h"
#include "addresstablemodel.h"


#include <QDataWidgetMapper>
#include <QMessageBox>
#include <QDialog>
#include <QGraphicsDropShadowEffect>
#include <QSettings>


TransactionFees::TransactionFees(const PlatformStyle *platformStyle, QWidget *parent) : QDialog(parent),
    ui(new Ui::TransactionFees),
    mapper(0)
{
    ui->setupUi(this);
    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    ui->customFee->setValue(CENT/10);
    ui->checkBoxMinimumFee->setChecked(true);


/*
    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setOffset(0);
    effect->setBlurRadius(20.0);
    ui->frame->setGraphicsEffect(effect);
*/

    QSettings settings;
    if(settings.value("changeAddressEnabled").toBool()){
        ui->customChangeAddress->setChecked(true);
        ui->changeAddress->setText(settings.value("changeAddress").toString());
    }
    else{
        ui->customChangeAddress->setChecked(false);
        ui->changeAddress->setText("");
    }

    ui->labelCoinControlChangeLabel->setText(settings.value("changeAddressLabel").toString());

    connect(ui->customChangeAddress, SIGNAL(stateChanged(int)), this, SLOT(coinControlChangeChecked(int)));
    connect(ui->changeAddress, SIGNAL(textEdited(const QString &)), this, SLOT(coinControlChangeEdited(const QString &)));

    connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(updateGlobalFeeVariables()));
    connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(coinControlUpdateLabels()));
    connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(setMinimumFee()));
    connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(updateFeeSectionControls()));
    connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(updateGlobalFeeVariables()));
    connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(coinControlUpdateLabels()));
    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(buttonBoxClicked()));
    ui->customFee->setSingleStep(CWallet::GetRequiredFee(1000));
    updateFeeSectionControls();
}

TransactionFees::~TransactionFees()
{

    QSettings settings;
    //settings.setValue("fFeeSectionMinimized", fFeeMinimized);
    //settings.setValue("nFeeRadio", true);
    //settings.setValue("nCustomFeeRadio", ui->groupCustomFee->checkedId());
    //settings.setValue("nSmartFeeSliderPosition", ui->sliderSmartFee->value());


    settings.setValue("nTransactionFee", (qint64)ui->customFee->value());
    settings.setValue("fPayOnlyMinFee", ui->checkBoxMinimumFee->isChecked());

    settings.setValue("changeAddressEnabled", ui->customChangeAddress->isChecked());
    if(ui->customChangeAddress->isChecked()){
        settings.setValue("changeAddress", ui->changeAddress->text());
        settings.setValue("changeAddressLabel", ui->labelCoinControlChangeLabel->text());
    }
    else{
        settings.setValue("changeAddress", "");
        settings.setValue("changeAddressLabel", "No change address");
    }


    delete ui;

}


void TransactionFees::updateGlobalFeeVariables()
{

        nTxConfirmTarget = defaultConfirmTarget;

        payTxFee = CFeeRate(ui->customFee->value());

        // if user has selected to set a minimum absolute fee, pass the value to coincontrol
        // set nMinimumTotalFee to 0 in case of user has selected that the fee is per KB
        CoinControlDialog::coinControl->nMinimumTotalFee = ui->radioCustomAtLeast->isChecked() ? ui->customFee->value() : 0;
}

void TransactionFees::setMinimumFee()
{
    ui->radioCustomPerKilobyte->setChecked(true);
    ui->customFee->setValue(CWallet::GetRequiredFee(1000));
}

void TransactionFees::updateFeeSectionControls()
{
//    ui->sliderSmartFee          ->setEnabled(ui->radioSmartFee->isChecked());
//    ui->labelSmartFee           ->setEnabled(ui->radioSmartFee->isChecked());
//    ui->labelSmartFee2          ->setEnabled(ui->radioSmartFee->isChecked());
//    ui->labelSmartFee3          ->setEnabled(ui->radioSmartFee->isChecked());
//    ui->labelFeeEstimation      ->setEnabled(ui->radioSmartFee->isChecked());
//    ui->labelSmartFeeNormal     ->setEnabled(ui->radioSmartFee->isChecked());
//    ui->labelSmartFeeFast       ->setEnabled(ui->radioSmartFee->isChecked());

    ui->checkBoxMinimumFee->setEnabled(true);
    ui->labelMinFeeWarning->setEnabled(true);
    ui->radioCustomPerKilobyte->setEnabled(!ui->checkBoxMinimumFee->isChecked());
    ui->radioCustomAtLeast->setEnabled(!ui->checkBoxMinimumFee->isChecked() && CoinControlDialog::coinControl->HasSelected());
    ui->customFee->setEnabled(!ui->checkBoxMinimumFee->isChecked());

}



// Coin Control: update labels
void TransactionFees::coinControlUpdateLabels()
{


}


// ok button
void TransactionFees::buttonBoxClicked()
{

    done(QDialog::Accepted); // closes the dialog
}

void TransactionFees::setModel(WalletModel *model)
{
    this->model = model;
}


// Coin Control: checkbox custom change address
void TransactionFees::coinControlChangeChecked(int state)
{
    if (state == Qt::Unchecked)
    {
        CoinControlDialog::coinControl->destChange = CNoDestination();
        ui->labelCoinControlChangeLabel->setText("No change address");
        ui->changeAddress->clear();
    }
    else{
        // use this to re-validate an already entered address
        coinControlChangeEdited(ui->changeAddress->text());
        ui->customChangeAddress->setChecked((state == Qt::Checked));
    }
}

// Coin Control: custom change address changed
void TransactionFees::coinControlChangeEdited(const QString& text)
{

    // Default to no change address until verified
    CoinControlDialog::coinControl->destChange = CNoDestination();
    ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");

    CBitcoinAddress addr = CBitcoinAddress(text.toStdString());

    if (text.isEmpty()) // Nothing entered
    {
        ui->labelCoinControlChangeLabel->setText("");
    }
    else if (!addr.IsValid()) // Invalid address
    {
        ui->labelCoinControlChangeLabel->setText(tr("Warning: Invalid Zoin address"));
    }
    else // Valid address
    {
        CKeyID keyid;
        addr.GetKeyID(keyid);
        if (!model->havePrivKey(keyid)) // Unknown change address
        {
            ui->labelCoinControlChangeLabel->setText(tr("Warning: Unknown change address"));
        }
        else // Known change address
        {
            ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:black;}");

            // Query label
            QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
            if (!associatedLabel.isEmpty())
                ui->labelCoinControlChangeLabel->setText(associatedLabel);
            else
                ui->labelCoinControlChangeLabel->setText(tr("(no label)"));

            CoinControlDialog::coinControl->destChange = addr.Get();
        }

    }
}
/*
void EditAddressDialog::setModel(AddressTableModel *model)
{
    this->model = model;
    if(!model)
        return;

    mapper->setModel(model);
    mapper->addMapping(ui->labelEdit, AddressTableModel::Label);
    mapper->addMapping(ui->addressEdit, AddressTableModel::Address);
}

void EditAddressDialog::loadRow(int row)
{
    mapper->setCurrentIndex(row);
}

bool EditAddressDialog::saveCurrentRow()
{
    if(!model)
        return false;

    switch(mode)
    {
    case NewReceivingAddress:
    case NewSendingAddress:
        address = model->addRow(
                mode == NewSendingAddress ? AddressTableModel::Send : AddressTableModel::Receive,
                ui->labelEdit->text(),
                ui->addressEdit->text());
        break;
    case EditReceivingAddress:
    case EditSendingAddress:
        if(mapper->submit())
        {
            address = ui->addressEdit->text();
        }
        break;
    }
    return !address.isEmpty();
}

void EditAddressDialog::accept()
{
    if(!model)
        return;

    if(!saveCurrentRow())
    {
        switch(model->getEditStatus())
        {
        case AddressTableModel::OK:
            // Failed with unknown reason. Just reject.
            break;
        case AddressTableModel::NO_CHANGES:
            // No changes were made during edit operation. Just reject.
            break;
        case AddressTableModel::INVALID_ADDRESS:
            QMessageBox::warning(this, windowTitle(),
                tr("The entered address \"%1\" is not a valid Zcoin address.").arg(ui->addressEdit->text()),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case AddressTableModel::DUPLICATE_ADDRESS:
            QMessageBox::warning(this, windowTitle(),
                tr("The entered address \"%1\" is already in the address book.").arg(ui->addressEdit->text()),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case AddressTableModel::WALLET_UNLOCK_FAILURE:
            QMessageBox::critical(this, windowTitle(),
                tr("Could not unlock wallet."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case AddressTableModel::KEY_GENERATION_FAILURE:
            QMessageBox::critical(this, windowTitle(),
                tr("New key generation failed."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;

        }
        return;
    }
    QDialog::accept();
}

QString EditAddressDialog::getAddress() const
{
    return address;
}

void EditAddressDialog::setAddress(const QString &address)
{
    this->address = address;
    ui->addressEdit->setText(address);
}
*/
