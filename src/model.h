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

#ifndef MODEL_H
#define MODEL_H

#include <fstream>
#include <vector>
#include <QVector>
#include <string>

#include "simulation.h"

typedef enum {Const_Dist,Triangle_Dist,Skew_Normal_Dist, Eich_Dist, Loaded_Dist} Dist_t;
// typedef enum {Tile6_104, Tile6_516} Model_t;
// typedef enum {Tile6_109, Tile6_516} Model_t;
typedef enum {Tile6_109, Tile6_523} Model_t;

typedef enum {NBISIGNAL=0,ICRHSIGNAL, TMT6SIGNAL, ENERGYSIGNAL, BOLOSIGNAL, OHMSIGNAL, IPSIGNAL, BTSIGNAL, NESIGNAL, FELMSIGNAL} PPFSignal_t;
typedef enum {PTOTTYPE=0, TMT6TYPE, ENERGYTYPE, TOPITYPE, POHMTYPE, IPTYPE, BTTYPE, NETYPE, FELMTYPE} PPFDataType;
typedef enum {SPSIGNAL=0,TC_XSIGNAL,TC_YSIGNAL,TC_ZSIGNAL} JPFSignal_t;

typedef std::map<double, std::vector<double> >::const_iterator i_tvect;
typedef std::map<double, double >::const_iterator i_tDoubleDouble;
typedef std::map<double, std::map<double,double> >::const_iterator i_tDoubleMap;

class QProgressDialog;
class QTableWidgetItem;
#ifdef USE_QCUSTOMPLOT
class QCustomPlot;
#endif

class Model{
	public:

		Model();
		virtual ~Model();

		void init();
		void abort();
#ifdef USE_QCUSTOMPLOT
		void solve(QProgressDialog& progress,QCustomPlot* inputPlot=NULL,QCustomPlot* outputPlot=NULL,QCustomPlot* powerPlot=NULL,QCustomPlot* sp1Plot=NULL);
#else
		void solve(QProgressDialog& progress);
#endif
		const char* getOutputFilePath();
		double getStepTime();
		void getXLimits(double& x_lower, double& x_upper);///@todo define as static const?
		void getTimeLimits(double& t_lower, double& t_upper);
		void getDistLimits(double& pdfLimit, double& cdfLimit);
#ifdef USE_QCUSTOMPLOT
		void plotHeatFluenceDistribution(QCustomPlot* plot, double time=-1);
		void plotHeatFluenceDistribution2(QCustomPlot* plot, int step);
		void plotSolveHeatOutput(QCustomPlot* plot, int step);
		void plotJPFHeatOutput(QCustomPlot* plot, bool overrideLimits);
		void plotPowerAndSP(QCustomPlot* powerPlot,QCustomPlot* sp1Plot);
		void plotPowerTime(QCustomPlot* plot, int step);
		void plotSPTime(QCustomPlot* plot1, int step);
#endif
		bool setSP(double value);
		void loadSPfile(const char* path);
		void loadSPJPF(int jpn);
		bool isSPLoaded();
		void unloadSP();
		bool setNBI(double value);
		void loadNBIfile(const char* path);
		void loadNBIPPF(int jpn);
		bool isNBILoaded();
		void unloadNBI();
		bool setICRH(double value);
		void loadICRHfile(const char* path);
		void loadLoadFile(const char* path, const char* coordPath, bool coordFile);
		void loadICRHPPF(int jpn);
		void loadTMT6PPF(int jpn);
		void loadOHMPPF(int jpn);
		bool isOHMLoaded();
		bool isICRHLoaded();
		void unloadICRH();
		void loadTCJPF(int jpn);
		bool isTCJPFLoaded();
		void exportTCData(std::string simTCFile,bool& simExported, std::string loadTCFile, bool& loadExported);
		void unloadTCJPF();
		bool isLoadLoaded();
		void unloadLoadFile();
		void loadMeasuredEnergy(int jpn);
		bool isMeasuredEnergyLoaded();
		void loadTOPI();

