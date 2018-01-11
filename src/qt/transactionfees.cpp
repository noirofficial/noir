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


#include <QDataWidgetMapper>
#include <QMessageBox>
#include <QDialog>
#include <QGraphicsDropShadowEffect>


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
