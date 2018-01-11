// Copyright (c) 2017 The Zoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef TRANSACTIONFEES_H
#define TRANSACTIONFEES_H

#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QPushButton>

class PlatformStyle;

namespace Ui {
    class TransactionFees;
}

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE


class TransactionFees : public QDialog
{
    Q_OBJECT
public:
    explicit TransactionFees(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~TransactionFees();


Q_SIGNALS:

private Q_SLOTS:
    //void accept();
    void updateGlobalFeeVariables();
    void setMinimumFee();
    void updateFeeSectionControls();
    void coinControlUpdateLabels();
    void buttonBoxClicked();

private:
    bool saveCurrentRow();
    Ui::TransactionFees *ui;
    QDataWidgetMapper *mapper;


};

#endif
