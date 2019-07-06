// SobolPointsGenerator.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"

// Frances Y. Kuo
//
// Email: <f.kuo@unsw.edu.au>
// School of Mathematics and Statistics
// University of New South Wales
// Sydney NSW 2052, Australia
// 
// Last updated: 21 October 2008
//
//   You may incorporate this source code into your own program 
//   provided that you
//   1) acknowledge the copyright owner in your program and publication
//   2) notify the copyright owner by email
//   3) offer feedback regarding your experience with different direction numbers
//
//
// -----------------------------------------------------------------------------
// Licence pertaining to sobol.cc and the accompanying sets of direction numbers
// -----------------------------------------------------------------------------
// Copyright (c) 2008, Frances Y. Kuo and Stephen Joe
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
// 
//     * Neither the names of the copyright holders nor the names of the
//       University of New South Wales and the University of Waikato
//       and its contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -----------------------------------------------------------------------------

#include <cstdlib> // *** Thanks to Leonhard Gruenschloss and Mike Giles   ***
#include <cmath>   // *** for pointing out the change in new g++ compilers ***

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>

#include <vector>

#include "SobolGenerator.h"

using namespace std;

// ----- SOBOL POINTS GENERATOR BASED ON GRAYCODE ORDER -----------------
// INPUT: 
//   N         number of points  (cannot be greater than 2^32)
//   D         dimension  (make sure that the data file contains enough data!!)      
//   dir_file  the input file containing direction numbers
//
// OUTPUT:
//   A 2-dimensional array POINTS, where
//     
//     POINTS[i][j] = the jth component of the ith point,
//   
//   with i indexed from 0 to N-1 and j indexed from 0 to D-1
//
// ----------------------------------------------------------------------

const double dividend = pow(2.0, 32);

double **sobol_points(unsigned N, unsigned D, const char *dir_file)
{
	ifstream infile(dir_file, ios::in);
	if (!infile) {
		cout << "Input file containing direction numbers cannot be found!\n";
		exit(1);
	}
	char buffer[1000];
	infile.getline(buffer, 1000, '\n');

	// L = max number of bits needed 
	unsigned L = (unsigned)ceil(log((double)N) / log(2.0));

	// C[i] = index from the right of the first zero bit of i
	unsigned *C = new unsigned[N];
	C[0] = 1;
	for (unsigned i = 1; i <= N - 1; i++) {
		C[i] = 1;
		unsigned value = i;
		while (value & 1) {
			value >>= 1;
			C[i]++;
		}
	}

	// POINTS[i][j] = the jth component of the ith point
	//                with i indexed from 0 to N-1 and j indexed from 0 to D-1
	double **POINTS = new double *[N];
	for (unsigned i = 0; i <= N - 1; i++) POINTS[i] = new double[D];
	for (unsigned j = 0; j <= D - 1; j++) POINTS[0][j] = 0;

	// ----- Compute the first dimension -----

	// Compute direction numbers V[1] to V[L], scaled by pow(2,32)
	unsigned *V = new unsigned[L + 1];
	for (unsigned i = 1; i <= L; i++) V[i] = 1 << (32 - i); // all m's = 1

	// Evalulate X[0] to X[N-1], scaled by pow(2,32)
	unsigned *X = new unsigned[N];
	X[0] = 0;
	for (unsigned i = 1; i <= N - 1; i++) {
		X[i] = X[i - 1] ^ V[C[i - 1]];
		POINTS[i][0] = (double)X[i] / dividend; // *** the actual points
		//        ^ 0 for first dimension
	}

	// Clean up
	delete[] V;
	delete[] X;


	// ----- Compute the remaining dimensions -----
	for (unsigned j = 1; j <= D - 1; j++) {

		// Read in parameters from file 
		unsigned d, s;
		unsigned a;
		infile >> d >> s >> a;
		unsigned *m = new unsigned[s + 1];
		for (unsigned i = 1; i <= s; i++) infile >> m[i];

		// Compute direction numbers V[1] to V[L], scaled by pow(2,32)
		unsigned *V = new unsigned[L + 1];
		if (L <= s) {
			for (unsigned i = 1; i <= L; i++) V[i] = m[i] << (32 - i);
		}
		else {
			for (unsigned i = 1; i <= s; i++) V[i] = m[i] << (32 - i);
			for (unsigned i = s + 1; i <= L; i++) {
				V[i] = V[i - s] ^ (V[i - s] >> s);
				for (unsigned k = 1; k <= s - 1; k++)
					V[i] ^= (((a >> (s - 1 - k)) & 1) * V[i - k]);
			}
		}

		// Evalulate X[0] to X[N-1], scaled by pow(2,32)
		unsigned *X = new unsigned[N];
		X[0] = 0;
		for (unsigned i = 1; i <= N - 1; i++) {
			X[i] = X[i - 1] ^ V[C[i - 1]];
			POINTS[i][j] = (double)X[i] / dividend; // *** the actual points
			//        ^ j for dimension (j+1)
		}

		// Clean up
		delete[] m;
		delete[] V;
		delete[] X;
	}
	delete[] C;

	return POINTS;
}


int main() {
	ofstream outfile("output.txt");
	int generateNPoints, dimensions;
	cin >> generateNPoints >> dimensions;

	outfile << setprecision(20);

	double **P = sobol_points(generateNPoints, dimensions, "new-joe-kuo-6.21201");
	//cout << setiosflags(ios::scientific) << setprecision(10);

	vector<double> point;
	auto sobolGenerator = new SobolGenerator(generateNPoints, dimensions);
	for (int i = 0; i <= generateNPoints && sobolGenerator->GetNext(point); ++i, point.clear()) {
		outfile << "[" << i << "] ";
		for (int j = 0; j < dimensions; ++j) {
			if (P[i][j] != point[j]) {
				cout << "Incorrect generated data for point " << i << " dimension " << j << endl;
				cin >> generateNPoints;
				return 1;
			}
			outfile << point[j] << " ";
		}
		outfile << endl;
	}

	outfile.close();
	cout << "Generation Finished" << endl;
	cin >> generateNPoints;
	return 0;
}
//
//int main() {
//	ifstream infile("new-joe-kuo-6.21201", ios::in);
//	if (!infile) {
//		cout << "Input file containing direction numbers cannot be found!\n";
//		exit(1);
//	}
//	char buffer[1000];
//	infile.getline(buffer, 1000, '\n');
//
//	int d, s, a, m;
//	ofstream outfile("gen_code.txt");
//
//	int i = 0;
//	while (!infile.eof()) {
//		infile >> d >> s >> a;
//		outfile << "Direction::Init(directions[" << i << "], " << d << ", " << a << ", " << s << ", ";
//		for (int i = 0; i < s; ++i) {
//			infile >> m;
//			outfile << m;
//			if (s-1 == i) {
//				outfile << ");" << endl;
//			}
//			else {
//				outfile << ", ";
//			}
//		}
//		i++;
//	}
//
//	outfile.close();
//}