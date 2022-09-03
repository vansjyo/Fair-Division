#include <bits/stdc++.h>
#include <pcg_random.hpp>
#include "helper.h"
#define EPS 0.00001f

void generateSample(int seed, string distribution_type, vector<double> parameters, vector<AgentNodes> &agents, vector<ItemNodes> &items, ofstream &sampleFile) {

    // set values

    // populate item Utility map and determine price of each item
    vector<int> init_values(items.size(),0);

    //define lambda function
    auto generate = [&] (auto rng, auto rnd_gen) {
        for(int j = 0; j < items.size(); j++) {
            ItemNodes *item = new ItemNodes();
            for(int i = 0; i < agents.size(); i++) {
                if(j==0) {
                    AgentNodes *agent = new AgentNodes();
                    agents[i] = *agent;
                    agents[i].index = i;
                }
                agents[i].itemUtilityMap.push_back(0);
                int vij = ((init_values[j]==0)?(rng(rnd_gen)*10000000):(rng(rnd_gen)*200)) + init_values[j];                
                sampleFile << vij << " ";
                
                agents[i].itemUtilityMap[j] = (double) vij;
                items[j].price = fmin(items[j].price, agents[i].itemUtilityMap[j]);
            }
            sampleFile << endl;
            items[j].index = j;
        }
    };
    
    if(distribution_type == "uniform") {
        // set parameters = [range_start, range_end]
        pcg32 rnd_gen(seed);
        std::uniform_int_distribution<int> rng((int) parameters[0], (int) parameters[1]);
        generate(rng, rnd_gen);
    }
    else if(distribution_type == "exponential") {
        // set parameters = [exponential_distribution_lambda]
        std::random_device rd; 
        std::mt19937 rnd_gen(rd ());
        std::exponential_distribution<> rng (parameters[0]);
        generate(rng, rnd_gen);
    }
    else if(distribution_type == "similar") {
        // set parameters = [standard_deviation_range_start, standard_deviation_range_end]
        pcg32 rnd_gen(seed);
        std::uniform_int_distribution<int> rng((int) parameters[0], (int) parameters[1]);
        for(int i = 1; i <= init_values.size(); i++) 
            init_values[i-1] = (i)*5000000;
        generate(rng, rnd_gen);
    }
    else if(distribution_type=="normal") {
        // set parameters = [mean, std]
        std::random_device rd{};
        std::mt19937 rnd_gen{rd()};
        std::normal_distribution<> rng{parameters[0], parameters[1]};
        generate(rng, rnd_gen);
    }  
    return;
}

bool doubleIsEqual(double v1, double v2, double epsilon) {
    if(abs(v2-v1)<epsilon)
        return true;
    else
        return false;
}

// find the minimum Bundle price
double findMinBundlePrice(vector<AgentNodes> agents) {
    double minBundlePrice = numeric_limits<double>::max();
    for(int i = 0; i < agents.size(); i++) {
        minBundlePrice = fmin(minBundlePrice, agents[i].bundlePrice);
    }
    return minBundlePrice;
}

// find all Least Spenders based on minBundle Price
vector<int> findLeastSpenders(vector<AgentNodes> agents, double minBundlePrice) {
    vector<int> leastSpenders;
    for(int i = 0; i < agents.size(); i++) {
        if(doubleIsEqual(agents[i].bundlePrice, minBundlePrice, EPS)) 
            leastSpenders.push_back(agents[i].index);
    }
    return leastSpenders;
}

// find the Big Spender or agent who has highest utility after removing highest utility item from their bundle
double findEFMaxBundlePrice(vector<AgentNodes> agents, vector<ItemNodes> items) {
    double EFMaxBundlePrice = 0;
    for(int i = 0; i < agents.size(); i++) {
        double maxItemPrice = 0;
        for(int j = 0; j < agents[i].allocationItems.size(); j++) {
            int item = agents[i].allocationItems[j]->index;
            maxItemPrice = fmax(maxItemPrice, items[item].price);
        }
        EFMaxBundlePrice = fmax(EFMaxBundlePrice, (agents[i].bundlePrice - maxItemPrice));
    }
    return EFMaxBundlePrice;
}

// tranfer the item to the 2nd last agent from path violator
void transferItem(int itemToTransfer, int transferFromAgent, int transferToAgent, vector<AgentNodes> &agents, vector<ItemNodes> &items) {
    cout << "----> Transferring item to Agent " << transferToAgent << endl;
    // add item to 2nd last agent
    cout << std::setprecision(13) << "Check: " << agents[transferToAgent].bundlePrice << " " << items[itemToTransfer].price <<  " " << agents[transferToAgent].bundlePrice+items[itemToTransfer].price << endl;
    agents[transferToAgent].allocationItems.push_back(&items[itemToTransfer]);
    agents[transferToAgent].bundlePrice+=items[itemToTransfer].price;

    // remove agents from path violaters bundle 
    for (auto iter = agents[transferFromAgent].allocationItems.begin(); iter != agents[transferFromAgent].allocationItems.end(); ++iter) {
        if(*iter==&items[itemToTransfer]) {
            agents[transferFromAgent].allocationItems.erase(iter);
            // cout << std::setprecision(11) << "Check: " << agents[transferFromAgent].bundlePrice << " " << items[itemToTransfer].price <<  " " << agents[transferFromAgent].bundlePrice-items[itemToTransfer].price << endl;
            agents[transferFromAgent].bundlePrice-=items[itemToTransfer].price;
            break;
        }
    }
    //update allocatedAgent for the item transferred
    items[itemToTransfer].allocatedAgent = transferToAgent;
}

