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

#include "modelexternalmesh.h"
#include <string>

ModelExternalMesh::ModelExternalMesh(std::string meshFile_in, std::string boundaryFile_in)
{
  meshFileName = meshFile_in;
  boundaryFileName = boundaryFile_in;
  this->xShift = 0.0;
  this->mirrorMesh = 0;
}

ModelExternalMesh::~ModelExternalMesh()
{

}

void ModelExternalMesh::generateModelFile()
{ // File location already stored in constructor. 
  // We could do a sanity check of the .dat file here... maybe

	this->tileFileName = meshFileName;
}

void ModelExternalMesh::printModelNodeLoads(std::ofstream& file)
{
	std::ifstream input;
	input.open(boundaryFileName);
	if (input.is_open()) {
		printf("Opened boundary file\n\n");
	}
	else {
		printf("Fail opening boundary file\n\n");
//		paramLoaded = false;
		return;
	}

	int node_number;
	std::string line;
	char* pEnd;

	//Parse numbers
	while (!input.eof()) {
		std::getline(input, line);
		if (!line.empty()) {
			node_number = std::strtod(line.c_str(), &pEnd);
			file << "    THERMALFLUENCE tile6." << node_number - 1 << " 0\n";
		}
	}


  file << "    \n";
  file << "    THERMALOUTPUT tile6.0 0\n";
  file << "    THERMALOUTPUT tile6.1 0\n";
  file << "    THERMALOUTPUT tile6.2 0\n";
  file << "    THERMALOUTPUT MAX_INTERFACE_TEMP\n";

}

 void ModelExternalMesh::constHeatFluence(int i,std::vector<double> x_coordinates,std::vector<double>& heatFluence){
//	 heatFluence[0] = 0;

 }
