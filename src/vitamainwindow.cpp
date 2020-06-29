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


#include "vitamainwindow.h"

#include <sstream>
#include <iostream>
#include <fstream>
#if defined(_WIN32) || defined(WIN32)
#  include <direct.h>
#  define chdir _chdir
#  define getcwd _getcwd
#else // POSIX
#  include <unistd.h>
#  define TRUE true
#  define FALSE false
#endif

#include "LMX/lmx.h"

#include <QtGui>
#include "QVTKWidget.h"
#include <QSlider>
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QTableWidget>
#include <QMenu>
#include <QHeaderView>
#include <QStatusBar>
#include <QToolBar>
#include <QLabel>
#include <QMenuBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QVBoxLayout>
#include <QPushButton>


#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkTextProperty.h>
#include <vtkCaptionActor2D.h>
#include <vtkMapper.h>

// #include "modeltile6_516.h"
#include "modeltile6_523.h"
// #include "modeltile6_566.h"
#include "modeltile6_104.h"
#include "modeltile6_109.h"
#include "modelexternalmesh.h"

#include "vtkinterface.h"
#include "graphwidget.h"

#include "simulation.h"

#ifdef USE_QCUSTOMPLOT
#include "qcustomplot.h"
#else
#include "plot2D.h"
#endif

VitaMainWindow::VitaMainWindow()
  : theModel(0)
  , ren(0)
  , inputPlot(0)
  , solvePlot(0)
  , environmentType(0)
  , isNBISPICRHLoaded(false)
  , vtkWidget(0)
 {
	vtkObject::GlobalWarningDisplayOff();

//    QString s = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
//    s = QDir(s).absolutePath();
//    cout << "CHANGING DIR TO: " << s.toStdString() << std::endl;
// 	int success=chdir(s.toStdString().c_str());
     int success=chdir("/tmp/");
     this->resize(1400,700);

     setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
     setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
     setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
     setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

     setWindowTitle(tr("VITA - VIrtual Thermal Assessments"));

     ///LMX Calls
     lmx::setMatrixType( 2 );// using gmm DenseMatrix
     lmx::setLinSolverType( 0 );// direct solver

     createActions();
     createMenus();
     createToolBars();
     createStatusBar();
     createDockWindows();

     init();

//      setCentralWidget(vtkWidget);

     setUpVTKInterface();
 }

void VitaMainWindow::init()
{
  vtkWidget = new QVTKWidget;
  QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(0));
  sizePolicy2.setHorizontalStretch(1);
  sizePolicy2.setVerticalStretch(1);
  sizePolicy2.setHeightForWidth(vtkWidget->sizePolicy().hasHeightForWidth());
  vtkWidget->setSizePolicy(sizePolicy2);
  vtkWidget->setMinimumSize(QSize(100, 100));

  createInputPlot();
  createOutputPlot();
  createPowerPlot();
  createSPPlot();


  // Put graphs in the vGraph container
#ifdef USE_QCUSTOMPLOT
  setCentralWidget(inputPlot);
  viewNewGraph(solvePlot);
  viewNewGraph(powerPlot);
  viewNewGraph(SPPlot1);
#endif

  changeModel(0);
}


VitaMainWindow::~VitaMainWindow()
{
  delete [] openAct; openAct=0;
  delete [] refreshAct; refreshAct=0;
  delete [] saveAct; saveAct=0;
  delete [] printAct; printAct=0;
  delete [] solveAct; solveAct=0;
  delete [] quitAct; quitAct=0;
  delete [] aboutAct; aboutAct=0;
  delete [] aboutQtAct; aboutQtAct=0;
  delete [] axisAct; axisAct=0;
  delete [] contoursAct; contoursAct=0;
  delete [] contourType; contourType=0;
  delete [] environmentType; environmentType=0;
  delete [] graphAct; graphAct=0;
  delete [] animateForwAct; animateForwAct=0;
  delete [] animateBackAct; animateBackAct=0;
  delete [] animatePauseAct; animatePauseAct=0;
  delete [] animateRecAct; animateRecAct=0;
  delete [] animateStopAct; animateStopAct=0;
  delete [] animateGroup; animateGroup=0;
  delete [] animateTimeLine; animateTimeLine=0;

  delete [] dock; dock=0;
  delete [] inputTable; inputTable=0;

  delete [] vtkWidget; vtkWidget=0;
  delete [] theInterface; theInterface=0;
  delete [] inputPlot; inputPlot=0;
}

void VitaMainWindow::fileNew()
{
  if (fileOpened){
	fileClose();
	tableselection(0);
  }
    QStringList items;
    items << tr("Tile 6 validation case A")
	  << tr("Tile 6 validation case B")
	  << tr("Tile 6 validation case C")
	  << tr("Tile 6 validation case D")
	  << tr("Tile 6 validation case E")
	  << tr("Tile 6 pulse 87218");

    bool ok;
    QString item = QInputDialog::getItem(this, tr("Select Template"),
                                         tr("Cases:"), items, 0, false, &ok);
    if (ok && !item.isEmpty()){
      if(item == "Tile 6 validation case A"){
	 inputTable->setItem(0,0,new QTableWidgetItem("12")); // NBI
	 inputTable->setItem(1,0,new QTableWidgetItem("5.75")); // ICRH
	 inputTable->setItem(2,0,new QTableWidgetItem("0.61")); // Radiated fraction
	 inputTable->setItem(3,0,new QTableWidgetItem("10")); // Pulse time
	 inputTable->setItem(4,0,new QTableWidgetItem("20")); // Analysis time
	 inputTable->setItem(5,0,new QTableWidgetItem("107")); // ST
	 inputTable->setItem(6,0,new QTableWidgetItem("2.9037")); // SP
	 inputTable->setItem(7,0,new QTableWidgetItem("0.030")); // Width
	 /*inputTable->setItem(8,0,new QTableWidgetItem("500"));*/ // Skew Shape
	 inputTable->setItem(9,0,new QTableWidgetItem("70")); // TWF
	 inputTable->setItem(10,0,new QTableWidgetItem("0")); // Sweep amplitude
	 inputTable->setItem(11,0,new QTableWidgetItem("0")); // Sweep frequency
	 inputTable->setItem(12,0,new QTableWidgetItem("0")); // Film coefficient
	 inputTable->setItem(13,0,new QTableWidgetItem("300")); // Sink Temperature
      }
      else if(item == "Tile 6 validation case B"){
	 inputTable->setItem(0,0,new QTableWidgetItem("12")); // NBI
	 inputTable->setItem(1,0,new QTableWidgetItem("2.43")); // ICRH
	 inputTable->setItem(2,0,new QTableWidgetItem("0.61")); // Radiated fraction
	 inputTable->setItem(3,0,new QTableWidgetItem("6.5")); // Pulse time
	 inputTable->setItem(4,0,new QTableWidgetItem("20")); // Analysis time
	 inputTable->setItem(5,0,new QTableWidgetItem("107")); // ST
	 inputTable->setItem(6,0,new QTableWidgetItem("2.905")); // SP
	 inputTable->setItem(7,0,new QTableWidgetItem("0.030")); // Width
	 /*inputTable->setItem(8,0,new QTableWidgetItem("500"));*/ // Skew Shape
	 inputTable->setItem(9,0,new QTableWidgetItem("100")); // TWF
	 inputTable->setItem(10,0,new QTableWidgetItem("0")); // Sweep amplitude
	 inputTable->setItem(11,0,new QTableWidgetItem("0")); // Sweep frequency
	 inputTable->setItem(12,0,new QTableWidgetItem("0")); // Film coefficient
	 inputTable->setItem(13,0,new QTableWidgetItem("300")); // Sink Temperature
      }
      else if(item == "Tile 6 validation case C"){
	 inputTable->setItem(0,0,new QTableWidgetItem("5")); // NBI
	 inputTable->setItem(1,0,new QTableWidgetItem("2.5")); // ICRH
	 inputTable->setItem(2,0,new QTableWidgetItem("0.61")); // Radiated fraction
	 inputTable->setItem(3,0,new QTableWidgetItem("10")); // Pulse time
	 inputTable->setItem(4,0,new QTableWidgetItem("20")); // Analysis time
	 inputTable->setItem(5,0,new QTableWidgetItem("107")); // ST
	 inputTable->setItem(6,0,new QTableWidgetItem("2.83711")); // SP
	 inputTable->setItem(7,0,new QTableWidgetItem("0.040")); // Width
	 /*inputTable->setItem(8,0,new QTableWidgetItem("500"));*/ // Skew Shape
	 inputTable->setItem(9,0,new QTableWidgetItem("100")); // TWF
	 inputTable->setItem(10,0,new QTableWidgetItem("0")); // Sweep amplitude
	 inputTable->setItem(11,0,new QTableWidgetItem("0")); // Sweep frequency
	 inputTable->setItem(12,0,new QTableWidgetItem("0")); // Film coefficient
	 inputTable->setItem(13,0,new QTableWidgetItem("300")); // Sink Temperature
      }
      else if(item == "Tile 6 validation case D"){
	 inputTable->setItem(0,0,new QTableWidgetItem("12")); // NBI
	 inputTable->setItem(1,0,new QTableWidgetItem("1.69")); // ICRH
	 inputTable->setItem(2,0,new QTableWidgetItem("0.61")); // Radiated fraction
	 inputTable->setItem(3,0,new QTableWidgetItem("6")); // Pulse time
	 inputTable->setItem(4,0,new QTableWidgetItem("20")); // Analysis time
	 inputTable->setItem(5,0,new QTableWidgetItem("107")); // ST
	 inputTable->setItem(6,0,new QTableWidgetItem("2.915")); // SP
	 inputTable->setItem(7,0,new QTableWidgetItem("0.02747")); // Width
//	 inputTable->setItem(8,0,new QTableWidgetItem("500")); // Skew Shape
	 inputTable->setItem(9,0,new QTableWidgetItem("100")); // TWF
	 inputTable->setItem(10,0,new QTableWidgetItem("0.00916")); // Sweep amplitude
	 inputTable->setItem(11,0,new QTableWidgetItem("3")); // Sweep frequency
	 inputTable->setItem(12,0,new QTableWidgetItem("0")); // Film coefficient
	 inputTable->setItem(13,0,new QTableWidgetItem("300")); // Sink Temperature
      }
      else if(item == "Tile 6 validation case E"){
	 inputTable->setItem(0,0,new QTableWidgetItem("12")); // NBI
	 inputTable->setItem(1,0,new QTableWidgetItem("1.69")); // ICRH
	 inputTable->setItem(2,0,new QTableWidgetItem("0.61")); // Radiated fraction
	 inputTable->setItem(3,0,new QTableWidgetItem("6")); // Pulse time
	 inputTable->setItem(4,0,new QTableWidgetItem("20")); // Analysis time
	 inputTable->setItem(5,0,new QTableWidgetItem("107")); // ST
	 inputTable->setItem(6,0,new QTableWidgetItem("2.915")); // SP
	 inputTable->setItem(7,0,new QTableWidgetItem("0.02747")); // Width
	 /*inputTable->setItem(8,0,new QTableWidgetItem("500"));*/ // Skew Shape
	 inputTable->setItem(9,0,new QTableWidgetItem("100")); // TWF
	 inputTable->setItem(10,0,new QTableWidgetItem("0.00916")); // Sweep amplitude
	 inputTable->setItem(11,0,new QTableWidgetItem("4")); // Sweep frequency
	 inputTable->setItem(12,0,new QTableWidgetItem("0")); // Film coefficient
	 inputTable->setItem(13,0,new QTableWidgetItem("300")); // Sink Temperature
      }
      else if(item == "Tile 6 pulse 87218"){
	 tableselection(1);
	 inputTable->setItem(0,0,new QTableWidgetItem("12")); // NBI
	 inputTable->setItem(1,0,new QTableWidgetItem("5.96")); // ICRH
	 inputTable->setItem(2,0,new QTableWidgetItem("0.61")); // Radiated fraction
	 inputTable->setItem(3,0,new QTableWidgetItem("3.5")); // Pulse time
	 inputTable->setItem(4,0,new QTableWidgetItem("10")); // Analysis time
	 inputTable->setItem(5,0,new QTableWidgetItem("107")); // ST
	 inputTable->setItem(6,0,new QTableWidgetItem("2.9037")); // SP
	 inputTable->setItem(7,0,new QTableWidgetItem("0.050")); // Width
	 inputTable->setItem(8,0,new QTableWidgetItem("3")); // Skew Shape
	 inputTable->setItem(9,0,new QTableWidgetItem("100")); // TWF
	 inputTable->setItem(10,0,new QTableWidgetItem("0.015")); // Sweep amplitude
	 inputTable->setItem(11,0,new QTableWidgetItem("4")); // Sweep frequency
	 inputTable->setItem(12,0,new QTableWidgetItem("0")); // Film coefficient
	 inputTable->setItem(13,0,new QTableWidgetItem("300")); // Sink Temperature
      }
    }
    vGraphs[1]->show();
    vGraphs[2]->show();
}


void VitaMainWindow::fileOpen()
{
  if (fileOpened) fileClose();
  fileName = QFileDialog::getOpenFileName( this,
                                             tr("open file dialog"),
                                             "/home/",
                                             tr("MecBryan geometry (*.mec);;All files(*.*)")
                                           );
  if ( !fileName.isEmpty() ){
    load( );
    statusBar()->showMessage( tr("Model loaded"), 2000 );
  }
  else
    statusBar()->showMessage( tr("Loading aborted"), 2000 );

//   if(!axisAct->isChecked())
//     axisAct->toggle();
  fileOpened = true;
}

