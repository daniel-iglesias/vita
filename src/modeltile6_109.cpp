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

#include "modeltile6_109.h"


ModelTile6_109::ModelTile6_109()
{
  this->tileFileName = "Tile6_109.dat";
  this->xShift = 2.98711;
}

ModelTile6_109::~ModelTile6_109()
{

}

void ModelTile6_109::generateModelFile()
{
  std::ofstream file;
  file.open (tileFileName);
  
  file << "109 216\n";
  file << "1 -7.101343e-04 -2.947011e-04 2.794743e-06\n";
  file << "2 -4.144763e-04 -2.425600e-02 -4.553649e-17\n";
  file << "3 9.101796e-02 -2.425600e-02 0.000000e+00\n";
  file << "4 1.835680e-01 -5.256000e-03 0.000000e+00\n";
  file << "5 1.817049e-01 3.319309e-02 -6.505213e-19\n";
  file << "6 6.815985e-02 -2.425600e-02 0.000000e+00\n";
  file << "7 4.530174e-02 -2.425600e-02 0.000000e+00\n";
  file << "8 2.244363e-02 -2.425600e-02 0.000000e+00\n";
  file << "9 1.855680e-01 1.445410e-02 -4.498950e-18\n";
  file << "10 1.617730e-01 -5.256000e-03 0.000000e+00\n";
  file << "11 1.399780e-01 -5.256000e-03 0.000000e+00\n";
  file << "12 1.181831e-01 -5.256000e-03 0.000000e+00\n";
  file << "13 9.638929e-02 -5.289026e-03 3.347710e-17\n";
  file << "14 9.830963e-03 -6.006798e-03 1.122403e-06\n";
  file << "15 2.187470e-02 -5.710756e-03 1.145264e-06\n";
  file << "16 3.498610e-02 -4.979808e-03 1.091806e-06\n";
  file << "17 4.141842e-02 -5.055218e-03 1.148784e-06\n";
  file << "18 4.825984e-02 -3.477657e-03 8.915719e-07\n";
  file << "19 5.509378e-03 -2.716292e-06 -9.921439e-08\n";
  file << "20 1.180407e-02 -6.239349e-06 -9.763172e-08\n";
  file << "21 1.809876e-02 -9.762405e-06 -9.604966e-08\n";
  file << "22 2.439346e-02 -1.328547e-05 -9.480210e-08\n";
  file << "23 3.068815e-02 -1.624814e-05 -8.306460e-08\n";
  file << "24 3.696452e-02 4.057304e-04 3.541966e-07\n";
  file << "25 4.313821e-02 1.612517e-03 1.059092e-06\n";
  file << "26 4.911150e-02 3.585007e-03 2.020587e-06\n";
  file << "27 5.488836e-02 6.084701e-03 0.000000e+00\n";
  file << "28 6.065637e-02 8.605260e-03 0.000000e+00\n";
  file << "29 6.642438e-02 1.112582e-02 0.000000e+00\n";
  file << "30 7.219239e-02 1.364638e-02 0.000000e+00\n";
  file << "31 7.796041e-02 1.616694e-02 0.000000e+00\n";
  file << "32 8.372842e-02 1.868749e-02 0.000000e+00\n";
  file << "33 8.949643e-02 2.120805e-02 0.000000e+00\n";
  file << "34 9.526444e-02 2.372861e-02 0.000000e+00\n";
  file << "35 1.010325e-01 2.624917e-02 0.000000e+00\n";
  file << "36 1.068005e-01 2.876973e-02 0.000000e+00\n";
  file << "37 1.126851e-01 3.099342e-02 0.000000e+00\n";
  file << "38 1.188012e-01 3.246462e-02 0.000000e+00\n";
  file << "39 1.250536e-01 3.315624e-02 0.000000e+00\n";
  file << "40 1.313474e-01 3.321063e-02 0.000000e+00\n";
  file << "41 1.376421e-01 3.320844e-02 0.000000e+00\n";
  file << "42 1.439368e-01 3.320625e-02 0.000000e+00\n";
  file << "43 1.502315e-01 3.320405e-02 0.000000e+00\n";
  file << "44 1.565262e-01 3.320186e-02 0.000000e+00\n";
  file << "45 1.628209e-01 3.319967e-02 0.000000e+00\n";
  file << "46 1.691156e-01 3.319748e-02 0.000000e+00\n";
  file << "47 1.754103e-01 3.319528e-02 0.000000e+00\n";
  file << "48 5.970730e-02 2.608291e-03 -1.995049e-07\n";
  file << "49 7.159245e-02 6.283023e-03 -8.254287e-07\n";
  file << "50 8.228939e-02 1.293032e-02 -2.029084e-06\n";
  file << "51 9.560329e-02 1.543347e-02 -2.420957e-06\n";
  file << "52 1.065634e-01 2.132571e-02 -3.478119e-06\n";
  file << "53 1.168169e-01 2.736029e-02 -4.567240e-06\n";
  file << "54 1.285525e-01 2.750285e-02 -4.516992e-06\n";
  file << "55 1.407816e-01 2.718377e-02 -4.374980e-06\n";
  file << "56 1.537928e-01 2.690758e-02 -4.236019e-06\n";
  file << "57 1.657972e-01 2.818428e-02 -4.401423e-06\n";
  file << "58 1.768238e-01 2.215766e-02 -3.173153e-06\n";
  file << "59 8.405363e-02 -1.368791e-02 3.085620e-06\n";
  file << "60 1.573947e-02 -6.639107e-03 1.282683e-06\n";
  file << "61 2.855023e-02 -4.893474e-03 1.032710e-06\n";
  file << "62 5.451169e-02 -5.406343e-04 3.698367e-07\n";
  file << "63 6.544069e-02 4.166347e-03 -4.603025e-07\n";
  file << "64 7.728384e-02 9.739958e-03 -1.450542e-06\n";
  file << "65 8.920572e-02 1.186501e-02 -1.779129e-06\n";
  file << "66 1.005465e-01 1.968474e-02 -3.203300e-06\n";
  file << "67 1.118928e-01 2.420396e-02 -3.994684e-06\n";
  file << "68 1.221554e-01 2.744151e-02 -4.547520e-06\n";
  file << "69 1.345940e-01 2.746237e-02 -4.469295e-06\n";
  file << "70 1.474221e-01 2.668047e-02 -4.234594e-06\n";
  file << "71 1.602002e-01 2.715531e-02 -4.241156e-06\n";
  file << "72 1.715645e-01 2.710397e-02 -4.156188e-06\n";
  file << "73 8.715526e-02 -1.135818e-03 6.997329e-07\n";
  file << "74 1.725214e-01 8.857554e-03 -6.518015e-07\n";
  file << "75 1.501593e-01 2.577487e-03 4.043387e-07\n";
  file << "76 1.288689e-01 3.787644e-03 3.159537e-08\n";
  file << "77 1.063846e-01 2.347077e-03 1.591354e-07\n";
  file << "78 7.348450e-02 -9.529404e-03 2.218515e-06\n";
  file << "79 5.951045e-02 -1.534247e-02 3.240575e-06\n";
  file << "80 9.670448e-02 5.764022e-03 -5.599253e-07\n";
  file << "81 1.586469e-01 1.035391e-02 -1.030390e-06\n";
  file << "82 4.800230e-02 -1.247576e-02 2.614915e-06\n";
  file << "83 4.682091e-03 -9.558543e-03 1.769279e-06\n";
  file << "84 7.034191e-02 -1.127346e-03 5.869632e-07\n";
  file << "85 5.478078e-02 -7.870352e-03 1.776812e-06\n";
  file << "86 8.307329e-02 6.227682e-03 -7.389245e-07\n";
  file << "87 3.919177e-02 -1.302722e-02 2.662395e-06\n";
  file << "88 7.766292e-02 1.008666e-03 2.258593e-07\n";
  file << "89 1.049245e-01 1.198329e-02 -1.697897e-06\n";
  file << "90 1.311254e-01 1.995417e-02 -3.052810e-06\n";
  file << "91 1.437654e-01 1.919855e-02 -2.824389e-06\n";
  file << "92 1.583409e-01 2.002874e-02 -2.887196e-06\n";
  file << "93 3.050546e-02 -1.767759e-02 3.496507e-06\n";
  file << "94 1.401957e-01 4.633218e-03 -5.563518e-08\n";
  file << "95 1.160990e-01 6.450060e-03 -5.632387e-07\n";
  file << "96 2.048033e-02 -1.436170e-02 2.794538e-06\n";
  file << "97 1.171350e-02 -1.533610e-02 2.923390e-06\n";
  file << "98 1.468050e-01 1.118381e-02 -1.267772e-06\n";
  file << "99 6.286831e-02 -4.834809e-03 1.248325e-06\n";
  file << "100 1.241421e-01 1.343784e-02 -1.849713e-06\n";
  file << "101 1.360920e-01 1.279620e-02 -1.647707e-06\n";
  file << "102 1.133263e-01 1.585580e-02 -2.384763e-06\n";
  file << "103 1.186065e-01 2.100399e-02 -3.336829e-06\n";
  file << "104 1.249754e-01 2.148896e-02 -3.387702e-06\n";
  file << "105 1.374152e-01 2.116429e-02 -3.243225e-06\n";
  file << "106 1.512828e-01 1.862239e-02 -2.664239e-06\n";
  file << "107 1.667289e-01 1.961789e-02 -2.752982e-06\n";
  file << "108 2.627839e-02 -1.097144e-02 2.182912e-06\n";
  file << "109 3.226443e-02 -1.048256e-02 2.128758e-06\n";
  file << "1 102 2 1 \n";
  file << "2 102 3 6 \n";
  file << "3 102 6 7 \n";
  file << "4 102 7 8 \n";
  file << "5 102 8 2 \n";
  file << "6 102 4 10 \n";
  file << "7 102 10 11 \n";
  file << "8 102 11 12 \n";
  file << "9 102 12 13 \n";
  file << "10 102 13 3 \n";
  file << "11 102 5 9 \n";
  file << "12 102 9 4 \n";
  file << "13 102 1 19 \n";
  file << "14 102 19 20 \n";
  file << "15 102 20 21 \n";
  file << "16 102 21 22 \n";
  file << "17 102 22 23 \n";
  file << "18 102 23 24 \n";
  file << "19 102 24 25 \n";
  file << "20 102 25 26 \n";
  file << "21 102 26 27 \n";
  file << "22 102 27 28 \n";
  file << "23 102 28 29 \n";
  file << "24 102 29 30 \n";
  file << "25 102 30 31 \n";
  file << "26 102 31 32 \n";
  file << "27 102 32 33 \n";
  file << "28 102 33 34 \n";
  file << "29 102 34 35 \n";
  file << "30 102 35 36 \n";
  file << "31 102 36 37 \n";
  file << "32 102 37 38 \n";
  file << "33 102 38 39 \n";
  file << "34 102 39 40 \n";
  file << "35 102 40 41 \n";
  file << "36 102 41 42 \n";
  file << "37 102 42 43 \n";
  file << "38 102 43 44 \n";
  file << "39 102 44 45 \n";
  file << "40 102 45 46 \n";
  file << "41 102 46 47 \n";
  file << "42 102 47 5 \n";
  file << "43 203 19 20 14 \n";
  file << "44 203 21 22 15 \n";
  file << "45 203 23 24 16 \n";
  file << "46 203 24 25 17 \n";
  file << "47 203 25 26 18 \n";
  file << "48 203 27 28 48 \n";
  file << "49 203 29 30 49 \n";
  file << "50 203 31 32 50 \n";
  file << "51 203 33 34 51 \n";
  file << "52 203 35 36 52 \n";
  file << "53 203 37 38 53 \n";
  file << "54 203 39 40 54 \n";
  file << "55 203 41 42 55 \n";
  file << "56 203 43 44 56 \n";
  file << "57 203 45 46 57 \n";
  file << "58 203 5 9 58 \n";
  file << "59 203 13 3 59 \n";
  file << "60 203 3 6 59 \n";
  file << "61 203 20 21 60 \n";
  file << "62 203 22 23 61 \n";
  file << "63 203 24 17 16 \n";
  file << "64 203 25 18 17 \n";
  file << "65 203 26 27 62 \n";
  file << "66 203 28 29 63 \n";
  file << "67 203 30 31 64 \n";
  file << "68 203 32 33 65 \n";
  file << "69 203 34 35 66 \n";
  file << "70 203 36 37 67 \n";
  file << "71 203 38 39 68 \n";
  file << "72 203 40 41 69 \n";
  file << "73 203 42 43 70 \n";
  file << "74 203 44 45 71 \n";
  file << "75 203 46 47 72 \n";
  file << "76 203 47 5 58 \n";
  file << "77 203 13 59 73 \n";
  file << "78 203 9 4 74 \n";
  file << "79 203 4 10 74 \n";
  file << "80 203 10 11 75 \n";
  file << "81 203 11 12 76 \n";
  file << "82 203 12 13 77 \n";
  file << "83 203 59 6 78 \n";
  file << "84 203 60 21 15 \n";
  file << "85 203 6 7 79 \n";
  file << "86 203 14 20 60 \n";
  file << "87 203 15 22 61 \n";
  file << "88 203 26 62 18 \n";
  file << "89 203 48 28 63 \n";
  file << "90 203 49 30 64 \n";
  file << "91 203 50 32 65 \n";
  file << "92 203 51 34 66 \n";
  file << "93 203 52 36 67 \n";
  file << "94 203 53 38 68 \n";
  file << "95 203 54 40 69 \n";
  file << "96 203 55 42 70 \n";
  file << "97 203 56 44 71 \n";
  file << "98 203 57 46 72 \n";
  file << "99 203 58 9 74 \n";
  file << "100 203 61 23 16 \n";
  file << "101 203 64 31 50 \n";
  file << "102 203 65 33 51 \n";
  file << "103 203 66 35 52 \n";
  file << "104 203 67 37 53 \n";
  file << "105 203 68 39 54 \n";
  file << "106 203 69 41 55 \n";
  file << "107 203 70 43 56 \n";
  file << "108 203 71 45 57 \n";
  file << "109 203 77 13 80 \n";
  file << "110 203 78 6 79 \n";
  file << "111 203 63 29 49 \n";
  file << "112 203 62 27 48 \n";
  file << "113 203 13 73 80 \n";
  file << "114 203 74 10 81 \n";
  file << "115 203 81 107 74 \n";
  file << "116 203 10 75 81 \n";
  file << "117 203 79 7 82 \n";
  file << "118 203 1 19 83 \n";
  file << "119 203 63 49 84 \n";
  file << "120 203 18 62 85 \n";
  file << "121 203 64 50 86 \n";
  file << "122 203 16 17 87 \n";
  file << "123 203 49 64 88 \n";
  file << "124 203 64 86 88 \n";
  file << "125 203 65 51 80 \n";
  file << "126 203 66 52 89 \n";
  file << "127 203 67 53 103 \n";
  file << "128 203 54 69 90 \n";
  file << "129 203 55 70 91 \n";
  file << "130 203 56 71 92 \n";
  file << "131 203 77 80 89 \n";
  file << "132 203 93 87 7 \n";
  file << "133 203 73 59 78 \n";
  file << "134 203 83 19 14 \n";
  file << "135 203 75 11 94 \n";
  file << "136 203 11 76 94 \n";
  file << "137 203 12 77 95 \n";
  file << "138 203 60 15 96 \n";
  file << "139 203 14 60 97 \n";
  file << "140 203 60 96 97 \n";
  file << "141 203 8 2 97 \n";
  file << "142 203 75 94 98 \n";
  file << "143 203 17 18 82 \n";
  file << "144 203 48 63 99 \n";
  file << "145 203 63 84 99 \n";
  file << "146 203 76 12 95 \n";
  file << "147 203 78 79 99 \n";
  file << "148 203 76 95 100 \n";
  file << "149 203 94 76 101 \n";
  file << "150 203 98 94 101 \n";
  file << "151 203 76 100 101 \n";
  file << "152 203 95 77 89 \n";
  file << "153 203 14 97 83 \n";
  file << "154 203 65 80 73 \n";
  file << "155 203 49 88 84 \n";
  file << "156 203 80 51 89 \n";
  file << "157 203 52 67 102 \n";
  file << "158 203 53 68 103 \n";
  file << "159 203 68 54 104 \n";
  file << "160 203 103 68 104 \n";
  file << "161 203 54 90 104 \n";
  file << "162 203 69 55 105 \n";
  file << "163 203 70 56 106 \n";
  file << "164 203 71 57 107 \n";
  file << "165 203 82 7 87 \n";
  file << "166 203 73 78 88 \n";
  file << "167 203 81 75 98 \n";
  file << "168 203 96 15 108 \n";
  file << "169 203 15 61 108 \n";
  file << "170 203 61 16 109 \n";
  file << "171 203 108 61 109 \n";
  file << "172 203 16 87 109 \n";
  file << "173 203 87 93 109 \n";
  file << "174 203 97 96 8 \n";
  file << "175 203 102 67 103 \n";
  file << "176 203 93 8 96 \n";
  file << "177 203 69 105 90 \n";
  file << "178 203 106 56 92 \n";
  file << "179 203 70 106 91 \n";
  file << "180 203 71 107 92 \n";
  file << "181 203 82 87 17 \n";
  file << "182 203 7 8 93 \n";
  file << "183 203 82 18 85 \n";
  file << "184 203 52 102 89 \n";
  file << "185 203 79 82 85 \n";
  file << "186 203 105 55 91 \n";
  file << "187 203 85 62 99 \n";
  file << "188 203 62 48 99 \n";
  file << "189 203 50 65 86 \n";
  file << "190 203 51 66 89 \n";
  file << "191 203 57 72 107 \n";
  file << "192 203 73 88 86 \n";
  file << "193 203 47 58 72 \n";
  file << "194 203 78 99 84 \n";
  file << "195 203 72 58 107 \n";
  file << "196 203 97 2 83 \n";
  file << "197 203 100 95 102 \n";
  file << "198 203 58 74 107 \n";
  file << "199 203 92 107 81 \n";
  file << "200 203 90 105 101 \n";
  file << "201 203 79 85 99 \n";
  file << "202 203 109 93 108 \n";
  file << "203 203 103 104 100 \n";
  file << "204 203 91 106 98 \n";
  file << "205 203 81 98 106 \n";
  file << "206 203 65 73 86 \n";
  file << "207 203 102 95 89 \n";
  file << "208 203 2 1 83 \n";
  file << "209 203 88 78 84 \n";
  file << "210 203 105 91 101 \n";
  file << "211 203 91 98 101 \n";
  file << "212 203 104 90 100 \n";
  file << "213 203 100 90 101 \n";
  file << "214 203 108 93 96 \n";
  file << "215 203 81 106 92 \n";
  file << "216 203 100 102 103 \n";
  
  file.close();
  
}

