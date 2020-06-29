/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  <copyright holder> <email>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifdef _WIN32
	# define NOMINMAX
	#include <Windows.h>
	#include <ctype.h>
#else // POSIX
#  include <unistd.h>
#  define TRUE true
#  define FALSE false
#endif
/// writeshadow dans solve shadow, shadowPosition, reso
#include "model.h"

#include <iostream>
#include <map>

#include <cmath>
#include <limits>
#include <iomanip>
#include <algorithm>
#include<climits>

#include <QProgressDialog>
#include "boost/math/constants/constants.hpp"
#include "boost/math/distributions/skew_normal.hpp"
#include "boost/math/special_functions/erf.hpp"


#include "vitamainwindow.h"
#include "common.h"
#include "stdio.h"

#ifdef USE_QCUSTOMPLOT
#include "qcustomplot.h"
#endif

using std::endl;

#ifdef __cplusplus
extern "C" {
#endif
#ifdef DJET_INSTALL
#include "ppf.h"
#include "flush.h"
#endif
#ifdef __cplusplus
}
#endif

#ifdef DJET_INSTALL
#include "getfix/getfix.h"
#endif


const char* Model::cpFileName = "CFC-Cp.dat";
const char* Model::kparFileName = "CFC-Kpar.dat";
const char* Model::mknixFileName = "input_tile6.mknix";
const char* Model::outputFileName = "tile6.mec";
const char* Model::title = "tile6";

const char *  jpfNames[] = {
		"pf/wa-losr<xs",
		"db/d1-m23t7a<tmp:043",
		"db/d1-m13t7a<tmp:017",
		"db/d1-m13t7b<tmp:018"
	};

const char* dda[10] = {"NBI ","ICRH", "9ATP", "9AQP", "BOLO", "EFIT", "MAGN", "SCAL", "KG1V", "ELMA"};
const char* Dtype[9] = {"PTOT", "TMT6", "EDT6", "TOPI", "POHM", "IPLA", "BT", "LID3", "FREQ"};


Model::Model()
	:modelSimulation(new mknix::Simulation())
	,stepTime(0.05)//0.0625, 0.01 for high accuracy
	,dist(Triangle_Dist)
	,TL(0.1897)
	,nTiles(96)
	,OR(0.7) //0.7 0.35
	,ST(0)
	,SP(),SPFileLoad(false),SPStartTime(0), SPPulseTime(0)
	,NBI(),NBIFileLoad(false),NBIStartTime(0), NBIPulseTime(0)
	,ICRH(),ICRHFileLoad(false),ICRHStartTime(0), ICRHPulseTime(0)
	,TMT6(),TMT6FileLoad(false),TMT6StartTime(0)
	,load(),loadFileLoad(false),loadStartTime(0), loadPulseTime(0), loadCoord()
	, OHM(), OHMLoad(false), OHMStartTime(0), OHMPulseTime(0)
	,TOPI(), TOPIStartTime(0), useTOPI(false)
	,JPFTCLoad(false)
	,PDFcomputed(false), moduleProjection(), PDFProjection()
	,shot(0)
	,useFlush(false)
	,useShadow(false)
	,AutoLambdaS(false)
	,B(2.5)
	,analysisStartTime(0)
	,MaxTemp(0)
	,peakPowerDensity(0)
	,shadow(2000), shadowPosition()
	,SPshift(0)
	,flushStartTime(0), flushEndTime(0)
	,xShift(0), mirrorMesh(1) // Note that the mirror is the default option, as JET tile 6 models require it
///ALICIA
	,reso(1000)
	,qmid_avg(), qMidPlane(), fxavg(0)
	,lambdaS(), neLoad(false), BtLoad(false), IpLoad(false), fELMLoad(false)
	,fELMPPF(), nePPF(), BtPPF(), IPPF()
{}

Model::~Model(){}

void Model::init(){
	generateSimulationInputFiles();
	modelSimulation->inputFromFile((char*)mknixFileName);

	xCoords = QVector<double>::fromStdVector(modelSimulation->getInterfaceNodesCoords());
	xMin = *std::min_element(xCoords.constBegin(), xCoords.constEnd());
	xMax = *std::max_element(xCoords.constBegin(), xCoords.constEnd());
}

void Model::abort(){
	delete modelSimulation;
	remove(outputFileName);
	modelSimulation = new mknix::Simulation();
	this->init();
}

bool Model::isFlushAvailable(int jpn){
	int err=1;
#ifdef DJET_INSTALL
	flushQuickInit(&shot, &analysisStartTime, &err);
#endif
	if (err == 0)
	  return true;
	else
	  return false;
}

#ifdef USE_QCUSTOMPLOT
void Model::solve(QProgressDialog& progress,QCustomPlot* inputPlot,QCustomPlot* outputPlot,QCustomPlot* powerPlot,QCustomPlot* sp1Plot){
	double Pm, x_upper, x_lower, CDFu, CDFl;
	getXLimits(x_lower, x_upper);
	modelSimulation->setInitialTemperatures(ST);
	modelSimulation->init();

	//Get Node Coordinates Vector
	std::vector<double> x_coordinates = xCoords.toStdVector();

	//Do pulse steps
	std::vector<double> heatFluence(x_coordinates.size());

	//Initialize progress dialog
	int totalIterations = pulseIterations+cooldownIterations;
	TCx.clear();
	TCy.clear();
	TCz.clear();
	maxTemp.clear();
	timeV.clear();
	//	progress.setModal(true);

	//Prepare output plot
	double minValue,maxValue;
	minValue=ST;
	maxValue=ST;
	outputPlot->xAxis->setRange(analysisStartTime, analysisStartTime+(totalIterations*stepTime));

	double CDFtotal(0);
	double time = analysisStartTime;
	double Btimeav(0), Btimeap(0);
	double output[4];
	Energy = 0;
	int err;

	for(int i=0;i<pulseIterations;++i){
#ifdef DJET_INSTALL
		if (useShadow) flushQuickInit(&shot, &time, &err);
#endif

		if (time == Btimeav) time = Btimeap;
		else Btimeav = time;
		if (i == 0) time = analysisStartTime;
		calculateHeatFluence(time,x_coordinates,heatFluence);
		CDFtotal=0;
		cout << "HEATfluence vector = " ;
		for(int j=0; j<heatFluence.size(); ++j){
		  std::cout << heatFluence[j] << ", " ;
		  CDFtotal+=heatFluence[j];
		}
		cout << std::endl << "TOTAL HEAT FLUENCE = " << CDFtotal << std::endl;
		modelSimulation->solveStep(&heatFluence[0],(double*)output);
		TCx.push_back(output[0]);
		TCy.push_back(output[1]);
		TCz.push_back(output[2]);
		maxTemp.push_back(output[3]);
		timeV.push_back(time);
		progress.setValue(i);
		if(progress.wasCanceled()){
			abort();
			return;
		}
		//Update Input Graph
		if(inputPlot!=NULL){
			plotHeatFluenceDistribution(inputPlot,time);
		}
		//Update Output Graph
		if(output[3]>maxValue){
			maxValue=output[3];
		}

		if(outputPlot!=NULL){
			outputPlot->yAxis->setRange(minValue, maxValue*1.2);
			outputPlot->graph(0)->setData(timeV,maxTemp);
			outputPlot->graph(1)->setData(timeV,TCx);
			outputPlot->graph(2)->setData(timeV,TCy);
			outputPlot->graph(3)->setData(timeV,TCz);
			outputPlot->replot();
		}

		if(powerPlot!=NULL){
			plotPowerTime(powerPlot,i);
		}
		if(sp1Plot!=NULL){
			plotSPTime(sp1Plot,i);
		}
		switch(dist){
			case Triangle_Dist:
				CDFu = triangleCDF(time, x_upper);
				CDFl = triangleCDF(time, x_lower);
				break;
			case Skew_Normal_Dist:
				CDFu = skewNormalCDF(time, x_upper);
				CDFl = skewNormalCDF(time, x_lower);
				break;
//Eich
			case Eich_Dist:
				CDFu = EichCDF(time, x_upper);
				CDFl = EichCDF(time, x_lower);
				break;

			case Loaded_Dist:
				CDFu = loadFileCDF(time, x_upper);
				CDFl = loadFileCDF(time, x_lower);
				break;
			}


		Energy += WF/100 * (CDFu-CDFl)  * (x_upper + x_lower) * M_PI * stepTime;
		time+=stepTime;
		Btimeap = time;
	}

	//Do cooldown steps
	for (int i=0;i<heatFluence.size();++i){
		heatFluence[i]=0;
	}
	for(int i=0;i<cooldownIterations;++i){
		modelSimulation->solveStep(&heatFluence[0],(double*)output);
		progress.setValue(pulseIterations+i);
		TCx.push_back(output[0]);
		TCy.push_back(output[1]);
		TCz.push_back(output[2]);
		maxTemp.push_back(output[3]);
		timeV.push_back(time);

		//Update GraphS
		if(inputPlot!=NULL){
			plotHeatFluenceDistribution(inputPlot,time);
		}
		if(output[3]>maxValue){
			maxValue=output[3];
		}
		if(outputPlot!=NULL){
			outputPlot->yAxis->setRange(minValue, maxValue*1.2);
			outputPlot->graph(0)->setData(timeV,maxTemp);
			outputPlot->graph(1)->setData(timeV,TCx);
			outputPlot->graph(2)->setData(timeV,TCy);
			outputPlot->graph(3)->setData(timeV,TCz);
			outputPlot->replot();
		}

		if(powerPlot!=NULL){
			plotPowerTime(powerPlot,i+pulseIterations);

		}
		if(sp1Plot!=NULL){
			plotSPTime(sp1Plot,i+pulseIterations);
		}


		time+=stepTime;
	}

	TCxPeak = *std::max_element(TCx.begin(), TCx.end());
	TCyPeak = *std::max_element(TCy.begin(), TCy.end());
	TCzPeak = *std::max_element(TCz.begin(), TCz.end());
	MaxTemp = *std::max_element(maxTemp.begin(), maxTemp.end());
// 	if (useShadow) {writeShadow("/home/lvitton/Livio Vitton/Projects/shadow.csv", reso);
// 		for (std::map<float, double>::const_iterator it = shadowTime.begin(); it != shadowTime.end(); ++it){
// 			cout<<"time =\t"<<it->first<<"nombre =\t"<<it->second<<std::endl;
// 		}
// 	}
	progress.setValue(totalIterations);
	//writeHeatFlux(); ///alicia
	modelSimulation->endSimulation();
}
#else
void Model::solve(QProgressDialog& progress){
	double Pm, x_upper, x_lower, CDFu, CDFl;
	getXLimits(x_lower, x_upper);
	modelSimulation->setInitialTemperatures(ST);
	modelSimulation->init();

	//Get Node Coordinates Vector
	std::vector<double> x_coordinates = xCoords.toStdVector();

	//Do pulse steps
	std::vector<double> heatFluence(x_coordinates.size());

	//Initialize progress dialog
	int totalIterations = pulseIterations+cooldownIterations;
	TCx.clear();
	TCy.clear();
	TCz.clear();
	maxTemp.clear();
	timeV.clear();
	//	progress.setModal(true);

	double minValue,maxValue;
	minValue=ST;
	maxValue=ST;

	double CDFtotal(0);
	double time = analysisStartTime;
	double Btimeav(0), Btimeap(0);
	double output[4];
	Energy = 0;
	int err;

	for(int i=0;i<pulseIterations;++i){
#ifdef DJET_INSTALL
		if (useShadow) flushQuickInit(&shot, &time, &err);
#endif

		if (time == Btimeav) time = Btimeap;
		else Btimeav = time;
		if (i == 0) time = analysisStartTime;
		calculateHeatFluence(time,x_coordinates,heatFluence);
		CDFtotal=0;
		cout << "HEATfluence vector = " ;
		for(int j=0; j<heatFluence.size(); ++j){
		  std::cout << heatFluence[j] << ", " ;
		  CDFtotal+=heatFluence[j];
		}
		cout << std::endl << "TOTAL HEAT FLUENCE = " << CDFtotal << std::endl;
		modelSimulation->solveStep(&heatFluence[0],(double*)output);
		TCx.push_back(output[0]);
		TCy.push_back(output[1]);
		TCz.push_back(output[2]);
		maxTemp.push_back(output[3]);
		timeV.push_back(time);
		progress.setValue(i);
		if(progress.wasCanceled()){
			abort();
			return;
		}
		switch(dist){
			case Triangle_Dist:
				CDFu = triangleCDF(time, x_upper);
				CDFl = triangleCDF(time, x_lower);
				break;
			case Skew_Normal_Dist:
				CDFu = skewNormalCDF(time, x_upper);
				CDFl = skewNormalCDF(time, x_lower);
				break;
//Eich
			case Eich_Dist:
				CDFu = EichCDF(time, x_upper);
				CDFl = EichCDF(time, x_lower);
				break;

			case Loaded_Dist:
				CDFu = loadFileCDF(time, x_upper);
				CDFl = loadFileCDF(time, x_lower);
				break;
			}


		Energy += WF/100 * (CDFu-CDFl)  * (x_upper + x_lower) * M_PI * stepTime;
		time+=stepTime;
		Btimeap = time;
	}

	//Do cooldown steps
	for (int i=0;i<heatFluence.size();++i){
		heatFluence[i]=0;
	}
	for(int i=0;i<cooldownIterations;++i){
		modelSimulation->solveStep(&heatFluence[0],(double*)output);
		progress.setValue(pulseIterations+i);
		TCx.push_back(output[0]);
		TCy.push_back(output[1]);
		TCz.push_back(output[2]);
		maxTemp.push_back(output[3]);
		timeV.push_back(time);

		if(output[3]>maxValue){
			maxValue=output[3];
		}
		time+=stepTime;
	}

	TCxPeak = *std::max_element(TCx.begin(), TCx.end());
	TCyPeak = *std::max_element(TCy.begin(), TCy.end());
	TCzPeak = *std::max_element(TCz.begin(), TCz.end());
	MaxTemp = *std::max_element(maxTemp.begin(), maxTemp.end());
// 	if (useShadow) {writeShadow("/home/lvitton/Livio Vitton/Projects/shadow.csv", reso);
// 		for (std::map<float, double>::const_iterator it = shadowTime.begin(); it != shadowTime.end(); ++it){
// 			cout<<"time =\t"<<it->first<<"nombre =\t"<<it->second<<std::endl;
// 		}
// 	}
	progress.setValue(totalIterations);
	//writeHeatFlux(); ///alicia
	modelSimulation->endSimulation();
}
#endif

const char* Model::getOutputFilePath(){
	return outputFileName;
}

double Model::getStepTime(){
	return stepTime;
}

void Model::getXLimits(double& x_lower, double& x_upper){
	x_lower = xMin;
	x_upper = xMax;
}

void Model::getTimeLimits(double& t_lower, double& t_upper){
	t_lower=analysisStartTime;
	int totalIterations = pulseIterations+cooldownIterations;
	t_upper= analysisStartTime+(stepTime*(totalIterations-1));
}

void Model::getDistLimits(double& pdfLimit, double& cdfLimit){
	switch(dist){
	case Const_Dist:
		break;
	case Triangle_Dist:
		triangleLimits(pdfLimit,cdfLimit);
		break;
	case Skew_Normal_Dist:
		skewNormalLimits(pdfLimit,cdfLimit);
		break;
//Eich
	case Eich_Dist:
		EichLimits(pdfLimit,cdfLimit);
		break;

	case Loaded_Dist:
		loadFileLimits(pdfLimit,cdfLimit);
		break;
	default:
		break;
	}
}

#ifdef USE_QCUSTOMPLOT
void Model::plotHeatFluenceDistribution(QCustomPlot* plot, double time){
	if(time<0){
		time=analysisStartTime;
	}
	int res = 200;
	double x_lower,x_upper, Btime;
	int err;
	getXLimits(x_lower,x_upper);
	double step = (x_upper-x_lower)/res;
	QVector<double> xDivertor(res+1), yPDF(res+1,0), yCDF(res+1,0);

	//Evaluate heat distribution
	for(int i=0;i<res+1;++i){
		xDivertor[i]=x_lower+(i*step);
	}
	if(time<=analysisStartTime+(pulseIterations*stepTime)){
		switch(dist){
		case Const_Dist:
			break;
		case Triangle_Dist:
			Btime = time;
#ifdef DJET_INSTALL
			if (useShadow) flushQuickInit(&shot, &Btime, &err);
#endif
			for(int i=0;i<xDivertor.size();++i){
				yPDF[i] = trianglePDF(time,xDivertor[i]);
				yCDF[i] = triangleCDF(time,xDivertor[i]);
			}
			break;
		case Skew_Normal_Dist:
			Btime = time;
#ifdef DJET_INSTALL
			if (useShadow) flushQuickInit(&shot, &Btime, &err);
#endif
			for(int i=0;i<xDivertor.size();++i){
				yPDF[i] = skewNormalPDF(time,xDivertor[i]);
				yCDF[i] = skewNormalCDF(time,xDivertor[i]);
			}
			break;
//Eich
		case Eich_Dist:
			Btime = time;
#ifdef DJET_INSTALL
			if (useShadow) flushQuickInit(&shot, &Btime, &err);
#endif
			for(int i=0;i<xDivertor.size();++i){
				yPDF[i] = EichPDF(time,xDivertor[i]);
				yCDF[i] = EichCDF(time ,xDivertor[i]);
			}
			break;
		case Loaded_Dist:
			for(int i=0;i<xDivertor.size();++i){
				yPDF[i] = loadFilePDF(time,xDivertor[i]);
				yCDF[i] = loadFileCDF(time ,xDivertor[i]);
				//printf("time %g \n", time);
			}
			break;

		default:
			break;
		}
	}

	//Get Node Coordinates
	QVector<double> yCoords(xCoords.size(),0);


	//Plot Results
    plot->graph(0)->setData(xCoords,yCoords);
    plot->graph(1)->setData(xDivertor, yPDF);
    plot->graph(2)->setData(xDivertor, yCDF);

    plot->replot();

}