void VitaMainWindow::load( )
{
  QFile f( fileName );
//   if ( !f.open( QIODevice::ReadOnly ) )
//     return;

  theInterface->readFile( fileName );

  std::vector<QString> tempStrings = theInterface->getResultsNames();
  std::vector<QString>::iterator itString;
  for( itString = tempStrings.begin();
       itString!= tempStrings.end();
       ++itString
     )
  {
    contourType->addItem( *itString );
  }
  contourType->setCurrentIndex(1);
//   contourType->setCurrentIndex(contourType->count()-1);
//   contoursAct->setChecked(TRUE);

  setCentralWidget(vtkWidget);
  createInputPlot();
  updateModel(NULL);
#ifdef USE_QCUSTOMPLOT
  viewNewGraph(inputPlot, true);
#endif

  // Shows the outputTable
   QTableWidgetItem *__qtablewidgetitem8 = new QTableWidgetItem();
  __qtablewidgetitem8->setText(QString::number(theModel->MaxTemp));
  __qtablewidgetitem8->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  outputTable->setItem(0, 0, __qtablewidgetitem8);
  QTableWidgetItem *__qtablewidgetitem9 = new QTableWidgetItem();
  __qtablewidgetitem9->setText(QString::number(theModel->TCxPeak));
  __qtablewidgetitem9->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  outputTable->setItem(1, 0, __qtablewidgetitem9);
  QTableWidgetItem *__qtablewidgetitem10 = new QTableWidgetItem();
  __qtablewidgetitem10->setText(QString::number(theModel->TCyPeak));
  __qtablewidgetitem10->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  outputTable->setItem(2, 0, __qtablewidgetitem10);
  QTableWidgetItem *__qtablewidgetitem11 = new QTableWidgetItem();
  __qtablewidgetitem11->setText(QString::number(theModel->TCzPeak));
  __qtablewidgetitem11->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  outputTable->setItem(3, 0, __qtablewidgetitem11);
    QTableWidgetItem *__qtablewidgetitem16 = new QTableWidgetItem();
  __qtablewidgetitem16->setText(QString::number(theModel->peakPowerDensity/1e6));
  __qtablewidgetitem16->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  outputTable->setItem(9, 0, __qtablewidgetitem16);
    QTableWidgetItem *__qtablewidgetitem62 = new QTableWidgetItem();
  __qtablewidgetitem62->setText(QString::number(theModel->calculateNBIRFEnergy()/1e6));
  __qtablewidgetitem62->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  outputTable->setItem(10, 0, __qtablewidgetitem62);
    QTableWidgetItem *__qtablewidgetitem63 = new QTableWidgetItem();
  __qtablewidgetitem63->setText(QString::number(theModel->calculateInputEnergy()/1e6));
  __qtablewidgetitem63->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  outputTable->setItem(11, 0, __qtablewidgetitem63);

    if (theModel->isTCJPFLoaded()){
	QTableWidgetItem *__qtablewidgetitem13 = new QTableWidgetItem();
	__qtablewidgetitem13->setText(QString::number(-theModel->TCxJPFmaxTemp+theModel->TCxPeak));
	__qtablewidgetitem13->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	outputTable->setItem(4, 0, __qtablewidgetitem13);
	QTableWidgetItem *__qtablewidgetitem14 = new QTableWidgetItem();
	__qtablewidgetitem14->setText(QString::number(-theModel->TCyJPFmaxTemp+theModel->TCyPeak));
	__qtablewidgetitem14->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	outputTable->setItem(5, 0, __qtablewidgetitem14);
	QTableWidgetItem *__qtablewidgetitem15 = new QTableWidgetItem();
	__qtablewidgetitem15->setText(QString::number(-theModel->TCzJPFmaxTemp+theModel->TCzPeak));
	__qtablewidgetitem15->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	outputTable->setItem(6, 0, __qtablewidgetitem15);

	outputTable->showRow(4);
	outputTable->showRow(5);
	outputTable->showRow(6);


  }else{
	outputTable->hideRow(4);
	outputTable->hideRow(5);
	outputTable->hideRow(6);
  }
   QTableWidgetItem *__qtablewidgetitem12 = new QTableWidgetItem();
  __qtablewidgetitem12->setText(QString::number(theModel->Energy/1e6));
  __qtablewidgetitem12->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  outputTable->setItem(7, 0, __qtablewidgetitem12);

     QTableWidgetItem *__qtablewidgetitem13 = new QTableWidgetItem();
  __qtablewidgetitem13->setText("Clic to choose a pulse");
  __qtablewidgetitem13->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  outputTable->setItem(8, 0, __qtablewidgetitem13);
  dock->show();


  tempStrings.clear();
  tempStrings = theInterface->getEnvironmentsNames();
  for( itString = tempStrings.begin();
      itString!= tempStrings.end();
      ++itString
      )
  {
    environmentType->addItem( *itString );
  }

  //   renLittle = ren;
     animateTimeLine->setMaximum( theInterface->getNumberOfSteps()-1 );
     animateTimeLine->setPageStep( theInterface->getNumberOfSteps()/10 );
     animateTimeLine->setValue(0);
     animateTimeLine->setSliderPosition(0);
     animationMenu->setEnabled(true);
     animationToolBar->setVisible(true);
     animationToolBar->setEnabled(true);

    solveAct->setEnabled(false);
}

void VitaMainWindow::setUpVTKInterface()
 {
   if(!ren) {
     ren = vtkRenderer::New();
   }
//    ren->SetBackground( 0.1, 0.2, 0.4 ); // dark blue
//  theRenderer->SetBackground( 1, 1, 1); // white
   ren->GradientBackgroundOn();
   ren->SetBackground(0.99,0.85,0.62); // light yellow
   ren->SetBackground2(0.99,0.93,0.83); // yellow
//   ren->SetBackground(0.41,0.56,1.0); // light blue
//   ren->SetBackground2(0.8,0.86,1); // blue
   theInterface = new VTKInterface( ren );
   vtkWidget->GetRenderWindow()->AddRenderer( ren );
 }

void VitaMainWindow::fileClose()
{

  viewContour(0);
  contoursAct->setChecked(false);
  std::vector<QString> tempStrings = theInterface->getResultsNames();
  contourType->setCurrentIndex(0);
  for( int i=tempStrings.size(); i>0; --i ){
    contourType->removeItem( i );
  }
  tempStrings.clear();
  tempStrings = theInterface->getResultsNames();
  if(environmentType){
    environmentType->setCurrentIndex(0);
    for( int i=tempStrings.size(); i>0; --i ){
      environmentType->removeItem( i );
    }
  }
  delete inputPlot; inputPlot=0;
  delete solvePlot; solvePlot=0;
  delete powerPlot; powerPlot=0;
  delete SPPlot1; SPPlot1=0;

  for(int i=0; i<vGraphs.size(); ++i){
    vGraphs[i]->hide();
    delete vGraphs[i];
    vGraphs[i]=0;
  }
  vGraphs.clear();

  if(vtkWidget){
    delete vtkWidget;
    vtkWidget=0;
  }
    init();
    theModel->shot = 0;

  solveAct->setEnabled(true);

  if(theInterface){
    delete theInterface;
    theInterface=0;
    setUpVTKInterface();
  }
  if(axisAct->isChecked())
    axisAct->toggle();

   if (theModel) changeModel(0);


  viewToggleContours();
  theInterface->closeFile();
  ren->Clear();
  ren->GetRenderWindow()->Render();
  fileOpened = false;
  animationMenu->setEnabled(false);
  animationToolBar->setEnabled(false);
  animationToolBar->setVisible(false);
  modelType->setCurrentIndex(0);
  modelDist->setCurrentIndex(0);
  dock->hide();
}

void VitaMainWindow::fileRefresh()
{
  if (fileOpened){
    fileClose();
    if ( !fileName.isEmpty() ){
      load( );
      statusBar()->showMessage( tr("Model refreshed"), 2000 );
    }
  }
  fileOpened = true;
}

 void VitaMainWindow::filePrint()
 {
     statusBar()->showMessage(tr("Ready"), 2000);
 }

 void VitaMainWindow::fileSave()
 {
     QString fileName = QFileDialog::getSaveFileName(this,
                         tr("Choose a file name"), "/home/",
                         tr("CSV files (*.csv)"));
     if (fileName.isEmpty())
         return;
     QFile file(fileName);
     if (!file.open(QFile::WriteOnly | QFile::Text)) {
         QMessageBox::warning(this, tr("Dock Widgets"),
                              tr("Cannot write file %1:\n%2.")
                              .arg(fileName)
                              .arg(file.errorString()));
         return;
     }
     theModel->writeHeatFlux(fileName.toStdString());
     QApplication::setOverrideCursor(Qt::WaitCursor);
     QApplication::restoreOverrideCursor();

     statusBar()->showMessage(tr("Saved '%1'").arg(fileName), 2000);
 }

 void VitaMainWindow::editSolve()
 {
	  solve();
 }

 void VitaMainWindow::helpAbout()
 {
    QMessageBox::about(this, tr("VITA v0.1"),
             tr("VITA is the GUI tool for interactive thermal assessments. It uses the  MkniX multibody simulation package as solver.\n\n"
                "Version 0.1 \n\n"
                "Authors: \nDaniel Iglesias\nPatrick Bunting \nSergio Esquembri"));
 }

 void VitaMainWindow::solve(){
	theModel->peakPowerDensity = 0;
	QProgressDialog progress("MkniX is solving your system...", "Abort solution", 0, theModel->pulseIterations+theModel->cooldownIterations, this);
	progress.setWindowModality(Qt::WindowModal);
#ifdef USE_QCUSTOMPLOT
	theModel->solve(progress,inputPlot,solvePlot,powerPlot,SPPlot1);
#else
	theModel->solve(progress);
#endif
	if(progress.wasCanceled()){
		printf("ABORTED\n");
		statusBar()->showMessage( tr("Execution aborted"), 2000 );
		fileClose();
	}else{
		printf("FINISHED\n");
		fileName=theModel->getOutputFilePath();
		load();
		fileOpened = true;
	}


 }

void VitaMainWindow::defaultInputTable(){
	QTableWidgetItem *__qtablewidgetitemE1 = new QTableWidgetItem();
	QTableWidgetItem *__qtablewidgetitemE2 = new QTableWidgetItem();
	QTableWidgetItem *__qtablewidgetitemE3 = new QTableWidgetItem();
	QTableWidgetItem *__qtablewidgetitemE4 = new QTableWidgetItem();
	QTableWidgetItem *__qtablewidgetitemE5 = new QTableWidgetItem();
	__qtablewidgetitemE1->setText(QString::fromUtf8("12"));
	__qtablewidgetitemE2->setText(QString::fromUtf8("5.75"));
	__qtablewidgetitemE3->setText(QString::fromUtf8("0.61"));
	__qtablewidgetitemE4->setText(QString::fromUtf8("10"));
	__qtablewidgetitemE5->setText(QString::fromUtf8("2.9037"));
	inputTable->setItem(0, 0, __qtablewidgetitemE1);
	inputTable->setItem(1, 0, __qtablewidgetitemE2);
	inputTable->setItem(2, 0, __qtablewidgetitemE3);
	inputTable->setItem(3, 0, __qtablewidgetitemE4);
	inputTable->setItem(6, 0, __qtablewidgetitemE5);

	updateModel(NULL);
}

void VitaMainWindow::OptionSimAccept(){
	bool ok=true;
	if (theModel->shot == 0) theModel -> shot = QInputDialog::getInt(this, tr("Introduce JET Pulse Number"),"JPN:",0,0,2147483647,1,&ok);
	if (!ok) dlg->reject();

	if (theModel->isFlushAvailable(theModel->shot)){
		switch(flushBox->checkState()){
		  case 0:
			theModel->useFlush = false;
			break;

		  case 2:
			    QTableWidgetItem *__qtablewidgetitemWF = new QTableWidgetItem();
			    __qtablewidgetitemWF->setText(QString::fromUtf8("Loaded"));
			    __qtablewidgetitemWF->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
			    inputTable->setItem(9, 0, __qtablewidgetitemWF);
			    theModel->WF = 100;
			    theModel->useFlush = true;
			break;
		}
		switch(TOPIBox->checkState()){
		  case 0:
			theModel->useTOPI = false;
			inputTable->showRow(2);
			break;
		  case 2:
			theModel->loadTOPI();
			if (theModel->useTOPI){
			      QTableWidgetItem *__qtablewidgetitemE1 = new QTableWidgetItem();
			    __qtablewidgetitemE1->setText(QString::fromUtf8("Loaded"));
			    __qtablewidgetitemE1->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
			    inputTable->setItem(2, 0, __qtablewidgetitemE1);
			}else TOPIBox->setCheckState(Qt::Unchecked);
			break;
		}
		switch(shadowBox->checkState()){
		  case 0:
			theModel->useShadow = false;
			break;
		  case 2:
			theModel->useShadow = true;
			break;
		}

		dlg->accept();
	}else{
		dlg->reject();
		theModel->shot = 0;
		statusBar()->showMessage( tr("Error calling flush"), 2000 );
		QMessageBox::warning(this,tr("flush init"),tr("Can't initialize flush with the specified pulse"));
	}

}

