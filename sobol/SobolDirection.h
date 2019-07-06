#pragma once

#ifndef SOBOL_DIRECTION_H
#define SOBOL_DIRECTION_H

struct Direction {
	unsigned short dimension;
	unsigned int degree;
	unsigned int coefficients;
	unsigned int *initialDirections;
	static void Init(Direction& instance, unsigned short dimension, unsigned int coefficients, unsigned int degree, ...);
};

class NewJoeKuo621201 {
public:
	NewJoeKuo621201();
	Direction directions[21201];
	unsigned short dimensions;
};

const NewJoeKuo621201 globalNewJoeKuo621201 = NewJoeKuo621201();

#endif SOBOL_DIRECTION_H