void Model::plotHeatFluenceDistribution2(QCustomPlot* plot, int step){
	double time;
	int totalIterations = pulseIterations + cooldownIterations;
	if(step<totalIterations){
		time=analysisStartTime+(step*stepTime);
	}else{
		time=analysisStartTime+(totalIterations*stepTime);
	}
	plotHeatFluenceDistribution(plot,time);
}

void Model::plotSolveHeatOutput(QCustomPlot* plot, int step){

	double time=0;
	if(timeV.size()!=0){
		if(step<timeV.size()){
			time=timeV[step];
		}else{
			time=timeV[timeV.size()-1];
		}
		QCPItemStraightLine* currTime = (QCPItemStraightLine*) plot->item(0);
		currTime->point1->setCoords(time,0);
		currTime->point2->setCoords(time,plot->yAxis->range().upper);

		plot->graph(0)->setData(timeV,maxTemp);
		plot->graph(1)->setData(timeV,TCx);
		plot->graph(2)->setData(timeV,TCy);
		plot->graph(3)->setData(timeV,TCz);
	}

	if(JPFTCLoad){
		plotJPFHeatOutput(plot,timeV.size()==0);
	}

	plot->replot();
}

void Model::plotJPFHeatOutput(QCustomPlot* plot, bool overrideLimits){
	plot->graph(4)->setData(JPFtimeV,JPFTC[0]);
	plot->graph(5)->setData(JPFtimeV,JPFTC[1]);
	plot->graph(6)->setData(JPFtimeV,JPFTC[2]);
	if (TMT6FileLoad) plot->graph(7)->setData(JPFtimeV,TMT6V);
	plot->xAxis->setRange(analysisStartTime,JPFtimeV[JPFtimeV.size()-1]);

	if(overrideLimits){
		plot->xAxis->setRange(JPFtimeV[0],JPFtimeV[JPFtimeV.size()-1]);
		plot->yAxis->setRange(JPFminTemp, JPFmaxTemp*1.2);
	}
}


void Model::plotPowerAndSP(QCustomPlot* powerPlot,QCustomPlot* sp1Plot){
	int totalIterations = pulseIterations+cooldownIterations;
	QVector<double> timeV(totalIterations);
	QVector<double> PmV(totalIterations);
	QVector<double> TPV(totalIterations);
	QVector<double> NBIV(totalIterations);
	QVector<double> ICRHV(totalIterations);
	QVector<double> OHMV(totalIterations);
	QVector<double> SPV(pulseIterations);

	double currNBI,currICRH,currOHM,currSP,TP,Pm,x0,x1,scale;
	double PmMax=0;

	double time = analysisStartTime;
	for(int i=0;i<totalIterations;++i){
		switch(dist){
			case Triangle_Dist:

				triangleParams(time,currNBI,currICRH,currOHM,currSP,TP,Pm,x0,x1);
				break;
			case Skew_Normal_Dist:
				powerPlot->show();

				sp1Plot->show();

				skewNormalParams(time,currNBI,currICRH,currOHM,currSP,TP,Pm,scale);
				break;
//Eich
			case Eich_Dist:
				EichParams(time,currNBI,currICRH,currOHM,currSP,TP,Pm,scale);
				break;
			case Loaded_Dist:

				return;
		}
		timeV.push_back(time);
		PmV.push_back(Pm);
		TPV.push_back(TP);
		NBIV.push_back(currNBI);
		ICRHV.push_back(currICRH);
		OHMV.push_back(currOHM);
		if(i<pulseIterations){
			SPV.push_back(currSP);
		}

		if(Pm>PmMax){
			PmMax=Pm;
		}

		time+=stepTime;
	}

	PmMax = *std::max_element(NBIV.begin(), NBIV.end());
	powerPlot->yAxis->setRange(0,PmMax*1.2);
	powerPlot->graph(0)->setData(timeV,PmV);
	powerPlot->graph(1)->setData(timeV,TPV);
	powerPlot->graph(2)->setData(timeV,NBIV);
	powerPlot->graph(3)->setData(timeV,ICRHV);
	powerPlot->graph(4)->setData(timeV,OHMV);

	sp1Plot->graph(0)->setData(QVector<double>(xCoords.size(),analysisStartTime),xCoords);
	timeV.remove(pulseIterations,cooldownIterations);
	sp1Plot->graph(1)->setData(timeV,SPV);

	sp1Plot->replot();
	powerPlot->replot();

}

void Model::plotPowerTime(QCustomPlot* plot, int step){
	int totalIterations = pulseIterations+cooldownIterations;
	double time=0;
	if(step<totalIterations){
		time=analysisStartTime+(step*stepTime);
	}else{
		time=analysisStartTime+((totalIterations)*stepTime);
	}
	QCPItemStraightLine* currTime = (QCPItemStraightLine*) plot->item(0);
	currTime->point1->setCoords(time,0);
	currTime->point2->setCoords(time,plot->yAxis->range().upper);

	plot->replot();
}

void Model::plotSPTime(QCustomPlot* plot1,int step){
	int totalIterations = pulseIterations+cooldownIterations;
	double time=0;
	if(step<totalIterations){
		time=analysisStartTime+(step*stepTime);
	}else{
		time=analysisStartTime+(totalIterations*stepTime);
	}

	QCPItemStraightLine* currTime = (QCPItemStraightLine*) plot1->item(0);
	currTime->point1->setCoords(time,0);
	currTime->point2->setCoords(time,plot1->yAxis->range().upper);

	plot1->replot();

}
#endif


bool Model::setSP(double value){
	if(SPFileLoad){
		return false;
	}else{
		SP[0]=value;
		return true;
	}
}

void Model::loadSPfile(const char* path){
	readParamInputFile(path,SP,SPStartTime,SPFileLoad, SPPulseTime);
	updatePulseIterations();
}

void Model::loadSPJPF(int jpn){
	readParamJPF(jpn,SPSIGNAL,SP,SPStartTime,SPFileLoad, SPPulseTime);
	updatePulseIterations();
}

void Model::shiftSP(double shift){
	cout<<shift<<std::endl;
	for (std::map<double, double >::iterator it = SP.begin(); it != SP.end(); ++it){
		it->second += shift-SPshift;
	}
	SPshift = shift;
#ifdef DJET_INSTALL
	if (useFlush && PDFcomputed) computePDF();
#endif
}

bool Model::isSPLoaded(){
	return SPFileLoad;
}

bool Model::isOHMLoaded(){
	return OHMLoad;
}

void Model::unloadSP(){
	SP.clear();
	SPFileLoad=false;
	SPStartTime=0;
	updateStartTime();
}

bool Model::setNBI(double value){
	if(NBIFileLoad){
		return false;
	}else{
		NBI[0]=value;
		return true;
	}
}

void Model::loadNBIfile(const char* path){
	readParamInputFile(path,NBI,NBIStartTime,NBIFileLoad, NBIPulseTime);
	updatePulseIterations();
}

void Model::loadTOPI(){
	double time;
	readParamPPF(shot, BOLOSIGNAL, TOPI, TOPIStartTime, useTOPI, TOPITYPE, time);
}

void Model::calculateLS(){
	double timeI, timeF;
	double lambda, s, lambdaDef, sDef;
	double db_drSP, db_drmid;
	double felmMean, nbiMean, icrhMean, neMean, ohmMean, btMean, ipMean, topiMean;
	bool ok, okohm, oknbi, okicrh;


	int err;
	long int np = 1;
	double Rmid1, Rmid2, SPloc;
	long int ierr;
	double expansion;
	double Bz, Br, sintheta;
	double alphar, alphaz;

	double x_upper = 5;
	double module, erfparam1, expparam1;

	double r1 = 280.425;
	double r2 = 280.703;
	double z1 = -171.158;
	double z2 = -171.158;

	double aL, bL, cL, dL, eL, kL;
	double aS, bS, cS, dS, eS, kS;


	ok = false;
	std::map<double,double> Bt, Ip, ne, fELM, nbi, icrh, ohm, sp, topi;
	readParamPPF(shot,NBISIGNAL,nbi,NBIStartTime,oknbi, PTOTTYPE, NBIPulseTime);
	readParamPPF(shot, OHMSIGNAL, ohm, OHMStartTime, okohm, POHMTYPE, OHMPulseTime);
	readParamPPF(shot, ICRHSIGNAL, icrh, timeI, okicrh, PTOTTYPE, timeF);
	if (okicrh || oknbi|| okohm) readParamPPF(shot, BTSIGNAL, Bt, timeI, ok, BTTYPE, timeF);
	if(ok) readParamPPF(shot, IPSIGNAL, Ip, timeI, ok, IPTYPE, timeF);
	if(ok) readParamPPF(shot, NESIGNAL, ne, timeI, ok, NETYPE, timeF);
	if(ok) readParamPPF(shot, FELMSIGNAL, fELM, timeI, ok, FELMTYPE, timeF);
	//if(ok) readParamInputFile("/home/lvitton/Livio Vitton/Projects/fELM_90287.txt", fELM, timeI, ok, timeF);
	if(ok) readParamJPF(shot,SPSIGNAL,sp,SPStartTime,ok, SPPulseTime);
	if(ok) {
		readParamPPF(shot, BOLOSIGNAL, topi, TOPIStartTime, ok, TOPITYPE, timeF);
		AutoLambdaS = true;
	}else{
		AutoLambdaS = false;
		return;
	}

	if (newScaling){


		aL = -0.24;
		bL = 0.52;
		cL = 0.4506;
		dL = 0.3976;
		eL = 0.2031;
		kL = 1.6;

		aS = 0.74;
		bS = -0.83;
		cS = 0.0984;
		dS = -0.7684;
		eS = 0.5422;
		kS = 1.6;
	}else{
		aL = -0.24;
		bL = 0.52;
		cL = -1;
		dL = 0.023;
		eL = 0.15;
		kL = 1.6;

		aS = 0.74;
		bS = -0.83;
		cS = -0.6;
		dS = 0.052;
		eS = -0.11;
		kS = 1.6;
	}

	updatePulseIterations();

	if (!okicrh) icrh[0] = 0;
	if (!oknbi) nbi[0] = 0;
	if (!okohm) ohm[0] = 0;

	felmMean = meanDoubleD(fELM, 1, 5);
	btMean = meanDoubleD(Bt, 1, 1.8);
	ipMean = meanDoubleD(Ip, -1e-6, 1.2);
	nbiMean = meanDoubleD(nbi, 1e-6, 12.2);
	icrhMean = meanDoubleD(icrh, 1e-6, 0.4);
	ohmMean = meanDoubleD(ohm, 1e-6, 0);
	neMean = meanDoubleD(ne, 1e-20, 0.45);
	topiMean = meanDoubleD(topi, 1e-6, 0.5);

// 	lambdaDef = 1.6*pow(ipMean ,-0.24)*pow(btMean, 0.52)*pow(neMean, -1)*pow((nbiMean + icrhMean + ohmMean - topiMean),0.023)*pow(felmMean,0.15);
// 	if (lambdaDef < 0.005*1e3) lambdaDef = 0.005 * 1e3;
// 	sDef = 1.6*pow(ipMean,0.74)*pow(btMean, -0.83)*pow(neMean, -0.6)*pow((nbiMean+icrhMean + ohmMean - topiMean),0.052)*pow(felmMean,-0.11);

	lambdaDef = kL*pow(ipMean ,aL)*pow(btMean, bL)*pow(neMean, cL)*pow((nbiMean + icrhMean + ohmMean - topiMean),dL)*pow(felmMean,eL);
	if (lambdaDef < 0.005*1e3) lambdaDef = 0.005 * 1e3;
	sDef = kS*pow(ipMean,aS)*pow(btMean, bS)*pow(neMean, cS)*pow((nbiMean+icrhMean + ohmMean - topiMean),dS)*pow(felmMean,eS);

	printf("lambdaDef = %g\nSDef = %g\n", lambdaDef, sDef);
	printf("fELM = %g\n", felmMean);
	printf("Bt = %g\n", btMean);
	printf("Ip = %g\n", ipMean);
	printf("NBI = %g\n",nbiMean);
	printf("ICRH = %g\n", icrhMean);
	printf("TOPI = %g\n", topiMean);
	printf("ohm = %g\n", ohmMean);
	printf("ne = %g\n", neMean);


	//+10 because flush can return time > analysisStartTime + stepTime * PulseIt
	for (double i = analysisStartTime; i < analysisStartTime + (pulseIterations+10) * stepTime; i += stepTime){
		timeI = i;
// 		flushQuickInit(&shot, &timeI, &err);
// 		SPloc = mknix::interpolate1D(i, sp) + SPshift;
// // 		r1 = getRT6(SPloc-1e-3);
// // 		r2 = getRT6(SPloc + 1e-3);
// // 		z1 = getZT6(SPloc - 1e-3);
// // 		z2 = getZT6(SPloc + 1e-3);
// // 		Flush_getMidPlaneProjRight(&np, &r1, &z1, &Rmid1, &ierr);
// // 		Flush_getMidPlaneProjRight(&np, &r2, &z2, &Rmid2, &ierr);
// // 		if(Rmid1-Rmid2 < 0)
// // 			expansion = std::sqrt(pow(r1 - r2, 2) + pow(z1-z2, 2))/std::abs(Rmid1-Rmid2);
// // 		else expansion = 4;
// 		z1 = getZT6(SPloc);
// 		r1 = getRT6(SPloc);
// 		z2 = 0;
// 		Flush_getdpsidr(&np, &r1, &z1, &db_drSP, &ierr);
// 		Flush_getMidPlaneProjRight(&np, &r1, &z1, &Rmid1, &ierr);
// 		Flush_getdpsidr(&np, &Rmid1, &z2, &db_drmid, &ierr);
// 		Flush_getBr(&np, &r1, &z1, &Br, &ierr);
// 		Flush_getBz(&np, &r1, &z1, &Bz, &ierr);
// 		getNormalT6( alphar, alphaz, SPloc);
// 		sintheta = (Br*alphar + Bz * alphaz)/std::sqrt(pow(Br,2) + pow(Bz,2));
//
// 		expansion = ((Rmid1 * db_drmid)/(r1 * db_drSP))/sintheta * std::sin(std::atan(Bz/Br));
// 		cout<<"expansion="<<expansion<<std::endl;
		if (mknix::interpolate1D(i, fELM) != 0){

			lambda = kL*pow(mknix::interpolate1D(i,Ip) * (-1e-6),aL)*pow(mknix::interpolate1D(i, Bt), bL)*pow(mknix::interpolate1D(i, ne) * 1e-20, cL)*pow((mknix::interpolate1D(i, nbi)+mknix::interpolate1D(i, icrh)+mknix::interpolate1D(i, ohm)- mknix::interpolate1D(i, topi))*1e-6,dL)*pow(mknix::interpolate1D(i, fELM),eL);
			s = kS*pow(mknix::interpolate1D(i, Ip)*(-1e-6),aS)*pow(mknix::interpolate1D(i, Bt), bS)*pow(mknix::interpolate1D(i, ne)*1e-20, cS)*pow((mknix::interpolate1D(i, nbi)+mknix::interpolate1D(i, icrh)+mknix::interpolate1D(i, ohm) - mknix::interpolate1D(i, topi))*1e-6,dS)*pow(mknix::interpolate1D(i, fELM),eS);
			printf("time: %g\nlambda = %g\nS = %g\n",i, lambda*1e-3, mknix::interpolate1D(i, fELM));

		}else{

			lambda = kL*pow(mknix::interpolate1D(i,Ip) * (-1e-6),aL)*pow(mknix::interpolate1D(i, Bt), bL)*pow(mknix::interpolate1D(i, ne) * 1e-20, cL)*pow((mknix::interpolate1D(i, nbi)+mknix::interpolate1D(i, icrh)+mknix::interpolate1D(i, ohm)- mknix::interpolate1D(i, topi))*1e-6,dL)*pow(felmMean,eL);
			s = kS*pow(mknix::interpolate1D(i, Ip)*(-1e-6),aS)*pow(mknix::interpolate1D(i, Bt), bS)*pow(mknix::interpolate1D(i, ne)*1e-20, dS)*pow((mknix::interpolate1D(i, nbi)+mknix::interpolate1D(i, icrh)+mknix::interpolate1D(i, ohm) - mknix::interpolate1D(i, topi))*1e-6,dS)*pow(felmMean,eS);
			printf("time: %g\nlambda = %g\nS = %g\n",i, lambda*1e-3, s*1e-3);


		}
		if (lambda > 0 && s > 0 && lambda < 0.1*1e3 && s < 0.1*1e3 && lambda > 0.005*1e3){
			lambda_qmap.insert(std::pair<double,double>(i,lambda * 1e-3));
			Smap.insert(std::pair<double,double>(i, s * 1e-3));
			cout<<lambda*1e-3/expansion<<"\t"<<s*1e-3/expansion<<"\t"<<i<<std::endl;
			printf("time: %g\nlambda = %g\nS = %g\n",i, lambda*1e-3, s*1e-3);
		}else if (lambda < 0.005*1e3 && lambda > 0 && s > 0 && lambda < 0.1*1e3 && s < 0.1*1e3){
			lambda_qmap.insert(std::pair<double,double>(i,0.005));
			Smap.insert(std::pair<double,double>(i, s * 1e-3));
		}else{
			lambda_qmap.insert(std::pair<double,double>(i, lambdaDef*1e-3));
			Smap.insert(std::pair<double,double>(i, sDef*1e-3));
		}


		/*lambda_qmap.insert(std::pair<double,double>(200, lambdaDef*1e-3));
		Smap.insert(std::pair<double,double>(200, sDef*1e-3));*///Add default value at the end of the map because of a strange call when computing the shadow


// 		if (i>analysisStartTime + 130 * stepTime && i<analysisStartTime + 145 * stepTime){
// 		printf("lambda = %g\nS = %g\n", lambda*1e-3, s*1e-3);
// 		printf("fELM = %g\n", mknix::interpolate1D(i, fELM));
// 		printf("fELM Average = %g\n", felmMean);
// 		printf("Bt = %g\n", mknix::interpolate1D(i, Bt));
// 		printf("Ip = %g\n", mknix::interpolate1D(i, Ip));
// 		printf("NBI = %g\n", mknix::interpolate1D(i, nbi));
// 		printf("ICRH = %g\n", mknix::interpolate1D(i, icrh));
// 		printf("TOPI = %g\n", mknix::interpolate1D(i, topi));
// 		printf("ohm = %g\n", mknix::interpolate1D(i, ohm));
// 		printf("ne = %g\n", mknix::interpolate1D(i, ne));
// 		}
	}

}

