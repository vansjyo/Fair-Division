// #include <bits/stdc++.h>
#include <pcg_random.hpp>
#include "output.h"
#define EPS 0.00001f
using namespace std;

int main() 
{
    // Define Inputs 
    bool DEBUG = true;                    // DEBUG Mode ON - true / OFF - false
    int samples = 30000, iteration = 1;  // number of samples to run the code for
    string dist_type = "uniform";         // distribution to generate valutions of agents from - set parameters below
    vector<double> parameters;
    if(dist_type == "uniform") 
        parameters = {1, 100};            // [range_start, range_end]
    else if(dist_type == "exponential")
        parameters = {1};                 // [exponential_distribution_lambda]
    else if(dist_type == "similar")
        parameters = {1,5};               // [standard_deviation_range_start, standard_deviation_range_end]
    else if(dist_type=="normal")
        parameters = {5,1};               // [mean, std]

    // defining log files
    ofstream logfile;
    ofstream sampleFile;
    ofstream minBundlePriceFile;
    ofstream EFMaxBundlePriceFile;
    ofstream minBundleValuationFile;

    logfile.open("Log.txt");
    sampleFile.open("Samples.txt");
    minBundlePriceFile.open("MinBundlePrice.txt");
    EFMaxBundlePriceFile.open("EFMaxBundlePrice.txt");
    minBundleValuationFile.open("MinBundleValuation.txt");

    logfile << "Iteration" << " " << "Agents" << " " << "Items" << " " << "Duration" << " " << "Price_Rise_Steps" << " " << "Tranfer_Steps" << endl;

    // run until number of samples
    while(iteration < samples) {

        (DEBUG)?(cout << "Working on Sample Number " << iteration << endl):(cout << "" << endl);

        // Get starting timepoint
        auto start = std::chrono::high_resolution_clock::now();

        // Uniform RNG for number of agents and items
        pcg32 rng(iteration);
        std::uniform_int_distribution<int> uniform_dist_agent(1, 50);
        std::uniform_int_distribution<int> uniform_dist_item(50, 150);

        // define inputs - initialize n - agents (iterator-> i), m - items (iterator-> j)
        int n = uniform_dist_agent(rng);
        int m = uniform_dist_item(rng);

        // logging info
        int priceRiseSteps = 0;
        int tranferSteps = 0;
        logfile << iteration << " " << n << " " << m << " ";
        minBundlePriceFile << iteration << " ";
        EFMaxBundlePriceFile << iteration << " ";
        minBundleValuationFile << iteration << " ";

        // initialize and generate the sample
        vector<AgentNodes> agents(n);
        vector<ItemNodes> items(m);
        DEBUG?(cout << "Generating Example... " <<  endl):(cout << "");
        generateSample(iteration, dist_type, parameters, agents, items, sampleFile);
        
        // populate MBB ratio for all agents
        for(int i = 0; i < n; i++) {
            double MBB = numeric_limits<double>::max();
            for(int j = 0; j < m; j++)
                MBB = fmin(MBB, agents[i].itemUtilityMap[j]/items[j].price);
            agents[i].MBB = MBB;
        }

        // populate MBB items, bundle price for every agent, an initial allocation and least spender's spending
        double minBundlePrice = numeric_limits<double>::max();
        for(int j = 0; j < m; j++) {
            int allocated_flag = 0;
            for(int i = 0; i < n; i++) {
                if(doubleIsEqual(items[j].price, agents[i].itemUtilityMap[j], EPS)) {
                    if(allocated_flag==0) {
                        agents[i].allocationItems.push_back(&items[j]);
                        agents[i].bundlePrice+=items[j].price;
                        items[j].allocatedAgent = i;
                    }
                    allocated_flag = 1;
                } 
                if(doubleIsEqual(agents[i].MBB, agents[i].itemUtilityMap[j]/items[j].price, EPS)) {
                    agents[i].MBBItems.push_back(&items[j]);
                }   
                minBundlePrice = (j==m-1)?fmin(minBundlePrice, agents[i].bundlePrice):minBundlePrice;
            }
        }

        cout << "Number of Agents: " << n << "; Number of Items: " << m << endl;
        printUtilityMap(agents.size(), items.size(), agents, items);

        // print Least Spenders given minimum bundle price
        vector<int> leastSpenders = findLeastSpenders(agents, minBundlePrice);
        DEBUG?(cout << "Least Spenders" << " -> "):(cout << "");
        DEBUG?(printIntVector(leastSpenders)):(printIntVector({}));

        // print inital allocation
        DEBUG?(printAgentAllocationMBB(agents, items)):(printIntVector({}));
        DEBUG?(printRevisedPrices(items)):(printIntVector({}));
        DEBUG?(cout << endl):(cout<< "");

        // estimate EFMaxBundlePrice
        double EFMaxBundlePrice = findEFMaxBundlePrice(agents, items);
        DEBUG?(cout << "Least Spenders Bundle Price: " << minBundlePrice << endl):(cout<< "");
        DEBUG?(cout << "Big Spender EFMax Bundle Price: " << EFMaxBundlePrice << endl):(cout<< "");


        // 1.-> Do BFS with Least Spender as source to find path violator----------------------------------------------------------------------------------------1.
        while( (minBundlePrice < EFMaxBundlePrice) && doubleIsEqual(minBundlePrice, EFMaxBundlePrice, EPS)==false ) {

            cout << "----> Allocation not currently pEF1" << endl;

            // 2.-> finding alternating paths from LS to path violater --------------------------------------------------------2.
            
            int path_found = 1; //denotes if path was found in the coming step or not
            unordered_set<int> leastSpenderComponentAgents, leastSpenderComponentItems; 
            while(1) {
                queue<Nodes*> q;
                vector<int> visitedAgent(n,0), visitedItem(m,0);
                vector<int> predAgentToItem(m,-1), predItemToAgent(n,0); //predAgent = preceding agent to an item

                // revise Least Spenders if path was found or exchange occured
                if(path_found) {
                    minBundlePrice = findMinBundlePrice(agents);
                    EFMaxBundlePrice = findEFMaxBundlePrice(agents, items);
                    DEBUG?(cout << "Big Spenders EFMax Bundle Price: " << EFMaxBundlePrice << endl):(cout<< "");
                    DEBUG?(cout << "Least Spenders Bundle Price: " << minBundlePrice << endl):(cout<< "");

                    minBundlePriceFile << minBundlePrice << " ";
                    EFMaxBundlePriceFile << EFMaxBundlePrice << " ";

                    if( (minBundlePrice > EFMaxBundlePrice) || doubleIsEqual(minBundlePrice, EFMaxBundlePrice, EPS)==true ) {
                        cout << "Allocation is pEF1+PO" << endl;
                        break;
                    }
                    leastSpenders = findLeastSpenders(agents, minBundlePrice);
                    DEBUG?(cout << "Least Spenders -> "):(cout<< "");
                    DEBUG?(printIntVector(leastSpenders)):(printIntVector({}));
                    leastSpenderComponentAgents.clear();
                    leastSpenderComponentItems.clear();
                }
                // else if no path was found but there exists multiple lease spenders move to next LS
                else {
                    leastSpenders.erase(leastSpenders.begin());
                }   

                int LS = leastSpenders[0];
                minBundleValuationFile << findMinBundleValuation(LS, agents) << " " << LS << " ; "; 
                int pathViolater = -1, itemViolater = -1;
                Nodes* node = &agents[LS];
                q.push(node);
                visitedAgent[LS] = 1;
                leastSpenderComponentAgents.insert(LS);

                // 3. -> while there exists something to explore-----------------------------------3.
                while(!q.empty()) {
                    if(q.front()->type=="AgentNode") {
                        AgentNodes* temp = dynamic_cast<AgentNodes*>(q.front());
                        q.pop();
                        for(ItemNodes* item:temp->MBBItems) {
                            int j = item->index;
                            if(visitedItem[j]==0 && (item->allocatedAgent)!=(temp->index)) {
                                predAgentToItem[j] = temp->index;
                                q.push(item);
                                visitedItem[j] = 1;
                                leastSpenderComponentItems.insert(j);
                            }
                        }
                    }
                    else if(q.front()->type=="ItemNode") {
                        ItemNodes* item = dynamic_cast<ItemNodes*>(q.front());
                        q.pop();
                        int i = item->allocatedAgent;
                        if(visitedAgent[i]==0) {
                            predItemToAgent[i] = item->index;
                            q.push(&agents[i]);
                            visitedAgent[i] = 1;
                            leastSpenderComponentAgents.insert(i);
                            if( path_found==1 && (minBundlePrice < (agents[i].bundlePrice - item->price)) && doubleIsEqual(minBundlePrice, agents[i].bundlePrice-item->price, EPS)==false ) {
                                cout << "----> Path Violator Found" << endl;
                                DEBUG?(cout << "Path Violater -> Agent - " << i << "; Item - " << item->index << endl):(cout << "");
                                // DEBUG?(cout << "Prev Item to Agent prev " << predItemToAgent[predAgentToItem[itemViolater]] << endl):(cout << "");
                                pathViolater = i;
                                itemViolater = item->index;
                                leastSpenderComponentItems.insert(item->index);
                                break;
                            }
                        }
                    }
                }
                // ------------------------------------------------------------------------------- 3.

                // transfer item to pred[itemViolater] from pathViolater and update bundle prices if a path violater is found
                if(pathViolater!=-1) {

                    transferItem(itemViolater, pathViolater, predAgentToItem[itemViolater], agents, items);
                    tranferSteps++;

                    // printAgentAllocationMBB(agents, items);
                    path_found = 1;
                }
                else if(q.empty()) {

                    // estimate EFMaxBundlePrice
                    EFMaxBundlePrice = findEFMaxBundlePrice(agents, items);
                    DEBUG?(cout << "Big Spenders EFMax Bundle Price: " << EFMaxBundlePrice << endl):(cout<< "");
                    DEBUG?(cout << "Least Spenders Bundle Price: " << minBundlePrice << endl):(cout<< "");

                    // if pEF1 condition satisfied, come out of the loop and return the allocation
                    if((minBundlePrice > EFMaxBundlePrice) || doubleIsEqual(minBundlePrice, EFMaxBundlePrice, EPS)==true ) {
                        cout << "Allocation is pEF1+PO" << endl;
                        break;
                    }

                    // else increase price of all items in LS component
                    DEBUG?(cout << "----> No alternating path from LS agent " << LS << " -> Decreasing Prices" << endl):(cout<< "");

                    // Add items allocated to least spender also in the Component
                    for(int i:leastSpenderComponentAgents) {
                        for(ItemNodes* item: agents[i].allocationItems) {
                            leastSpenderComponentItems.insert(item->index);
                        }
                    }

                    // print the least spender component
                    DEBUG?(cout << "LS Component: Agents -> "):(cout<< "");
                    DEBUG?(printIntSet(leastSpenderComponentAgents)):(printIntVector({}));
                    DEBUG?(cout << "LS Component: Items -> "):(cout<< "");
                    DEBUG?(printIntSet(leastSpenderComponentItems)):(printIntVector({}));

                    // compute alpha 1, alpha 2 and beta
                    double alpha1 = computeAlpha1(leastSpenderComponentAgents, leastSpenderComponentItems, agents, items);
                    double alpha2 = computeAlpha2(leastSpenderComponentAgents, agents, minBundlePrice);
                    double beta = fmax(alpha1, 0);
                    cout << "Beta value is " << beta << endl;

                    DEBUG?(cout << "Alpha 1 -> " << alpha1 << "; Alpha 2 -> " << alpha2 << endl):(cout<< "");
                    DEBUG?(cout << "----> Decreasing Price of LS Component by beta = " << beta << endl):(cout<< "");
                    
                    // raise the prices of all items in the Least Spender component
                    updateItemPrices(leastSpenderComponentItems, items, beta);
                    priceRiseSteps++;

                    // update bundles of LS component Agents
                    updateAgentBundles(leastSpenderComponentAgents, leastSpenderComponentItems, agents, items, beta);

                    DEBUG?(printRevisedPrices(items)):(printIntVector({}));

                    DEBUG?(printAgentAllocationMBB(agents, items)):(printIntVector({}));
                    path_found = 1;

                }
            }
            // -------------------------------------------------------------------------------------------------------------2.
            
            // redefine minimum Price Bundle and EFMax Bundle Price
            minBundlePrice = findMinBundlePrice(agents);
            EFMaxBundlePrice = findEFMaxBundlePrice(agents, items);
        }
        // -----------------------------------------------------------------------------------------------------------------------------------------------------1.

        // capturing total runtime
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        // logging details
        logfile << duration.count() << " " << priceRiseSteps << " " << tranferSteps << endl;
        minBundlePriceFile << endl;
        EFMaxBundlePriceFile << endl;
        minBundleValuationFile << endl;
        cout << endl << "------ Allocation is now pEF1+fPO -------" << endl << endl;
        printAgentAllocationMBB(agents, items);

        // Final Brute Force Check for pEF1
        if(is_EF1_fPO(agents, items)==false) {
            cout << "ERROR - ALLOCATION INCORRECT" << endl;
            return 0;
        }
        else {
            cout << "ALLOCATION - CORRECT" << endl;
        }
            
        iteration++;
    }

    // closing log files
    logfile.close();
    sampleFile.close();
    minBundlePriceFile.close();
    EFMaxBundlePriceFile.close();
    minBundleValuationFile.close();

    return 0;
}

