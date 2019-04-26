// Copyright (c) 2018 The Noir Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "menupage.h"
#include "ui_menupage.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "bitcoingui.h"

#include "overviewpage.h"

#include "zerocoin_params.h"
#include "clientmodel.h"

#include "guiutil.h"
#include "guiconstants.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>

#define DECORATION_SIZE 64
#define NUM_ITEMS 3

MenuPage::MenuPage(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::MenuPage),
  clientModel(0),
  walletModel(0)
{
   ui->setupUi(this);
   ui->Noirnode->setEnabled(true);

   ui->Zerocoin->setEnabled(false);

   connect(ui->Overview, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Send, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Receive, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Zerocoin, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Transactions, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Address, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Community, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->LearnMore, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Noirnode, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Voting, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   ui->Overview->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
   ui->Send->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
   ui->Receive->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
   ui->Zerocoin->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
   ui->Transactions->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
   ui->Address->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
   ui->Community ->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
   ui->Noirnode ->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
   ui->Voting ->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");

}

MenuPage::~MenuPage()
{
    delete ui;
}

void MenuPage::ClickedItem(){

    ui->Overview->setCheckable(false);
    ui->Send->setCheckable(false);
    ui->Receive->setCheckable(false);
    ui->Zerocoin->setCheckable(false);
    ui->Transactions->setCheckable(false);
    ui->Address->setCheckable(false);
    ui->Community->setCheckable(false);
    ui->Noirnode->setCheckable(false);
    ui->Voting->setCheckable(false);

    ui->Overview->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Send->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Receive->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Zerocoin->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Transactions->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Address->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Community ->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Noirnode ->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Voting ->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");


    int screen = 0;
    QObject *sender = QObject::sender();

    if(sender == ui->Overview)
        screen = 0;
    if(sender == ui->Send)
        screen = 1;
    if(sender == ui->Receive)
        screen = 2;
    if(sender == ui->Zerocoin)
        screen = 3;
    if(sender == ui->Transactions)
        screen = 4;
    if(sender == ui->Address)
        screen = 5;
    if(sender == ui->Community)
        screen = 6;
    if(sender == ui->LearnMore)
        screen = 7;
    if(sender == ui->Noirnode)
        screen = 8;
    if(sender == ui->Voting)
        screen = 9;

    switch(screen){
    case 0:
        ui->Overview->setCheckable(true);
        ui->Overview->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 1:
        ui->Send->setCheckable(true);
        ui->Send->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 2:
        ui->Receive->setCheckable(true);
        ui->Receive->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 3:
        ui->Zerocoin->setCheckable(true);
        ui->Zerocoin->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 4:
        ui->Transactions->setCheckable(true);
        ui->Transactions->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 5:
        ui->Address->setCheckable(true);
        ui->Address->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 6:
        ui->Community->setCheckable(true);
        ui->Community->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 7:
        ui->LearnMore->setCheckable(true);
        break;
    case 8:
        ui->Noirnode->setCheckable(true);
        ui->Noirnode->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 9:
        ui->Voting->setCheckable(true);
        ui->Voting->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    default:
        break;

    }
}

void MenuPage::LinkMenu(BitcoinGUI *gui){

    //connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(ui->Overview, SIGNAL(pressed()), gui, SLOT(gotoOverviewPage()));
    connect(ui->Send, SIGNAL(pressed()), gui, SLOT(gotoSendCoinsPage()));
    connect(ui->Receive, SIGNAL(pressed()), gui, SLOT(gotoReceiveCoinsPage()));
    connect(ui->Zerocoin, SIGNAL(pressed()), gui, SLOT(gotoZerocoinPage()));
    connect(ui->Transactions, SIGNAL(pressed()), gui, SLOT(gotoHistoryPage()));
    connect(ui->Address, SIGNAL(pressed()), gui, SLOT(gotoAddressBookPage()));
    connect(ui->Community, SIGNAL(pressed()), gui, SLOT(gotoCommunityPage()));
    connect(ui->Noirnode, SIGNAL(pressed()), gui, SLOT(gotoNoirnodePage()));
    connect(ui->Voting, SIGNAL(pressed()), gui, SLOT(gotoVotingPage()));
    connect(ui->LearnMore, SIGNAL(pressed()), gui, SLOT(gotoLearnMorePage()));

}

void MenuPage::ClickedItemNonSlot(int s){

    ui->Overview->setCheckable(false);
    ui->Send->setCheckable(false);
    ui->Receive->setCheckable(false);
    ui->Zerocoin->setCheckable(false);
    ui->Transactions->setCheckable(false);
    ui->Address->setCheckable(false);
    ui->Community->setCheckable(false);
    ui->Noirnode->setCheckable(false);
    ui->Voting->setCheckable(false);

    ui->Overview->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Send->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Receive->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Zerocoin->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Transactions->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Address->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Community ->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Noirnode ->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    ui->Voting ->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");


    switch(s){
    case 0:
        ui->Overview->setCheckable(true);
        ui->Overview->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 1:
        ui->Send->setCheckable(true);
        ui->Send->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 2:
        ui->Receive->setCheckable(true);
        ui->Receive->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 3:
        ui->Zerocoin->setCheckable(true);
        ui->Zerocoin->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 4:
        ui->Transactions->setCheckable(true);
        ui->Transactions->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 5:
        ui->Address->setCheckable(true);
        ui->Address->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 6:
        ui->Community->setCheckable(true);
        ui->Community->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 7:
        ui->LearnMore->setCheckable(true);
        break;
    case 8:
        ui->Noirnode->setCheckable(true);
        ui->Noirnode->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    case 9:
        ui->Voting->setCheckable(true);
        ui->Voting->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left;");
        break;
    default:
        break;

    }
}


void MenuPage::SimulateOverviewClick(){
    ClickedItemNonSlot(0);
}
void MenuPage::SimulateSendClick(){
    ClickedItemNonSlot(1);
}
void MenuPage::SimulateReceiveClick(){
    ClickedItemNonSlot(2);
}
void MenuPage::SimulateZerocoinClick(){
    ClickedItemNonSlot(3);
}
void MenuPage::SimulateTransactionsClick(){
    ClickedItemNonSlot(4);
}
void MenuPage::SimulateAddressClick(){
    ClickedItemNonSlot(5);
}
void MenuPage::SimulateCommunityClick(){
    ClickedItemNonSlot(6);
}
void MenuPage::SimulateLearnMoreClick(){
    ClickedItemNonSlot(7);
}
void MenuPage::SimulateNoirnodeClick(){
    ClickedItemNonSlot(8);
}
void MenuPage::SimulateVotingClick(){
    ClickedItemNonSlot(9);
}




