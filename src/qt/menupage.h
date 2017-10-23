
// Copyright (c) 2017 The Zoin Developers
// Created by Matthew Tawil
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef MENUPAGE_H
#define MENUPAGE_H

#include <QWidget>
#include "guiutil.h"
#include "bitcoingui.h"



namespace Ui {
    class MenuPage;
}

class ClientModel;
class WalletModel;

class MenuPage : public QWidget
{
    Q_OBJECT
public:
    explicit MenuPage(QWidget *parent = nullptr);
    ~MenuPage();
    void LinkMenu(BitcoinGUI *gui);
//private:
    Ui::MenuPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

signals:

public slots:
    void ClickedItem();
};

#endif // MENUPAGE_H




