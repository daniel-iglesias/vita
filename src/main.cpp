/***************************************************************************
 *   Copyright (C) 2007 by Daniel Iglesias   *
 *   daniel@extremo   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include <QApplication>
#include <QSplashScreen>
#include <qthread.h>
#include "vitamainwindow.h"


class SleeperThread : public QThread
{
public:
static void sleep(unsigned long secs)
{
QThread::sleep(secs);
}
};

int main(int argc, char *argv[])
{
      Q_INIT_RESOURCE(application);
      QApplication app(argc, argv);
      QSplashScreen* splash = new QSplashScreen(QPixmap(":/images/splash-vita.png"));
      splash->show();
      VitaMainWindow * mw = new VitaMainWindow();
	mw->setWindowIcon(QIcon(":/images/icon-vita.png"));
      SleeperThread::sleep(0.05);
      splash->finish(mw);
      mw->show();
      return app.exec();
}

