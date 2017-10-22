// Copyright (c) 2017 The Zoin Developers
// Created by Matthew Tawil
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "communitypage.h"
#include "ui_communitypage.h"
#include "clientmodel.h"
#include "walletmodel.h"

#include "guiutil.h"
#include "guiconstants.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QGraphicsDropShadowEffect>

CommunityPage::CommunityPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommunityPage),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);
}



CommunityPage::~CommunityPage()
{
    delete ui;
}
