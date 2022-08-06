#include <bits/stdc++.h>
#include "output.h"

void printUtilityMap(int n, int m, vector<AgentNodes> agents, vector<ItemNodes> items) {
    cout << "Printing Sample: " << endl;
    for(int i = -1; i < n; i++) {
        if(i<0) cout << left << setw(nameWidth) << setfill(separator) << "             ";
        else cout << "Agent " << to_string(i) << " - > ";
        for(int j = 0; j < m; j++) {
            if(i<0) cout << left << setw(nameWidth) << setfill(separator) << "I"+to_string(j);
            else cout << left << setw(nameWidth) << setfill(separator) << setprecision(9) << agents[i].itemUtilityMap[j];
        }
        cout << endl;
    }
    cout << endl;
}

// print an int vector
void printIntVector(vector<int> v) {
    for(auto i:v)
        cout << left << setw(nameWidth) << setfill(separator) << i << ", ";
    cout << endl;
}

void printIntSet(unordered_set<int> v) {
    for(auto& i:v)
        cout << left << setw(nameWidth) << setfill(separator) << i << ", ";
    cout << endl;
}

void printAgentAllocationMBB(vector<AgentNodes> agents) {
    cout << endl << "---------- Allocations: ----------- " << endl;
	for(int i = 0; i < agents.size(); i++) {
		agents[i].printAgentAllocation();
	}
	cout << " ------------- MBB: --------------- " << endl;
	for(int i = 0; i < agents.size(); i++) {
		agents[i].printAgentMBB();
		// cout << "Bundle Price of Agent " << i << " -> " << agents[i].bundlePrice << endl;
	}
    cout << "------------- P(Xi): -------------- " << endl;
	for(int i = 0; i < agents.size(); i++) {
        cout << left << setw(nameWidth) << setfill(separator) << to_string(i)+":[ " << setprecision(11) << agents[i].bundlePrice << " ]  ";
	}
    cout << endl;

    for(int i = 0; i < agents.size(); i++) {
        cout << "{";
        for(int j = 0; j < agents[i].MBBItems.size(); j++) {
            cout << (agents[i].MBBItems[j]->index)+100 << ", ";
        // cout << left << setw(nameWidth) << setfill(separator) << items[j].price;
	    }
        cout << "}, ";
	}
    

    cout << endl << "----------------------------------- " << endl << endl;
}

void printRevisedPrices(vector<ItemNodes> items) {
    cout << "------------- P(j): -------------- " << endl;
	for(int j = 0; j < items.size(); j++) {
        cout << left << setw(nameWidth) << setfill(separator) << to_string(j)+":[ " << setprecision(11) << items[j].price << " ] ";
        // cout << left << setw(nameWidth) << setfill(separator) << items[j].price;
	}
    cout << endl << "----------------------------------- " << endl << endl;
}