double Model::meanDoubleD(std::map<double,double> Map, double factor, double standardLim){
	double count, value;
	count = 0;
	value = 0;
	for (i_tDoubleDouble it = Map.begin(); it != Map.end(); ++it){
		if (it->second*factor > standardLim){
			value += factor * it->second;
			count  += 1;
		}
	}
	if (count == 0) return 0;
	else return value/count;

}

void Model::loadNBIPPF(int jpn){
	readParamPPF(jpn,NBISIGNAL,NBI,NBIStartTime,NBIFileLoad, PTOTTYPE, NBIPulseTime);
	updatePulseIterations();
}

void Model::loadOHMPPF(int jpn){
	readParamPPF(jpn,OHMSIGNAL,OHM,OHMStartTime,OHMLoad, POHMTYPE, OHMPulseTime);
	updatePulseIterations();
}

bool Model::isNBILoaded(){
	return NBIFileLoad;
}

void Model::unloadNBI(){
	NBI.clear();
	NBIFileLoad=false;
	NBIStartTime=0;
	updateStartTime();
}

void Model::unloadLoadFile(){
	load.clear();
	loadFileLoad = FALSE;
	loadStartTime = 0;
	updateStartTime();
}


bool Model::setICRH(double value){
	if(ICRHFileLoad){
		return false;
	}else{
		ICRH[0]=value;
		return true;
	}
}

bool Model::isLoadLoaded(){
	return loadFileLoad;
}

bool Model::isMeasuredEnergyLoaded(){
	return EnergyFileLoad;
}

void Model::loadICRHfile(const char* path){
	readParamInputFile(path,ICRH,ICRHStartTime,ICRHFileLoad, ICRHPulseTime);
	updatePulseIterations();
}

void Model::loadLoadFile(const char* path, const char* coordPath, bool coordFile){
	readLoadInputFile(path, coordPath, coordFile, load,loadStartTime,loadFileLoad);
	updatePulseIterations();
}

void Model::loadICRHPPF(int jpn){
	readParamPPF(jpn,ICRHSIGNAL,ICRH,ICRHStartTime,ICRHFileLoad, PTOTTYPE, ICRHPulseTime);
	updatePulseIterations();
}

void Model::loadMeasuredEnergy(int jpn){
	double EnergyTime, time;
	readParamPPF(jpn,ENERGYSIGNAL,mEnergy,EnergyTime,EnergyFileLoad, ENERGYTYPE, time);
	if (mEnergy.size()<2)measuredEnergy = mEnergy.begin()->second;
	else{
		std::vector<double> vec;
		for (i_tDoubleDouble it=mEnergy.begin(); it!=mEnergy.end(); ++it){
			  vec.insert(vec.end(), it->second);
		}
		measuredEnergy = *std::max_element(vec.begin(), vec.end());
	}
}


void Model::loadTMT6PPF(int jpn){
	double time;
	readParamPPF(jpn,TMT6SIGNAL,TMT6,TMT6StartTime,TMT6FileLoad, TMT6TYPE, time);
	if (TMT6FileLoad){
		for(int i=0;i<JPFtimeV.size();++i){
			TMT6V.append(mknix::interpolate1D(JPFtimeV[i], TMT6));
		}
	}else{
		TMT6V[0] = 0;
	}
}

bool Model::isICRHLoaded(){
	return ICRHFileLoad;
}

void Model::unloadICRH(){
	ICRH.clear();
	ICRHFileLoad=false;
	ICRHStartTime=0;
	updateStartTime();
}

void Model::loadTCJPF(int jpn){
	int err=0;
    char title[53], units[11];
    int size =0;
    JPFTC->clear();
    JPFTCLoad=FALSE;

#ifdef DJET_INSTALL
	getnwds_((char *)jpfNames[TC_XSIGNAL], &jpn, &size, &err, (long) strlen(jpfNames[TC_XSIGNAL]));

	if( err || size <= 0){
		printf("[ERROR] Error reading Signal ID:%s\n ",jpfNames[TC_XSIGNAL]);
		return;
	}else{
		float *data =NULL;
		data = (float*) malloc(size*sizeof(float));
		float *time = NULL;
		time = (float*) malloc(size*sizeof(float));

		if(data==NULL || time==NULL){
			printf("[ERROR] Error allocating memory JPF:%s\n",jpfNames[TC_XSIGNAL]);
			free(data);
			free(time);
			return;
		}else{
			bool initialize=true;
			for(int i=0;i<3;++i){
				getdat_((char *)jpfNames[TC_XSIGNAL+i], &jpn, data, time, &size, title, units, &err,
						  (long) strlen(jpfNames[TC_XSIGNAL+i]), sizeof(title) - 1l, sizeof(units) - 1l);
				if(err){
					printf("\n[ERROR] Error retrieving Signal data ID: %s\n",jpfNames[TC_XSIGNAL+i]);

				}else{
					JPFTC[i].resize(size);
					for(int j=0;j<size;++j){
						JPFTC[i][j]=data[j];
						if(initialize){
							initialize=false;
							JPFmaxTemp=data[j];
							JPFminTemp=data[j];
						}
						if(data[j]>JPFmaxTemp){
							JPFmaxTemp=data[j];
						}
						if(data[j]<JPFminTemp){
							JPFminTemp=data[j];
						}
					}
				}
			}
			TCxJPFmaxTemp = *std::max_element(JPFTC[0].begin(),JPFTC[0].end());
			TCyJPFmaxTemp = *std::max_element(JPFTC[1].begin(),JPFTC[1].end());
			TCzJPFmaxTemp = *std::max_element(JPFTC[2].begin(),JPFTC[2].end());
			if(!err){
				JPFtimeV.resize(size);
				JPFTCLoad = TRUE;
				for(int i=0;i<size;++i){
					JPFtimeV[i]=time[i];
				}
			}
		}
		free(data);
		free(time);

	}

#endif // DJET_INSTALL
}

bool Model::isTCJPFLoaded(){
	return JPFTCLoad;
}

void Model::exportTCData(std::string simTCFile,bool& simExported, std::string loadTCFile, bool& loadExported){

	if(simTCFile.empty()){
		simExported=false;
		printf("No file for simulated thermocouple data\n");
	}else{
		simExported=writeTCtoFile(simTCFile,timeV,TCx,TCy,TCz);
	}

// 	if(loadTCFile.empty()){
// 		loadExported=false;
// 		printf("No file for loaded thermocouple data\n");
// 	}else{
// 		loadExported=writeTCtoFile(loadTCFile,JPFtimeV,JPFTC[0],JPFTC[1],JPFTC[2]);
// 	}

}

void Model::unloadTCJPF(){
	for(int i=0;i<3;++i){
		JPFTC[i].clear();
	}
	TMT6V.clear();
	TMT6FileLoad = FALSE;
	JPFtimeV.clear();
	JPFTCLoad=false;
}


void Model::updateStartTime(){
	if(SPStartTime!=0 && NBIStartTime!=0){
		analysisStartTime=std::min(SPStartTime,NBIStartTime);
	}else{
		analysisStartTime=std::max(SPStartTime,NBIStartTime);
	}

	if(analysisStartTime!=0 && ICRHStartTime!=0){
		analysisStartTime=std::min(analysisStartTime,ICRHStartTime);
	}else{
		analysisStartTime=std::max(analysisStartTime,ICRHStartTime);
	}
	if(analysisStartTime!=0 && OHMStartTime!=0){
		analysisStartTime=std::min(analysisStartTime,OHMStartTime);
	}else{
		analysisStartTime=std::max(analysisStartTime,OHMStartTime);
	}

	if (loadFileLoad) analysisStartTime = loadStartTime;


	printf("Analysis time %g\n",analysisStartTime);
	printf("SP time %g\n",SPStartTime);
	printf("NBI time %g\n",NBIStartTime);
	printf("ICRH time %g\n",ICRHStartTime);
	printf("OHM time %g\n",OHMStartTime);
}

void Model::updatePulseIterations(){
	if(SPPulseTime!=0 && NBIPulseTime!=0){
		pulseTime=std::max(SPPulseTime,NBIPulseTime);
	}

	if(pulseTime!=0 && ICRHPulseTime!=0){
		pulseTime=std::max(pulseTime,ICRHPulseTime);
	}
	if (pulseTime!=0 && loadPulseTime!=0){
		pulseTime = std::max(pulseTime,loadPulseTime);
	}
	if (pulseTime!=0 && OHMPulseTime!=0){
		pulseTime = std::max(pulseTime,OHMPulseTime);
	}
	double stepTime = getStepTime();
	pulseIterations = pulseTime/stepTime+1;
}

void Model::generateSimulationInputFiles(){
	generateCpFile();
	generateKparFile();
	generateModelFile();
	generateMknix();
}

void Model::generateCpFile(){
	 std::ofstream file;
	 file.open (cpFileName);
	 file << "25\t704\n";
	 file << "200\t1174\n";
	 file << "300\t1362\n";
	 file << "400\t1508\n";
	 file << "500\t1620\n";
	 file << "600\t1706\n";
	 file << "700\t1777\n";
	 file << "800\t1836\n";
	 file << "900\t1883\n";
	 file << "1000\t1924\n";
	 file << "1200\t2050\n";
	 file << "2000\t2200\n";
	 file.close();
}

void Model::generateKparFile(){
	std::ofstream file;
	file.open (kparFileName);

	file << "20\t318\n";
	file << "100\t286\n";
	file << "200\t253\n";
	file << "300\t225\n";
	file << "400\t202\n";
	file << "500\t183\n";
	file << "600\t166\n";
	file << "700\t153\n";
	file << "800\t141\n";
	file << "900\t130\n";
	file << "1000\t122\n";
	file << "1100\t114\n";
	file << "1200\t107\n";
	file << "1300\t101\n";
	file << "1400\t96\n";
	file << "1500\t91\n";
	file << "1600\t87\n";
	file << "1700\t83\n";
	file << "1800\t79\n";
	file << "1900\t76\n";
	file << "2000\t73\n";
	file << "3000\t70\n";

	file.close();
}

void Model::generateMknix(){
	std::ofstream file;
	file.open (mknixFileName);

	file << "TITLE "<< title << "\n";
	file << "\n";
	file << "\n";
	file << "DIMENSION 2\n";
	file << "\n";
	file << "MATERIALS // Total number of materials:\n";
	file << "  THERMAL 1 1348 70 0 1800 // material #, Capacity, Conductivity, Thermal expansion and density\n";
	file << "  FILES\n";
	file << "    CAPACITY 1 "<< cpFileName << "\n";
	file << "    CONDUCTIVITY 1 " << kparFileName << "\n";
	file << "  ENDFILES\n";
	file << "ENDMATERIALS\n";
	file << "\n";
	file << "\n";
	file << "SYSTEM tiles\n";
	file << "  FLEXBODIES\n";
	file << "    FEMESH tile6\n";
	file << "      FORMULATION THERMAL\n";
	file << "      METHOD RPIM\n";
	file << "      BOUNDARY CLOCKWISE\n";
	file << "      MESH\n";
	file << "      1 1 4.5\n";
	file << "      TRIANGLES " << tileFileName << "\n";
	file << "    ENDFEMESH\n";
	file << "  ENDFLEXBODIES\n";
	file << "\n";
	if(mirrorMesh)
		file << "MIRROR x\n";
	file << "SHIFT "<< xShift <<" 0 0\n";
	file << "\n";
	file << "  JOINTS\n";
	file << "  ENDJOINTS\n";
	file << "\n";
	file << "  LOADS\n";

	printModelNodeLoads(file);

	file << "\n";
	file << "ENDLOADS\n";
	file << "\n";
	file << "ENDSYSTEM\n";
	file << "\n";
	file << "ANALYSIS\n";
	file << "  THERMALDYNAMIC\n";
	file << "    EPSILON 1E-8\n";
	file << "    INTEGRATOR BDF-1\n";
	file << "    TIME 0.0 10 " << stepTime << " \n";
	file << "  ENDTHERMALDYNAMIC\n";
	file << "ENDANALYSIS\n";

	file.close();
}

