#include "pch.h"

#include <iostream>
#include "SobolGenerator.h"

const double dividend = pow(2.0, 32);

SobolGenerator::SobolGenerator(unsigned int maxGenerating, unsigned short dimensions) {
	this->maxGenerating = maxGenerating;
	this->dimensions = dimensions;
	requiredBits = (unsigned int)ceil(log((double)this->maxGenerating) / log(2.0));

	previousC = 1;
	previousXByDimension = new unsigned int[dimensions];
	memset(previousXByDimension, 0, sizeof(unsigned int) * dimensions);

	V = new unsigned int[requiredBits + 1];
	memset(V, 0, sizeof(unsigned int) * (requiredBits + 1));

	currentGenerating = 0;
}

bool SobolGenerator::GetNext(vector<double>& point) {
	if (currentGenerating == maxGenerating) {
		return false;
	}

	if (0 == currentGenerating) {
		for (unsigned int i = 0; i < dimensions; ++i) {
			point.push_back(0.0);
		}
		++currentGenerating;
		return true;
	}

	unsigned int C = 0, X = 0;

	for (unsigned int i = 1; i <= requiredBits; ++i) {
		V[i] = 1 << (32 - i);
	}

	C = 1;
	auto temp = currentGenerating;
	while (temp & 1) {
		temp >>= 1;
		C++;
	}

	X = previousXByDimension[0] ^ V[previousC];
	point.push_back((double)X / dividend);
	previousXByDimension[0] = X;

	Direction direction;
	for (unsigned short nthDimension = 2; nthDimension <= dimensions; ++nthDimension) {
		direction = globalNewJoeKuo621201.directions[nthDimension - 2]; // direction data store dimension data start by 2 --Zachary
		if (requiredBits <= direction.degree) {
			for (unsigned int i = 1; i <= requiredBits; ++i) {
				V[i] = direction.initialDirections[i - 1] << (32 - i);
			}
		}
		else {
			for (unsigned char i = 1; i <= direction.degree; ++i) {
				V[i] = direction.initialDirections[i - 1] << (32 - i);
			}
			for (unsigned int i = direction.degree + 1; i <= requiredBits; ++i) {
				V[i] = V[i - direction.degree] ^ (V[i - direction.degree] >> direction.degree);
				for (unsigned char k = 1; k <= direction.degree - 1; ++k) {
					V[i] ^= (((direction.coefficients >> (direction.degree - 1 - k)) & 1) * V[i - k]);
				}
			}
		}

		X = previousXByDimension[nthDimension - 1] ^ V[previousC];
		point.push_back((double)X / dividend);
		previousXByDimension[nthDimension - 1] = X;
	}

	previousC = C;
	++currentGenerating;
	return true;
}

SobolGenerator::~SobolGenerator() {
	delete[] V;
	delete[] previousXByDimension;
}