void VitaMainWindow::optionSim(){
	dlg = new QDialog();
	QVBoxLayout *layout = new QVBoxLayout;

	flushBox = new QCheckBox("Magnetic projection",this);
	TOPIBox = new QCheckBox("BOLO/TOPI", this);
	shadowBox = new QCheckBox("compute Shadow of T5 and T7", this);
	if (theModel->useFlush) flushBox->setCheckState(Qt::Checked);
	if (theModel->useTOPI) TOPIBox->setCheckState(Qt::Checked);
	if (theModel->useShadow) shadowBox->setCheckState(Qt::Checked);

	QPushButton *button1 = new QPushButton("Ok", this);
	button1->setDefault(true);
	connect(button1, SIGNAL(clicked()), this, SLOT(OptionSimAccept()));
	if (modelDist->currentIndex() == 2) layout->addWidget(flushBox);
	layout->addWidget(TOPIBox);
	layout->addWidget(shadowBox);
	layout->addWidget(button1);

	dlg->setLayout(layout);
	dlg->show();
}


 void VitaMainWindow::changeModel(int modelIndex ){
    int index;

    index = modelDist->currentIndex();
	///@TODO WARNING!!! This will erase all files loaded!!
   if(!theModel){
     delete [] theModel;
     theModel=0;
   }
   switch(modelIndex){
     case 0:
        theModel = new ModelTile6_109();
       break;
     case 1:
//        theModel = new ModelTile6_516();
       theModel = new ModelTile6_523();
//        theModel = new ModelTile6_566();
       break;
     case 2:
       theModel = new ModelTile6_104();
       break;
     case 3:
		 QString fileName1 = QFileDialog::getOpenFileName(this,
			 tr("open file dialog"),
			 "/home/",
			 tr("All files(*.*)")
		 );
		 QString fileName2 = QFileDialog::getOpenFileName(this,
			 tr("open file dialog"),
			 "/home/",
			 tr("All files(*.*)")
		 );
		 if (!fileName1.isEmpty() && !fileName2.isEmpty()) {
			 theModel = new ModelExternalMesh(fileName1.toStdString(), fileName2.toStdString());
			 statusBar()->showMessage(tr("External mesh files loaded"), 2000);
		 }
		 else {
			 statusBar()->showMessage(tr("External mesh files aborted"), 2000);
		 }

       break;
   }

   changeDist(index);
   theModel->init();
   defaultInputTable();



 }

 void VitaMainWindow::changeDist(int distIndex ){
	 theModel->useFlush = false;
	 theModel->useShadow = false;
	 switch(distIndex){
	 case 0:
		 theModel->dist=Triangle_Dist;
		 break;
	 case 1:
		 theModel->dist=Skew_Normal_Dist;
	 	 break;
	 case 2:
		 theModel->dist=Eich_Dist;
		 modelParams->setCurrentIndex(0);
	 	 break;
	 }
    for(int i=1; i<3; ++i){
    if (vGraphs.size() > i)
	 vGraphs[i]->show();
    }

	 updateModel(NULL);
 }

 void VitaMainWindow::ParamsEstim(int EstimIndex)
 {
    double Bt, Ip, ne, NBI, ICRH, fELM, lambda_q, S;
    bool ok;


    QTableWidgetItem *__qtablewidgetitemE1 = new QTableWidgetItem();
    QTableWidgetItem *__qtablewidgetitemE2 = new QTableWidgetItem();
    QTableWidgetItem *__qtablewidgetitemE3 = new QTableWidgetItem();

    switch(EstimIndex){
     case 0:
	    theModel->AutoLambdaS = false;
	    theModel->newScaling = false;
	    __qtablewidgetitemE1->setText(QString::fromUtf8("0.01"));
	    inputTable->setItem(7, 0, __qtablewidgetitemE1);

	    __qtablewidgetitemE3->setText(QString::fromUtf8("0.005"));
	    inputTable->setItem(8, 0, __qtablewidgetitemE3);

	    break;

     case 1:
	    theModel->newScaling = false;
	    theModel->AutoLambdaS = false;
	    Bt = QInputDialog::getDouble(this, tr("introduce Bt"), "Bt (T):",0,0,2147483647,2,&ok);
	    if (ok) Ip = QInputDialog::getDouble(this, tr("introduce Ip"), "Ip (MA):",0,0,2147483647,2,&ok);
	    if (ok) ne = QInputDialog::getDouble(this, tr("introduce ne"), "ne (10^20 m^-2):",0,0,2147483647,3,&ok);
	    if (ok) NBI = QInputDialog::getDouble(this, tr("introduce NBI"), "NBI (MW):",0,0,2147483647,2,&ok);
	    if (ok) ICRH = QInputDialog::getDouble(this, tr("introduce ICRH"), "ICRH (MW):",0,0,2147483647,2,&ok);
	    if (ok) fELM = QInputDialog::getDouble(this, tr("introduce fELM"), "fELM (Hz):",0,0,2147483647,2,&ok);

	    if (ok) {
		    lambda_q =1.6*pow(Ip,-0.24)*pow(Bt, 0.52)*pow(ne, -1)*pow((NBI+ICRH)*(1-theModel->RF),0.023)*pow(fELM,0.15);
		    S =1.6*pow(Ip,0.74)*pow(Bt, -0.83)*pow(ne, -0.6)*pow((NBI+ICRH)*(1-theModel->RF),0.052)*pow(fELM,-0.11);

		    if (lambda_q   && S ){
			__qtablewidgetitemE1->setText(QString::number(lambda_q/1000));
			__qtablewidgetitemE1->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
			inputTable->setItem(7, 0, __qtablewidgetitemE1);

			__qtablewidgetitemE3->setText(QString::number(S/1000));
			__qtablewidgetitemE3->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
			inputTable->setItem(8, 0, __qtablewidgetitemE3);
		    }else{
			__qtablewidgetitemE1->setText(QString::fromUtf8("0.01"));
			__qtablewidgetitemE1->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
			inputTable->setItem(7, 0, __qtablewidgetitemE1);

			__qtablewidgetitemE3->setText(QString::fromUtf8("0.005"));
			__qtablewidgetitemE3->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
			inputTable->setItem(8, 0, __qtablewidgetitemE3);
			modelParams->setCurrentIndex(0);
		    }
	    }else{
		modelParams->setCurrentIndex(0);
	    }


	    break;
     case 2:
// 	    QString message = "Not implemented yet";
// 	    QMessageBox::information(this, "Error", message);
// 	    modelParams->setCurrentIndex(0);
	    if (theModel->shot == 0) theModel->shot = QInputDialog::getInt(this, tr("Introduce JET Pulse Number"),"JPN:",0,0,2147483647,1,&ok);
	    theModel->newScaling = false;
	   if (ok||theModel->shot != 0)
	     theModel->calculateLS();
	   else{
		modelParams->setCurrentIndex(0);
		theModel ->shot = 0;
		break;
	   }
	   if (theModel->AutoLambdaS){
		statusBar()->showMessage( tr("lambda_q & S loaded"), 2000 );
		__qtablewidgetitemE1->setText("Loaded");
		__qtablewidgetitemE1->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		inputTable->setItem(7, 0, __qtablewidgetitemE1);


		__qtablewidgetitemE3->setText("Loaded");
		__qtablewidgetitemE3->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		inputTable->setItem(8, 0, __qtablewidgetitemE3);


		__qtablewidgetitemE2->setText("Loaded");
		__qtablewidgetitemE2->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		inputTable->setItem(3, 0, __qtablewidgetitemE2);
	    }else{
		modelParams->setCurrentIndex(0);
		QMessageBox::warning(this,tr("NBI load"),tr("Can't load lambda_q and S from PPF"));
		statusBar()->showMessage( tr("Problem loading lambda_q & S"), 2000 );
	    }
	    break;
      case 3:
// 	    QString message = "Not implemented yet";
// 	    QMessageBox::information(this, "Error", message);
// 	    modelParams->setCurrentIndex(0);
	    if (theModel->shot == 0) theModel->shot = QInputDialog::getInt(this, tr("Introduce JET Pulse Number"),"JPN:",0,0,2147483647,1,&ok);
	    theModel->newScaling = true;
	   if (ok||theModel->shot != 0)
	     theModel->calculateLS();
	   else{
		modelParams->setCurrentIndex(0);
		theModel ->shot = 0;
		break;
	   }
	   if (theModel->AutoLambdaS){
		statusBar()->showMessage( tr("lambda_q & S loaded"), 2000 );
		__qtablewidgetitemE1->setText("Loaded");
		__qtablewidgetitemE1->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		inputTable->setItem(7, 0, __qtablewidgetitemE1);


		__qtablewidgetitemE3->setText("Loaded");
		__qtablewidgetitemE3->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		inputTable->setItem(8, 0, __qtablewidgetitemE3);


		__qtablewidgetitemE2->setText("Loaded");
		__qtablewidgetitemE2->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		inputTable->setItem(3, 0, __qtablewidgetitemE2);
	    }else{
		theModel->newScaling = false;
		modelParams->setCurrentIndex(0);
		QMessageBox::warning(this,tr("NBI load"),tr("Can't load lambda_q and S from PPF"));
		statusBar()->showMessage( tr("Problem loading lambda_q & S"), 2000 );
	    }
	    break;
   }

   updateModel( inputTable->item(7,0));
   updateModel( inputTable->item(8,0));
 }

 void VitaMainWindow::tableselection(int distIndex)
 {


    QTableWidgetItem *__qtablewidgetitemE1 = new QTableWidgetItem();
    QTableWidgetItem *__qtablewidgetitemE3 = new QTableWidgetItem();
    QTableWidgetItem *__qtablewidgetitemE4 = new QTableWidgetItem();
    QTableWidgetItem *__qtablewidgetitemE2 = new QTableWidgetItem();
    QTableWidgetItem *__qtablewidgetitemE5 = new QTableWidgetItem();
    QTableWidgetItem *__qtablewidgetitemE6 = new QTableWidgetItem();


    switch(distIndex){
      case(0):
	       __qtablewidgetitemE1->setText(QString::fromUtf8("WIDTH (m)"));
	      inputTable->setVerticalHeaderItem(7, __qtablewidgetitemE1);

	      __qtablewidgetitemE3->setText(QString::fromUtf8("0.03"));
	      inputTable->setItem(7, 0, __qtablewidgetitemE3);

	      __qtablewidgetitemE4->setText(QString::fromUtf8("0.03-0.05 m"));
	      __qtablewidgetitemE4->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	      inputTable->setItem(7, 1, __qtablewidgetitemE4);

	      inputTable->hideRow(8);
	      modelParamsAction->setVisible(false);
	      modelParamsLabelAction->setVisible(false);
	      Option->setVisible(true);
// 	      __qtablewidgetitemE2->setText(QString::fromUtf8(""));
// 	      inputTable->setVerticalHeaderItem(8, __qtablewidgetitemE2);
//
// 	      __qtablewidgetitemE5->setText(QString::fromUtf8(""));
// 	      inputTable->setItem(8, 0, __qtablewidgetitemE5);
//
// 	      __qtablewidgetitemE6->setText(QString::fromUtf8(""));
// 	      __qtablewidgetitemE6->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
// 	      inputTable->setItem(8, 1, __qtablewidgetitemE6);



	      break;
      case(1):


	      modelParamsAction->setVisible(FALSE);
	      modelParamsLabelAction->setVisible(FALSE);
	      Option->setVisible(TRUE);

	      __qtablewidgetitemE1->setText(QString::fromUtf8("WIDTH (m)"));
	      inputTable->setVerticalHeaderItem(7, __qtablewidgetitemE1);

	      __qtablewidgetitemE3->setText(QString::fromUtf8("0.03"));
	      inputTable->setItem(7, 0, __qtablewidgetitemE3);

	      __qtablewidgetitemE4->setText(QString::fromUtf8("0.03-0.05 m"));
	      __qtablewidgetitemE4->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	      inputTable->setItem(7, 1, __qtablewidgetitemE4);

	      inputTable->showRow(8);
	      __qtablewidgetitemE2->setText(QString::fromUtf8("SKEW SHAPE"));
	      inputTable->setVerticalHeaderItem(8, __qtablewidgetitemE2);

	      __qtablewidgetitemE5->setText(QString::fromUtf8("500"));
	      inputTable->setItem(8, 0, __qtablewidgetitemE5);

	      __qtablewidgetitemE6->setText(QString::fromUtf8("-500-500"));
	      __qtablewidgetitemE6->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	      inputTable->setItem(8, 1, __qtablewidgetitemE6);
	      break;
      case(2):

	      modelParams->setCurrentIndex(0);
	      modelParamsAction->setVisible(TRUE);
	      modelParamsLabelAction->setVisible(TRUE);
	      Option->setVisible(TRUE);

	      __qtablewidgetitemE1->setText(QString::fromUtf8("LAMBDA_Q (m)"));
	      inputTable->setVerticalHeaderItem(7, __qtablewidgetitemE1);

	      __qtablewidgetitemE3->setText(QString::fromUtf8("0.01"));
	      inputTable->setItem(7, 0, __qtablewidgetitemE3);

	      __qtablewidgetitemE4->setText(QString::fromUtf8("0.005-??"));
	      __qtablewidgetitemE4->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	      inputTable->setItem(7, 1, __qtablewidgetitemE4);

	      inputTable->showRow(8);
	      __qtablewidgetitemE2->setText(QString::fromUtf8("S (m)"));
	      inputTable->setVerticalHeaderItem(8, __qtablewidgetitemE2);

	      __qtablewidgetitemE5->setText(QString::fromUtf8("0.005"));
	      inputTable->setItem(8, 0, __qtablewidgetitemE5);

	      __qtablewidgetitemE6->setText(QString::fromUtf8("???"));
	      __qtablewidgetitemE6->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	      inputTable->setItem(8, 1, __qtablewidgetitemE6);
	      break;
      }
      updateModel( __qtablewidgetitemE3);
      updateModel( __qtablewidgetitemE5);


 }

 void VitaMainWindow::updateModel(QTableWidgetItem* item){

	 theModel->setNBI(inputTable->item(0,0)->text().toDouble()*1E6);
	 theModel->setICRH(inputTable->item(1,0)->text().toDouble()*1E6);
	 theModel->RF = inputTable->item(2,0)->text().toDouble();
	 double stepTime = theModel->getStepTime();
	 if (!theModel->isICRHLoaded() && !theModel->isNBILoaded() && !theModel->isSPLoaded() && !theModel->isLoadLoaded() && !theModel->AutoLambdaS){
		theModel->pulseTime = inputTable->item(3,0)->text().toDouble();
		theModel->pulseIterations=theModel->pulseTime/stepTime+1;
	 }
	 if(inputTable->item(4,0)->text().toDouble()> theModel->pulseIterations * stepTime){
		double cooldownTime = inputTable->item(4,0)->text().toDouble() - theModel->pulseTime;
		theModel->cooldownIterations=cooldownTime/stepTime;
	 }else{
		QTableWidgetItem *__qtablewidgetitemE1 = new QTableWidgetItem();
		__qtablewidgetitemE1->setText(QString::number(theModel->pulseIterations *stepTime + 10));
		inputTable->setItem(4, 0, __qtablewidgetitemE1);
		double cooldownTime = inputTable->item(4,0)->text().toDouble() - theModel->pulseTime;
		theModel->cooldownIterations=cooldownTime/stepTime;
	 }
	 theModel->ST = inputTable->item(5,0)->text().toDouble();
	 theModel->setSP(inputTable->item(6,0)->text().toDouble());
	 theModel->W = inputTable->item(7,0)->text().toDouble();
	 theModel->skewShape = inputTable->item(8,0)->text().toDouble();;
	 if (!theModel->useFlush) theModel->WF = inputTable->item(9,0)->text().toDouble();
	 theModel->SA = inputTable->item(10,0)->text().toDouble();
	 theModel->SF = inputTable->item(11,0)->text().toDouble();

	 if (!theModel->AutoLambdaS){
		theModel->lambda_q = inputTable->item(7,0)->text().toDouble();
		theModel->S = inputTable->item(8,0)->text().toDouble();
	 }
	 //theModel->FC = inputTable->item(12,0)->text().toDouble();

	 double x_lower,x_upper,pdfLimit,cdfLimit,t_lower,t_upper;

	 theModel->getXLimits(x_lower,x_upper);
	 theModel->getTimeLimits(t_lower,t_upper);
	 theModel->getDistLimits(pdfLimit,cdfLimit);

#ifdef USE_QCUSTOMPLOT
	 inputPlot->xAxis->setRange(x_lower, x_upper);
	 inputPlot->yAxis->setRange(0, pdfLimit*1.2);
	 inputPlot->yAxis2->setRange(0, cdfLimit*1.2);
	 solvePlot->xAxis->setRange(t_lower,t_upper);
	 powerPlot->xAxis->setRange(t_lower,t_upper);

	 SPPlot1->xAxis->setRange(t_lower,t_upper);
	 SPPlot1->yAxis->setRange(x_lower-0.1, x_upper+0.1);
#endif

#ifdef USE_QCUSTOMPLOT
	 theModel->plotHeatFluenceDistribution(inputPlot);
	 theModel->plotPowerAndSP(powerPlot,SPPlot1);
	 theModel->plotSolveHeatOutput(solvePlot,0);
#endif
}

