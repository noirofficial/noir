// Copyright (c) 2017 The Zoin Developers
// Created by Matthew Tawil
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "learnmorepage.h"
#include "ui_learnmorepage.h"
#include "clientmodel.h"
#include "walletmodel.h"

#include "guiutil.h"
#include "guiconstants.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QDesktopServices>
#include <QUrl>
#include <QWidget>

LearnMorePage::LearnMorePage(QWidget *parent): QWidget(parent), ui(new Ui::LearnMorePage) { ui->setupUi(this); }
LearnMorePage::~LearnMorePage() {delete ui;}
