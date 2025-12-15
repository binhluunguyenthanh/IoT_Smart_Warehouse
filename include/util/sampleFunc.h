#ifndef SAMPLEFUNC_H
#define SAMPLEFUNC_H

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
using namespace std;

#include "util/Point.h"
// #include "util/ArrayLib.h"



int minHeapComparator(int& a, int& b);
int maxHeapComparator(int& a, int& b);

int minHeapComparator(Point& a, Point& b);
int maxHeapComparator(Point& a, Point& b);
int minHeapComparator(Point*& a, Point*& b);
int maxHeapComparator(Point*& a, Point*& b);
////////////////////////////////////////////////////////////
string point2str(Point& point);
string point2str(Point*& point);

string int2str(int& value);
string int2str(int*& value);

#endif /* SAMPLEFUNC_H */