void VitaMainWindow::loadSPFile(){
	chdir("/home/");
	QString filePath = QFileDialog::getOpenFileName( this,
											 tr("open file dialog"),
											 "/home/",
											 tr("All files(*.*)")
										   );
	if ( !filePath.isEmpty() ){
		theModel->loadSPfile(filePath.toStdString().data());
		statusBar()->showMessage( tr("SP File loaded"), 2000 );
	}else{
		statusBar()->showMessage( tr("SP load aborted"), 2000 );
	}
	chdir("/tmp/");
	updateModel(NULL);
}


void VitaMainWindow::loadSPJPF(){
	bool ok;
	int jpn = QInputDialog::getInt(this, tr("Introduce JET Pulse Number"),"JPN:",0,0,2147483647,1,&ok);

	if(ok){
		theModel->loadSPJPF(jpn);
		if(theModel->isSPLoaded()){
			QTableWidgetItem *__qtablewidgetitemE1 = new QTableWidgetItem();
			QTableWidgetItem *__qtablewidgetitemE2 = new QTableWidgetItem();
			__qtablewidgetitemE1->setText(QString::fromUtf8("Loaded"));
			__qtablewidgetitemE2->setText(QString::fromUtf8("Shift"));
			__qtablewidgetitemE1->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
			statusBar()->showMessage( tr("SP JPF loaded"), 2000 );
			inputTable->setItem(6, 1, __qtablewidgetitemE1);
			inputTable->setItem(6, 0, __qtablewidgetitemE2);
		}else{
			QMessageBox::warning(this,tr("SP load"),tr("Can't load SP JPF for specified pulse.\n Signal don't exist."));
			statusBar()->showMessage( tr("Problem loading SP JPF"), 2000 );
		}
	}else{
		statusBar()->showMessage( tr("SP load aborted"), 2000 );
	}

	updateModel(NULL);
}

void VitaMainWindow::unloadSP(){
	theModel->unloadSP();
	updateModel(NULL);
}

void VitaMainWindow::loadNBIFile(){
  QString filePath = QFileDialog::getOpenFileName( this,
						   tr("open file dialog"),
						   "/home/",
						   tr("All files(*.*)")
  );
  if ( !filePath.isEmpty() ){
    theModel->loadNBIfile(filePath.toStdString().data());
    statusBar()->showMessage( tr("NBI File loaded"), 2000 );
    QTableWidgetItem *__qtablewidgetitemTime = new QTableWidgetItem();
    __qtablewidgetitemTime->setText(QString::fromUtf8("Loaded"));
    __qtablewidgetitemTime->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    inputTable->setItem(3, 0, __qtablewidgetitemTime);
  }else{
    statusBar()->showMessage( tr("NBI load aborted"), 2000 );
  }
  updateModel(NULL);
}

void VitaMainWindow::loadNBIPPF(){
	bool ok;
	int jpn = QInputDialog::getInt(this, tr("Introduce JET Pulse Number"),"JPN:",0,0,2147483647,1,&ok);

	if(ok){
		theModel->loadNBIPPF(jpn);
		if(theModel->isNBILoaded()){
			QTableWidgetItem *__qtablewidgetitemE1 = new QTableWidgetItem();
			__qtablewidgetitemE1->setText(QString::fromUtf8("Loaded"));
			statusBar()->showMessage( tr("NBI PPF loaded"), 2000 );
			inputTable->setItem(0, 0, __qtablewidgetitemE1);
			QTableWidgetItem *__qtablewidgetitemTime = new QTableWidgetItem();
			__qtablewidgetitemTime->setText(QString::fromUtf8("Loaded"));
			__qtablewidgetitemTime->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
			inputTable->setItem(3, 0, __qtablewidgetitemTime);
		}else{
			QMessageBox::warning(this,tr("NBI load"),tr("Can't load NBI PPF for specified pulse.\n DDA don't exist."));
			statusBar()->showMessage( tr("Problem loading NBI PPF"), 2000 );
		}
	}else{
		statusBar()->showMessage( tr("NBI load aborted"), 2000 );
	}

	updateModel(NULL);
}

void VitaMainWindow::unloadNBI(){
	theModel->unloadNBI();
	updateModel(NULL);
}

void VitaMainWindow::loadICRHFile(){
	QString filePath = QFileDialog::getOpenFileName( this,
			tr("open file dialog"),
			 "/home/",
			 tr("All files(*.*)")
			);
	if ( !filePath.isEmpty() ){
		theModel->loadICRHfile(filePath.toStdString().data());
		QTableWidgetItem *__qtablewidgetitemTime = new QTableWidgetItem();
		__qtablewidgetitemTime->setText(QString::fromUtf8("Loaded"));
		__qtablewidgetitemTime->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		inputTable->setItem(3, 0, __qtablewidgetitemTime);
		statusBar()->showMessage( tr("ICRH File loaded"), 2000 );
	}else{
		statusBar()->showMessage( tr("ICRH load aborted"), 2000 );
	}
	updateModel(NULL);
}

void VitaMainWindow::unloadLoadFile(){
	theModel->unloadLoadFile();
	updateModel(NULL);
}

void VitaMainWindow::loadLoad(){
	QString filePath = QFileDialog::getOpenFileName( this,
			tr("open file dialog"),
			 "/home/",
			 tr("All files(*.*)")
			);

	QString coordPath = QFileDialog::getOpenFileName( this,
			tr("open coordinate file"),
			 "/home/",
			 tr("All files(*.*)")
			);

	QTableWidgetItem *__qtablewidgetitemICRH = new QTableWidgetItem();
	__qtablewidgetitemICRH->setText(QString::fromUtf8("Loaded"));
	__qtablewidgetitemICRH->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	QTableWidgetItem *__qtablewidgetitemNBI = new QTableWidgetItem();
	__qtablewidgetitemNBI->setText(QString::fromUtf8("Loaded"));
	__qtablewidgetitemNBI->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	QTableWidgetItem *__qtablewidgetitemSP = new QTableWidgetItem();
	__qtablewidgetitemSP->setText(QString::fromUtf8("Loaded"));
	__qtablewidgetitemSP->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	QTableWidgetItem *__qtablewidgetitemTime = new QTableWidgetItem();
	__qtablewidgetitemTime->setText(QString::fromUtf8("Loaded"));
	__qtablewidgetitemTime->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	if ( !filePath.isEmpty()){
	  if (!coordPath.isEmpty())
		theModel->loadLoadFile(filePath.toStdString().data(), coordPath.toStdString().data(), true);
	  else
		theModel->loadLoadFile(filePath.toStdString().data(), coordPath.toStdString().data(), false);
		if (theModel->isLoadLoaded()){
			inputTable->setItem(0, 0, __qtablewidgetitemNBI);
			inputTable->setItem(1, 0, __qtablewidgetitemICRH);
			inputTable->setItem(6, 0, __qtablewidgetitemSP);
			inputTable->setItem(3, 0, __qtablewidgetitemTime);
			statusBar()->showMessage( tr("load File loaded"), 2000 );
			theModel->dist = Loaded_Dist;
			vGraphs[1]->hide();
			vGraphs[2]->hide();
		}else{
			statusBar()->showMessage( tr("load file load failed"), 2000 );
		}
	}else{
		statusBar()->showMessage( tr("load file load aborted"), 2000 );
	}
	updateModel(NULL);
}

void VitaMainWindow::loadICRHPPF(){
	bool ok;
	int jpn = QInputDialog::getInt(this, tr("Introduce JET Pulse Number"),"JPN:",0,0,2147483647,1,&ok);

	if(ok){
		theModel->loadICRHPPF(jpn);
		if(theModel->isICRHLoaded()){
			QTableWidgetItem *__qtablewidgetitemE1 = new QTableWidgetItem();
			__qtablewidgetitemE1->setText(QString::fromUtf8("Loaded"));
			__qtablewidgetitemE1->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
			statusBar()->showMessage( tr("ICRH PPF loaded"), 2000 );
			inputTable->setItem(1, 0, __qtablewidgetitemE1);
			QTableWidgetItem *__qtablewidgetitemTime = new QTableWidgetItem();
			__qtablewidgetitemTime->setText(QString::fromUtf8("Loaded"));
			__qtablewidgetitemTime->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
			inputTable->setItem(3, 0, __qtablewidgetitemTime);
		}else{
			QMessageBox::warning(this,tr("ICRH load"),tr("Can't load ICRH PPF for specified pulse.\n DDA don't exist."));
			statusBar()->showMessage( tr("Problem loading ICRH PPF"), 2000 );
		}
	}else{
		statusBar()->showMessage( tr("ICRH load aborted"), 2000 );
	}

	updateModel(NULL);
}


void VitaMainWindow::loadOHM(){
	bool ok;
	int jpn = QInputDialog::getInt(this, tr("Introduce JET Pulse Number"),"JPN:",0,0,2147483647,1,&ok);

	if(ok){
		QTableWidgetItem *__qtablewidgetitemTime = new QTableWidgetItem();
		__qtablewidgetitemTime->setText(QString::fromUtf8("Loaded"));
		__qtablewidgetitemTime->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		theModel->loadOHMPPF(jpn);
		if(!theModel->isOHMLoaded()){
			QMessageBox::warning(this,tr("OHM load"),tr("Can't load OHM PPF for specified pulse.\n DDA don't exist."));
		}else {
			inputTable->setItem(3, 0, __qtablewidgetitemTime);
			statusBar()->showMessage( tr("OHM PPF loaded"), 2000 );
		}
	}else{
		statusBar()->showMessage( tr("Load aborted"), 2000 );
	}

	updateModel(NULL);
}


void VitaMainWindow::loadAll(){

	bool ok;
	int jpn = QInputDialog::getInt(this, tr("Introduce JET Pulse Number"),"JPN:",0,0,2147483647,1,&ok);

	if(ok){
		theModel->loadICRHPPF(jpn);
		theModel->loadNBIPPF(jpn);
		theModel->loadSPJPF(jpn);
		theModel->loadICRHPPF(jpn);
		theModel->loadOHMPPF(jpn);
		QTableWidgetItem *__qtablewidgetitemICRH = new QTableWidgetItem();
		__qtablewidgetitemICRH->setText(QString::fromUtf8("Loaded"));
		__qtablewidgetitemICRH->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		QTableWidgetItem *__qtablewidgetitemNBI = new QTableWidgetItem();
		__qtablewidgetitemNBI->setText(QString::fromUtf8("Loaded"));
		__qtablewidgetitemNBI->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		QTableWidgetItem *__qtablewidgetitemSP = new QTableWidgetItem();
		__qtablewidgetitemSP->setText(QString::fromUtf8("Loaded"));
		__qtablewidgetitemSP->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		QTableWidgetItem *__qtablewidgetitemShift = new QTableWidgetItem();
		__qtablewidgetitemShift->setText(QString::fromUtf8("Shift"));
		QTableWidgetItem *__qtablewidgetitemTime = new QTableWidgetItem();
		__qtablewidgetitemTime->setText(QString::fromUtf8("Loaded"));
		__qtablewidgetitemTime->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		if(!theModel->isICRHLoaded()){
			QMessageBox::warning(this,tr("ICRH load"),tr("Can't load ICRH PPF for specified pulse.\n DDA don't exist."));
		}else inputTable->setItem(1, 0, __qtablewidgetitemICRH);

		if(!theModel->isNBILoaded()){
			QMessageBox::warning(this,tr("NBI load"),tr("Can't load NBI PPF for specified pulse.\n DDA don't exist."));
		}else inputTable->setItem(0, 0, __qtablewidgetitemNBI);
		if(!theModel->isSPLoaded()){
			QMessageBox::warning(this,tr("SP load"),tr("Can't load SP JPF for specified pulse.\n Signal don't exist."));
		}else{
			inputTable->setItem(6, 1, __qtablewidgetitemSP);
			inputTable->setItem(6, 0, __qtablewidgetitemShift);
		}
		if(!theModel->isOHMLoaded()){
			QMessageBox::warning(this,tr("OHM load"),tr("Can't load OHM PPF for specified pulse.\n DDA don't exist."));
		}
		if (theModel->isSPLoaded()&&theModel->isNBILoaded()&&theModel->isICRHLoaded() && theModel->isOHMLoaded()){
			statusBar()->showMessage( tr("SP JPF, OHM, NBI and ICRH PPF loaded"), 2000 );

		}else{
			statusBar()->showMessage( tr("problem loading SP JPF, NBI and ICRH PPF"), 2000 );
		}
		if (theModel->isSPLoaded()||theModel->isNBILoaded()||theModel->isICRHLoaded() || theModel->isOHMLoaded()){
			inputTable->setItem(3, 0, __qtablewidgetitemTime);
		}
		theModel->shot = jpn;
		isNBISPICRHLoaded = true;


	}else{
		statusBar()->showMessage( tr("Load aborted"), 2000 );
	}

	updateModel(NULL);
}

