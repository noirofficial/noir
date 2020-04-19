// Copyright (c) 2018 The Noir Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "menupage.h"
#include "ui_menupage.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "bitcoingui.h"

#include "overviewpage.h"

#include "sigma_params.h"
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

   ui->Sigma->setEnabled(true);

   connect(ui->Overview, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Send, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Receive, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Sigma, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Transactions, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Address, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Community, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->LearnMore, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Noirnode, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   connect(ui->Voting, SIGNAL(pressed()), this, SLOT(ClickedItem()));
   QSettings settings;

   if(settings.value("Design").toInt() == 0){
       ui->pushButton->setStyleSheet("color: white; height:80px; width:120px; background-color:#ffffff;");
       ui->frame_2->setStyleSheet("border: 0; background-color:#ffffff");
       ui->frame_3->setStyleSheet("background-color:#ffffff");
       ui->Overview->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
       ui->Overview->setChecked(true);
       ui->Send->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
       ui->Receive->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
       ui->Sigma->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
       ui->Transactions->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
       ui->Address->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
       ui->Community ->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
       ui->Noirnode ->setStyleSheet("color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
       ui->Voting ->setStyleSheet("color: rgb(238, 238, 236); border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left;");
    } else if (settings.value("Design").toInt() == 1){
       ui->pushButton->setStyleSheet("color: white; height:80px; width:120px; background-color:#000000;");
       ui->frame_2->setStyleSheet("border: 0; background-color:#000000");
       ui->frame_3->setStyleSheet("background-color:#000000");
       ui->Overview->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
       ui->Overview->setChecked(true);
       ui->Send->setStyleSheet("color: #FFFFFF; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;");
       ui->Receive->setStyleSheet("color: #FFFFFF; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;");
       ui->Sigma->setStyleSheet("color: #FFFFFF; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;");
       ui->Transactions->setStyleSheet("color: #FFFFFF; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;");
       ui->Address->setStyleSheet("color: #FFFFFF; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;");
       ui->Community ->setStyleSheet("color: #FFFFFF; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;");
       ui->Noirnode ->setStyleSheet("color: #FFFFFF; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;");
       ui->Voting ->setStyleSheet("color: rgb(238, 238, 236); border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;");
   }

}

MenuPage::~MenuPage()
{
    delete ui;
}

void MenuPage::ClickedItem(){

    ui->Overview->setCheckable(false);
    ui->Send->setCheckable(false);
    ui->Receive->setCheckable(false);
    ui->Sigma->setCheckable(false);
    ui->Transactions->setCheckable(false);
    ui->Address->setCheckable(false);
    ui->Community->setCheckable(false);
    ui->Noirnode->setCheckable(false);
    ui->Voting->setCheckable(false);

    QSettings settings;

    if(settings.value("Design").toInt() == 0){
        ui->Overview->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Send->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Receive->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Sigma->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Transactions->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Address->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Community ->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Noirnode ->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Voting ->setStyleSheet("QPushButton{color: rgb(238, 238, 236); border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");

        int screen = 0;
        QObject *sender = QObject::sender();

        if(sender == ui->Overview)
            screen = 0;
        if(sender == ui->Send)
            screen = 1;
        if(sender == ui->Receive)
            screen = 2;
        if(sender == ui->Sigma)
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
            ui->Overview->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 1:
            ui->Send->setCheckable(true);
            ui->Send->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 2:
            ui->Receive->setCheckable(true);
            ui->Receive->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 3:
            ui->Sigma->setCheckable(true);
            ui->Sigma->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 4:
            ui->Transactions->setCheckable(true);
            ui->Transactions->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 5:
            ui->Address->setCheckable(true);
            ui->Address->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 6:
            ui->Community->setCheckable(true);
            ui->Community->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 7:
            ui->LearnMore->setCheckable(true);
            break;
        case 8:
            ui->Noirnode->setCheckable(true);
            ui->Noirnode->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 9:
            ui->Voting->setCheckable(true);
            ui->Voting->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        default:
            break;

        }
    } else if (settings.value("Design").toInt() == 1){
        ui->Overview->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Send->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Receive->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Sigma->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Transactions->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Address->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Community ->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Noirnode ->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Voting ->setStyleSheet("QPushButton{color: #0E0E0E; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}"); 

        int screen = 0;
        QObject *sender = QObject::sender();

        if(sender == ui->Overview)
            screen = 0;
        if(sender == ui->Send)
            screen = 1;
        if(sender == ui->Receive)
            screen = 2;
        if(sender == ui->Sigma)
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
            ui->Overview->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 1:
            ui->Send->setCheckable(true);
            ui->Send->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 2:
            ui->Receive->setCheckable(true);
            ui->Receive->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 3:
            ui->Sigma->setCheckable(true);
            ui->Sigma->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 4:
            ui->Transactions->setCheckable(true);
            ui->Transactions->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 5:
            ui->Address->setCheckable(true);
            ui->Address->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 6:
            ui->Community->setCheckable(true);
            ui->Community->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 7:
            ui->LearnMore->setCheckable(true);
            break;
        case 8:
            ui->Noirnode->setCheckable(true);
            ui->Noirnode->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 9:
            ui->Voting->setCheckable(true);
            ui->Voting->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        default:
            break;

        }       
    }


    
}

