#include <bits/stdc++.h>
using namespace std;

const char separator    = ' ';
const int nameWidth     = 6;
const int numWidth      = 6;

class Nodes {
    public:
        string type;
        int index;
        Nodes() {
            index = 0;
        }
        virtual void printType() {
            cout << type << endl;
        }
};
class ItemNodes: public Nodes {
    public:
        float price;
        int allocatedAgent;
        ItemNodes() : Nodes() {
            price = 0;
            allocatedAgent = {};
            type = "ItemNode";
        }
        void printType() {
            cout << type << endl;
        }
};
class AgentNodes: public Nodes {
    public: 
        vector<float> itemUtilityMap;
        float bundlePrice;
        float MBB;
        vector<ItemNodes*> allocationItems;
        vector<ItemNodes*> MBBItems;

        AgentNodes() : Nodes() {
            itemUtilityMap = {};
            bundlePrice = 0;
            allocationItems = {};
            MBBItems = {};
            MBB = 1;
            type = "AgentNode";
        }

        void printType() {
            cout << type << endl;
        }

        void printAgentAllocation() {
            cout << left << setw(nameWidth) << setfill(separator) << "Agent " << index << " -> ";
            for (auto i: allocationItems) {
                cout << left << setw(nameWidth) << setfill(separator) << (*i).index;
            }
            cout << endl;
        }

        void printAgentMBB() {
            cout << left << setw(nameWidth) << setfill(separator) << "Agent " << index << " -> ";
            for (auto i: MBBItems) {
                cout << left << setw(nameWidth) << setfill(separator) << (*i).index;
            }
            cout << endl;
        }
        
};

bool floatIsEqual(float v1, float v2, float epsilon);

float findMinBundlePrice(vector<AgentNodes> agents);

vector<int> findLeastSpenders( vector<AgentNodes> agents, float minBundlePrice);

float findEFMaxBundlePrice(vector<AgentNodes> agents, vector<ItemNodes> items);

float computeAlpha1(unordered_set<int> LSComponentAgents, unordered_set<int> LSComponentItems, vector<AgentNodes> agents, vector<ItemNodes> items);

float computeAlpha2(unordered_set<int> LSComponentAgents,  vector<AgentNodes> agents, float minBundlePrice);

void updateItemPrices(unordered_set<int> LSComponentItems, vector<ItemNodes> &items, float beta);

void updateAgentBundles(unordered_set<int> LSComponentAgents, unordered_set<int> LSComponentItems, vector<AgentNodes> &agents, vector<ItemNodes> &items, float beta);

void transferItem(int itemToTransfer, int transferFromAgent, int tranferToAgent, vector<AgentNodes> &agents, vector<ItemNodes> &items);