void VitaMainWindow::unloadICRH(){
	theModel->unloadICRH();
	updateModel(NULL);
}

void VitaMainWindow::loadTCJPF(){
	bool ok,createPlot;
	int jpn = QInputDialog::getInt(this, tr("Introduce JET Pulse Number"),"JPN:",0,0,2147483647,1,&ok);


	createPlot=!theModel->isTCJPFLoaded();

	if(ok){
		theModel->loadTCJPF(jpn);
		theModel->loadTMT6PPF(jpn);

		if(theModel->isTCJPFLoaded()){
			statusBar()->showMessage( tr("TC JPF loaded"), 2000 );
		}else{
			QMessageBox::warning(this,tr("TC load"),tr("Can't load TC JPF for specified pulse.\n Signal don't exist."));
			statusBar()->showMessage( tr("Problem loading TC JPF"), 2000 );
		}
	}else{
		statusBar()->showMessage( tr("TC load aborted"), 2000 );
	}

	if(createPlot){
		extendOutputPlot();
	}

	if (dock->isVisible()){
		QTableWidgetItem *__qtablewidgetitem13 = new QTableWidgetItem();
		__qtablewidgetitem13->setText(QString::number(-theModel->TCxJPFmaxTemp+theModel->TCxPeak));
		__qtablewidgetitem13->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		outputTable->setItem(4, 0, __qtablewidgetitem13);
		QTableWidgetItem *__qtablewidgetitem14 = new QTableWidgetItem();
		__qtablewidgetitem14->setText(QString::number(-theModel->TCyJPFmaxTemp+theModel->TCyPeak));
		__qtablewidgetitem14->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		outputTable->setItem(5, 0, __qtablewidgetitem14);
		QTableWidgetItem *__qtablewidgetitem15 = new QTableWidgetItem();
		__qtablewidgetitem15->setText(QString::number(-theModel->TCzJPFmaxTemp+theModel->TCzPeak));
		__qtablewidgetitem15->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		outputTable->setItem(6, 0, __qtablewidgetitem15);
		outputTable->showRow(4);
		outputTable->showRow(5);
		outputTable->showRow(6);
	}
	updateModel(NULL);
	fileOpened = true;
}

void VitaMainWindow::loadMeasuredEnergy(){
	bool ok;
	int jpn = QInputDialog::getInt(this, tr("Introduce JET Pulse Number"),"JPN:",0,0,2147483647,1,&ok);
	if(ok){
		theModel->loadMeasuredEnergy(jpn);
		QTableWidgetItem *__qtablewidgetitem13 = new QTableWidgetItem();
	      __qtablewidgetitem13->setText(QString::number(theModel->measuredEnergy/1e6));
	      __qtablewidgetitem13->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	      outputTable->setItem(8, 0, __qtablewidgetitem13);
	}

}

void VitaMainWindow::exportTC(){
	QString simFileName = QFileDialog::getSaveFileName(this,tr("Select exported Temperature file name"),"/home/",tr("CSV files (*.csv)"));
	QString loadedFileName;
// 	if(theModel->isTCJPFLoaded()){
// 		loadedFileName = QFileDialog::getSaveFileName(this,tr("Select loaded TC file name"),"/home/",tr("CSV files (*.csv)"));
// 	}
	bool simExported,loadExported;
	theModel->exportTCData(simFileName.toStdString(),simExported,loadedFileName.toStdString(),loadExported);

	if(simExported && (loadExported || !theModel->isTCJPFLoaded())){
		statusBar()->showMessage( tr("Temperature Data exported"), 2000 );
	}else{
		QString msg;
		if(!simExported){
			msg.append("SimTC data ");
		}
// 		if(!loadExported && theModel->isTCJPFLoaded()){
// 			msg.append("LoadedTC data ");
// 		}
		msg.append("NOT exported");
		statusBar()->showMessage( msg, 2000 );
	}
}

void VitaMainWindow::unloadTCJPF(){
	theModel->unloadTCJPF();
	updateModel(NULL);
}

void VitaMainWindow::createInputPlot()
{
     // generate some data:
	 QPen* pen=NULL;
     if (inputPlot){
       delete inputPlot;
       inputPlot=0;
     }
#ifdef USE_QCUSTOMPLOT
     inputPlot = new QCustomPlot;

     inputPlot->yAxis2->setVisible(true);
     inputPlot->legend->setVisible(true);
     inputPlot->xAxis->setLabel("Surface (m)");
     inputPlot->yAxis->setLabel("PDF Power (W)");
     inputPlot->yAxis2->setLabel("CDF Power (W)");

     inputPlot->addGraph();	//Divertor Area
	 inputPlot->graph(0)->setName("Divertor Area");
	 pen = new QPen(Qt::red);
	 pen->setWidth(10);
	 inputPlot->graph(0)->setPen(*pen);

     inputPlot->addGraph();	//Heat PDF
	 inputPlot->graph(1)->setName("Heat Fluence PDF");
	 pen = new QPen(Qt::blue);
	 pen->setWidth(2);
	 inputPlot->graph(1)->setPen(*pen);

     inputPlot->addGraph(inputPlot->xAxis,inputPlot->yAxis2);// Heat CDF
	 inputPlot->graph(2)->setName("Heat Fluence CDF");
	 pen = new QPen(Qt::green);
	 pen->setWidth(2);
	 inputPlot->graph(2)->setPen(*pen);
#endif
}

void VitaMainWindow::createOutputPlot(){
	QPen* pen=NULL;
     if (solvePlot){
       delete solvePlot;
       solvePlot=0;
     }
#ifdef USE_QCUSTOMPLOT
	solvePlot = new QCustomPlot;
	solvePlot->legend->setVisible(true);
	solvePlot->xAxis->setLabel("Time (s)");
	solvePlot->yAxis->setLabel("Temp (ºC)");

	solvePlot->addGraph(); //Max Temp
	solvePlot->graph(0)->setName("Max Temp");
	pen = new QPen(Qt::cyan);
	pen->setWidth(2);
	solvePlot->graph(0)->setPen(*pen);

	solvePlot->addGraph(); //TC.x
	solvePlot->graph(1)->setName("TC x");
	pen = new QPen(Qt::magenta);
	pen->setWidth(2);
	solvePlot->graph(1)->setPen(*pen);

	solvePlot->addGraph(); //TC.y
	solvePlot->graph(2)->setName("TC y");
	pen = new QPen(Qt::green);
	pen->setWidth(2);
	solvePlot->graph(2)->setPen(*pen);

	solvePlot->addGraph(); //TC.z
	solvePlot->graph(3)->setName("TC z");
	pen = new QPen(Qt::blue);
	pen->setWidth(2);
	solvePlot->graph(3)->setPen(*pen);


	QCPItemStraightLine* currTime = new QCPItemStraightLine(solvePlot);
	currTime->point1->setType(QCPItemPosition::ptPlotCoords);
	currTime->point1->setCoords(0,0);
	currTime->point2->setType(QCPItemPosition::ptPlotCoords);
	currTime->point2->setCoords(0,0);
	pen = new QPen(Qt::red);
	pen->setWidth(4);
	currTime->setPen(*pen);
	solvePlot->addItem(currTime);
#endif
}

void VitaMainWindow::extendOutputPlot(){
	QPen* pen=NULL;
#ifdef USE_QCUSTOMPLOT
	solvePlot->addGraph(); //TC.x
	solvePlot->graph(4)->setName("JPF TC x");
	pen = new QPen(Qt::darkMagenta);
	pen->setWidth(2);
	solvePlot->graph(4)->setPen(*pen);

	solvePlot->addGraph(); //TC.y
	solvePlot->graph(5)->setName("JPF TC y");
	pen = new QPen(Qt::darkGreen);
	pen->setWidth(2);
	solvePlot->graph(5)->setPen(*pen);

	solvePlot->addGraph(); //TC.z
	solvePlot->graph(6)->setName("JPF TC z");
	pen = new QPen(Qt::darkBlue);
	pen->setWidth(2);
	solvePlot->graph(6)->setPen(*pen);

	solvePlot->addGraph(); //TC.z
	solvePlot->graph(7)->setName("TMT6 PPF");
	pen = new QPen(Qt::darkCyan);
	pen->setWidth(2);
	solvePlot->graph(7)->setPen(*pen);


	QCPItemStraightLine* currTime = new QCPItemStraightLine(solvePlot);
	currTime->point1->setType(QCPItemPosition::ptPlotCoords);
	currTime->point1->setCoords(0,0);
	currTime->point2->setType(QCPItemPosition::ptPlotCoords);
	currTime->point2->setCoords(0,0);
	pen = new QPen(Qt::red);
	pen->setWidth(4);
	currTime->setPen(*pen);
	solvePlot->addItem(currTime);
#endif

}

void VitaMainWindow::createPowerPlot(){
	QPen* pen=NULL;
#ifdef USE_QCUSTOMPLOT
	powerPlot = new QCustomPlot;
	powerPlot->legend->setVisible(true);
	powerPlot->xAxis->setLabel("Time");
	powerPlot->yAxis->setLabel("Power (W)");


	powerPlot->addGraph();
	powerPlot->graph(0)->setName("Peak Power");
	pen = new QPen(Qt::cyan);
	pen->setWidth(2);
	powerPlot->graph(0)->setPen(*pen);

	powerPlot->addGraph();
	powerPlot->graph(1)->setName("Total Power");
	pen = new QPen(Qt::magenta);
	pen->setWidth(2);
	powerPlot->graph(1)->setPen(*pen);

	powerPlot->addGraph();
	powerPlot->graph(2)->setName("NBI");
	pen = new QPen(Qt::green);
	pen->setWidth(2);
	powerPlot->graph(2)->setPen(*pen);

	powerPlot->addGraph();
	powerPlot->graph(3)->setName("ICRH");
	pen = new QPen(Qt::blue);
	pen->setWidth(2);
	powerPlot->graph(3)->setPen(*pen);

	powerPlot->addGraph();
	powerPlot->graph(4)->setName("Ohm");
	pen = new QPen(Qt::yellow);
	pen->setWidth(2);
	powerPlot->graph(4)->setPen(*pen);

	QCPItemStraightLine* currTime = new QCPItemStraightLine(powerPlot);
	currTime->point1->setType(QCPItemPosition::ptPlotCoords);
	currTime->point1->setCoords(0,0);
	currTime->point2->setType(QCPItemPosition::ptPlotCoords);
	currTime->point2->setCoords(0,0);
	pen = new QPen(Qt::red);
	pen->setWidth(4);
	currTime->setPen(*pen);
	powerPlot->addItem(currTime);
#endif
}

