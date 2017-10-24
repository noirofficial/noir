// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "splashscreen.h"
#include "clientversion.h"
#include "util.h"

#include <QPainter>
#undef loop /* ugh, remove this when the #define loop is gone from util.h */
#include <QApplication>
#include <QFontDatabase>

SplashScreen::SplashScreen(const QPixmap &pixmap, Qt::WindowFlags f) :
    QSplashScreen(pixmap, f)
{
    // set reference point, paddings
    int paddingTopCol2          = 180;
    int line1 = 0;
    int line2 = 13;
    int line3 = 26;
    int line4 = 39;
    int line5 = -26;

    float fontFactor            = 1.3;

    // define text to place
    QString titleText       = QString(QApplication::applicationName()).replace(QString("-testnet"), QString(""), Qt::CaseSensitive); // cut of testnet, place it as single object further down
    QString versionText     = QString("Version %1 ").arg(QString::fromStdString(FormatFullVersion()));
    QString copyrightText1   = QChar(0xA9)+QString(" %1 ").arg(COPYRIGHT_YEAR) + QString(tr("The Zoin developers"));
    QString copyrightText2   = QChar(0xA9)+QString(" 2016 ") + QString(tr("The ZCoin developers"));
    QString copyrightText3   = QChar(0xA9)+QString(" 2011-2014 ") + QString(tr("The Zerocoin developers"));
    QString copyrightText4   = QChar(0xA9)+QString(" 2009-2014 ") + QString(tr("The Bitcoin developers"));

    //int id = QFontDatabase::addApplicationFont(":/fonts/SourceSansPro-Regular");
    //QString font = QFontDatabase::applicationFontFamilies(id).at(0);

    QString font            = "Arial";

    // load the bitmap for writing some text over it
    QPixmap newPixmap;
    if(GetBoolArg("-testnet")) {
        newPixmap     = QPixmap(":/images/splash_testnet");
    }
    else {
        newPixmap     = QPixmap(":/images/splash");
    }

    QPainter pixPaint(&newPixmap);
    pixPaint.setPen(QColor(0,0,0));

    pixPaint.setFont(QFont(font, 9*fontFactor));
    pixPaint.drawText(QRect(0, paddingTopCol2+line5, 358, 299), Qt::AlignHCenter,versionText);

    // draw copyright stuff
    pixPaint.setFont(QFont(font, 9*fontFactor));

    pixPaint.drawText(QRect(0, paddingTopCol2+line1, 358, 299 ), Qt::AlignHCenter, copyrightText1);
    pixPaint.drawText(QRect(0, paddingTopCol2+line2, 358, 299), Qt::AlignHCenter,copyrightText2);
    pixPaint.drawText(QRect(0, paddingTopCol2+line3, 358, 299), Qt::AlignHCenter,copyrightText3);
    pixPaint.drawText(QRect(0, paddingTopCol2+line4, 358, 299), Qt::AlignHCenter,copyrightText4);
    pixPaint.end();

    this->setPixmap(newPixmap);


}