void ModelTile6_109::printModelNodeLoads(std::ofstream& file)
{
  file << "    THERMALFLUENCE tile6.4 0\n";
  file << "    THERMALFLUENCE tile6.46 0\n";
  file << "    THERMALFLUENCE tile6.45 0\n";
  file << "    THERMALFLUENCE tile6.44 0\n";
  file << "    THERMALFLUENCE tile6.43 0\n";
  file << "    THERMALFLUENCE tile6.42 0\n";
  file << "    THERMALFLUENCE tile6.41 0\n";
  file << "    THERMALFLUENCE tile6.40 0\n";
  file << "    THERMALFLUENCE tile6.39 0\n";
  file << "    THERMALFLUENCE tile6.38 0\n";
  file << "    THERMALFLUENCE tile6.37 0\n";
  file << "    THERMALFLUENCE tile6.36 0\n";
  file << "    THERMALFLUENCE tile6.35 0\n";
  file << "    THERMALFLUENCE tile6.34 0\n";
  file << "    THERMALFLUENCE tile6.33 0\n";
  file << "    THERMALFLUENCE tile6.32 0\n";
  file << "    THERMALFLUENCE tile6.31 0\n";
  file << "    THERMALFLUENCE tile6.30 0\n";
  file << "    THERMALFLUENCE tile6.29 0\n";
  file << "    THERMALFLUENCE tile6.28 0\n";
  file << "    THERMALFLUENCE tile6.27 0\n";
  file << "    THERMALFLUENCE tile6.26 0\n";
  file << "    THERMALFLUENCE tile6.25 0\n";
  file << "    THERMALFLUENCE tile6.24 0\n";
  file << "    THERMALFLUENCE tile6.23 0\n";
  file << "    THERMALFLUENCE tile6.22 0\n";
  file << "    THERMALFLUENCE tile6.21 0\n";
  file << "    THERMALFLUENCE tile6.20 0\n";
  file << "    THERMALFLUENCE tile6.19 0\n";
  file << "    THERMALFLUENCE tile6.18 0\n";
  file << "    THERMALFLUENCE tile6.0 0\n";
  file << "    \n";
  file << "    THERMALOUTPUT tile6.102 0\n";
  file << "    THERMALOUTPUT tile6.98 0\n";
  file << "    THERMALOUTPUT tile6.14 0\n";
  file << "    THERMALOUTPUT MAX_INTERFACE_TEMP\n";

}

 void ModelTile6_109::constHeatFluence(int i,std::vector<double> x_coordinates,std::vector<double>& heatFluence){
//do not use...
	 heatFluence[0] = 0;
	 heatFluence[1] = 0;
	 heatFluence[2] = 0;
	 heatFluence[3] = 0;
	 heatFluence[4] = 0;
	 heatFluence[5] = 0;
	 heatFluence[6] = 0;
	 heatFluence[7] = 7.17E3;
	 heatFluence[8] = 6.53E3;
	 heatFluence[9] = 5.05E3;
	 heatFluence[10] = 3.56E3;
	 heatFluence[11] = 2.08E3;
	 heatFluence[12] = 6.02E2;
	 heatFluence[13] = 0;
	 heatFluence[14] = 0;
	 heatFluence[15] = 0;
	 heatFluence[16] = 0;
	 heatFluence[17] = 0;
	 heatFluence[18] = 0;
	 heatFluence[19] = 0;
	 heatFluence[20] = 0;
	 heatFluence[21] = 0;
	 heatFluence[22] = 0;
	 heatFluence[23] = 0;
	 heatFluence[24] = 0;
	 heatFluence[25] = 0;
	 heatFluence[26] = 0;
	 heatFluence[27] = 0;
	 heatFluence[28] = 0;
	 heatFluence[29] = 0;
	 heatFluence[30] = 0;
 }