void VitaMainWindow::createSPPlot(){
	QPen* pen=NULL;
#ifdef USE_QCUSTOMPLOT
	SPPlot1= new QCustomPlot;
	SPPlot1->legend->setVisible(true);
	SPPlot1->xAxis->setLabel("Time (s)");
	SPPlot1->yAxis->setLabel("Surface (m)");

	SPPlot1->addGraph();
	SPPlot1->graph(0)->setName("Divertor Area");
	pen = new QPen(Qt::red);
	pen->setWidth(10);
	SPPlot1->graph(0)->setPen(*pen);

	SPPlot1->addGraph();
	SPPlot1->graph(1)->setName("SP");
	pen = new QPen(Qt::cyan);
	pen->setWidth(2);
	SPPlot1->graph(1)->setPen(*pen);

	QCPItemStraightLine* currTime = new QCPItemStraightLine(SPPlot1);
	currTime->point1->setType(QCPItemPosition::ptPlotCoords);
	currTime->point1->setCoords(0,0);
	currTime->point2->setType(QCPItemPosition::ptPlotCoords);
	currTime->point2->setCoords(0,0);
	pen = new QPen(Qt::blue);
	pen->setWidth(4);
	currTime->setPen(*pen);
	SPPlot1->addItem(currTime);
#endif
}

 void VitaMainWindow::createActions()
 {

     newAct = new QAction(QIcon(":/images/filenew.png"), tr("&New File..."),
                                this);
     newAct->setShortcut(tr("Ctrl+N"));
     newAct->setStatusTip(tr("New file from template"));
     connect(newAct, SIGNAL(triggered()), this, SLOT(fileNew()));

     openAct = new QAction(QIcon(":/images/fileopen.png"), tr("&Open File..."),
                                this);
     openAct->setShortcut(tr("Ctrl+O"));
     openAct->setStatusTip(tr("Open a Geometry/Results"));
     connect(openAct, SIGNAL(triggered()), this, SLOT(fileOpen()));

     refreshAct = new QAction(QIcon(":/images/filerefresh.png"), tr("&Refresh Data..."),
                                this);
     refreshAct->setShortcut(tr("F5"));
     openAct->setStatusTip(tr("Refresh Geometry/results"));
     connect(refreshAct, SIGNAL(triggered()), this, SLOT(fileRefresh()));

     saveAct = new QAction(QIcon(":/images/filesave.png"), tr("&Save..."), this);
     saveAct->setShortcut(tr("Ctrl+S"));
     saveAct->setStatusTip(tr("Save the file"));
     connect(saveAct, SIGNAL(triggered()), this, SLOT(fileSave()));

     printAct = new QAction(QIcon(":/images/fileprint.png"), tr("&Print..."), this);
     printAct->setShortcut(tr("Ctrl+P"));
     printAct->setStatusTip(tr("Print the current caption"));
     connect(printAct, SIGNAL(triggered()), this, SLOT(filePrint()));

     nbiFileAct = new QAction(QIcon(":/images/icon-edit-NBI.png"), tr("N&BI file data load"), this);
     nbiFileAct->setShortcut(tr("Ctrl+B"));
     nbiFileAct->setStatusTip(tr("Loads a table for NBI power over time"));
     connect(nbiFileAct, SIGNAL(triggered()), this, SLOT(loadNBIFile()));

     nbiPPFAct = new QAction(QIcon(":/images/icon-edit-NBI.png"), tr("NBI PPF data load"), this);
//	 nbiPPFAct->setShortcut(tr("Ctrl+B"));
	 nbiPPFAct->setStatusTip(tr("Loads NBI power over time from JET's PPFs"));
	 connect(nbiPPFAct, SIGNAL(triggered()), this, SLOT(loadNBIPPF()));

     icrhFileAct = new QAction(QIcon(":/images/icon-edit-ICRH.png"), tr("&ICRH file data load"), this);
     icrhFileAct->setShortcut(tr("Ctrl+I"));
     icrhFileAct->setStatusTip(tr("Loads a table for ICRH power over time"));
     connect(icrhFileAct, SIGNAL(triggered()), this, SLOT(loadICRHFile()));

     icrhPPFAct = new QAction(QIcon(":/images/icon-edit-ICRH.png"), tr("ICRH PPF data load"), this);
//	 icrhPPFAct->setShortcut(tr("Ctrl+I"));
	 icrhPPFAct->setStatusTip(tr("Loads ICRH power over time from JET's PPFs"));
	 connect(icrhPPFAct, SIGNAL(triggered()), this, SLOT(loadICRHPPF()));

      ohmPPFAct = new QAction(QIcon(":/images/icon-edit-ICRH.png"), tr("OHM PPF data load"), this);
      connect(ohmPPFAct, SIGNAL(triggered()), this, SLOT(loadOHM()));

SpNbiIcrhPPFAct = new QAction(QIcon(":/images/icon-edit-ICRH.png"), tr("SP JPF, OHM NBI & ICRH PPF data load"), this);
	 SpNbiIcrhPPFAct->setStatusTip(tr("Loads SP, ICRH and NBI power over time from JET's PPFs"));
	 connect(SpNbiIcrhPPFAct, SIGNAL(triggered()), this, SLOT(loadAll()));


     spFileAct = new QAction(QIcon(":/images/icon-edit-SP.png"), tr("S&P data load"), this);
     spFileAct->setShortcut(tr("Ctrl+P"));
     spFileAct->setStatusTip(tr("Loads a table for Strike Point radial location over time"));
     connect(spFileAct, SIGNAL(triggered()), this, SLOT(loadSPFile()));

     spJPFAct= new QAction(QIcon(":/images/icon-edit-SP.png"), tr("SP JPF data load"), this);
//	 spJPFAct->setShortcut(tr("Ctrl+P"));
	 spJPFAct->setStatusTip(tr("Loads Strike Point radial location from JET's JPFs"));
     connect(spJPFAct, SIGNAL(triggered()), this, SLOT(loadSPJPF()));

     tcJPFAct= new QAction(QIcon(":/images/icon-edit-TC.png"), tr("TC JPF data load"), this);
 //	 spJPFAct->setShortcut(tr("Ctrl+P"));
	 tcJPFAct->setStatusTip(tr("Loads Thermocouples temperature data from JET's JPFs"));
	 connect(tcJPFAct, SIGNAL(triggered()), this, SLOT(loadTCJPF()));

      loadAct = new QAction(QIcon(":/images/icon-edit-TC.png"), tr("Load heat flux data"), this);
	loadAct->setStatusTip(tr("Load a load data from JET's JPFs"));
	connect(loadAct, SIGNAL(triggered()), this, SLOT(loadLoad()));

	exportTCAct= new QAction(QIcon(":/images/filesave-TC.png"), tr("TC data export"), this);
	//	 exportTCAct->setShortcut(tr("Ctrl+P"));
	exportTCAct->setStatusTip(tr("Export simulated and loaded thermocouple data to a file"));
	connect(exportTCAct, SIGNAL(triggered()), this, SLOT(exportTC()));


     modelType = new QComboBox( this );
     modelType->setObjectName(QString::fromUtf8("Model Type"));
     modelType->setDisabled(false);
     modelType->setMaximumWidth(400);
     modelType->addItem(QString::fromUtf8("Tile 6 - Quick"));
     modelType->addItem(QString::fromUtf8("Tile 6 - Accurate"));
     modelType->addItem(QString::fromUtf8("Tile 6 - WALLS RT"));
	 modelType->addItem(QString::fromUtf8("Load external mesh"));
	 connect( modelType, SIGNAL(currentIndexChanged(int)), this, SLOT(changeModel(int)) );

     modelDist = new QComboBox( this );
     modelDist->setObjectName(QString::fromUtf8("Model Heat Distribution"));
     modelDist->setDisabled(false);
     modelDist->setMaximumWidth(400);
     modelDist->addItem(QString::fromUtf8("Triangular"));
     modelDist->addItem(QString::fromUtf8("Skew Normal"));
     modelDist->addItem(QString::fromUtf8("Eich"));
     connect( modelDist, SIGNAL(currentIndexChanged(int)), this, SLOT(changeDist(int)) );
     connect( modelDist, SIGNAL(currentIndexChanged(int)), this, SLOT(tableselection(int)) );

     modelParams = new QComboBox( this );
     modelParams->setObjectName(QString::fromUtf8("Parameters cacul method"));
     modelParams->setDisabled(false);
     modelParams->setMaximumWidth(400);
     modelParams->addItem(QString::fromUtf8("Manual"));
     modelParams->addItem(QString::fromUtf8("Manual V.Riccardo's scaling"));
     modelParams->addItem(QString::fromUtf8("Auto. V.Riccardo's scaling"));
     modelParams->addItem(QString::fromUtf8("New auto. scaling"));
     connect( modelParams, SIGNAL(currentIndexChanged(int)), this, SLOT(ParamsEstim(int)) );

     Option = new QAction(this);
     Option->setText(QString::fromUtf8("Options"));
     connect(Option, SIGNAL(triggered()), this, SLOT(optionSim()));


     solveAct = new QAction(QIcon(":/images/solve.png"), tr("&Solve"), this);
     solveAct->setShortcut(tr("Ctrl+X"));
     solveAct->setStatusTip(tr("Solve the load case"));
     connect(solveAct, SIGNAL(triggered()), this, SLOT(editSolve()));

     quitAct = new QAction(QIcon(":/images/filequit.png"), tr("&Quit"), this);
     quitAct->setShortcut(tr("Ctrl+Q"));
     quitAct->setStatusTip(tr("Quit the application"));
     connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));

     aboutAct = new QAction(tr("&About"), this);
     aboutAct->setStatusTip(tr("Show the application's About box"));
     connect(aboutAct, SIGNAL(triggered()), this, SLOT(helpAbout()));

     aboutQtAct = new QAction(tr("About &Qt"), this);
     aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
     connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

     axisAct = new QAction( QIcon(":/images/viewaxis.png"), tr("Toggle Axis"), this);
     axisAct->setCheckable( true );
     axisAct->setStatusTip(tr("Toggles reference axis"));
     connect( axisAct, SIGNAL(triggered()), this, SLOT( viewToggleAxis() ) );

     contoursAct = new QAction( QIcon(":/images/viewscalar.png"), tr("Toggle Contours"), this);
     contoursAct->setCheckable( true );
     contoursAct->setStatusTip(tr("Toggles Contours of Scalar Values in Nodes"));
     connect( contoursAct, SIGNAL(triggered()), this, SLOT( viewToggleContours() ) );

     contourType = new QComboBox( this );
     contourType->setObjectName(QString::fromUtf8("Contour Type"));
     contourType->setDisabled(true);
     contourType->setMaximumWidth(400);
     contourType->addItem(QString::fromUtf8("No Contour"));
     connect( contoursAct, SIGNAL(toggled(bool)), contourType, SLOT(setEnabled(bool)) );
     connect( contourType, SIGNAL(currentIndexChanged(int)), this, SLOT(viewContour(int)) );

//      environmentType = new QComboBox( this );
//      environmentType->setObjectName(QString::fromUtf8("Environemt Type"));
//      environmentType->setDisabled(true);
//      environmentType->setMaximumWidth(400);
//      environmentType->addItem(QString::fromUtf8("No Environment"));
//      connect( contoursAct, SIGNAL(toggled(bool)), environmentType, SLOT(setEnabled(bool)) );
//      connect( environmentType, SIGNAL(currentIndexChanged(int)), this, SLOT(viewEnvironment(int)) );

     graphAct = new QAction( QIcon(":/images/viewgraph.png"), tr("New Graph"), this);
     graphAct->setCheckable( false );
     graphAct->setStatusTip(tr("Creates a new graph widget"));
     connect( graphAct, SIGNAL(triggered()), this, SLOT( viewNewGraph() ) );

     animateForwAct = new QAction( QIcon(":/images/animationplayf.png"), tr("Animate forward"), this);
     animateForwAct->setCheckable( true );
     animateForwAct->setStatusTip(tr("Run animation forward"));
     connect( animateForwAct, SIGNAL(triggered()), this, SLOT(viewAnimate()) );

     animateBackAct = new QAction( QIcon(":/images/animationplayb.png"), tr("Animate backward"), this);
     animateBackAct->setCheckable( true );
     animateBackAct->setStatusTip(tr("Run animation backward"));
     connect( animateBackAct, SIGNAL(triggered()), this, SLOT(viewAnimate()) );

     animatePauseAct = new QAction( QIcon(":/images/animationpause.png"), tr("Pause animation"), this);
     animatePauseAct->setCheckable( true );
     animatePauseAct->setStatusTip(tr("Stops animation in current frame"));
     connect( animatePauseAct, SIGNAL(triggered()), this, SLOT(viewAnimate()) );

     animateRecAct = new QAction( QIcon(":/images/animationrec.png"), tr("Record animation"), this);
     animateRecAct->setCheckable( true );
     animateRecAct->setStatusTip(tr("Records animation"));
     connect( animateRecAct, SIGNAL(triggered()), this, SLOT(viewRec()) );

     animateStopAct = new QAction( QIcon(":/images/animationstop.png"), tr("Stop animation"), this);
     animateStopAct->setCheckable( true );
     animateStopAct->setStatusTip(tr("Stops animation and goes back to start"));
     connect( animateStopAct, SIGNAL(triggered()), this, SLOT(viewAnimate()) );

     animateGroup = new QActionGroup(this);
     animateGroup->addAction(animateForwAct);
     animateGroup->addAction(animateBackAct);
     animateGroup->addAction(animatePauseAct);
     animateGroup->addAction(animateStopAct);
     animateStopAct->setChecked( true );

     animateTimeLine = new QSlider( this );
     animateTimeLine->setObjectName(QString::fromUtf8("animateTimeLine"));
     animateTimeLine->setOrientation(Qt::Horizontal);
