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


#ifndef VITAMAINWINDOW_H
#define VITAMAINWINDOW_H

#include <QMainWindow>
#include <QCheckBox>
#include <QDialogButtonBox>
#include "model.h"

 class QHBoxLayout;
 class QAction;
 class QActionGroup;
 class QListWidget;
 class QSlider;
 class QMenu;
 class QComboBox;
 class QTextEdit;
 class QVTKWidget;
 class QTimer;
 class QDockWidget;
 class QTableWidget;
 class QGridLayout;
 class QProgressBar;
 class QPushButton;
 class QLabel;

#ifdef USE_QCUSTOMPLOT
 class QCustomPlot;
#else
 class Plot2D;
#endif

 class vtkRenderer;
 class vtkAxesActor;
 class vtkOrientationMarkerWidget;

 class VTKInterface;
 class GraphWidget;

 class Model;

 class VitaMainWindow : public QMainWindow
 {
     Q_OBJECT

 public:

     VitaMainWindow();
     ~VitaMainWindow();

 private slots:
     void fileNew();
     void fileOpen();
     void fileClose();
     void fileRefresh();
     void fileSave();
     void filePrint();
     void editSolve();
     void viewToggleAxis();
     void viewToggleContours();
     void viewContour(int);
     void viewEnvironment(int);
     void viewNewGraph();
#ifdef USE_QCUSTOMPLOT
     void viewNewGraph(QCustomPlot*, bool view=1);
#endif
     void viewAnimate();
     void viewRec();
     void nextFrame();
     void setFrame(int);
     void helpAbout();
     void solve();
     void changeModel(int);
     void changeDist(int);
     void cancelSim(){ runWasCancelled=true; }
     void updateModel(QTableWidgetItem* item);
     void loadSPFile();
     void loadSPJPF();
     void loadMeasuredEnergy();
     void unloadSP();
     void loadNBIFile();
     void loadNBIPPF();
     void unloadNBI();
     void loadICRHFile();
     void loadICRHPPF();
     void unloadICRH();
     void loadTCJPF();
     void loadAll();
     void exportTC();
     void unloadTCJPF();
     void tableselection(int);
     void ParamsEstim(int);
     void loadLoad();
     void unloadLoadFile();
     void optionSim();
     void OptionSimAccept();
     void loadOHM();
     void shiftSPloc(QTableWidgetItem* item);

 private:
     void init();
     void createInputPlot();
     void createOutputPlot();
     void extendOutputPlot();
     void createPowerPlot();
     void createSPPlot();
     void createActions();
     void createMenus();
     void createToolBars();
     void createStatusBar();
     void createDockWindows();
     void setUpVTKInterface();
     void load();
     void defaultInputTable();

     QHBoxLayout *hboxLayout;
     QTextEdit *textEdit;
     QVTKWidget *vtkWidget;
     QLabel * theImageWidget;
     QPixmap * theImageMap;

     QMenu *fileMenu;
     QMenu *editMenu;
     QMenu *viewMenu;
     QMenu *animationMenu;
     QMenu *helpMenu;
     QToolBar *fileToolBar;
     QToolBar *editToolBar;
     QToolBar *viewToolBar;
     QToolBar *animationToolBar;
     QAction *newAct;
     QAction *openAct;
     QAction *refreshAct;
     QAction *saveAct;
     QAction *printAct;
     QAction *nbiFileAct;
     QAction *nbiPPFAct;
     QAction *icrhFileAct;
     QAction *ohmPPFAct;
     QAction *SpNbiIcrhPPFAct;
     QAction *icrhPPFAct;
     QAction *spFileAct;
     QAction *spJPFAct;
     QAction *tcJPFAct;
     QAction *loadAct;
     QAction *exportTCAct;
     QLabel * modelTypeLabel;
     QComboBox * modelType;
     QComboBox * modelParams;
     QLabel * modelParamsLabel;
     QAction * modelParamsAction;
     QAction * modelParamsLabelAction;
     QLabel * modelDistLabel;
     QComboBox * modelDist;
     QAction *solveAct;
     QAction *axisAct;
     QAction *contoursAct;
     QAction *graphAct;
     QComboBox *contourType;
     QComboBox *environmentType;
     QAction *animateForwAct;
     QAction *animateBackAct;
     QAction *animateStopAct;
     QAction *animatePauseAct;
     QAction *animateRecAct;
     QSlider *animateTimeLine;
     QActionGroup *animateGroup;
     QAction *aboutAct;
     QAction *aboutQtAct;
     QAction *quitAct;

     QAction *Option;
     QCheckBox *flushBox;
     QCheckBox *TOPIBox;
     QCheckBox *shadowBox;
     QDialog *dlg;

     QTimer* timer;

     QDockWidget *dock;
     QWidget * dockContents;
     QGridLayout * gridLayout_2;
     QProgressBar * progressBar;
     QPushButton * solveButton;
     QPushButton * cancelButton;
#ifdef USE_QCUSTOMPLOT
     QCustomPlot * inputPlot;
     QCustomPlot * solvePlot;
     QCustomPlot * powerPlot;
     QCustomPlot * SPPlot1;
#else
     GraphWidget * inputPlot;
     GraphWidget * solvePlot;
     GraphWidget * powerPlot;
     GraphWidget * SPPlot1;
#endif

     QPushButton * flushButton;
     QPushButton * TOPIButton;
     QDialogButtonBox *buttonBox;

     vtkRenderer* ren;
     QTableWidget* inputTable;
     QTableWidget* outputTable;
     vtkAxesActor* axes;
     vtkOrientationMarkerWidget* widget;

     std::vector< GraphWidget* > vGraphs;

     QString fileName;
     VTKInterface * theInterface;
     bool fileOpened;

     bool isNBISPICRHLoaded;

     Model * theModel;
     bool runWasCancelled;
 };

#endif