void Model::readParamInputFile(const char* path, std::map<double,double>& param, double& paramStartTime, bool& paramLoaded, double& paramPulseTime){
	std::ifstream input;
	input.open(path);
	if(input.is_open()){
		printf("Opened\n\n");
	}else{
		printf("Fail opening\n\n");
		paramLoaded=false;
		return ;
	}

	std::string line;
	char* pEnd;
	double time,value;
	bool found=false;

	//Get rid of header
	while(!input.eof() && !found){
		std::getline(input,line);
		line.erase(0,line.find_first_not_of(" "));
#ifdef _WIN32
		if(!line.empty() && iswdigit(line[0])){//first pair
#else
		if(!line.empty() && std::isdigit(line[0])){//first pair
#endif
			param.clear();
			found=true;
			paramLoaded=true;
			time = std::strtod(line.c_str(),&pEnd);
			value= std::strtod(pEnd,NULL);
			paramStartTime=time;
			param.insert(std::pair<double,double>(time,value));
		}
	}
	if(!found){
		paramLoaded=false;
		return;
	}
	//Parse numbers
	while(!input.eof()){
		std::getline(input,line);
		if(!line.empty()){
			time = std::strtod(line.c_str(),&pEnd);
			value= std::strtod(pEnd,NULL);
			param.insert(std::pair<double,double>(time,value));
		}
	}
	paramPulseTime = param.rbegin()->first - paramStartTime;
	input.close();
	updateStartTime();

}

void Model::readLoadInputFile(const char* path, const char* coordPath, bool coordFile, std::map<double,std::map<double,double> >& param, double& paramStartTime, bool& paramLoaded){
	param.clear();
	double x_upper, x_lower;
	getXLimits(x_lower, x_upper);
	std::ifstream input;
	input.open(path);
	if(input.is_open()){
		printf("Opened\n\n");
	}else{
		printf("Fail opening\n\n");
		paramLoaded=false;
		return ;
	}

	std::string line;
	char* pEnd;
	char* temppEnd = "";
	double time;
	double a;

	std::vector<double> vec;
	std::map<double,double> Map;
	bool found=false;
	bool ALICIA = FALSE;
	int count = 1;
	paramStartTime = 0;
	while(!input.eof() && !found){
		std::getline(input,line);
		line.erase(0,line.find_first_not_of(" "));
		line.erase(0,line.find_first_not_of("\t"));
#ifdef _WIN32
		if (!line.empty() && iswdigit(line[0])) {//first pair
#else
		if (!line.empty() && std::isdigit(line[0])) {//first pair
#endif
			param.clear();
			found=true;
			paramLoaded=true;
			//Need to check the correspondance between coordinates system
			a = std::strtod(line.c_str(),&pEnd) + x_lower;
			loadCoord.insert(loadCoord.end(),a);
			while(*pEnd){
				a = std::strtod(pEnd,&pEnd) + x_lower;
				//loadCoord.insert(loadCoord.end(),a);
// 				printf("premier while %g \n", a);
				if (temppEnd == pEnd){
					break;
				}else{
					temppEnd = pEnd;
					if (a<0) ALICIA = TRUE; //ALICIA present negative value although THEODOR don't (there is maybe a better way to find if it is ALICIA or THEODOR)
					loadCoord.insert(loadCoord.end(),a);
				}
			}
		}
	}
	printf("ok\n");
	if (coordFile){
		buildLoadPosition(coordPath, found);
	}
	if(!found){
		cout<<"crotte"<<std::endl;
		paramLoaded=false;
		return;
	}
	//Parse numbers
	std::map<double,std::vector<double> > tempMap;
	while(!input.eof()){

		std::getline(input,line);
		if(!line.empty()){
			time = std::strtod(line.c_str(),&pEnd);
			if (paramStartTime == 0) paramStartTime = time;
			//cout<<line<<std::endl;
			while (*pEnd != 0){

				a = std::strtod(pEnd,&pEnd);
				vec.insert(vec.end(),a);
				//if (a != 0) std::cout<<a<<std::endl;

// 				printf("deuxieme while %g \n", a);
				if (temppEnd == pEnd){
					break;
				}else{
					temppEnd = pEnd;
				}
			}
			tempMap.insert(std::pair<double,std::vector<double> >(time,vec));
			vec.clear();

		}
		if(time>= paramStartTime + count * stepTime){
			MeanDoubVec(tempMap, vec);
			tempMap.clear();
			 ALICIAMidPlane( paramStartTime + count * stepTime, vec);
			for (int i=0; i < loadCoord.size(); i++){
				Map.insert(std::pair<double,double>(loadCoord[i], vec[i]));

			}
			load.insert(std::pair<double,std::map<double,double> >(paramStartTime + count * stepTime, Map));
			count+=1;
// 			std::cout<<std::fixed;
// 			std::cout<<std::setprecision(35)<<"time\t"<<paramStartTime + count * stepTime<<'\n';
			Map.clear();
		}


	}
	paramStartTime += stepTime; //the first average is done including the profile at paramStartTime
	loadPulseTime = load.rbegin()->first - loadStartTime;
	input.close();
	updateStartTime();
}

void Model::buildLoadPosition(const char* path, bool& found){
	double x_upper, x_lower;
	double constShift = 1.4408;
	getXLimits(x_lower, x_upper);
	std::ifstream input;
	input.open(path);
	if(input.is_open()){
		printf("Opened\n\n");
	}else{
		printf("Fail opening\n\n");
		found=false;
		return ;
	}

	std::string line;
	int iter = 0;
	bool ok = false;
	double a;
	char* pEnd;

	while(!input.eof() && !ok){
		std::getline(input,line);
		line.erase(0,line.find_first_not_of(" "));
#ifdef _WIN32
		if (!line.empty() && iswdigit(line[0])) {//first pair
#else
		if (!line.empty() && std::isdigit(line[0])) {//first pair
#endif
			a = std::strtod(line.c_str(), &pEnd);
			if (a != loadCoord[iter]-x_lower){
				found = false;
				return;
			}else{
				loadCoord[iter] = std::strtod(pEnd, &pEnd)/1000 + constShift;
				iter += 1;
				ok = true;
			}
		}
	}

	while(!input.eof()){
		std::getline(input,line);
		a = std::strtod(line.c_str(), &pEnd);
		if (!(a > loadCoord[iter]-x_lower-1e-5 && a < loadCoord[iter]-x_lower+1e-5) && iter < loadCoord.size()){
			found = false;
			return;
		}else if (iter<loadCoord.size()){
			loadCoord[iter] = std::strtod(pEnd, &pEnd)/1000 + constShift;
			iter+=1;
			ok = true;
		}
	}
	found = true;
}


double Model::loadFilePDF(double time, double x){
	double value, max, min;
	i_tDoubleMap it = load.lower_bound(time);
	i_tDoubleMap prev;
	prev = it;
	if (it != load.begin()/* && it != load.end()*/){
		--prev;
	}
	if (std::abs(time - prev->first) < std::abs(time - it->first)){
		it = prev;
	}
// 	if (time < 45){
// 	  std::cout<<std::fixed;
// 	  std::cout<< time << it->first << '\n';
// 	}

	if (it == load.end()) return 0;
	if ((x >= *loadCoord.rbegin() && x <= *loadCoord.begin()) || (x <= *loadCoord.rbegin() && x >= *loadCoord.begin())){
		value = mknix::interpolate1D(x, it->second);
		if (value > peakPowerDensity) peakPowerDensity = value;
		return value;
	}else
		return 0;
}

double Model::loadFileCDF(double time, double x){
	double value = 0;
	double x_lower = 2.8;
	for (double i=x_lower; i<x; i+= 1e-4){
		value += 1e-4 * (loadFilePDF(time, i)+loadFilePDF(time, i+1e-4))/2;
	}
	return value;
}

void Model::loadFileHeatFluence(double time,std::vector<double> x_coordinates,std::vector<double>& heatFluence){
	 double prevMean;
	 double prevValue;
	 double postMean;
	 double postValue;

	 prevMean = (x_coordinates[0]+x_coordinates[1])/2;
	 prevValue = loadFileCDF(time,prevMean);

	 heatFluence[0]=0;
	 for(int i=1;i<heatFluence.size()-1;++i){
		 postMean= (x_coordinates[i]+x_coordinates[i+1])/2;
		 postValue= loadFileCDF(time,postMean);
		 heatFluence[i]= postValue-prevValue;
		 prevValue=postValue;
	 }
	 heatFluence[heatFluence.size()-1]=0;
}

void Model::loadFileLimits(double& pdfLimit, double& cdfLimit){
	pdfLimit = 0;
	for(i_tDoubleMap it = load.begin(); it != load.end(); ++it) {
		      for (i_tDoubleDouble it2 = it->second.begin();it2 !=  it->second.end(); ++it2){
				if (it2->second> pdfLimit) pdfLimit = it2 -> second;
		      }
		}
	cdfLimit = 800000;
}

void Model::MeanDoubVec(std::map<double, std::vector<double> > Map, std::vector<double>& vec){

	vec.clear();
	double size,sizeVec, sum;
	std::vector<double> tempVec;
	sum = 0;
	size = Map.size();
	sizeVec = loadCoord.size();
	for (int j = 0; j<sizeVec; j++){
		for(i_tvect it = Map.begin(); it != Map.end(); ++it) {
		      sum += it->second[j];
		      //printf("second %g \n", it->second[j]);
		}
		vec.insert(vec.end(), sum/size);
		sum = 0;
		//if (sum/size < 1)printf("Moyenne %g \n", sum/size);
	}
}


void Model::readParamPPF(int jpn,PPFSignal_t signal, std::map<double,double>& param, double& paramStartTime, bool& paramLoaded,PPFDataType dtype, double& paramPulseTime){
	param.clear();
	paramLoaded=false;
	int zero = 0;
	int error = 0;
	char ihdat[60];
	int iwdat[13],irdat[13]={
			0,  //X first point
			1,  //X last point
			0,  //T first point
			1,  //T last point (Will be set later to NT
			1,  //Data dimension
			1,  //X Dimension
			1,  //T Dimension (Will be set later to NT
			0, 	//X not required
			0,  //T required
			0,	//Not used
			0,	//Not used
			0,	//Not used
			0	//Not used
			};
#ifdef DJET_INSTALL
	if (Dtype[dtype] == Dtype[1] || Dtype[dtype] == Dtype[2]){
		PPFUID("KL9PPF","R", 8, 1);
	}else if(Dtype[dtype] == Dtype[FELMTYPE]){
		PPFUID("CHAIN1","R", 8, 1);
	}else{
		PPFUID("JETPPF","R", 8, 1);
	}
	PPFGO(&zero, &zero, &zero, &error);
	if (error != 0){
	   printf("PPFGO error\n");
	   return ;
	}
	int nx,nt,systat,ustat;
	char format;
	char units[8];
	char comm[24];
	PPFDTI(&jpn,&zero,(char*)dda[signal],(char*)Dtype[dtype],&nx,&nt,&format,units,comm,&systat,&ustat,&error,4,4,1,8,24);

	irdat[3] = nt;
	irdat[4] = nt;
	irdat[5] = nx;
	irdat[6] = nt;

	float* data = (float*)malloc(nt*sizeof(float));
	float* time = (float*) malloc(nt*sizeof(float));
	float* test = (float*) malloc(nt*sizeof(float));
	printf("NX %d\n NT%d\n format %c\n, units %s\n comm%s\n, systat %d\n ustat %d\n ierr%d\n ",nx,nt,format,units,comm,systat,ustat,error);

	PPFGET(&jpn, (char*)dda[signal], (char*)Dtype[dtype], irdat, ihdat, iwdat, data, test, time, &error, 4, 4, 60);
	if(error!=0){
		printf("PPFGET error\n");
		return;
	}else{
		param.clear();
	}
	paramStartTime=time[0];
	paramPulseTime = time[nt-1]-paramStartTime;
	for (int i=0; i<nt;++i){
		param.insert(std::pair<double,double>(time[i],data[i]));
	}
	free(data);
	free(time);
	paramLoaded=true;
	updateStartTime();
#endif // DJET_INSTALL

}

void Model::readParamJPF(int jpn,JPFSignal_t signal, std::map<double,double>& param, double& paramStartTime, bool& paramLoaded, double& paramPulseTime){
	int err=0;
    char title[53], units[11];
    int size =0;
    paramLoaded=false;

#ifdef DJET_INSTALL
	getnwds_((char *)jpfNames[signal], &jpn, &size, &err, (long) strlen(jpfNames[signal]));

	if( err || size <= 0){
		printf("[ERROR] Error reading Signal ID:%s\n ",jpfNames[signal]);
		return;
	}else{
		float *data =NULL;
		data = (float*) malloc(size*sizeof(float));
		float *time = NULL;
		time = (float*) malloc(size*sizeof(float));

		if(data==NULL || time==NULL){
			printf("[ERROR] Error allocating memory JPF:%s\n",jpfNames[signal]);
			free(data);
			free(time);
			return;
		}else{

			getdat_((char *)jpfNames[signal], &jpn, data, time, &size, title, units, &err,
					  (long) strlen(jpfNames[signal]), sizeof(title) - 1l, sizeof(units) - 1l);
			if(err){
				printf("\n[ERROR] Error retrieving Signal data ID: %s\n",jpfNames[signal]);

			}else{

				//IF SP, remove trailing -0.099 values
				int first=0;
				int last = size-1;
				float lowLimit = data[0];
				float upperLimit = 3.3062;
				if(signal == SPSIGNAL){
					while(first<last){
						if(data[first+1]==lowLimit || data[first+1]>=upperLimit){
							++first;
						}else{
							break;
						}
					}
					while(last>first){
						if(data[last-1]==lowLimit || data[last-1]>=upperLimit){
							--last;
						}else{
							break;
						}
					}
				}

				param.clear();
				for(int i=first;i<=last;++i){
					param.insert(std::pair<double,double>(time[i],data[i]));
				}
				paramStartTime=time[first];
				paramPulseTime = time[last]-paramStartTime;
				printf("SP LIMITS: First data = %f, last Data = %f\n",time[first], time[last]);
				paramLoaded=true;
				updateStartTime();

			}

		}
		free(data);
		free(time);
	}
#endif // DJET_INSTALL
}

void Model::calculateHeatFluence(double time,std::vector<double> x_coordinates,std::vector<double>& heatFluence){

	switch(dist){
		case Const_Dist:
			constHeatFluence(time,x_coordinates,heatFluence);
			break;
		case Triangle_Dist:
			triangleHeatFluence(time,x_coordinates,heatFluence);
			break;
		case Skew_Normal_Dist:
			skewNormalHeatFluence(time,x_coordinates,heatFluence);
			break;
		//Eich
		case Eich_Dist:
			EichHeatFluence(time,x_coordinates,heatFluence);
			break;
		case Loaded_Dist:
			loadFileHeatFluence(time, x_coordinates, heatFluence);
			break;
		default:
//			statusBar()->showMessage( tr("Distribution Unknown"), 2000 );
			break;
	}
}

void Model::getShadowLimit(double& lowlimit, double& uplimit){
	double x_lower,x_upper, Btime;
	getXLimits(x_lower,x_upper);
	double step = (x_upper-x_lower)/reso;
	double r5 = 281.471;
	double r7 = 289.768;
	double z5 = -167.203;
	double z7 = -168.233;
	double r1;
	long int np, ierr;
	float time; //Shadow
	np = 1;
	double Br5, Bz5, Br7, Bz7;
#ifdef DJET_INSTALL
	Flush_getBr(&np, &r5, &z5, &Br5, &ierr);
	Flush_getBz(&np, &r5, &z5, &Bz5, &ierr);
	Flush_getBr(&np, &r7, &z7, &Br7, &ierr);
	Flush_getBz(&np, &r7, &z7, &Bz7, &ierr);
#endif
	uplimit = 495.253;//<=>uplimit = 5
	lowlimit = 6.668;//<=>lowlimit = 0
	r1 = (-171.158 - z5 + Bz5/Br5*r5)/(Bz5/Br5);
	if (r1<285.703 && r1 >= 280.425) lowlimit = r1;
	r1 = (-112.018-z5+Bz5/Br5 * r5)/(Bz5/Br5+0.207);
	if (r1<=287.846 && r1 > 285.703){ lowlimit = r1;}
	r1 = (-45.651-z5+Bz5/Br5 * r5)/(Bz5/Br5+0.438);
	if (r1<=293.644 && r1>287.846 ) { lowlimit = r1;}
	r1 = (-110.01-z5+Bz5/Br5 * r5)/(Bz5/Br5+0.218);
	if (r1<=295.732 && r1> 293.644) { lowlimit = r1;}
	r1 = (-174.595 - z5 + Bz5/Br5 * r5) / (Bz5/Br5);
	if (r1 > 295.732 && r1 <= 298.698) lowlimit = r1;


	r1 = (-171.158 - z7 + Bz7/Br7*r7)/(Bz7/Br7);
	if (r1<285.703 && r1 >= 280.425) uplimit = r1;
	r1 = (-112.018-z7+Bz7/Br7 * r7)/(Bz7/Br7+0.207);
	if (r1<=287.846 && r1 > 285.703){ uplimit = r1;}
	r1 = (-45.651-z7+Bz7/Br7 * r7)/(Bz7/Br7+0.438);
	if (r1<=293.644 && r1>287.846 ) { uplimit = r1;}
	r1 = (-110.01-z7+Bz7/Br7 * r7)/(Bz7/Br7+0.218);
	if (r1<=295.732 && r1> 293.644) { uplimit = r1;}
	r1 = (-174.595 - z7 + Bz7/Br7 * r7) * Br7/Bz7;
	if (r1 > 295.732 && r1 <= 298.698) uplimit = r1;

	//printf("PDF: lambda = %g\nS = %g\n", lambda_q, S);
	if (shadowPosition.empty()){
		cout<<"Hellooooo"<<std::endl;
		for (int i = 0; i <reso+1; ++i){
			shadowPosition.insert(shadowPosition.end(), x_lower + i*step);
		}
	}


	lowlimit = (lowlimit-6.668)/97.717;
	uplimit = (uplimit-6.668)/97.717;

	int i = 0;
#ifdef DJET_INSTALL
	Flush_getTime(&time, &ierr);
#endif
	if (static_cast<double>(time) >= analysisStartTime+stepTime && static_cast<double>(time) <= analysisStartTime + (pulseIterations - 10)*stepTime){
	  for (double x = x_lower; x<= x_upper; x+=step){
		if (x>lowlimit && x<uplimit) shadow[i] += 1;
		i+=1;
	  }


	  if (shadowTime.lower_bound(time) == shadowTime.end()){
		shadowTime.insert(std::pair<float, double>(time, 1));
	  }else{
		shadowTime.lower_bound(time) -> second += 1;
	  }
	}
}

void Model::triangleHeatFluence(double time,std::vector<double> x_coordinates,std::vector<double>& heatFluence){

	 double prevMean;
	 double prevValue;
	 double postMean;
	 double postValue;

	 prevMean = (x_coordinates[0]+x_coordinates[1])/2;
	 prevValue = triangleCDF(time,prevMean);

	 heatFluence[0]=0;
	 for(int i=1;i<heatFluence.size()-1;++i){
		 postMean= (x_coordinates[i]+x_coordinates[i+1])/2;
		 postValue= triangleCDF(time,postMean);
		 heatFluence[i]=postValue-prevValue;
		 prevValue=postValue;
	 }
	 heatFluence[heatFluence.size()-1]=0;
}

void Model::triangleParams(double time, double& NBIout,double& ICRHout, double& OHMout,
							double& SPout, double& TPout, double& Pmout, double& x0out, double& x1out){

	if(SPFileLoad || SF==0){
		SPout=mknix::interpolate1D(time,SP);
	}else{
		SPout=mknix::interpolate1D(time,SP)+SA*sin((2*M_PI*SF)*(time-analysisStartTime));
	}


	if(time>(analysisStartTime+(stepTime*(pulseIterations-1)))){
		NBIout=0;
		ICRHout=0;
		OHMout=0;
		TPout=0;
		Pmout=0;
	}else{
		NBIout = mknix::interpolate1D(time,NBI);
		ICRHout = mknix::interpolate1D(time,ICRH);
		if (OHMLoad) OHMout=mknix::interpolate1D(time, OHM);
		else OHMout = 0;
		if (!useTOPI) TPout = (NBIout+ICRHout+OHMout)*(1-RF);
		else TPout = (NBIout+ICRHout+OHMout)-mknix::interpolate1D(time,TOPI);
		Pmout = (2*TPout)*OR / (W*TL*nTiles*(WF/100));
	}
	x0out = SPout;
	x1out = W+x0out;
}

double Model::trianglePDF(double time, double x){
	double currNBI,currICRH,currOHM,currSP,TP,Pm,x0,x1;
	triangleParams(time,currNBI,currICRH,currOHM,currSP,TP,Pm,x0,x1);
	double value;

	if (useShadow){
		double lowlimit, uplimit;
		getShadowLimit(lowlimit, uplimit);
		if (x<uplimit && x>lowlimit){
			if(x<x0){
				value=0;
			}else if(x>x1){
				value=0;
			 }else{
				value=Pm * ( (x1-x) / W );
			}
		}else return 0;
	}else{
		if(x<x0){
			value=0;
		}else if(x>x1){
			value=0;
		}else{
			value=Pm * ( (x1-x) / W );
		}
	}
	if (value > peakPowerDensity) peakPowerDensity = value;
	return value;
}

double Model::triangleCDF(double time, double x){
	double currNBI,currICRH,currOHM,currSP,TP,Pm,x0,x1;
	triangleParams(time,currNBI,currICRH,currOHM,currSP,TP,Pm,x0,x1);
	double value;
	if (useShadow){
		double lowlimit, uplimit;
		getShadowLimit(lowlimit, uplimit);
		if (x<uplimit && x>lowlimit){
			if(x<x0){
				value=0;
			}else if (x>x1){
				if (lowlimit < x0){
					value=(Pm/2)*(W);
				}else if (lowlimit > x0 && lowlimit < x1){
					value=(Pm/W) * ( (x1*x1) - (pow(x1,2)/2) - ( x0 * ( x1 - ( x0/2 ) ) ) ) - (Pm/W) * ( (x1*lowlimit) - (pow(lowlimit,2)/2) - ( x0 * ( x1 - ( x0/2 ) ) ) );
				}else{
					value = 0;
				}
			}else{
				if (lowlimit > x0){
					value=(Pm/W) * ( (x1*x) - (pow(x,2)/2) - ( x0 * ( x1 - ( x0/2 ) ) ) ) - (Pm/W) * ( (x1*lowlimit) - (pow(lowlimit,2)/2) - ( x0 * ( x1 - ( x0/2 ) ) ) );
				}else{
					value = Pm/W * ( (x1*x) - (pow(x,2)/2) - ( x0 * ( x1 - ( x0/2 ) ) ) );
				}
			}
			return value;
		}else if(uplimit>lowlimit && x > uplimit && lowlimit < x1 && uplimit > x0){
			if (lowlimit < x0 && uplimit <x1){
				value=(Pm/W) * ( (x1*uplimit) - (pow(uplimit,2)/2) - ( x0 * ( x1 - ( x0/2 ) ) ) );
			}else if (lowlimit < x0 && uplimit > x1){
				value=(Pm/2)*(W);
			}else if (lowlimit > x0 && uplimit > x1){
				value=(Pm/2)*(W)- (Pm/W) * ( (x1*lowlimit) - (pow(lowlimit,2)/2) - ( x0 * ( x1 - ( x0/2 ) ) ) );
			}else if (lowlimit > x0 && uplimit < x1){
				value=(Pm/W) * ( (x1*uplimit) - (pow(uplimit,2)/2) - ( x0 * ( x1 - ( x0/2 ) ) ) )- (Pm/W) * ( (x1*lowlimit) - (pow(lowlimit,2)/2) - ( x0 * ( x1 - ( x0/2 ) ) ) );
			}
			return value;
		}else return 0;
	}else{
		if(x<x0){
			value=0;
		}else if (x>x1){
			value=(Pm/2)*(W);
		}else{
			value=(Pm/W) * ( (x1*x) - (pow(x,2)/2) - ( x0 * ( x1 - ( x0/2 ) ) ) );
		}

		return value;
	}
}

void Model::triangleLimits(double& pdfLimit, double& cdfLimit){
	double currNBI,currICRH,currSP;

	double time=analysisStartTime;
	double currMax=0;
	double timeMax=0;
	for(int i=0;i<pulseIterations;++i){
		currNBI = mknix::interpolate1D(time,NBI);
		currICRH = mknix::interpolate1D(time,ICRH);
		if(currNBI+currICRH>currMax){
			currMax=currNBI+currICRH;
			if(SPFileLoad || SF==0){
				currSP=mknix::interpolate1D(time,SP);
			}else{
				currSP=mknix::interpolate1D(time,SP)+SA*sin((2*M_PI*SF)*(time-analysisStartTime));
			}
			timeMax=time;
		}
		time+=stepTime;
	}

	double currOHM,TP,Pm,x0,x1;
	triangleParams(timeMax,currNBI,currICRH,currOHM,currSP,TP,Pm,x0,x1);

	pdfLimit=Pm;
	cdfLimit=Pm / 2 * W;
}

void Model::skewNormalHeatFluence(double time,std::vector<double> x_coordinates,std::vector<double>& heatFluence){
	 double prevMean;
	 double prevValue;
	 double postMean;
	 double postValue;

	 prevMean = (x_coordinates[0]+x_coordinates[1])/2;
	 prevValue = skewNormalCDF(time,prevMean);

	 heatFluence[0]=0;
	 for(int i=1;i<heatFluence.size()-1;++i){
		 postMean= (x_coordinates[i]+x_coordinates[i+1])/2;
		 postValue= skewNormalCDF(time,postMean);
		 heatFluence[i]= postValue-prevValue;
		 prevValue=postValue;
	 }
	 heatFluence[heatFluence.size()-1]=0;
}

void Model::skewNormalParams(double time, double& NBIout,double& ICRHout, double& OHMout,
							double& SPout, double& TPout, double& Pmout, double& scaleout){

	if(SPFileLoad || SF==0){
		SPout=mknix::interpolate1D(time,SP);
	}else{
		SPout=mknix::interpolate1D(time,SP)+SA*sin((2*M_PI*SF)*(time-analysisStartTime));
	}

	if(time>(analysisStartTime+(stepTime*(pulseIterations-1)))){
		NBIout=0;
		ICRHout=0;
		OHMout=0;
		TPout=0;
		Pmout=0;
		scaleout=0;
	}else{
		NBIout = mknix::interpolate1D(time,NBI);
		ICRHout = mknix::interpolate1D(time,ICRH);
		if (OHMLoad) OHMout=mknix::interpolate1D(time, OHM);
		else OHMout = 0;
		if (!useTOPI) TPout = (NBIout+ICRHout+OHMout)*(1-RF);
		else TPout = (NBIout+ICRHout+OHMout)-mknix::interpolate1D(time,TOPI);
		Pmout = TPout*OR / (TL*nTiles*(WF/100));
	}
	scaleout = W/B ;
}


double Model::skewNormalPDF(double time, double x){
	double currNBI,currICRH,currOHM,currSP,TP,Pm,scale;
	skewNormalParams(time,currNBI,currICRH,currOHM,currSP,TP,Pm,scale);


	boost::math::skew_normal_distribution<double> skew(currSP,scale,skewShape);
// 	printf("\tscale = %g\n",scale);
// 	printf("\tPDF Skew = %g\n",boost::math::pdf(skew, x));
	if (useShadow){
		double lowlimit, uplimit;
		getShadowLimit(lowlimit, uplimit);
		if (x>lowlimit && x<uplimit){
			if (Pm * boost::math::pdf(skew, x) > peakPowerDensity) peakPowerDensity = Pm * boost::math::pdf(skew, x);
			return Pm * boost::math::pdf(skew, x);
		}else{
			return 0;
		}
	}else{
		if (Pm * boost::math::pdf(skew, x) > peakPowerDensity) peakPowerDensity = Pm * boost::math::pdf(skew, x);
		return Pm * boost::math::pdf(skew, x);
	}

}

double Model::skewNormalCDF(double time, double x){
	double currNBI,currICRH,currOHM,currSP,TP,Pm,scale;
		skewNormalParams(time,currNBI,currICRH,currOHM,currSP,TP,Pm,scale);

	boost::math::skew_normal_distribution<double> skew(currSP, scale, skewShape);

	if (useShadow){
		double lowlimit, uplimit;
		getShadowLimit(lowlimit, uplimit);
		if (x>lowlimit && x<uplimit)
			return Pm*boost::math::cdf(skew, x) - Pm*boost::math::cdf(skew, lowlimit);
		else if (x>uplimit && uplimit > lowlimit)
			return Pm*boost::math::cdf(skew, uplimit) - Pm*boost::math::cdf(skew, lowlimit);
		else return 0;
	}else{
		return Pm*boost::math::cdf(skew, x);
	}
}

void Model::skewNormalLimits(double& pdfLimit, double& cdfLimit){
	double currNBI,currICRH,currOHM,currSP,TP,Pm,scale;

	double time=analysisStartTime;
	double currMax=0;
	double timeMax=0;
	for(int i=0;i<pulseIterations;++i){
		currNBI = mknix::interpolate1D(time,NBI);
		currICRH = mknix::interpolate1D(time,ICRH);
		if(currNBI+currICRH>currMax){
			currMax=currNBI+currICRH;
			if(SPFileLoad || SF==0){
				currSP=mknix::interpolate1D(time,SP);
			}else{
				currSP=mknix::interpolate1D(time,SP)+(SA*sin((2*M_PI*SF)*(time-analysisStartTime)));
			}
			timeMax=time;
		}
		time+=stepTime;
	}

	skewNormalParams(timeMax,currNBI,currICRH,currOHM,currSP,TP,Pm,scale);
	boost::math::skew_normal_distribution<double> skew(currSP,scale,skewShape);

	pdfLimit= Pm * boost::math::pdf(skew,boost::math::mode(skew));
	cdfLimit= Pm;
}


//Eich
void Model::EichHeatFluence(double time,std::vector<double> x_coordinates,std::vector<double>& heatFluence){
	 double prevMean;
	 double prevValue;
	 double postMean;
	 double postValue;

	 prevMean = (x_coordinates[0]+x_coordinates[1])/2;
	 prevValue = EichCDF(time,prevMean);

	 heatFluence[0]=0;
	 for(int i=1;i<heatFluence.size()-1;++i){
		 postMean= (x_coordinates[i]+x_coordinates[i+1])/2;
		 postValue= EichCDF(time,postMean);
		 heatFluence[i]= postValue-prevValue;
		 prevValue=postValue;
	 }
	 heatFluence[heatFluence.size()-1]=0;
}

void Model::EichParams(double time, double& NBIout,double& ICRHout, double& OHMout,
							double& SPout, double& TPout, double& Pmout, double& scaleout){
	if(SPFileLoad || SF==0){
		SPout=mknix::interpolate1D(time,SP);
	}else{
		SPout=mknix::interpolate1D(time,SP)+SA*sin((2*M_PI*SF)*(time-analysisStartTime));
	}

	if(time>(analysisStartTime+(stepTime*(pulseIterations-1)))){
		NBIout=0;
		ICRHout=0;
		OHMout=0;
		TPout=0;
		Pmout=0;
		scaleout=0;
	}else{
		NBIout = mknix::interpolate1D(time,NBI);
		ICRHout = mknix::interpolate1D(time,ICRH);
		if (OHMLoad) OHMout=mknix::interpolate1D(time, OHM);
		else OHMout = 0;
		if (!useTOPI) TPout = (NBIout+ICRHout+OHMout)*(1-RF);
		else TPout = (NBIout+ICRHout+OHMout)-mknix::interpolate1D(time,TOPI);
		//printf("time= %g \nICRH=%g \n",time, ICRHout) ;



		Pmout = TPout*OR / (TL*nTiles*(WF/100));
	}

	scaleout = W/B ;

}

void Model::computePDF(){
	int precision = 500;
	int err;
	double x_upper, x_lower, Btimeav, Btimeap;
	double currNBI,currICRH,currOHM,currSP,TP,Pm,scale, expparam, erfparam, expparam1, erfparam1, value;
	std::map<double,double> Map;
	getXLimits(x_lower, x_upper);
	double Br, Bt, Bz, alphar, alphaz, alphazTemp, alphat;

	double module_div, module_mid;

	double Rdiv, Zdiv, RSP, ZSP, RSPmid, Rmid;
	long int ierr;
	long int np = 1;

	bool firstStep;

	double upper_int = 8;

	for (double i = analysisStartTime; i<= analysisStartTime + stepTime * (pulseIterations+10); i+= stepTime){
		if (i == analysisStartTime) firstStep = true;
		else firstStep = false;
#ifdef DJET_INSTALL
		flushQuickInit(&shot, &i, &err);
#endif

		if (i == Btimeav) i = Btimeap;
		else Btimeav = i;
		if (firstStep) i = analysisStartTime;
		module_div = 0;
		EichParams(i,currNBI,currICRH,currOHM,currSP,TP,Pm,scale);
		for (double x = 2.8; x<=3; x+=  (x_upper - x_lower)/precision){
			if (AutoLambdaS){
// 				lambda_q = lambda_qmap.lower_bound(i)->second;
// 				S = Smap.lower_bound(i)->second;
				lambda_q = mknix::interpolate1D(i, lambda_qmap);
				S = mknix::interpolate1D(i, Smap);
			//printf("lambda = %g\nS = %g\n", lambda_q, S);
			}
			if (lambda_q < 0.006) lambda_q = 0.006;
			Rdiv = getRT6(x);
			Zdiv = getZT6(x);
			RSP = getRT6(currSP);
			ZSP = getZT6(currSP);
#ifdef DJET_INSTALL
			Flush_getMidPlaneProjRight(&np, &RSP, &ZSP, &RSPmid, &ierr);
			Flush_getMidPlaneProjRight(&np, &Rdiv, &Zdiv, &Rmid, &ierr);
#endif
			RSPmid /= 100;
			Rmid /= 100;


			erfparam = S/(2*lambda_q)-(Rmid-RSPmid)/S;
			expparam = pow(S/(2*lambda_q),2)-(Rmid-RSPmid)/lambda_q;
			erfparam1 = S/(2*lambda_q)-(0-RSPmid)/S;
			expparam1 = pow(S/(2*lambda_q),2)-(0-RSPmid)/lambda_q;

			module_mid = lambda_q*((boost::math::erf((upper_int-RSPmid)/S)-boost::math::erf(-RSPmid/S))
			-(exp(pow(S/(2*lambda_q),2)-(upper_int-RSPmid)/lambda_q) * boost::math::erfc(S/(2*lambda_q)-(upper_int-RSPmid)/S)
			    -(exp(expparam1) * boost::math::erfc(erfparam1))));


			if (module_mid > 0)
			value =  Pm /*/ module_mid*/ * exp(expparam) * boost::math::erfc(erfparam);
			else {
			  value = 0;
			}
#ifdef DJET_INSTALL
			Flush_getBr(&np, &Rdiv, &Zdiv, &Br, &ierr);
			Flush_getBt(&np, &Rdiv, &Zdiv, &Bt, &ierr);
			Flush_getBz(&np, &Rdiv, &Zdiv, &Bz, &ierr);
#endif

			getNormalT6(alphar, alphazTemp, x);//+0.02 if no coordinate file
			alphat = -alphazTemp * std::sin(0.61 * M_PI/180);
			alphaz = alphazTemp * std::cos(0.61 * M_PI/180);
			if (Br < 0) value = 0;

			double cosine_tile = (-((alphar * Br + alphat * Bt + alphaz * Bz)/std::sqrt(pow(Br, 2) + pow(Bt, 2) + pow(Bz, 2))));
			double cosine_flat = (-( 1 * Bz/std::sqrt(pow(Br, 2) + pow(Bt, 2) + pow(Bz, 2))));

			value *= cosine_tile/cosine_flat;
			module_div += value * (x_upper - x_lower)/precision;
			Map.insert(std::pair<double,double>(x, value));
		}
		PDFProjection.insert(std::pair<double,std::map<double,double> >(i, Map));
		if (module_mid > 1e-10)
		moduleProjection.insert(std::pair<double,double>(i, module_mid));
		else moduleProjection.insert(std::pair<double,double>(i, 0));
		Map.clear();
		Btimeap = i + stepTime;
		printf("time= %g: module= %g\n", i, module_mid);
	}
	PDFcomputed = true;
}

double Model::EichPDF(double time, double x){
	double currNBI,currICRH,currOHM,currSP,TP,Pm,scale, expparam, erfparam, expparam1, erfparam1, module, value, PDFvalue;
	double x_upper = 5;
	double factor;

	double Rdiv, Zdiv, RSP, ZSP, RSPmid, Rmid;
	long int ierr;
	long int np = 1;
	double Br, Bt, Bz;
	double alphar, alphat, alphaz, alphazTemp;

	if (AutoLambdaS){
// 			lambda_q = lambda_qmap.lower_bound(time)->second;
// 			S = Smap.lower_bound(time)->second;
			lambda_q = mknix::interpolate1D(time, lambda_qmap);
			S = mknix::interpolate1D(time, Smap);
// 		printf("time= %g : lambda = %g : S = %g\n", time, lambda_q, S);
	}
	EichParams(time,currNBI,currICRH,currOHM,currSP,TP,Pm,scale);
	erfparam = S/(2*lambda_q)-(x-currSP)/S;
	expparam = pow(S/(2*lambda_q),2)-(x-currSP)/lambda_q;
	erfparam1 = S/(2*lambda_q)-(0-currSP)/S;
	expparam1 = pow(S/(2*lambda_q),2)-(0-currSP)/lambda_q;
	module = lambda_q*((boost::math::erf((x_upper-currSP)/S)-boost::math::erf(-currSP/S))
		 -(exp(pow(S/(2*lambda_q),2)-(x_upper-currSP)/lambda_q) * boost::math::erfc(S/(2*lambda_q)-(x_upper-currSP)/S)
		    -exp(expparam1) * boost::math::erfc(erfparam1)));
	if (module != 0 && Pm != 0){
		value =  Pm / module * exp(expparam) * boost::math::erfc(erfparam);
		//printf("theta=%g \n", alphar);
		if (useFlush){
#ifdef DJET_INSTALL
			checkFlushInitialisation(time);
#endif
			if (!PDFcomputed) computePDF();
			module = mknix::interpolate1D(time, moduleProjection);
			std::map<double,double> Map;
			i_tDoubleMap it = PDFProjection.lower_bound(time);
			i_tDoubleMap prevIt = it;
			--prevIt;

			cout<<std::setprecision(6)<<"time\t"<<time<<std::endl;
			cout<<std::setprecision(6)<<"first\t"<<it->first<<std::endl;
			cout<<std::setprecision(6)<<"--\t"<<prevIt->first<<std::endl;
			if (std::abs(it->first - time) > std::abs(prevIt->first - time))
			{Map = prevIt->second;
			cout<<"prev"<<std::endl;}
			else
				Map = it->second;



// 			if (lambda_q < 0.006) lambda_q = 0.006;
// 			Rdiv = getRT6(x);
// 			Zdiv = getZT6(x);
// 			RSP = getRT6(currSP);
// 			ZSP = getZT6(currSP);
// 			Flush_getMidPlaneProjRight(&np, &RSP, &ZSP, &RSPmid, &ierr);
// 			Flush_getMidPlaneProjRight(&np, &Rdiv, &Zdiv, &Rmid, &ierr);
// 			RSPmid /= 100;
// 			Rmid /= 100;
//
//
// 			erfparam = S/(2*lambda_q)-(Rmid-RSPmid)/S;
// 			expparam = pow(S/(2*lambda_q),2)-(Rmid-RSPmid)/lambda_q;
// 			erfparam1 = S/(2*lambda_q)-(0-RSPmid)/S;
// 			expparam1 = pow(S/(2*lambda_q),2)-(0-RSPmid)/lambda_q;
//
// 			module = lambda_q*((boost::math::erf((x_upper-RSPmid)/S)-boost::math::erf(-RSPmid/S))
// 			-(exp(pow(S/(2*lambda_q),2)-(x_upper-RSPmid)/lambda_q) * boost::math::erfc(S/(2*lambda_q)-(x_upper-RSPmid)/S)
// 			    -(exp(expparam1) * boost::math::erfc(erfparam1))));
//
//
// 			if (module > 0)
// 			value =  Pm / module * exp(expparam) * boost::math::erfc(erfparam);
// 			else {cout<<"module\t"<<module<<std::endl;
// 			  return 0;
// 			}
//
// 			Flush_getBr(&np, &Rdiv, &Zdiv, &Br, &ierr);
// 			Flush_getBt(&np, &Rdiv, &Zdiv, &Bt, &ierr);
// 			Flush_getBz(&np, &Rdiv, &Zdiv, &Bz, &ierr);
//
// 			getNormalT6(alphar, alphazTemp, x);//+0.02 if no coordinate file
// 			alphat = -alphazTemp * std::sin(0.61 * M_PI/180);
// 			alphaz = alphazTemp * std::cos(0.61 * M_PI/180);
// 			if (Br < 0) return 0;
//
//
// 			double cosine_tile = (-((alphar * Br + alphat * Bt + alphaz * Bz)/std::sqrt(pow(Br, 2) + pow(Bt, 2) + pow(Bz, 2))));
// 			double cosine_flat = (-( 1 * Bz/std::sqrt(pow(Br, 2) + pow(Bt, 2) + pow(Bz, 2))));
// 			cout<<time<<std::endl;
// 			cout<<"flat\t"<<cosine_flat<<std::endl;
// 			cout<<"tile\t"<<cosine_tile<<std::endl;
// 			cout<<"Br\t"<<Br<<std::endl;
// 			cout<<"Bt\t"<<Bt<<std::endl;
// 			cout<<"Bz\t"<<Bz<<std::endl;
// 			value =  value *  cosine_tile/cosine_flat;

			if (!useShadow) {
// 				return value * (Br*alphar+Bz*alphaz)/Bz;
// 				if (module != 0) {
				  value = mknix::interpolate1D(x, Map)/module;
				  if (value > peakPowerDensity) peakPowerDensity = value;

				  return value;

// 				}
// 				else return 0;
			}else {
				if (module != 0){ PDFvalue = mknix::interpolate1D(x,Map)/module;
// 					PDFvalue = value;
					if (PDFvalue > peakPowerDensity) peakPowerDensity = PDFvalue;
				}
				else PDFvalue = 0;
			}

		}if(useShadow) {
			double lowlimit, uplimit;
			getShadowLimit(lowlimit, uplimit);

			value =  Pm / module * exp(expparam) * boost::math::erfc(erfparam);
			if (!useFlush){
				if (x> lowlimit && x<uplimit && module != 0){
				  if (value > peakPowerDensity) peakPowerDensity = value;
					return value;
				}

				else return 0;
			}else{
			      if (x> lowlimit && x<uplimit && module != 0){
				return PDFvalue;
			      }
			      else return 0;
			}
		}else{
			if (value > peakPowerDensity) peakPowerDensity = value;
			return value;
		}

	}else{
		return 0;
	}


}

double Model::EichCDF(double time, double x){
	double currNBI,currICRH,currOHM,currSP,TP,Pm,scale, value, erfparam, expparam, erfparam1, expparam1, module;
	double x_lower, x_upper;
	getXLimits(x_lower, x_upper);
	x_upper = 5;

	double Rdiv, Zdiv, RSP, ZSP, RSPmid, Rmid;
	long int ierr;
	long int np = 1;
	double Br, Bt, Bz;
	double alphar, alphat, alphaz, alphazTemp;


	if (AutoLambdaS){
// 			lambda_q = lambda_qmap.lower_bound(time)->second;
// 			S = Smap.lower_bound(time)->second;
			lambda_q = mknix::interpolate1D(time, lambda_qmap);
			S = mknix::interpolate1D(time, Smap);
		//printf("lambda = %g\nS = %g\n", lambda_q, S);
	}
	EichParams(time,currNBI,currICRH,currOHM,currSP,TP,Pm,scale);
	value = 0;
	erfparam = S/(2*lambda_q)-(x-currSP)/S;
	expparam = pow(S/(2*lambda_q),2)-(x-currSP)/lambda_q;
	erfparam1 = S/(2*lambda_q)-(0-currSP)/S;
	expparam1 = pow(S/(2*lambda_q),2)-(0-currSP)/lambda_q;
	module = lambda_q*((boost::math::erf((x_upper-currSP)/S)-boost::math::erf(-currSP/S))
		 -(exp(pow(S/(2*lambda_q),2)-(x_upper-currSP)/lambda_q) * boost::math::erfc(S/(2*lambda_q)-(x_upper-currSP)/S)
		    -exp(expparam1) * boost::math::erfc(erfparam1)));
	if (useFlush && !useShadow){
#ifdef DJET_INSTALL
		checkFlushInitialisation(time);
		if (!PDFcomputed) computePDF();
		std::map<double,double> Map;
		i_tDoubleMap it = PDFProjection.lower_bound(time);
		i_tDoubleMap prevIt = it;
		--prevIt;
		if (std::abs(it->first - time) > std::abs(prevIt->first - time))
			Map = prevIt->second;
		else
			Map = it->second;
		module = mknix::interpolate1D(time, moduleProjection);
		double x_low, x_up;
		getXLimits(x_low, x_up);
		if (module != 0){
		      value = 0;
		      for (double pos = x_lower; pos<x; pos += 1e-3){
			    value += 1e-3 * mknix::interpolate1D(pos, Map)/module;
		    }
		    return value;
		}else return 0;
//


// 		    for(double i = x_lower; i<=x-1e-3; i = i + 1e-3){
// 			    value += 1e-3/2 * (EichPDF(time, i) + EichPDF(time,i+1e-3));
// 		    }

// 		if (lambda_q < 0.006) lambda_q = 0.006;
// 		x_upper = 8;
// 		Rdiv = getRT6(x);
// 		Zdiv = getZT6(x);
// 		RSP = getRT6(currSP);
// 		ZSP = getZT6(currSP);
// 		Flush_getMidPlaneProjRight(&np, &RSP, &ZSP, &RSPmid, &ierr);
// 		Flush_getMidPlaneProjRight(&np, &Rdiv, &Zdiv, &Rmid, &ierr);
// 		RSPmid /= 100;
// 		Rmid /= 100;
//
//
// 		erfparam = S/(2*lambda_q)-(Rmid-RSPmid)/S;
// 		expparam = pow(S/(2*lambda_q),2)-(Rmid-RSPmid)/lambda_q;
// 		erfparam1 = S/(2*lambda_q)-(0-RSPmid)/S;
// 		expparam1 = pow(S/(2*lambda_q),2)-(0-RSPmid)/lambda_q;
//
// 		cout<<"lambda\t"<<lambda_q<<"S\t"<<S<<std::endl;
// 		cout<<"RSP:"<<RSPmid<<"\tRmid:"<<Rmid<<std::endl;
// 		module = lambda_q*((boost::math::erf((x_upper-RSPmid)/S)-boost::math::erf(-RSPmid/S))
// 		 -(exp(pow(S/(2*lambda_q),2)-(x_upper-RSPmid)/lambda_q) * boost::math::erfc(S/(2*lambda_q)-(x_upper-RSPmid)/S)
// 		    -(exp(expparam1) * boost::math::erfc(erfparam1))));
//
// 		cout<<"module1="<<boost::math::erf((x_upper-RSPmid)/S)<<std::endl;
// 		cout<<"module2="<<boost::math::erf(-RSPmid/S)<<std::endl;
// 		cout<<"module3="<<exp(pow(S/(2*lambda_q),2)-(x_upper-RSPmid)/lambda_q)<<std::endl;
// 		cout<<"module4="<<boost::math::erfc(S/(2*lambda_q)-(x_upper-RSPmid)/S)<<std::endl;
// 		cout<<"module5="<<exp(pow(S/(2*lambda_q),2))<<std::endl;
// 		cout<<"module5'="<<exp(-(0-RSPmid)/lambda_q)<<std::endl;
// 		cout<<"module6="<<boost::math::erfc(erfparam1)<<std::endl;
// 		cout<<module<<std::endl;
// 		if (module > 0)
// 		value =  Pm/module * (-lambda_q*(exp(expparam) * boost::math::erfc(erfparam)-exp(expparam1) * boost::math::erfc(erfparam1))
// 		+ lambda_q * (boost::math::erf((Rmid-RSPmid)/S)-boost::math::erf(-RSPmid/S)));
// 		else return 0;
//
// 		Flush_getBr(&np, &Rdiv, &Zdiv, &Br, &ierr);
// 		Flush_getBt(&np, &Rdiv, &Zdiv, &Bt, &ierr);
// 		Flush_getBz(&np, &Rdiv, &Zdiv, &Bz, &ierr);
//
// 		getNormalT6(alphar, alphazTemp, x);//+0.02 if no coordinate file
// 		alphat = -alphazTemp * std::sin(0.61 * M_PI/180);
// 		alphaz = alphazTemp * std::cos(0.61 * M_PI/180);
//
// 		cout<<"cos"<< Bt <<"\t"<< Br <<"\t"<<Bz<<std::endl;
//
// 		if (Br < 0) return 0;
// 		double cosine_tile = (-((alphar * Br + alphat * Bt + alphaz * Bz)/std::sqrt(pow(Br, 2) + pow(Bt, 2) + pow(Bz, 2))));
// 		double cosine_flat = (-( 1 * Bz/std::sqrt(pow(Br, 2) + pow(Bt, 2) + pow(Bz, 2))));
// 		return value * cosine_tile/cosine_flat;
#endif
	}else if (useShadow){
		if(module != 0){
			double lowlimit, uplimit;

			getShadowLimit(lowlimit, uplimit);
			//cout<<"up\t"<<uplimit<<"low\t"<<lowlimit<<std::endl;
			erfparam1 = S/(2*lambda_q)-(lowlimit-currSP)/S;
			expparam1 = pow(S/(2*lambda_q),2)-(lowlimit-currSP)/lambda_q;
			if (!useFlush){
				if (x> lowlimit && x<uplimit){
					value =  Pm/module * (-lambda_q*(exp(expparam) * boost::math::erfc(erfparam)-exp(expparam1) * boost::math::erfc(erfparam1))
					+ lambda_q * (boost::math::erf((x-currSP)/S)-boost::math::erf((lowlimit-currSP)/S)));
					return value;
				}
				else if(x>uplimit && uplimit > lowlimit){

					erfparam = S/(2*lambda_q)-(uplimit-currSP)/S;
					expparam = pow(S/(2*lambda_q),2)-(uplimit-currSP)/lambda_q;
					value =  Pm/module * (-lambda_q*(exp(expparam) * boost::math::erfc(erfparam)-exp(expparam1) * boost::math::erfc(erfparam1))
					+ lambda_q * (boost::math::erf((uplimit-currSP)/S)-boost::math::erf((lowlimit-currSP)/S)));
					return value;
				}else{
					return 0;
				}
			}else{
				if (!PDFcomputed) computePDF();
				std::map<double,double> Map;
				i_tDoubleMap it = PDFProjection.lower_bound(time);
				i_tDoubleMap prevIt = it;
				--prevIt;
				if (std::abs(it->first - time) > std::abs(prevIt->first - time))
					Map = prevIt->second;
				else
					Map = it->second;
// 				module = mknix::interpolate1D(time, moduleProjection);
// 				x_upper = 8;
// 				double Rup, Zup, Rlow, Zlow, RupMid, RlowMid;
// 				checkFlushInitialisation(time);
// 				if (lambda_q < 0.006) lambda_q = 0.006;
// 				Rdiv = getRT6(x);
// 				Zdiv = getZT6(x);
// 				RSP = getRT6(currSP);
// 				ZSP = getZT6(currSP);
// 				Rup = getRT6(uplimit);
// 				Zup = getZT6(uplimit);
// 				Rlow = getRT6(lowlimit);
// 				Zlow = getZT6(lowlimit);
//
// 				Flush_getMidPlaneProjRight(&np, &RSP, &ZSP, &RSPmid, &ierr);
// 				Flush_getMidPlaneProjRight(&np, &Rdiv, &Zdiv, &Rmid, &ierr);
// 				Flush_getMidPlaneProjRight(&np, &Rup, &Zup, &RupMid, &ierr);
// 				Flush_getMidPlaneProjRight(&np, &Rlow, &Zlow, &RlowMid, &ierr);
// 				RSPmid /= 100;
// 				Rmid /= 100;
// 				RupMid /= 100;
// 				RlowMid /= 100;
//
//
// 				Flush_getBr(&np, &Rdiv, &Zdiv, &Br, &ierr);
// 				Flush_getBt(&np, &Rdiv, &Zdiv, &Bt, &ierr);
// 				Flush_getBz(&np, &Rdiv, &Zdiv, &Bz, &ierr);
//
// 				getNormalT6(alphar, alphazTemp, x);
// 				alphat = -alphazTemp * std::sin(0.61 * M_PI/180);
// 				alphaz = alphazTemp * std::cos(0.61 * M_PI/180);
// 				if (Br < 0) return 0;
				if (x>= lowlimit && x<=uplimit){
					for (double pos = lowlimit; pos < x; pos += 1e-3){
						if (module != 0) value += 1e-3 * mknix::interpolate1D(pos, Map)/module;
					}
					return value;
//
// 					erfparam = S/(2*lambda_q)-(Rmid-RSPmid)/S;
// 					expparam = pow(S/(2*lambda_q),2)-(Rmid-RSPmid)/lambda_q;
// 					erfparam1 = S/(2*lambda_q)-(0-RSPmid)/S;
// 					expparam1 = pow(S/(2*lambda_q),2)-(0-RSPmid)/lambda_q;
// 					module = lambda_q*((boost::math::erf((x_upper-RSPmid)/S)-boost::math::erf(-RSPmid/S))
// 					-(exp(pow(S/(2*lambda_q),2)-(x_upper-RSPmid)/lambda_q) * boost::math::erfc(S/(2*lambda_q)-(x_upper-RSPmid)/S)
// 					    -exp(pow(S/(2*lambda_q),2)-(0-RSPmid)/lambda_q) * boost::math::erfc(S/(2*lambda_q)-(0-RSPmid)/S)));
// 					if (module>0)
// 					value =  Pm/module * (-lambda_q*(exp(expparam) * boost::math::erfc(erfparam)-exp(expparam1) * boost::math::erfc(erfparam1))
// 					+ lambda_q * (boost::math::erf((Rmid-RSPmid)/S)-boost::math::erf(0-RSPmid/S)));
// 					else return 0;
// 					double cosine_tile = (-((alphar * Br + alphat * Bt + alphaz * Bz)/std::sqrt(pow(Br, 2) + pow(Bt, 2) + pow(Bz, 2))));
// 					double cosine_flat = (-( 1 * Bz/std::sqrt(pow(Br, 2) + pow(Bt, 2) + pow(Bz, 2))));
// 					return value *  cosine_tile/cosine_flat;
//
				}else if(module !=0 && x>uplimit && uplimit > lowlimit){
					for (double pos = lowlimit; pos < uplimit; pos += 1e-3){
						if (module != 0) value += 1e-3 * mknix::interpolate1D(pos, Map)/module;
					}

					return value;


// 					erfparam = S/(2*lambda_q)-(RupMid-RSPmid)/S;
// 					expparam = pow(S/(2*lambda_q),2)-(RupMid-RSPmid)/lambda_q;
// 					erfparam1 = S/(2*lambda_q)-(0-RSPmid)/S;
// 					expparam1 = pow(S/(2*lambda_q),2)-(0-RSPmid)/lambda_q;
// 					module = lambda_q*((boost::math::erf((x_upper-RSPmid)/S)-boost::math::erf(-RSPmid/S))
// 					-(exp(pow(S/(2*lambda_q),2)-(x_upper-RSPmid)/lambda_q) * boost::math::erfc(S/(2*lambda_q)-(x_upper-RSPmid)/S)
// 					    -exp(pow(S/(2*lambda_q),2)-(0-RSPmid)/lambda_q) * boost::math::erfc(S/(2*lambda_q)-(0-RSPmid)/S)));
// 					if (module > 0)
// 					value =  Pm/module * (-lambda_q*(exp(expparam) * boost::math::erfc(erfparam)-exp(expparam1) * boost::math::erfc(erfparam1))
// 					+ lambda_q * (boost::math::erf((RupMid-RSPmid)/S)-boost::math::erf(0-RSPmid/S)));
// 					else return 0;
//
// 					Rdiv = getRT6(uplimit);
// 					Zdiv = getZT6(uplimit);
// 					Flush_getBr(&np, &Rdiv, &Zdiv, &Br, &ierr);
// 					Flush_getBt(&np, &Rdiv, &Zdiv, &Bt, &ierr);
// 					Flush_getBz(&np, &Rdiv, &Zdiv, &Bz, &ierr);
//
// 					getNormalT6(alphar, alphazTemp, uplimit);
// 					alphat = -alphazTemp * std::sin(0.61 * M_PI/180);
// 					alphaz = alphazTemp * std::cos(0.61 * M_PI/180);
//
// 					double cosine_tile = (-((alphar * Br + alphat * Bt + alphaz * Bz)/std::sqrt(pow(Br, 2) + pow(Bt, 2) + pow(Bz, 2))));
// 					double cosine_flat = (-( 1 * Bz/std::sqrt(pow(Br, 2) + pow(Bt, 2) + pow(Bz, 2))));
// 					return value *  cosine_tile/cosine_flat;

				}else return 0;
			}

		}else{
		      return 0;
		}
	}else if(module != 0){

		value =  Pm/module * (-lambda_q*(exp(expparam) * boost::math::erfc(erfparam)-exp(expparam1) * boost::math::erfc(erfparam1))
		+ lambda_q * (boost::math::erf((x-currSP)/S)-boost::math::erf(-currSP/S)));
		return value;



	}else{
	      return 0;
	}
}

void Model::EichLimits(double& pdfLimit, double& cdfLimit){
	double currNBI,currICRH,currOHM,currSP,TP,Pm,scale,x_upper, expparam1, erfparam1, module;
	x_upper = 5;
	double time=analysisStartTime;
	double currMax=0;
	double timeMax=0;
	for(int i=0;i<pulseIterations;++i){
		currNBI = mknix::interpolate1D(time,NBI);
		currICRH = mknix::interpolate1D(time,ICRH);
		if(currNBI+currICRH>currMax){
			currMax=currNBI+currICRH;
			if(SPFileLoad || SF==0){
				currSP=mknix::interpolate1D(time,SP);
			}else{
				currSP=mknix::interpolate1D(time,SP)+(SA*sin((2*M_PI*SF)*(time-analysisStartTime)));
			}
			timeMax=time;
		}
		time+=stepTime;
	}


	if (AutoLambdaS){
			lambda_q = mknix::interpolate1D(time, lambda_qmap);
			S = mknix::interpolate1D(time, Smap);
// 			printf("lambda = %g\nS = %g\n", lambda_q, S);
	}
	erfparam1 = S/(2*lambda_q)-(0-currSP)/S;
	expparam1 = pow(S/(2*lambda_q),2)-(0-currSP)/lambda_q;
	module = lambda_q*((boost::math::erf((x_upper-currSP)/S)-boost::math::erf(-currSP/S))
		 -(exp(pow(S/(2*lambda_q),2)-(x_upper-currSP)/lambda_q) * boost::math::erfc(S/(2*lambda_q)-(x_upper-currSP)/S)
		    -exp(expparam1) * boost::math::erfc(erfparam1)));

	EichParams(timeMax,currNBI,currICRH,currOHM,currSP,TP,Pm,scale);
	pdfLimit = 2*Pm/module * lambda_q / (lambda_q + 1.64 * S);
	cdfLimit= Pm;
}


bool Model::writeTCtoFile(std::string path,QVector<double> time, QVector<double> x, QVector<double> y, QVector<double> z){

	double sizemax = std::max(time.size(), JPFtimeV.size());
	double sizemin = std::min(time.size(), JPFtimeV.size());
	std::ofstream stream(path.c_str(), std::ofstream::out);

	if(!stream.is_open()){
		printf("Can't save data in %s\n",path.c_str());
		return false;
	}else{
		if(JPFTCLoad && !x.empty()){
			stream << "Time \t maxTemperature \t TCx \t TCy \t TCz \t IR maxTemperature \t TCx JPF \t TCy JPF \t TCz JPF \n";
			stream << std::setprecision(std::numeric_limits<double>::digits10+1) ;
			for(int i=0;i<sizemin;++i){
				for(int j=0; j<sizemax; j++){
					if (time[i] >= JPFtimeV[j] && time[i] <= JPFtimeV[j+1]){
						stream << time[i] << "," << maxTemp[i] << "," << x[i] << "," << y[i] << "," << z[i] << "," << TMT6V[j]<< ","<< JPFTC[0][j] << "," << JPFTC[1][j] << "," << JPFTC[2][j] << "\n";
					}
				 }
			}
			stream.close();
		}else if(JPFTCLoad && x.empty() && TMT6FileLoad){
			  stream << "Time \t IR maxTemperature \t TCx JPF \t TCy JPF \t TCz JPF \n";
			  stream << std::setprecision(std::numeric_limits<double>::digits10+1) ;
			  for(int i=0; i<JPFtimeV.size(); i++){
				  stream << TMT6V[i]<< ","<<  JPFtimeV[i] << "," << JPFTC[0][i] << "," << JPFTC[1][i] << "," << JPFTC[2][i] << "\n";
			  }
			  stream.close();
		}else{
			stream << "Time \t maxTemperature \t TCx \t TCy \t TCz \n";
			stream << std::setprecision(std::numeric_limits<double>::digits10+1) ;
			for(int i=0; i<time.size(); i++){
				stream << time[i] << "," << maxTemp[i] << "," << x[i] << "," << y[i] << "," << z[i] << "\n";
			}
			stream.close();
		}
	}

	return true;
}

bool Model::writeShadow(std::string path, int res){

	std::ofstream stream(path.c_str(), std::ofstream::out);

	if(!stream.is_open()){
		printf("Can't save data in %s\n",path.c_str());
		return false;
	}else{
		for (int i = 0; i<res+1; ++i){
			stream<<shadowPosition[i]<<",";
		}
		stream<<"\n";
		for (int i = 0; i<res+1; ++i){
			stream<<shadow[i]<<",";
		}
		stream.close();
	}
	return true;
}


void Model::getNormalT6(double& alphar, double& alphaz, double x){
	double r = getRT6(x);
			double z = (boost::math::erf(0.284*(r-290.745))-101.681)*1.727;

			alphar = cos(M_PI/2 - atan(2/sqrt(M_PI) * 1.727* 0.284 * exp(-pow(0.284*(r-290.745),2))));
			alphaz = sin(M_PI/2 - atan(2/sqrt(M_PI) * 1.727* 0.284 * exp(-pow(0.284*(r-290.745),2))));
// 	if (r<=285.703){
// 		alphar = 0;
// 		alphaz = 1;
// 	}
// 	else if (r<=287.846 && r > 285.703){
// 		alphar = cos(M_PI/2 - atan(0.207));
// 		alphaz = sin(M_PI/2 - atan(0.207));
// 	}
// 	else if (r<=293.644 && r>287.846 ){
// 		alphar = cos(M_PI/2 - atan(0.438));
// 		alphaz = sin(M_PI/2 - atan(0.438));
// 	}
// 	else if (r<=295.732 && r> 293.644){
// 		alphar = cos(M_PI/2 - atan(0.218));
// 		alphaz = sin(M_PI/2 - atan(0.218));
// 	}else{
// 		alphar = 0;
// 		alphaz = 1;
// 	}
}

double Model::getZT6( double x){
	double r = getRT6(x);

	return (boost::math::erf(0.284*(r-290.745))-101.681)*1.727;
// 	if (r<=285.703){
// 		return -171.158;
// 	}
// 	else if (r<=287.846 && r > 285.703){
// 		return -0.207*r-112.018;
// 	}
// 	else if (r<=293.644 && r>287.846 ){
// 		return -0.438*r-45.651;
// 	}
// 	else if (r<=295.732 && r> 293.644){
// 		return -0.218*r-110.01;
// 	}else{
// 		return -174.595;
// 	}
}

double Model::getRT6(double x){
	return x*97.717+6.668;
}

double Model::getFluxExpansion(double time, double x){ //Not used yet
#ifdef DJET_INSTALL
	float flushTime;
	long int ierr;
	int err;
	long int np = 1;
	Flush_getTime(&flushTime, &ierr);
	if ((static_cast<double>(flushTime) > time + stepTime/2 || static_cast<double>(flushTime) < time - stepTime/2)) {flushQuickInit(&shot, &time, &err); std::cout<<"oui"<<std::endl;}

	double Br, Bz, Rmid, Zmid,Rdiv, Zdiv;
	double beta, costheta;
	double alphar, alphat, alphaz;
	double fx;
	double db_drdiv, db_drmid;

	qmid_avg.resize(loadCoord.size());
	Zmid = 0;


	Rdiv = getRT6(x);
	Zdiv = getZT6(x);
	Flush_getMidPlaneProjRight(&np, &Rdiv, &Zdiv, &Rmid, &ierr);
	Flush_getBr(&np, &Rdiv, &Zdiv, &Br, &ierr);
	Flush_getBz(&np, &Rdiv, &Zdiv, &Bz, &ierr);
	beta = std::atan(-Bz/Br); //Angle between horizontal and B
	getNormalT6(alphar, alphaz, x);
	Flush_getdpsidr(&np, &Rdiv, &Zdiv, &db_drdiv, &ierr);
	Flush_getdpsidr(&np, &Rmid, &Zmid, &db_drmid, &ierr);
	costheta = -(Br*alphar + Bz * alphaz)/std::sqrt(pow(Br,2) + pow(Bz,2)); //theta is the angle between alpha and B

	fx = ((Rmid * db_drmid)/(Rdiv * db_drdiv))/costheta * std::sin(beta);
	if (fx<0) return 0;
	else return fx;
}

void Model::checkFlushInitialisation(double time){
	float flushTime;
	long int ierr;
	int err;
	if (flushStartTime == 0 && flushEndTime == 0){
		flushStartTime = analysisStartTime;
		flushEndTime = analysisStartTime + pulseIterations * stepTime;
		flushQuickInit(&shot, &flushStartTime, &err);
		flushQuickInit(&shot, &flushEndTime, &err);
	}
	Flush_getTime(&flushTime, &ierr);
	if ((static_cast<double>(flushTime) > time + stepTime/2 || static_cast<double>(flushTime) < time - stepTime/2) && time > flushStartTime && time < flushEndTime) flushQuickInit(&shot, &time, &err);

#else
	return(0);
#endif
}

void Model::ALICIAMidPlane(double time, std::vector<double> q){
	int err;
	long int ierr;
	long int np = 1;
	int pulse = 90287;

	double timeF, timeI;
	if (!SPFileLoad) loadSPJPF(pulse);
	if (!OHMLoad) loadOHMPPF(pulse);
	if (!NBIFileLoad) loadNBIPPF(pulse);
	if (!ICRHFileLoad) loadICRHPPF(pulse);
	if (!BtLoad) readParamPPF(pulse, BTSIGNAL, BtPPF, timeI, BtLoad, BTTYPE, timeF);
	if (!IpLoad) readParamPPF(pulse, IPSIGNAL, IPPF, timeI, IpLoad, IPTYPE, timeF);
	if (!neLoad) readParamPPF(pulse, NESIGNAL, nePPF, timeI, neLoad, NETYPE, timeF);
	if (!fELMLoad) readParamPPF(pulse, FELMSIGNAL, fELMPPF, timeI, fELMLoad, FELMTYPE, timeF);
	if (!useTOPI) readParamPPF(pulse, BOLOSIGNAL, TOPI, timeI, useTOPI, TOPITYPE, timeF);


	if (!OHMLoad) {
		OHM[0] = 0;
		OHMLoad = TRUE;
	}
	if (!NBIFileLoad){
		NBI[0] = 0;
		NBIFileLoad = TRUE;
	}
	if (!ICRHFileLoad){
		ICRH[0] = 0;
		ICRHFileLoad = TRUE;
	}
	if (!BtLoad){
		BtPPF[0] = 0;
		BtLoad = TRUE;
	}
	if (!IpLoad){
		IPPF[0] = 0;
		IpLoad = TRUE;
	}
	if (!neLoad){
		nePPF[0] = 0;
		neLoad = TRUE;
	}
	if (!fELMLoad){
		fELMPPF[0] = 0;
		fELMLoad = TRUE;
	}

	double Bt, Br, Bz, Rmid, Zmid,Rdiv, Zdiv;
	double beta, alpha_3D;
	double alphar, alphat, alphaz, alphazTemp;
	double fx, fx_h;
	double db_drdiv, db_drmid;
	double bg = q[4];
	std::vector<double> vec;

	qmid_avg.resize(loadCoord.size(), 0);
	Zmid = 0;
	fx_h = 0;

	#ifdef DJET_INSTALL
	flushQuickInit(&pulse, &time, &err);
	for (int i = 0; i< loadCoord.size(); ++i){
		  Rdiv = getRT6(loadCoord[i]);//+0.02 if no coordinate file
		  Zdiv = getZT6(loadCoord[i]);//+0.02 if no coordinate file
		  Flush_getMidPlaneProjRight(&np, &Rdiv, &Zdiv, &Rmid, &ierr);
		  Flush_getBr(&np, &Rdiv, &Zdiv, &Br, &ierr);
		  Flush_getBt(&np, &Rdiv, &Zdiv, &Bt, &ierr);
		  Flush_getBz(&np, &Rdiv, &Zdiv, &Bz, &ierr);
		  beta = std::atan(-Bz/Br);
		  getNormalT6(alphar, alphazTemp, loadCoord[i]);//+0.02 if no coordinate file
		  alphat = -alphazTemp * std::sin(0.61 * M_PI/180);
		  alphaz = alphazTemp * std::cos(0.61 * M_PI/180);
		  alpha_3D = std::acos((alphar * Br + alphat * Bt + alphaz * Bz)/std::sqrt(pow(Br, 2) + pow(Bt, 2) + pow(Bz, 2)));
		  Flush_getdpsidr(&np, &Rdiv, &Zdiv, &db_drdiv, &ierr);
		  Flush_getdpsidr(&np, &Rmid, &Zmid, &db_drmid, &ierr);
		  fx = getFluxExpansion(time, loadCoord[i]); //+0.02 if no coordinate file
		  vec.push_back(-fx * (q[i] - bg)/cos(alpha_3D));
		  qmid_avg[i] +=  -fx * (q[i] - bg)/cos(alpha_3D);
		  fxavg += fx;
	}
#endif
// 	if (time>48.525 && time < 48.535)
	 getLambdaS(time, vec);
	qMidPlane.insert(std::pair<double, std::vector<double> >(time, vec));
	if (time >= /*89295 50*/ /*90271 57.8*/ /*90287 54*/ 53){
  		write_qmidplane();
 		writeLambdaS();
	}
}

bool Model::write_qmidplane(){
	std::string path = "/home/lvitton/Livio Vitton/Projects/qMidPlane.csv";
	std::ofstream stream(path.c_str(), std::ofstream::out);

	if(!stream.is_open()){
		printf("Can't save data in %s\n",path.c_str());
		return false;
	}else{
		for (int i = loadCoord.size()-1; i>=0; --i){
			stream<<qmid_avg[i]/qMidPlane.size()<<",";
		}
		stream<<"\n";
		for (i_tvect it = qMidPlane.begin(); it != qMidPlane.end(); ++it){
			stream<<it->first<<",";
			for (int i = loadCoord.size()-1; i>=0; --i){
				stream<<it->second[i]<<",";
			}
			stream<<"\n";
		}
		//cout<<fxavg/(qMidPlane.size()*loadCoord.size())<<std::endl;
		stream.close();
	}
	return true;
}

void Model::getLambdaS(double time, std::vector<double> q){
	double lambdaFit, Sfit, qFit;
	double prevChi, chi, prevChiTot, chiTot;

	double currNBI,currICRH,currOHM,currSP,TP,Pm,scale;

	double lambdaPrec = 1e-4;
	double Sprec = 1e-4;
	double qPrec = 1e6;
	double SPprec = 1e-4;

	std::vector<double> params(16);

	EichParams(time,currNBI,currICRH,currOHM,currSP,TP,Pm,scale);

	params[3] = mknix::interpolate1D(time, IPPF);
	params[4] = mknix::interpolate1D(time, BtPPF);
	params[5] = mknix::interpolate1D(time, nePPF);
	params[6] = currNBI;
	params[7] = currICRH;
	params[8] = currOHM;
	params[9] = mknix::interpolate1D(time, TOPI);
	params[10] = mknix::interpolate1D(time, fELMPPF);
	params[13] = currSP;
	params[14] = 1.6*pow(mknix::interpolate1D(time,IPPF) * (-1e-6),-0.24)*pow(mknix::interpolate1D(time, BtPPF), 0.52)*pow(mknix::interpolate1D(time, nePPF) * 1e-20, -1)*pow((currNBI+currICRH+currOHM- mknix::interpolate1D(time, TOPI))*1e-6,0.023)*pow(mknix::interpolate1D(time, fELMPPF),0.15);
	params[15] = 1.6*pow(mknix::interpolate1D(time, IPPF)*(-1e-6),0.74)*pow(mknix::interpolate1D(time, BtPPF), -0.83)*pow(mknix::interpolate1D(time, nePPF)*1e-20, -0.6)*pow((currNBI+currICRH+currOHM - mknix::interpolate1D(time, TOPI))*1e-6,0.052)*pow(mknix::interpolate1D(time, fELMPPF),-0.11);

	lambdaFit = 0.01;
	Sfit = 0.01;
	qFit = *std::max_element(q.begin(), q.end());
	currSP -= 4e-2;
	chi = computeChi(currSP, lambdaFit, Sfit, qFit, q);
	if (chi == 1e32) currSP += 4e-2;
	chi = computeChi(currSP, lambdaFit, Sfit, qFit, q);
	if (chi == 1e32) {
		params[0] = 0;
		params[1] = 0;
		params[2] = 0;
		params[11] = 1e32;
		params[12] = 0;
		lambdaS.insert(std::pair<double, std::vector<double> >(time, params));
		return;
	}

	chiTot = chi;


	do{
		prevChiTot = chiTot;
		if (computeChi(currSP, lambdaFit, Sfit, qFit-qPrec, q) < computeChi(currSP, lambdaFit, Sfit, qFit+qPrec, q))
			qPrec *= -1;
		chi = computeChi(currSP, lambdaFit, Sfit, qFit+qPrec, q);
		do{
			prevChi = chi;
			qFit += qPrec;
			chi = computeChi(currSP, lambdaFit, Sfit, qFit, q);
		}while(prevChi >= chi);
		if (computeChi(currSP-SPprec, lambdaFit, Sfit, qFit, q) < computeChi(currSP+SPprec, lambdaFit, Sfit, qFit, q))
			SPprec *= -1;
		chi = computeChi(currSP+SPprec, lambdaFit, Sfit, qFit, q);
		do{
			prevChi = chi;
			currSP += SPprec;
			chi = computeChi(currSP, lambdaFit, Sfit, qFit, q);
// 			cout<<"prevchi="<<prevChi<<std::endl;
// 			cout<<"chi="<<chi<<std::endl;
			if (chi == 1e32){
				params[0] = 0;
				params[1] = 0;
				params[2] = 0;
				params[11] = 1e32;
				params[12] = 0;
				lambdaS.insert(std::pair<double, std::vector<double> >(time, params));
				return;
			}
		}while(prevChi >= chi);
		if (computeChi(currSP, lambdaFit, Sfit-Sprec, qFit, q) < computeChi(currSP, lambdaFit, Sfit+Sprec, qFit, q))
			Sprec *= -1;
		chi = computeChi(currSP, lambdaFit, Sfit+Sprec, qFit, q);
		do{
			prevChi = chi;
			Sfit += Sprec;
			chi = computeChi(currSP, lambdaFit, Sfit, qFit, q);
			if (Sfit > 0.5 || Sfit < 0){
				params[0] = 0;
				params[1] = 0;
				params[2] = 0;
				params[11] = 1e32;
				params[12] = currSP;
				lambdaS.insert(std::pair<double, std::vector<double> >(time, params));
				return;
			}
// 			cout<<"prevchi="<<prevChi<<std::endl;
// 			cout<<"chi="<<chi<<std::endl;
		}while(prevChi >= chi);
		if (computeChi(currSP, lambdaFit-lambdaPrec, Sfit, qFit, q) < computeChi(currSP, lambdaFit+lambdaPrec, Sfit, qFit, q))
			lambdaPrec *= -1;
		chi = computeChi(currSP, lambdaFit+lambdaPrec, Sfit, qFit, q);
		do{
			prevChi = chi;
			lambdaFit += lambdaPrec;
			chi = computeChi(currSP, lambdaFit, Sfit, qFit, q);
// 			cout<<"prevchi="<<prevChi<<std::endl;
			if (lambdaFit > 0.5 || lambdaFit < 0){
				params[0] = 0;
				params[1] = 0;
				params[2] = 0;
				params[11] = 1e32;
				params[12] = currSP;
				lambdaS.insert(std::pair<double, std::vector<double> >(time, params));
				return;
			}
		}while(prevChi >= chi);
		chiTot = chi;
// 		cout<<"prevChi= "<<prevChiTot<<"\tchi= "<<chiTot<<std::endl;
// 		cout<<"et un de plus"<<std::endl;
	}while (prevChiTot > chiTot);
	cout<<"time"<<time<<"\tlambda="<<lambdaFit<<"\tS="<<Sfit<<"\tq="<<qFit<<"\tSP"<<currSP<<std::endl;
	cout<<"chi=\t"<<chiTot<<std::endl;
	params[0] = lambdaFit;
	params[1] = Sfit;
	params[2] = qFit;
	params[11] = chiTot;
	params[12] = currSP;
	lambdaS.insert(std::pair<double, std::vector<double> >(time, params));

}

double Model::computeChi(double SP, double lambdaFit, double Sfit,double qFit, std::vector<double> q){
#ifdef DJET_INSTALL
	double erfparam, expparam, module, erfparam1, expparam1;
	long int ierr;
	long int np = 1;
	double rSP, zSP, Rdiv, Zdiv, Rmid;
	double x_upper = 5;
	double chi = 0;
	if (SP < 2.80154 || SP > 2.98854)
	  return 1e32;
	rSP = getRT6(SP);
	zSP = getZT6(SP);
	Flush_getMidPlaneProjRight(&np, &rSP, &zSP, &SP, &ierr);
	SP /= 100;
	erfparam1 = Sfit/(2*lambdaFit)-(0-SP)/Sfit;
	expparam1 = pow(Sfit/(2*lambdaFit),2)-(0-SP)/lambdaFit;
	module = lambdaFit*((boost::math::erf((x_upper-SP)/Sfit)-boost::math::erf(-SP/Sfit))
	  -(exp(pow(Sfit/(2*lambdaFit),2)-(x_upper-SP)/lambdaFit) * boost::math::erfc(Sfit/(2*lambdaFit)-(x_upper-SP)/Sfit)
	  -exp(expparam1) * boost::math::erfc(erfparam1)));
	if (module != 0){
		for (int i = 4; i<q.size()-4; ++i){
			Rdiv = getRT6(loadCoord[i]); //+0.02 if no coordinate file
			Zdiv = getZT6(loadCoord[i]);//+0.02 if no coordinate file
			Flush_getMidPlaneProjRight(&np, &Rdiv, &Zdiv, &Rmid, &ierr);
			Rmid /= 100;
			erfparam = Sfit/(2*lambdaFit)-(Rmid-SP)/Sfit;
			expparam = pow(Sfit/(2*lambdaFit),2)-(Rmid-SP)/lambdaFit;
// 			cout<<qFit/*/module*/ * boost::math::erfc(erfparam) * exp(expparam)<<"\t"<<Rmid<<std::endl;
			chi += pow(qFit/*/module*/ * boost::math::erfc(erfparam) * exp(expparam) - q[i], 2);
		}
// 		cout<<"lambda="<<lambdaFit<<"\t"<<"S="<<Sfit<<"\t"<<"q="<<qFit<<std::endl;
// 		cout<<chi<<std::endl;
// 		cout<<"C'est la fin"<<SP<<std::endl;
		return chi;
	}else{
		return 1e32;
	}
#else
	return 1e32;
#endif
}


bool Model::writeLambdaS(){
	std::string path = "/home/lvitton/Livio Vitton/Projects/lambdaS.csv";
	std::ofstream stream(path.c_str(), std::ofstream::out);

	if(!stream.is_open()){
		printf("Can't save data in %s\n",path.c_str());
		return false;
	}else{
		stream<<"time"<<","<<"Lambda_q"<<","<<"S"<<","<<"q0/module"<<","<<"Ip"<<","<<"Bt"<<","<<"ne"<<","<<"NBI"<<","<<"ICRH"<<","<<"OHM"<<","<<"Radiated Power"<<","<<"fELM"<<","<<"chi"<<","<<"SP fit"<<","<<"SP PPF"<<","<<"Lambda valeria"<<","<<"S valeria"<<"\n";
		for (i_tvect it = lambdaS.begin(); it!=lambdaS.end(); ++it){
			stream<<it->first<<",";
			for (int i = 0; i<it->second.size(); ++i){
				stream<<it->second[i]<<",";
			}
			stream<<"\n";
		}

		stream.close();
	}
	return true;
}

void Model::writeHeatFlux(std::string path){
	double x_lower,x_upper;
	int err;
	getXLimits(x_lower,x_upper);
	double step = (x_upper-x_lower)/reso;
	double Btimeav, Btimeap;
	std::ofstream stream(path.c_str(), std::ofstream::out);

	if(!stream.is_open()){
		printf("Can't save data in %s\n",path.c_str());
	}else{
		//stream<<"time"<<",";
		for (double x = x_lower; x<= x_upper; x += step){
				stream<<x<<" ";
		}
		stream<<"\n";
		for (double time = analysisStartTime; time<analysisStartTime + (pulseIterations-10) * stepTime; time += stepTime){

#ifdef DJET_INSTALL
			flushQuickInit(&shot, &time, &err);
#endif
			if (time == Btimeav) time = Btimeap;
			else Btimeav = time;
			stream<<time<<"\t";
			cout<<time<<std::endl;
			for (double x = x_lower; x<= x_upper; x += step){
				switch(dist){
					case Triangle_Dist:
						stream<<trianglePDF(time, x)<<" ";
						break;
					case Skew_Normal_Dist:
						stream<<skewNormalPDF(time, x)<<" ";
						break;
		//Eich
					case Eich_Dist:
						stream<<EichPDF(time, x)<<" ";
						break;
					}
			}
			stream<<"\n";
			Btimeap = time + stepTime;
		}
	}
}


double Model::calculateNBIRFEnergy(){
    double t_energy = 0.0;
    i_tDoubleDouble it = NBI.begin();
    i_tDoubleDouble it_previous = NBI.begin();

    if(NBI.size()>0){
        ++it;
        for (it; it != NBI.end(); ++it){
            t_energy += 0.5*(it_previous->second + it->second)*(it->first - it_previous->first);
            it_previous = it;
        }
    }
    std::cout << "Energy NBI = " << t_energy << std::endl;
    if(ICRH.size()>0){
        it = ICRH.begin();
        it_previous = ICRH.begin();
        ++it;
        for (it; it != ICRH.end(); ++it){
            t_energy += 0.5*(it_previous->second + it->second)*(it->first - it_previous->first);
            it_previous = it;
        }
    }
    std::cout << "Energy NBI+ICRH = " << t_energy << std::endl;
    return t_energy;
}

double Model::calculateInputEnergy(){
    double t_energy = 0.0;
    i_tDoubleDouble it = NBI.begin();
    i_tDoubleDouble it_previous = NBI.begin();
    if(NBI.size()>0){
        std::ofstream pFile("/tmp/power_NBI.txt");
        pFile << it->first << "\t" << it->second << std::endl;
        ++it;
        for (it; it != NBI.end(); ++it){
            pFile << it->first << "\t" << it->second << std::endl;
            t_energy += 0.5*(it_previous->second + it->second)*(it->first - it_previous->first);
            it_previous = it;
        }
    }
    std::cout << "Energy NBI = " << t_energy << std::endl;
    if(OHM.size()>0){
        it = OHM.begin();
        it_previous = OHM.begin();
        std::ofstream pFile("/tmp/power_OHM.txt");
        pFile << it->first << "\t" << it->second << std::endl;
        ++it;
        for (it; it != OHM.end(); ++it){
            pFile << it->first << "\t" << it->second << std::endl;
            t_energy += 0.5*(it_previous->second + it->second)*(it->first - it_previous->first);
            it_previous = it;
        }
    }
    std::cout << "Energy NBI+OHM = " << t_energy << std::endl;
    if(ICRH.size()>0){
        it = ICRH.begin();
        it_previous = ICRH.begin();
        std::ofstream pFile("/tmp/power_ICRH.txt");
        pFile << it->first << "\t" << it->second << std::endl;
        ++it;
        for (it; it != ICRH.end(); ++it){
            pFile << it->first << "\t" << it->second << std::endl;
            t_energy += 0.5*(it_previous->second + it->second)*(it->first - it_previous->first);
            it_previous = it;
        }
    }
    std::cout << "Energy NBI+OHM+ICRH = " << t_energy << std::endl;
    return t_energy;
}