//      animateTimeLine->setMaximumWidth(400);
     connect( animateTimeLine, SIGNAL(valueChanged(int)), this, SLOT(setFrame(int)) );

     timer = new QTimer( this );
     timer->setInterval( 100 );
     connect(timer, SIGNAL(timeout()), this, SLOT(nextFrame()));
 }

 void VitaMainWindow::createMenus()
 {
     fileMenu = menuBar()->addMenu(tr("&File"));
     fileMenu->addAction(newAct);
     fileMenu->addAction(openAct);
     fileMenu->addAction(refreshAct);
     fileMenu->addAction(saveAct);
     fileMenu->addAction(exportTCAct);
     fileMenu->addAction(printAct);
     fileMenu->addSeparator();
     fileMenu->addAction(quitAct);

     editMenu = menuBar()->addMenu(tr("&Edit"));
     editMenu->addAction(nbiFileAct);
     editMenu->addAction(icrhFileAct);
     editMenu->addAction(spFileAct);
     editMenu->addSeparator();
     editMenu->addAction(nbiPPFAct);
     editMenu->addAction(icrhPPFAct);
     editMenu->addAction(spJPFAct);
     editMenu->addAction(ohmPPFAct);
     editMenu->addAction(SpNbiIcrhPPFAct);
     editMenu->addSeparator();
     editMenu->addAction(tcJPFAct);
     editMenu->addSeparator();
     editMenu->addAction(loadAct);
     editMenu->addSeparator();
     editMenu->addAction(solveAct);

     viewMenu = menuBar()->addMenu(tr("&View"));
     viewMenu->addAction(axisAct);
     viewMenu->addAction(graphAct);
     viewMenu->addAction(contoursAct);

     animationMenu = viewMenu->addMenu(tr("&Animate"));
     animationMenu->setEnabled(false);
     animationMenu->addAction(animateStopAct);
     animationMenu->addAction(animateForwAct);
     animationMenu->addAction(animatePauseAct);
     animationMenu->addAction(animateBackAct);

     menuBar()->addSeparator();

     helpMenu = menuBar()->addMenu(tr("&Help"));
     helpMenu->addAction(aboutAct);
     helpMenu->addAction(aboutQtAct);
 }

 void VitaMainWindow::createToolBars()
 {
     fileToolBar = addToolBar(tr("File"));
     fileToolBar->addAction(newAct);
     fileToolBar->addAction(openAct);
     fileToolBar->addAction(refreshAct);
     fileToolBar->addAction(saveAct);
     fileToolBar->addAction(exportTCAct);
     fileToolBar->addAction(printAct);
     fileToolBar->addSeparator();
     fileToolBar->addAction(quitAct);

     editToolBar = addToolBar(tr("Edit"));
     modelTypeLabel = new QLabel("Model type:");
     editToolBar->addWidget(modelTypeLabel);
     editToolBar->addWidget(modelType);
     editToolBar->addAction(nbiFileAct);
     editToolBar->addAction(icrhFileAct);
     editToolBar->addAction(spFileAct);
     editToolBar->addSeparator();
     editToolBar->addAction(tcJPFAct);
     editToolBar->addSeparator();
     modelDistLabel = new QLabel("Load type:");
     editToolBar->addWidget(modelDistLabel);
     editToolBar->addWidget(modelDist);
     modelParamsLabel = new QLabel("Parameters estimation:");
     modelParamsLabelAction = editToolBar->addWidget(modelParamsLabel);
     modelParamsAction = editToolBar->addWidget(modelParams);
     editToolBar->addAction(Option);
     editToolBar->addAction(solveAct);


     modelParamsAction->setVisible(FALSE);
     modelParamsLabelAction->setVisible(FALSE);


     viewToolBar = addToolBar(tr("View"));
//      viewToolBar->addAction(axisAct);
     viewToolBar->addAction(graphAct);
     viewToolBar->addAction(contoursAct);
     viewToolBar->addWidget(contourType);
//      viewToolBar->addWidget(environmentType);

     this->addToolBarBreak();

     animationToolBar = addToolBar(tr("Animation"));
     animationToolBar->setEnabled(false);
     animationToolBar->setVisible(false);
     animationToolBar->addAction(animateStopAct);
     animationToolBar->addAction(animateBackAct);
     animationToolBar->addAction(animatePauseAct);
     animationToolBar->addAction(animateRecAct);
     animationToolBar->addAction(animateForwAct);
     animationToolBar->addWidget(animateTimeLine);
}

 void VitaMainWindow::createStatusBar()
 {
     statusBar()->showMessage(tr("Ready"));
 }



 void VitaMainWindow::createDockWindows()
 {
     dock = new QDockWidget(tr("Input"), this);
     dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
     QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
     sizePolicy.setHeightForWidth(dock->sizePolicy().hasHeightForWidth());
     dock->setSizePolicy(sizePolicy);
     dock->setMinimumSize(QSize(547, 100));
     dock->setMaximumSize(547,600);
     dock->setFeatures(QDockWidget::AllDockWidgetFeatures);

     inputTable = new QTableWidget(dock);
     if (inputTable->columnCount() < 2)
       inputTable->setColumnCount(2);
     QTableWidgetItem *__qtablewidgetitem15 = new QTableWidgetItem();
     __qtablewidgetitem15->setText(QString::fromUtf8("Value"));
     inputTable->setHorizontalHeaderItem(0, __qtablewidgetitem15);
     QTableWidgetItem *__qtablewidgetitem16 = new QTableWidgetItem();
     __qtablewidgetitem16->setText(QString::fromUtf8("Limit"));
     inputTable->setHorizontalHeaderItem(1, __qtablewidgetitem16);
     if (inputTable->rowCount() < 14)
       inputTable->setRowCount(14);
        QTableWidgetItem *__qtablewidgetitem17 = new QTableWidgetItem();
        __qtablewidgetitem17->setText(QString::fromUtf8("NBI POWER (MW)"));
        inputTable->setVerticalHeaderItem(0, __qtablewidgetitem17);
        QTableWidgetItem *__qtablewidgetitem18 = new QTableWidgetItem();
        __qtablewidgetitem18->setText(QString::fromUtf8("ICRH POWER (MW)"));
        inputTable->setVerticalHeaderItem(1, __qtablewidgetitem18);
        QTableWidgetItem *__qtablewidgetitem19 = new QTableWidgetItem();
        __qtablewidgetitem19->setText(QString::fromUtf8("RADIATED FRACTION"));
        inputTable->setVerticalHeaderItem(2, __qtablewidgetitem19);
        QTableWidgetItem *__qtablewidgetitem20 = new QTableWidgetItem();
        __qtablewidgetitem20->setText(QString::fromUtf8("PULSE DURATION (s)"));
        inputTable->setVerticalHeaderItem(3, __qtablewidgetitem20);
        QTableWidgetItem *__qtablewidgetitem21 = new QTableWidgetItem();
        __qtablewidgetitem21->setText(QString::fromUtf8("ANALYSIS DURATION (s)"));
        inputTable->setVerticalHeaderItem(4, __qtablewidgetitem21);
        QTableWidgetItem *__qtablewidgetitem22 = new QTableWidgetItem();
        __qtablewidgetitem22->setText(QString::fromUtf8("STARTING TEMPERATURE (°C)"));
        inputTable->setVerticalHeaderItem(5, __qtablewidgetitem22);
        QTableWidgetItem *__qtablewidgetitem23 = new QTableWidgetItem();
        __qtablewidgetitem23->setText(QString::fromUtf8("STRIKE POINT (m)"));
        inputTable->setVerticalHeaderItem(6, __qtablewidgetitem23);

		QTableWidgetItem *__qtablewidgetitem24 = new QTableWidgetItem();
	      __qtablewidgetitem24->setText(QString::fromUtf8("WIDTH (m)"));
	      inputTable->setVerticalHeaderItem(7, __qtablewidgetitem24);
	      QTableWidgetItem *__qtablewidgetitemS1 = new QTableWidgetItem();

	      inputTable->hideRow(8);
	      __qtablewidgetitemS1->setText(QString::fromUtf8(""));
	      inputTable->setVerticalHeaderItem(8, __qtablewidgetitemS1);



        QTableWidgetItem *__qtablewidgetitem25 = new QTableWidgetItem();
        __qtablewidgetitem25->setText(QString::fromUtf8("TOROIDAL WETTING FACTOR (%)"));
        inputTable->setVerticalHeaderItem(9, __qtablewidgetitem25);
        QTableWidgetItem *__qtablewidgetitem26 = new QTableWidgetItem();
        __qtablewidgetitem26->setText(QString::fromUtf8("SWEEP AMPLITUDE (m)"));
        inputTable->setVerticalHeaderItem(10, __qtablewidgetitem26);
        QTableWidgetItem *__qtablewidgetitem27 = new QTableWidgetItem();
        __qtablewidgetitem27->setText(QString::fromUtf8("SWEEP FREQUENCY (Hz)"));
        inputTable->setVerticalHeaderItem(11, __qtablewidgetitem27);
        QTableWidgetItem *__qtablewidgetitem28 = new QTableWidgetItem();
        __qtablewidgetitem28->setText(QString::fromUtf8("FILM COEFFICIENT (W/(m^2)K)"));
        inputTable->setVerticalHeaderItem(12, __qtablewidgetitem28);
        QTableWidgetItem *__qtablewidgetitem29 = new QTableWidgetItem();
        __qtablewidgetitem29->setText(QString::fromUtf8("SINK TEMPERATURE (°C)"));
        inputTable->setVerticalHeaderItem(13, __qtablewidgetitem29);



        QTableWidgetItem *__qtablewidgetitem30 = new QTableWidgetItem();
        __qtablewidgetitem30->setText(QString::fromUtf8("12"));
        inputTable->setItem(0, 0, __qtablewidgetitem30);
        QTableWidgetItem *__qtablewidgetitem31 = new QTableWidgetItem();
        __qtablewidgetitem31->setText(QString::fromUtf8("0-15 MW"));
        __qtablewidgetitem31->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        inputTable->setItem(0, 1, __qtablewidgetitem31);
        QTableWidgetItem *__qtablewidgetitem32 = new QTableWidgetItem();
        __qtablewidgetitem32->setText(QString::fromUtf8("5.75"));
        inputTable->setItem(1, 0, __qtablewidgetitem32);
        QTableWidgetItem *__qtablewidgetitem33 = new QTableWidgetItem();
        __qtablewidgetitem33->setText(QString::fromUtf8("0-6 MW"));
        __qtablewidgetitem33->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        inputTable->setItem(1, 1, __qtablewidgetitem33);
        QTableWidgetItem *__qtablewidgetitem34 = new QTableWidgetItem();
        __qtablewidgetitem34->setText(QString::fromUtf8("0.61"));
        inputTable->setItem(2, 0, __qtablewidgetitem34);
        QTableWidgetItem *__qtablewidgetitem35 = new QTableWidgetItem();
        __qtablewidgetitem35->setText(QString::fromUtf8("0.2-0.7"));
        __qtablewidgetitem35->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        inputTable->setItem(2, 1, __qtablewidgetitem35);
        QTableWidgetItem *__qtablewidgetitem36 = new QTableWidgetItem();
        __qtablewidgetitem36->setText(QString::fromUtf8("10"));
        inputTable->setItem(3, 0, __qtablewidgetitem36);
        QTableWidgetItem *__qtablewidgetitem37 = new QTableWidgetItem();
        __qtablewidgetitem37->setText(QString::fromUtf8("1-15"));
        __qtablewidgetitem37->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        inputTable->setItem(3, 1, __qtablewidgetitem37);
        QTableWidgetItem *__qtablewidgetitem38 = new QTableWidgetItem();
        __qtablewidgetitem38->setText(QString::fromUtf8("20"));
        inputTable->setItem(4, 0, __qtablewidgetitem38);
        QTableWidgetItem *__qtablewidgetitem39 = new QTableWidgetItem();
        __qtablewidgetitem39->setText(QString::fromUtf8("1-100"));
        __qtablewidgetitem39->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable);
        inputTable->setItem(4, 1, __qtablewidgetitem39);
        QTableWidgetItem *__qtablewidgetitem40 = new QTableWidgetItem();
        __qtablewidgetitem40->setText(QString::fromUtf8("107"));
        inputTable->setItem(5, 0, __qtablewidgetitem40);
        QTableWidgetItem *__qtablewidgetitem41 = new QTableWidgetItem();
        __qtablewidgetitem41->setText(QString::fromUtf8("~110 °C"));
        __qtablewidgetitem41->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        inputTable->setItem(5, 1, __qtablewidgetitem41);
        QTableWidgetItem *__qtablewidgetitem42 = new QTableWidgetItem();
        __qtablewidgetitem42->setText(QString::fromUtf8("2.9037"));
        inputTable->setItem(6, 0, __qtablewidgetitem42);
        QTableWidgetItem *__qtablewidgetitem43 = new QTableWidgetItem();
        __qtablewidgetitem43->setText(QString::fromUtf8("2.81419-2.98711 m"));
        __qtablewidgetitem43->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        inputTable->setItem(6, 1, __qtablewidgetitem43);
        QTableWidgetItem *__qtablewidgetitem44 = new QTableWidgetItem();
        __qtablewidgetitem44->setText(QString::fromUtf8("0.03"));
        inputTable->setItem(7, 0, __qtablewidgetitem44);
        QTableWidgetItem *__qtablewidgetitem45 = new QTableWidgetItem();
        __qtablewidgetitem45->setText(QString::fromUtf8("0.03-0.05 m"));
        __qtablewidgetitem45->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        inputTable->setItem(7, 1, __qtablewidgetitem45);

        QTableWidgetItem *__qtablewidgetitemMIO3 = new QTableWidgetItem();
        __qtablewidgetitemMIO3->setText(QString::fromUtf8(""));
        inputTable->setItem(8, 0, __qtablewidgetitemMIO3);
        QTableWidgetItem *__qtablewidgetitemMIO4 = new QTableWidgetItem();
		__qtablewidgetitemMIO4->setText(QString::fromUtf8(""));
		__qtablewidgetitemMIO4->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		inputTable->setItem(8, 1, __qtablewidgetitemMIO4);


        QTableWidgetItem *__qtablewidgetitem46 = new QTableWidgetItem();
        __qtablewidgetitem46->setText(QString::fromUtf8("70"));
        inputTable->setItem(9, 0, __qtablewidgetitem46);
        QTableWidgetItem *__qtablewidgetitem47 = new QTableWidgetItem();
        __qtablewidgetitem47->setText(QString::fromUtf8("~70%"));
        __qtablewidgetitem47->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        inputTable->setItem(9, 1, __qtablewidgetitem47);
        QTableWidgetItem *__qtablewidgetitem48 = new QTableWidgetItem();
        __qtablewidgetitem48->setText(QString::fromUtf8("0"));
        inputTable->setItem(10, 0, __qtablewidgetitem48);
        QTableWidgetItem *__qtablewidgetitem49 = new QTableWidgetItem();
        __qtablewidgetitem49->setText(QString::fromUtf8("0-0.05 m"));
        __qtablewidgetitem49->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        inputTable->setItem(10, 1, __qtablewidgetitem49);
        QTableWidgetItem *__qtablewidgetitem50 = new QTableWidgetItem();
        __qtablewidgetitem50->setText(QString::fromUtf8("0"));
        inputTable->setItem(11, 0, __qtablewidgetitem50);
        QTableWidgetItem *__qtablewidgetitem51 = new QTableWidgetItem();
        __qtablewidgetitem51->setText(QString::fromUtf8("0-4 Hz"));
        __qtablewidgetitem51->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        inputTable->setItem(11, 1, __qtablewidgetitem51);
        QTableWidgetItem *__qtablewidgetitem52 = new QTableWidgetItem();
        __qtablewidgetitem52->setText(QString::fromUtf8("0"));
        inputTable->setItem(12, 0, __qtablewidgetitem52);
        QTableWidgetItem *__qtablewidgetitem53 = new QTableWidgetItem();
        __qtablewidgetitem53->setText(QString::fromUtf8("~200 W/(m^2)K"));
        __qtablewidgetitem53->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        inputTable->setItem(12, 1, __qtablewidgetitem53);
        QTableWidgetItem *__qtablewidgetitem54 = new QTableWidgetItem();
        __qtablewidgetitem54->setText(QString::fromUtf8("300"));
        inputTable->setItem(13, 0, __qtablewidgetitem54);
        QTableWidgetItem *__qtablewidgetitem55 = new QTableWidgetItem();
        __qtablewidgetitem55->setText(QString::fromUtf8("~300 °C"));
        __qtablewidgetitem55->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        inputTable->setItem(13, 1, __qtablewidgetitem55);


        inputTable->setObjectName(QString::fromUtf8("inputTable"));
        QSizePolicy sizePolicy5(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(inputTable->sizePolicy().hasHeightForWidth());
        inputTable->setSizePolicy(sizePolicy5);
        inputTable->setMinimumSize(QSize(0, 300));
        inputTable->setMaximumSize(QSize(16777215, 420));
        inputTable->setAutoFillBackground(false);
        inputTable->setAutoScroll(true);
        inputTable->setDragEnabled(false);
        inputTable->setAlternatingRowColors(true);
        inputTable->horizontalHeader()->setStretchLastSection(true);

        connect(inputTable,SIGNAL(itemChanged(QTableWidgetItem *)), this, SLOT(updateModel(QTableWidgetItem *)));
	connect(inputTable, SIGNAL(itemChanged(QTableWidgetItem *)), this, SLOT(shiftSPloc(QTableWidgetItem *)));

     dock->setWidget(inputTable);
     addDockWidget(Qt::LeftDockWidgetArea, dock);
     viewMenu->addAction(dock->toggleViewAction());
     dock->setFloating(false);

//         gridLayout_2->addWidget(outputTable, 3, 0, 1, 4);
     dock = new QDockWidget(tr("Output"), this);
        outputTable = new QTableWidget(dock);
        if (outputTable->columnCount() < 1)
            outputTable->setColumnCount(1);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        __qtablewidgetitem->setText(QString::fromUtf8("VALUE"));
        outputTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        if (outputTable->rowCount() < 12)
            outputTable->setRowCount(12);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        __qtablewidgetitem1->setText(QString::fromUtf8("PEAK SURFACE TEMPERATURE (°C)"));
        outputTable->setVerticalHeaderItem(0, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        __qtablewidgetitem2->setText(QString::fromUtf8("THERMOCOUPLE X (°C)"));
        outputTable->setVerticalHeaderItem(1, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        __qtablewidgetitem3->setText(QString::fromUtf8("THERMOCOUPLE Y (°C)"));
        outputTable->setVerticalHeaderItem(2, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        __qtablewidgetitem4->setText(QString::fromUtf8("THERMOCOUPLE Z (°C)"));
        outputTable->setVerticalHeaderItem(3, __qtablewidgetitem4);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        __qtablewidgetitem5->setText(QString::fromUtf8("TCx estimated - TCx measured"));
        outputTable->setVerticalHeaderItem(4, __qtablewidgetitem5);
        QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
        __qtablewidgetitem6->setText(QString::fromUtf8("TCy estimated - TCy measured"));
        outputTable->setVerticalHeaderItem(5, __qtablewidgetitem6);
        QTableWidgetItem *__qtablewidgetitemO15 = new QTableWidgetItem();
        __qtablewidgetitemO15->setText(QString::fromUtf8("TCz estimated - TCz measured"));
        outputTable->setVerticalHeaderItem(6, __qtablewidgetitemO15);
        QTableWidgetItem *__qtablewidgetitem7 = new QTableWidgetItem();
        __qtablewidgetitem7->setText(QString::fromUtf8("ESTIMATED ENERGY IN TILE (MJ)"));
        outputTable->setVerticalHeaderItem(7, __qtablewidgetitem7);
        QTableWidgetItem *__qtablewidgetitemO17 = new QTableWidgetItem();
        __qtablewidgetitemO17->setText(QString::fromUtf8("MEASURED ENERGY IN TILE (MJ)"));
        outputTable->setVerticalHeaderItem(8, __qtablewidgetitemO17);
        QTableWidgetItem *__qtablewidgetitem8 = new QTableWidgetItem();
        outputTable->setItem(0, 0, __qtablewidgetitem8);
        QTableWidgetItem *__qtablewidgetitem9 = new QTableWidgetItem();
        outputTable->setItem(1, 0, __qtablewidgetitem9);
        QTableWidgetItem *__qtablewidgetitem10 = new QTableWidgetItem();
        outputTable->setItem(2, 0, __qtablewidgetitem10);
        QTableWidgetItem *__qtablewidgetitem11 = new QTableWidgetItem();
        outputTable->setItem(3, 0, __qtablewidgetitem11);
        QTableWidgetItem *__qtablewidgetitem12 = new QTableWidgetItem();
        outputTable->setItem(4, 0, __qtablewidgetitem12);
        QTableWidgetItem *__qtablewidgetitem13 = new QTableWidgetItem();
        outputTable->setItem(5, 0, __qtablewidgetitem13);
	QTableWidgetItem *__qtablewidgetitemO16 = new QTableWidgetItem();
        outputTable->setItem(6, 0, __qtablewidgetitemO16);
        QTableWidgetItem *__qtablewidgetitem14 = new QTableWidgetItem();
        outputTable->setItem(7, 0, __qtablewidgetitem14);
	QTableWidgetItem *__qtablewidgetitemO18 = new QTableWidgetItem();
	__qtablewidgetitemO18->setText(QString::fromUtf8("Clic to choose a pulse"));
	__qtablewidgetitemO18->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        outputTable->setItem(8, 0, __qtablewidgetitemO18);
	 QTableWidgetItem *__qtablewidgetitemO19 = new QTableWidgetItem();
        __qtablewidgetitemO19->setText(QString::fromUtf8("PEAK POWER DENSITY (MW/m^2)"));
        outputTable->setVerticalHeaderItem(9, __qtablewidgetitemO19);
	QTableWidgetItem *__qtablewidgetitemO20 = new QTableWidgetItem();
        outputTable->setItem(9, 0, __qtablewidgetitemO20);
    QTableWidgetItem *__qtablewidgetitemO60 = new QTableWidgetItem();
 	    __qtablewidgetitemO60->setText(QString::fromUtf8("NBI+RF HEATING ENERGY (MJ)"));
        outputTable->setVerticalHeaderItem(10, __qtablewidgetitemO60);
	QTableWidgetItem *__qtablewidgetitemO61 = new QTableWidgetItem();
        outputTable->setItem(10, 0, __qtablewidgetitemO61);
    QTableWidgetItem *__qtablewidgetitemO64 = new QTableWidgetItem();
 	    __qtablewidgetitemO64->setText(QString::fromUtf8("TOTAL HEATING ENERGY (MJ)"));
        outputTable->setVerticalHeaderItem(11, __qtablewidgetitemO64);
	QTableWidgetItem *__qtablewidgetitemO65 = new QTableWidgetItem();
        outputTable->setItem(11, 0, __qtablewidgetitemO65);

        outputTable->setObjectName(QString::fromUtf8("outputTable"));
        outputTable->setEnabled(true);
        QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(outputTable->sizePolicy().hasHeightForWidth());
        outputTable->setSizePolicy(sizePolicy1);
        outputTable->setMinimumSize(QSize(0, 200));
        outputTable->setMaximumSize(QSize(16777215, 600));
        outputTable->setAlternatingRowColors(true);
        outputTable->horizontalHeader()->setStretchLastSection(true);

	connect(outputTable,SIGNAL(itemClicked(QTableWidgetItem*)), this, SLOT(loadMeasuredEnergy()));

     dock->setWidget(outputTable);
     addDockWidget(Qt::LeftDockWidgetArea, dock);
     viewMenu->addAction(dock->toggleViewAction());
     dock->hide();
 }


void VitaMainWindow::shiftSPloc(QTableWidgetItem* item){

	if (item == inputTable->item(6,0) && theModel->isSPLoaded()){
		bool ok;
	 std::cout<<"APPELEEE"<< std::endl;
		inputTable->item(6,0)->text().toDouble(&ok);
		if (ok)
			theModel->shiftSP(inputTable->item(6,0)->text().toDouble());
		updateModel(NULL);
	}
}

void VitaMainWindow::viewToggleAxis()
{
  if(axisAct->isChecked()){

    /* Define the axis */
    axes = vtkAxesActor::New();
    widget = vtkOrientationMarkerWidget::New();

    double a[3];

    axes->SetShaftTypeToCylinder();
    axes->SetXAxisLabelText( "X" );
    axes->SetYAxisLabelText( "Y" );
    axes->SetZAxisLabelText( "Z" );
    axes->SetTotalLength( 20.0, 20.0, 20.0 );
    axes->SetCylinderRadius( 1.0 * axes->GetCylinderRadius() );
    axes->SetConeRadius    ( 0.7 * axes->GetConeRadius() );
    axes->GetNormalizedTipLength(a);
    axes->SetNormalizedTipLength(2.0*a[0],2.0*a[1],2.0*a[2]);

    vtkTextProperty* tprop = axes->GetXAxisCaptionActor2D()->GetCaptionTextProperty();
    tprop->ItalicOff();
    tprop->ShadowOff();
//     tprop->SetFontFamilyToCourier();
    tprop->SetColor(1,1,1);

    axes->GetYAxisCaptionActor2D()->GetCaptionTextProperty()->ShallowCopy( tprop );
    axes->GetZAxisCaptionActor2D()->GetCaptionTextProperty()->ShallowCopy( tprop );

  // this static function improves the appearance of the text edges
  // since they are overlaid on a surface rendering of the cube's faces
    vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();

  // set up the widget
//     widget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
    widget->SetOrientationMarker( axes );
    widget->SetInteractor( vtkWidget->GetInteractor() );
    widget->SetViewport( -.1, -.1, 0.27, 0.35 );
    widget->SetEnabled( 1 );
    widget->InteractiveOff();
//     widget->InteractiveOn();

//     cout << "Activating axis..." << std::endl;
  }
  else if( !axisAct->isChecked() ){
    axes->Delete();
    widget->SetEnabled( 0 );
    ren->GetRenderWindow()->Render();
//     widget->Delete();
//     cout << "Deactivating axis..." << std::endl;
  }
    ren->GetRenderWindow()->Render();

}

void VitaMainWindow::viewToggleContours()
{
  theInterface->toggleContourBar( contoursAct->isChecked() );
  if( contoursAct->isChecked() ) viewContour( contourType->currentIndex() );
  else viewContour( 0 );
}

void VitaMainWindow::viewContour( int index )
{
  if( contourType->count() > 1)
    theInterface->viewContour( index );
}

void VitaMainWindow::viewEnvironment( int index )
{
  if( environmentType->count() > 1)
    theInterface->viewEnvironment( index );
}


void VitaMainWindow::viewNewGraph()
{
  std::ostringstream size;
  size << "Graph-" << vGraphs.size();
//   graphTitle.append( size.str() );
  QString graphTitle( size.str().c_str() );
  vGraphs.push_back( new GraphWidget(graphTitle, this) );
  vGraphs.back()->readFile( fileName );

  vGraphs.back()->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  addDockWidget(Qt::BottomDockWidgetArea , vGraphs.back());
  if(this->isMaximized()){
    vGraphs.back()->setFloating(false);
    vGraphs.back()->setGeometry(this->width()*.55,
                                int(this->height()*.1),
                                600,
                                this->height()/10
                              );
  }
  else{
    vGraphs.back()->setFloating(false);
    vGraphs.back()->setGeometry(this->width(),
                                int(this->height()*.3),
                                100,
                                350
                               );
  }
  vGraphs.back()->show();
}

#ifdef USE_QCUSTOMPLOT
void VitaMainWindow::viewNewGraph(QCustomPlot* thePlot, bool view)
{
  std::ostringstream size;
  size << "Std Graph-" << vGraphs.size();
  QString graphTitle( size.str().c_str() );
  vGraphs.push_back( new GraphWidget(graphTitle, this, thePlot) );

   QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  vGraphs.back()->setSizePolicy(sizePolicy);
  vGraphs.back()->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea );
  addDockWidget(Qt::BottomDockWidgetArea , vGraphs.back());

  for(int i=0; i<vGraphs.size(); ++i){
    vGraphs[i]->setMinimumSize(200, 200);
    vGraphs[i]->setMaximumSize(this->centralWidget()->width(), this->height());
    if(vGraphs.size()>1)
     vGraphs[i]->resize(this->centralWidget()->width()/vGraphs.size(), this->height()*0.4);
    cout << "SIZE (" << i << ") = " << this->centralWidget()->width()/vGraphs.size() << ", " << this->height()*0.5 << std::endl;
  }
    this->centralWidget()->resize(centralWidget()->width(), this->height()*0.5);
  if(!view) vGraphs.back()->hide();

}
#endif


void VitaMainWindow::viewAnimate()
{
    if ( animateForwAct->isChecked() ) timer->start(1);
    else if ( animateBackAct->isChecked() ) timer->start(1);
    else if ( animatePauseAct->isChecked() ) timer->stop();
    else{
      timer->stop();
      animateTimeLine->setValue( 0 );
//       theInterface->stepFirst();
    }
}

void VitaMainWindow::viewRec()
{
  theInterface->viewRec( animateRecAct->isChecked() );
}

void VitaMainWindow::nextFrame()
{
  if ( animateForwAct->isChecked() ){
//     theInterface->stepForward();
    animateTimeLine->setValue( animateTimeLine->value()+1 );
  }
  else{
//     theInterface->stepBack();
    animateTimeLine->setValue( animateTimeLine->value()-1 );
  }
}

void VitaMainWindow::setFrame( int frame ){
  theInterface->setStep( frame );
#ifdef USE_QCUSTOMPLOT
  theModel->plotHeatFluenceDistribution2(inputPlot,frame);
  theModel->plotSolveHeatOutput(solvePlot,frame);
  theModel->plotPowerTime(powerPlot,frame);
  theModel->plotSPTime(SPPlot1,frame);
#endif
}

