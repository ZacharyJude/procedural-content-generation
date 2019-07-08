#pragma once

#ifndef SOBOL_GENERATOR_H
#define SOBOL_GENERATOR_H

#include <cstdarg>
#include <cmath>
#include <vector>
#include <iostream>

#include "SobolDirection.h"

using namespace std;

class SobolGenerator {
public:
	SobolGenerator(unsigned int maxGenerating, unsigned short dimensions);
	~SobolGenerator();
	bool GetNext(vector<double>& point);

private:
	unsigned int maxGenerating, currentGenerating;
	unsigned short dimensions;
	unsigned int requiredBits;
	unsigned int *V;
	unsigned int previousC, *previousXByDimension;
};

#endif //SOBOL_GENERATOR_H