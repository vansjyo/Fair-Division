#include <bits/stdc++.h>
#include "helper.h"
#define EPS 0.0001f

bool floatIsEqual(float v1, float v2, float epsilon) {
    if(abs(v2-v1)<epsilon)
        return true;
    else
        return false;
}

// find the minimum Bundle price
float findMinBundlePrice(vector<AgentNodes> agents) {
    float minBundlePrice = numeric_limits<float>::max();
    for(int i = 0; i < agents.size(); i++) {
        minBundlePrice = fmin(minBundlePrice, agents[i].bundlePrice);
    }
    return minBundlePrice;
}

// find all Least Spenders based on minBundle Price
vector<int> findLeastSpenders(vector<AgentNodes> agents, float minBundlePrice) {
    vector<int> leastSpenders;
    for(int i = 0; i < agents.size(); i++) {
        if(floatIsEqual(agents[i].bundlePrice, minBundlePrice, EPS)) 
            leastSpenders.push_back(agents[i].index);
    }
    return leastSpenders;
}

// find the Big Spender or agent who has highest utility after removing highest utility item from their bundle
float findEFMaxBundlePrice(vector<AgentNodes> agents, vector<ItemNodes> items) {
    float EFMaxBundlePrice = 0;
    for(int i = 0; i < agents.size(); i++) {
        float maxItemPrice = 0;
        for(int j = 0; j < agents[i].allocationItems.size(); j++) {
            int item = agents[i].allocationItems[j]->index;
            maxItemPrice = fmax(maxItemPrice, items[item].price);
        }
        EFMaxBundlePrice = fmax(EFMaxBundlePrice, (agents[i].bundlePrice - maxItemPrice));
    }
    return EFMaxBundlePrice;
}

// tranfer the item to the 2nd last agent from path violator
void transferItem(int itemToTransfer, int transferFromAgent, int tranferToAgent, vector<AgentNodes> &agents, vector<ItemNodes> &items) {
    cout << "----> Transferring item to Agent " << tranferToAgent << endl;
    // add item to 2nd last agent
    agents[tranferToAgent].allocationItems.push_back(&items[itemToTransfer]);
    agents[tranferToAgent].bundlePrice+=items[itemToTransfer].price;
    // remove agents from path violaters bundle 
    for (auto iter = agents[transferFromAgent].allocationItems.begin(); iter != agents[transferFromAgent].allocationItems.end(); ++iter) {
        if(*iter==&items[itemToTransfer]) {
            agents[transferFromAgent].allocationItems.erase(iter);
            agents[transferFromAgent].bundlePrice-=items[itemToTransfer].price;
            break;
        }
    }
    //update allocatedAgent for the item transferred
    items[itemToTransfer].allocatedAgent = tranferToAgent;
}

// compute ratio aplha1
float computeAlpha1(unordered_set<int> LSComponentAgents, unordered_set<int> LSComponentItems, vector<AgentNodes> agents, vector<ItemNodes> items) {
    float alpha1 = numeric_limits<float>::max();
    for(auto& i:LSComponentAgents) {
        for(int j = 0; j < items.size(); j++) {
            if(LSComponentItems.find(j)==LSComponentItems.end())
                alpha1 = fmin(alpha1, (agents[i].MBB)*(items[j].price)/agents[i].itemUtilityMap[j]);
        }
    }
    return alpha1;
}

// compute ratio aplha2
float computeAlpha2(unordered_set<int> LSComponentAgents,  vector<AgentNodes> agents, float minBundlePrice) {
    float alpha2 = numeric_limits<float>::max();
    for(int i = 0; i < agents.size() ; i++) {
        if(floatIsEqual(agents[i].bundlePrice, minBundlePrice, EPS)) {
            for(int h = 0; h < agents.size(); h++) {
                if( LSComponentAgents.find(h)==LSComponentAgents.end() )
                    alpha2 = fmin(alpha2, (agents[h].bundlePrice)/agents[i].bundlePrice);
            }
        }
    }
    return alpha2;
}

// update Item Prices by beta
void updateItemPrices(unordered_set<int> LSComponentItems, vector<ItemNodes> &items, float beta) {
    for(int j = 0; j < items.size(); j++) {
        if(LSComponentItems.find(j)!=LSComponentItems.end())
            items[j].price*=beta;
    }
}

// update Agent bundle prices, MBB ratio by beta and update MBB items
void updateAgentBundles(unordered_set<int> LSComponentAgents, unordered_set<int> LSComponentItems, vector<AgentNodes> &agents, vector<ItemNodes> &items, float beta) {
    for(auto& i:LSComponentAgents) {
        agents[i].bundlePrice*=beta;
        agents[i].MBB = agents[i].MBB/beta;
        // for(int j = 0; j < items.size(); j++) {
        //     if(LSComponentItems.find(j)==LSComponentItems.end() && floatIsEqual(agents[i].MBB, agents[i].itemUtilityMap[j]/items[j].price, EPS))
        //         agents[i].MBBItems.push_back(&items[j]);
        // }
    }

    // populate MBB ratio for all agents
    for(int i = 0; i < agents.size(); i++) {
        float MBB = 0;
        for(int j = 0; j < items.size(); j++) {
            MBB = fmax(MBB, agents[i].itemUtilityMap[j]/items[j].price);
        }
        agents[i].MBB = MBB;
    }

    for(int i = 0; i < agents.size(); i++) {
        agents[i].MBBItems.clear();
        for(int j = 0; j < items.size(); j++) {
            if(floatIsEqual(agents[i].MBB, agents[i].itemUtilityMap[j]/items[j].price, EPS)) {
                agents[i].MBBItems.push_back(&items[j]);
            } 
        }
    }
}

bool is_EF1_fPO(vector<AgentNodes> agents, vector<ItemNodes> items) {
    for(int i = 0; i < agents.size(); i++) {
        for(int k = 0; k < agents.size(); k++) {

            float maxPrice = numeric_limits<float>::min();
            for(ItemNodes* item:agents[k].allocationItems) {
                maxPrice = fmax(maxPrice, item->price);
            }


            if(agents[i].bundlePrice < (agents[k].bundlePrice - maxPrice) && floatIsEqual(agents[i].bundlePrice, agents[k].bundlePrice-maxPrice, EPS)==false )
                return false;

        }
    }
    
    return true;
}
