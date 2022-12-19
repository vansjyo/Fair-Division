#include <bits/stdc++.h>
#include "helper.h"
using namespace std;

void printUtilityMap(int n, int m, vector<AgentNodes> &agents, vector<ItemNodes> &items);

void printIntVector(vector<int> v);

void printIntSet(unordered_set<int> v);

// void printAgentAllocation(vector<ItemNodes*> v, int agent);

// void printAgentMBB(vector<ItemNodes*> v, int agent);

void printAgentAllocationMBB(vector<AgentNodes> &agents, vector<ItemNodes> &items);

void printRevisedPrices(vector<ItemNodes> &items);

void generateExcel(vector<AgentNodes> &agents, vector<ItemNodes> &items, ofstream &fileHandle);

void drawVerificationCurve(int iteration, vector<double> &vecA, string vecA_desc, vector<double> &vecB, string vecB_desc);