void MenuPage::LinkMenu(BitcoinGUI *gui){

    //connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(ui->Overview, SIGNAL(pressed()), gui, SLOT(gotoOverviewPage()));
    connect(ui->Send, SIGNAL(pressed()), gui, SLOT(gotoSendCoinsPage()));
    connect(ui->Receive, SIGNAL(pressed()), gui, SLOT(gotoReceiveCoinsPage()));
    connect(ui->Sigma, SIGNAL(pressed()), gui, SLOT(gotoSigmaPage()));
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
    ui->Sigma->setCheckable(false);
    ui->Transactions->setCheckable(false);
    ui->Address->setCheckable(false);
    ui->Community->setCheckable(false);
    ui->Noirnode->setCheckable(false);
    ui->Voting->setCheckable(false);

    QSettings settings;

    if(settings.value("Design").toInt() == 0){
        ui->Overview->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Send->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Receive->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Sigma->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Transactions->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Address->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Community ->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Noirnode ->setStyleSheet("QPushButton{color: #000000; border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");
        ui->Voting ->setStyleSheet("QPushButton{color: rgb(238, 238, 236); border-left :5px solid #FFFFFF;height: 60px;padding-left: 5px; text-align:left; background-color: #FFFFFF;}QPushButton:hover{ background-color: rgb(243, 243, 243); }");

        switch(s){
        case 0:
            ui->Overview->setCheckable(true);
            ui->Overview->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 1:
            ui->Send->setCheckable(true);
            ui->Send->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 2:
            ui->Receive->setCheckable(true);
            ui->Receive->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 3:
            ui->Sigma->setCheckable(true);
            ui->Sigma->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 4:
            ui->Transactions->setCheckable(true);
            ui->Transactions->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 5:
            ui->Address->setCheckable(true);
            ui->Address->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 6:
            ui->Community->setCheckable(true);
            ui->Community->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 7:
            ui->LearnMore->setCheckable(true);
            break;
        case 8:
            ui->Noirnode->setCheckable(true);
            ui->Noirnode->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        case 9:
            ui->Voting->setCheckable(true);
            ui->Voting->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: rgb(238, 238, 236);");
            break;
        default:
            break;

        }
    } else if (settings.value("Design").toInt() == 1){
        ui->Overview->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Send->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Receive->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Sigma->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Transactions->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Address->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Community ->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Noirnode ->setStyleSheet("QPushButton{color: #404040; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");
        ui->Voting ->setStyleSheet("QPushButton{color: #0E0E0E; border-left :5px solid #000000;height: 60px;padding-left: 5px; text-align:left; background-color: #000000;}QPushButton:hover{ background-color: #0E0E0E; border-left :5px solid #0E0E0E;}");  

        switch(s){
        case 0:
            ui->Overview->setCheckable(true);
            ui->Overview->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 1:
            ui->Send->setCheckable(true);
            ui->Send->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 2:
            ui->Receive->setCheckable(true);
            ui->Receive->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 3:
            ui->Sigma->setCheckable(true);
            ui->Sigma->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 4:
            ui->Transactions->setCheckable(true);
            ui->Transactions->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 5:
            ui->Address->setCheckable(true);
            ui->Address->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 6:
            ui->Community->setCheckable(true);
            ui->Community->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 7:
            ui->LearnMore->setCheckable(true);
            break;
        case 8:
            ui->Noirnode->setCheckable(true);
            ui->Noirnode->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        case 9:
            ui->Voting->setCheckable(true);
            ui->Voting->setStyleSheet("color: #480027; border-left :5px solid #480027;height: 60px;padding-left: 5px; text-align:left; background-color: #0F0F0F;");
            break;
        default:
            break;

        }        
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
void MenuPage::SimulateSigmaClick(){
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