		void shiftSP(double shift);

		bool useFlush;
		bool isFlushAvailable(int jpn);
		void checkFlushInitialisation(double time);
		bool useTOPI;
		bool useShadow;
		int shot;

		bool AutoLambdaS;
		void calculateLS();

		int pulseIterations; //!< pulseTime=pulseIterations/stepTime
		int cooldownIterations; //!< cooldownTime=cooldownIterations/stepTime
		double pulseTime;
		double W; 	//!< Width
		double RF;	//!<
		double WF;	//!< Wetted Fraction
		double ST;	//!< Starting temperature
		double B;	//!< Beta @todo not documented
		double SA;	//!< Sweep Amplitude
		double SF;	//!< Sweep Frequency
		double skewShape;
		double lambda_q;
		double S;
		double SPshift;
		int reso;

		std::map<double,double> lambda_qmap;
		std::map<double,double> Smap;
		std::vector<int> shadow;
		std::vector<double> shadowPosition;

		double MaxTemp;
		double TCxPeak;
		double TCyPeak;
		double TCzPeak;
		double Energy;
		double TCxJPFmaxTemp;
		double TCyJPFmaxTemp;
		double TCzJPFmaxTemp;
		double measuredEnergy;
		double peakPowerDensity;

		bool newScaling;

		void writeHeatFlux(std::string path);

		double calculateNBIRFEnergy();
		double calculateInputEnergy();

		Dist_t dist;

	protected:
		virtual void generateModelFile()=0;
		virtual void printModelNodeLoads(std::ofstream& file)=0;
		virtual void constHeatFluence(int i,std::vector<double> x_coordinates,std::vector<double>& heatFluence)=0;

		static const char* mknixFileName;
		static const char* title;
		static const char* cpFileName;
		static const char* kparFileName;
		double stepTime; //!< Integrator delta_t
		const double TL; //!< Tile Length
		const double nTiles; //!< Number of tiles
		const double OR; //!<Outboard ratio
		/*const char* */ std::string tileFileName;
		double xShift;
		bool mirrorMesh;

		std::map<double,double> SP;	//!< Strike Point
		std::map<double,double> NBI;	//!<
		std::map<double,double> ICRH;   //!<
		std::map<double,double> TMT6;
		std::map<double,double> mEnergy;
		std::map<double,double> TOPI;
		std::map<double,double> OHM;


		std::map<double,std::map<double,double> > load;
		std::map<double,std::map<double,double> > PDFProjection;
		std::map<double,double> moduleProjection;
		bool PDFcomputed;
	private:
		void updateStartTime();
		void updatePulseIterations();

		void generateSimulationInputFiles();
		void generateCpFile();
		void generateKparFile();
		virtual void generateMknix();
		void readParamInputFile(const char* path, std::map<double,double>& param, double& paramStartTime, bool& paramLoaded, double& paramPulseTime);
		void readLoadInputFile(const char* path, const char* coordPath, bool coordFile, std::map<double,std::map<double,double> >& param, double& paramStartTime, bool& paramLoaded);
		void readParamPPF(int jpn,PPFSignal_t signal, std::map<double,double>& param, double& paramStartTime, bool& paramLoaded, PPFDataType dtype, double& paramPulseTime);
		void readParamJPF(int jpn,JPFSignal_t signal, std::map<double,double>& param, double& paramStartTime, bool& paramLoaded, double& paramPulseTime);
		void calculateHeatFluence(double time,std::vector<double> x_coordinates,std::vector<double>& heatFluence);
		void triangleHeatFluence(double time,std::vector<double> x_coordinates,std::vector<double>& heatFluence);
		void skewNormalHeatFluence(double time,std::vector<double> x_coordinates,std::vector<double>& heatFluence);
		void EichHeatFluence(double time,std::vector<double> x_coordinates,std::vector<double>& heatFluence);
		void loadFileHeatFluence(double time,std::vector<double> x_coordinates,std::vector<double>& heatFluence);
		void triangleParams(double time, double& NBIout,double& ICRHout, double& OHMout,
									double& SPout, double& TPout, double& Pmout, double& x0out, double& x1out);