// compute ratio aplha1
double computeAlpha1(unordered_set<int> LSComponentAgents, unordered_set<int> LSComponentItems, vector<AgentNodes> agents, vector<ItemNodes> items) {
    double alpha1 = numeric_limits<double>::min();
    for(auto& i:LSComponentAgents) {
        for(int j = 0; j < items.size(); j++) {
            if(LSComponentItems.find(j)==LSComponentItems.end())
                alpha1 = fmax(alpha1, (agents[i].MBB)*(items[j].price)/agents[i].itemUtilityMap[j]);
            // cout << alpha1 << " : " << i << endl;
        }
    }
    return alpha1;
}

// compute ratio aplha2
double computeAlpha2(unordered_set<int> LSComponentAgents,  vector<AgentNodes> agents, double minBundlePrice) {
    double alpha2 = numeric_limits<double>::min();
    if(doubleIsEqual(minBundlePrice, 0, EPS))
        return 0.0f;
    for(int i = 0; i < agents.size() ; i++) {
        if(doubleIsEqual(agents[i].bundlePrice, minBundlePrice, EPS)) {
            for(int h = 0; h < agents.size(); h++) {
                if( LSComponentAgents.find(h)==LSComponentAgents.end() )
                    alpha2 = fmax(alpha2, (agents[h].bundlePrice)/agents[i].bundlePrice);
            }
        }
    }
    return alpha2;
}

// update Item Prices by beta
void updateItemPrices(unordered_set<int> LSComponentItems, vector<ItemNodes> &items, double beta) {
    for(int j = 0; j < items.size(); j++) {
        if(LSComponentItems.find(j)!=LSComponentItems.end())
            items[j].price*=beta;
    }
}

// update Agent bundle prices, MBB ratio by beta and update MBB items
void updateAgentBundles(unordered_set<int> LSComponentAgents, unordered_set<int> LSComponentItems, vector<AgentNodes> &agents, vector<ItemNodes> &items, double beta) {
    for(auto& i:LSComponentAgents) {
        agents[i].bundlePrice*=beta;
        agents[i].MBB = agents[i].MBB/beta;
    }
    for(int i = 0; i < agents.size(); i++) {
        double MBB = numeric_limits<double>::max();
        for(int j = 0; j < items.size(); j++) {
            MBB = fmin(MBB, agents[i].itemUtilityMap[j]/items[j].price);
        }
        agents[i].MBB = MBB;
    }

    for(int i = 0; i < agents.size(); i++) {
        agents[i].MBBItems.clear();
        for(int j = 0; j < items.size(); j++) {
            if(doubleIsEqual(agents[i].MBB, agents[i].itemUtilityMap[j]/items[j].price, EPS)) {
                agents[i].MBBItems.push_back(&items[j]);
            } 
        }
    }
}

double findBundleValuation(int bundleAgent, int referenceAgent, vector<AgentNodes> agents) {
    double valuation = 0;
    for(ItemNodes* item:agents[bundleAgent].allocationItems) {
        valuation+=agents[referenceAgent].itemUtilityMap[item->index];
    } 
    return valuation;
}

bool is_EF1_fPO(vector<AgentNodes> agents, vector<ItemNodes> items) {
    for(int i = 0; i < agents.size(); i++) {
        for(int k = 0; k < agents.size(); k++) {

            double maxPrice = numeric_limits<double>::min();
            for(ItemNodes* item:agents[k].allocationItems) {
                maxPrice = fmax(maxPrice, item->price);
            }

            if(agents[i].bundlePrice < (agents[k].bundlePrice - maxPrice) && doubleIsEqual(agents[i].bundlePrice, agents[k].bundlePrice-maxPrice, EPS)==false )
                return false;
        }
    }

    // 
    for(int i = 0; i < agents.size(); i++) {
        for(int k = 0; k < agents.size(); k++) {

            double maxValuation = numeric_limits<double>::min();
            for(ItemNodes* item:agents[i].allocationItems) {
                maxValuation = fmax(maxValuation, agents[i].itemUtilityMap[item->index]);
            }

            if(findBundleValuation(i, i, agents) - maxValuation > findBundleValuation(k, i, agents) && doubleIsEqual(findBundleValuation(i, i, agents) - maxValuation, findBundleValuation(k, i, agents), EPS)==false ) {
                cout << "Failed for agent " <<  i << "-" << findBundleValuation(i, i, agents)-maxValuation << " and agent " << k  << "-" << findBundleValuation(k, i, agents) << endl;
                return false;
            }

        }
    }
    
    return true;
}


