// Copyright (c) 2017 The Zoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef TRANSACTIONFEES_H
#define TRANSACTIONFEES_H

#include <QObject>
#include <QWidget>
#include <QDialog>


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
    explicit TransactionFees(QWidget *parent = nullptr);
    ~TransactionFees();

Q_SIGNALS:

public Q_SLOTS:
    void accept();

private:
    bool saveCurrentRow();

    Ui::TransactionFees *ui;
    QDataWidgetMapper *mapper;

};

#endif
