# VITA
Virtual Interactive Thermal Analysis for the JET divertor.

## Description
VITA is a forward simulation code featuring a GUI for ease of use. Its main goal is to allow both quick and accurate analysis of divertor tiles to users by setting global machine parameters, recreating previous stored pulses, or a mix of both. The time varying boundary conditions and integration parameters are automatically set, therefore not requiring the user to deal with numerical details.

It is designed for pulse preparation activities, post-pulse checks, and integrity assessments of damaged components. It may also be used to test alternative divertor configurations under experimental conditions. It includes the following capabilities:
* Several ways for defining pulse parameters---manually, or reading them from files or stored signals---and automatic setting of boundary conditions.
* Connection to the JET database for the readout of experimental measurements.
* Selection of the divertor tile and accuracy of the approximation.
* Direct plotting of diagnostic synthetic signals.
* Tabulated output of maximum temperature at the surface and thermocouple measurement points, along with energy values.

VITA uses MkniX library for generating and solving the 2D numerical FEM models, and is distributed under the LGPL v2 license (see License section below).

## Installation

VITA uses the cmake build system for cross-platform compatibility (version 2.8 or greater).

Before building VITA, the source code and shared library of MkniX shall be available locally. Please use Mknix devel branch.

Build VITA using (from the VITA root directory):

```
cmake -B<build directory> -H. -DCMAKE_BUILD_TYPE=<Debug|Release> -DMKNIX_LINK_DIR=<path/to/dir/containing-libmknix_static.a> -DMKNIX_SOURCE_DIR=<path/to/dir/containing/MkniX>
```

The use of cmake-gui is recommended. Alternatively, an example of the cmake one-liner command is provided (note that paths might differ for each user):

```
cmake -B../VITA-build -H. -DCMAKE_BUILD_TYPE=Release -DMKNIX_LINK_DIR=../MkniX-devel-build/src/ -DMKNIX_SOURCE_DIR=../MkniX
```

### Options
There are two building options accessible via the cmake command or GUI:
* JET_INSTALL allows building the code with full access to the JET database. This shall be done only in servers or worksations with the JET Flush and PPF libraries available.
* QCUSTOMPLOT enables interactive plots for preparing the simulation, monitor the solving process, and reviewing the results. Nevertheless, note that QCustomPlot GPL license is not compatible with VITA LGPL license. Please use an available commercial license or contact the developer if you intend using this option.

## Examples

VITA is provided with some preset validation cases. These are available through the Menu File->Open...

## Usage

### Input power parameters: 
The total power input to the plasma arrives from either resistive heating, NBI or RF sources. Eachof the three signals can be defined as a constant value or a table from a file allowing complex manual load inputs. In the case where an experimental pulse is to be recreated, each of these values can be read from their corresponding signal in the JET database.

### Plasma parameters: 
The total power arriving to the divertor at any moment in time corresponds to the total input minus the radiated power. This is taken into account as a factor in the range [0-1] called the radiated fraction. The outboard-inboard power ratio is estimated as 1/3 inboard, 2/3 outboard, and the footprint can be defined using three different functions:
* A triangular function is the simplest way of defining the shape when little information is known in advance of the simulation. Only the width is needed for defining the footprint, allowing for a rough estimation of the bulk temperature evolution.
* A skewed Gaussian is a used as compromise between accuracy on the estimated footprint and speed. It is defined by its width and skewness values. 
* The convolution of an exponential with a Gaussian has been proven to be the best fit to the experimental observations. This function defines the profile of the scrape-off layer (SOL) at the equatorial plane. The parameters defining this function correspond to the power fall-off width and the spreading factor. Their values can be manually fixed or estimated as a function of the plasma current, toroidal field, integrated density, SOL power, ELM frequency, and the standard deviation of the radial field current.

### Magnetic parameters:
In the latter case, the power density needs to be projected from the equatorial to the divertor plane. By default the flux expansion is used, but an option is available for performing a 3D magnetic projection using the magnetic field components and the equilibrium reconstruction provided by the Flush code at each calculation time step. A second option allows the magnetic shadowing of the surrounding tiles to be taken into account.

The strike point position can be defined manually as a fixed location, or a regular sweep across it. It is also possible to input its evolution as a table or read it directly from an stored signal in the experimental database.

### Analysis parameters:
Once the physical quantities which define the loading conditions have been set, the Diritchlet boundary conditions are automatically defined in the model. The power density footprint is combined with the strike point time evolution, defining the power at each boundary point. The use of analytical functions for the heat flux profile allows calculating the exact power density at every surface node in an energy consistent manner (i.e. eliminating interpolation errors). In addition, the application of meshfree shape functions greatly increases the accuracy of surface temperature simulation. In the case where the loading parameters have been manually specified, the duration of the heating stage can be defined by the pulse time. Finally, the total simulation time is input using the analysis duration parameter.

## License

Copyright (C) 2007-2019 by Daniel Iglesias

Copyright (c) 2013-2018, European Atomic Energy Community (EURATOM)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public License
as published by the Free Software Foundation; either version 2.1
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

