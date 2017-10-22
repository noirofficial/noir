// Copyright (c) 2017 The Zoin Developers
// Created by Matthew Tawil
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "menupage.h"
#include "ui_menu.h"
#include "clientmodel.h"
#include "walletmodel.h"

#include "guiutil.h"
#include "guiconstants.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QGraphicsDropShadowEffect>

#define DECORATION_SIZE 64
#define NUM_ITEMS 3

MenuPage::MenuPage(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::MenuPage),
  clientModel(0),
  walletModel(0)
{

   ui->setupUi(this);

}






MenuPage::~MenuPage()
{
    delete ui;
}