		void buildLoadPosition(const char* path, bool& found);
		double trianglePDF(double time, double x);
		double triangleCDF(double time, double x);
		void triangleLimits(double& pdfLimit, double& cdfLimit);
		void skewNormalParams(double time, double& NBIout,double& ICRHout, double& OHMout,
							double& SPout, double& TPout, double& Pmout, double& scaleout);
		void EichParams(double time, double& NBIout,double& ICRHout, double& OHMout,
							double& SPout, double& TPout, double& Pmout, double& scaleout);
		double skewNormalPDF(double time, double x);
		double skewNormalCDF(double time, double x);
		double EichPDF(double time, double x);
		double EichCDF(double time, double x);
		double loadFilePDF(double time, double x);
		double loadFileCDF(double time, double x);

		void loadFileLimits(double& pdfLimit, double& cdfLimit);
		void skewNormalLimits(double& pdfLimit, double& cdfLimit);
		void EichLimits(double& pdfLimit, double& cdfLimit);
		bool writeTCtoFile(std::string path,QVector<double> time, QVector<double> x, QVector<double> y, QVector<double> z);
		void MeanDoubVec(std::map<double, std::vector<double> > Map, std::vector<double>& vec);
		mknix::Simulation *modelSimulation;
		static const char* outputFileName;

		///ALICIA
		void ALICIAMidPlane(double time, std::vector<double> q);
		std::map<double,std::vector<double> > qMidPlane;
		std::map<double,std::vector<double> > lambdaS;
		std::vector<double> qmid_avg;
		void getLambdaS(double time, std::vector<double> q);
		bool write_qmidplane();
		bool writeLambdaS();
		double computeChi(double SP, double lambdaFit, double Sfit,double qFit, std::vector<double> q);
		std::map<double,double> BtPPF;
		std::map<double,double> nePPF;
		std::map<double,double> fELMPPF;
		std::map<double,double> IPPF;
		std::map<float,double> shadowTime;

		bool BtLoad;
		bool neLoad;
		bool fELMLoad;
		bool IpLoad;
		///end

		bool writeShadow(std::string path, int res);

		void computePDF();
		void getShadowLimit(double& lowlimit, double& uplimit);

		void getNormalT6(double& alphar, double& alphaz, double x);
		double getZT6( double x);
		double getRT6(double x);

		double meanDoubleD(std::map<double, double> Map, double factor, double standardLim);

		bool SPFileLoad;
		double SPStartTime;
		double SPPulseTime;
		bool NBIFileLoad;
		double NBIStartTime;
		double NBIPulseTime;
		bool TMT6FileLoad;
		double TMT6StartTime;
		bool ICRHFileLoad;
		double ICRHStartTime;
		double ICRHPulseTime;
		bool JPFTCLoad;
		double loadStartTime;
		bool loadFileLoad;
		double loadPulseTime;
		bool EnergyFileLoad;
		bool OHMLoad;
		double OHMStartTime;
		double OHMPulseTime;

		double TOPIStartTime;

		double getFluxExpansion(double time, double x);

		std::vector<double> loadCoord;

		double analysisStartTime;
		QVector<double> xCoords;
		double xMin, xMax;
		QVector<double> TCx;
		QVector<double> TCy;
		QVector<double> TCz;
		QVector<double> maxTemp;
		QVector<double> timeV;
		QVector<double> TMT6V;



		QVector<double> JPFTC[3];
		QVector<double> JPFtimeV;
		double  JPFmaxTemp;
		double  JPFminTemp;

		double flushStartTime;
		double flushEndTime;

		double fxavg;

};

#endif // MODEL_H
