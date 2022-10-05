#include <bits/stdc++.h>
#include <pcg_random.hpp>
#include "output.h"
#define EPS 0.00001f
using namespace std;

int main() 
{
    // Define Inputs 
    bool DEBUG = true;                    // DEBUG Mode ON - true / OFF - false
    int samples = 34196, iteration = 34195;   // number of samples to run the code for
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
    ofstream myExcel;
    ofstream logfile;                         // logs the entire outpur
    ofstream sampleFile;                      // logs the sample
    ofstream minEnvyDiffFile;                 // tracks the minimum of [d_i(X_h) - d_i(X_i) + max(d_ij)] over all i.j 
    ofstream minBundlePriceFile;              // tracks the minimum bundle price over iterations
    ofstream EFMaxValuationFile;              // tracks di(Xi) - di_max = disutility of least spender after removing highest disutility chore
    ofstream EFMaxBundlePriceFile;            // tracks EFMax bundle price over iterations
    ofstream nashEFMaxWelfareFile;            // tracks product of EFMax Valuations of all agents (if > 0)
    ofstream minBundleValuationFile;          // tracks the disutility/valuation of the minimum bundle
    ofstream EFMaxPlusMinValuationFile;       // tracks di(Xi) - di_max + di_min of least spender 
    ofstream minAndEFMaxBundlePriceDiffFile;  // tracks the difference between the minimum bundle price and the EFMax bundle Price

    myExcel.open("./Logs/ExcelLog.txt", std::ios_base::app);
    logfile.open("./Logs/Log.txt");
    sampleFile.open("./Logs/Samples.txt");
    minEnvyDiffFile.open("./Logs/MinEnvyDiff.txt");
    minBundlePriceFile.open("./Logs/MinBundlePrice.txt");
    EFMaxValuationFile.open("./Logs/EFMaxValuation.txt");
    EFMaxBundlePriceFile.open("./Logs/EFMaxBundlePrice.txt");
    nashEFMaxWelfareFile.open("./Logs/NashEFMaxWelfareFile.txt");
    minBundleValuationFile.open("./Logs/MinBundleValuation.txt");
    EFMaxPlusMinValuationFile.open("./Logs/EFMaxPlusMinValuationFile.txt");
    minAndEFMaxBundlePriceDiffFile.open("./Logs/MinAndEFMaxBundlePriceDiff.txt");
    
    // setting file headers
    logfile << "Iteration" << " " << "Agents" << " " << "Items" << " " << "Duration" << " " << "Price_Rise_Steps" << " " << "Tranfer_Steps" << endl;
    minEnvyDiffFile.precision(1);
    EFMaxValuationFile.precision(1);
    nashEFMaxWelfareFile.precision(1);
    EFMaxPlusMinValuationFile.precision(1);
    minAndEFMaxBundlePriceDiffFile.precision(1);

    // run until number of samples
    while(iteration < samples) {

        (DEBUG)?(cout << "Working on Sample Number " << iteration << endl):(cout << "" << endl);

        // Get starting timepoint
        auto start = std::chrono::high_resolution_clock::now();

        // Uniform RNG for determining number of agents and items
        pcg32 rng(iteration);
        std::uniform_int_distribution<int> uniform_dist_agent(2, 5);
        std::uniform_int_distribution<int> uniform_dist_item(1, 15);

        // define inputs - initialize n - agents (iterator-> i), m - items (iterator-> j)
        int n = uniform_dist_agent(rng);
        int m = uniform_dist_item(rng);

        // logging info
        int priceRiseSteps = 0;
        int tranferSteps = 0;
        myExcel << endl << "Sample-" <<  iteration << endl;
        logfile << iteration << " " << n << " " << m << " ";
        minEnvyDiffFile << iteration << " ";
        minBundlePriceFile << iteration << " ";
        EFMaxValuationFile << iteration << " ";
        EFMaxBundlePriceFile << iteration << " ";
        nashEFMaxWelfareFile << iteration << " ";
        minBundleValuationFile << iteration << " ";
        EFMaxPlusMinValuationFile << iteration << " ";

        // initialize and generate the sample
        vector<AgentNodes> agents(n);
        vector<ItemNodes> items(m);
        unordered_map<int, long double> valuationMap;                    // stores LS and their corresponding metrics to track
        unordered_map<int, long double> afterReceivingItemValuationMap;  // stores LS and their corresponding valuations after directly receiving an item
        DEBUG?(cout << "Generating Example... " <<  endl):(cout << "");
        generateSample(iteration, dist_type, parameters, agents, items, sampleFile);

        // populate MBB ratio/items, bundle price for every agent, an initial allocation and least spender's spending (pass by reference)
        double minBundlePrice = numeric_limits<double>::max();
        populateInstance(agents, items, minBundlePrice);

        // print input instance
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
            int LS = -1;
            unordered_set<int> leastSpenderComponentAgents, leastSpenderComponentItems; 
            while(1) {
                queue<Nodes*> q;
                vector<int> visitedAgent(n,0), visitedItem(m,0);
                vector<int> predAgentToItem(m,-1), predItemToAgent(n,0); //predAgent = preceding agent to an item
                int LSToBSAgent = -1;

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

                int prevLS = LS;
                LS = leastSpenders[0];

                // log values
                generateExcel(agents, items, myExcel);
                double minBundleValuation = findBundleValuation(LS, LS, agents);
                minEnvyDiffFile << std::fixed << findMinEnvyDiff(agents) << " ";
                minBundleValuationFile << minBundleValuation << " " << LS << " ; ";
                EFMaxValuationFile << findEFMaxValuation(agents, items, LS) << LS << ";";
                EFMaxPlusMinValuationFile << findEFMaxPlusMinValuation(agents, items, LS) << LS << " ";
                minAndEFMaxBundlePriceDiffFile << std::fixed << (EFMaxBundlePrice - minBundlePrice) << " ";
                nashEFMaxWelfareFile << std::fixed << findNashEFMaxWelfare(agents, items) << " ";
                
                DEBUG?(cout << "Least Spenders " << LS << "'s Valuation " << minBundleValuation << endl):(cout << "");
                
                // insert the metric to check for monotonicity in a map
                long double metric = (long double) findEFMaxValuation(agents, items, LS);
                if(valuationMap.find(LS)==valuationMap.end()) {
                    valuationMap.insert({LS, metric});
                } 
                else {
                    long double prevValuation = valuationMap.at(LS);
                    if(prevValuation < metric && (abs(prevValuation - metric)< EPS)==false) {
                        valuationMap.at(LS) = metric;
                    }
                    // if the metric value has strictly decreased from it previous value when LS was least spender, then exit
                    else if(prevValuation > metric && (abs(prevValuation - metric) < EPS)==false && prevLS!=LS) {
                        cout << "Exited: PREV_VALUATION_AFTER_LS_AGAIN proof not satisfied, prev: " << prevValuation << " now: " << metric << endl;
                        // return 0;
                    }
                }

                // check if any of the past Least Spenders have become the Big Spenders
                for(unordered_map<int, long double>::iterator it = valuationMap.begin(); it!=valuationMap.end(); it++) {
                    if( doubleIsEqual(EFMaxBundlePrice, findEFMaxBundlePrice(agents, items, it->first), EPS)==true ) {
                        cout << "Exited: PREV_LS_BECOMES_BS not satisfied. Agent " << it->first << " becomes the Big Spender with bundle price: " << findEFMaxBundlePrice(agents, items, it->first) << endl;
                        LSToBSAgent = it->first;
                        // if(is_EF1_fPO(agents, items)==false) {
                        //     cout << "EF1 condition not satisfied";
                        //     return 0;
                        // } 
                        // return 0;
                    }
                }

                // initialize BFS params
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

                    // perform transfer, update bundles and graph
                    transferItem(itemViolater, pathViolater, predAgentToItem[itemViolater], agents, items);
                    if(LSToBSAgent!=-1 && pathViolater!=LSToBSAgent) {
                        cout << "EXIT:LS_TO_BS - LS turned BS was not path violator" << endl;
                        // system("canberra-gtk-play -f ~/Downloads/pno-cs.wav"); // play sound once the program finishes LOL!
                        // return 0;
                    }

                    // if the LS directly receives an item, log it
                    if(predAgentToItem[itemViolater]==LS) {
                        long double metric = findBundleValuation(LS, LS, agents);
                        if(afterReceivingItemValuationMap.find(LS)==afterReceivingItemValuationMap.end()) {
                            afterReceivingItemValuationMap.insert({LS, metric});
                        }
                        else {
                            long double prevValuation = afterReceivingItemValuationMap.at(LS);
                            if(prevValuation < metric && (abs(prevValuation - metric)< EPS)==false) {
                                valuationMap.at(LS) = metric;
                            }
                            // if the metric value has strictly decreased from it previous value when LS was least spender, then exit
                            else if(prevValuation > metric && (abs(prevValuation - metric) < EPS)==false) {
                                cout << "Exited: PREV_AFTER_RECEIVING_ITEM_PROOF not satisfied prev: " << prevValuation << " now: " << metric << endl;
                                // return 0;
                              }
                        }
                    }

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
        myExcel << endl;
        logfile << duration.count() << " " << priceRiseSteps << " " << tranferSteps << endl;
        minEnvyDiffFile << std::fixed << findMinEnvyDiff(agents) << endl;
        minBundlePriceFile << endl;
        EFMaxValuationFile << endl;
        EFMaxBundlePriceFile << endl;
        minBundleValuationFile << endl;
        minAndEFMaxBundlePriceDiffFile << (EFMaxBundlePrice - minBundlePrice) << endl;
        
        cout << endl << "------ Allocation is now pEF1+fPO -------" << endl << endl;
        printAgentAllocationMBB(agents, items);

        // Final Brute Force Check for pEF1
        if(is_PEF1_fPO(agents, items)==false || is_EF1_fPO(agents, items)==false) {
            cout << "ERROR - ALLOCATION INCORRECT" << endl;
            return 0;
        }
        else {
            cout << "ALLOCATION - CORRECT" << endl;
        }
            
        iteration++;
    }

    // closing log files
    myExcel.close();
    logfile.close();
    sampleFile.close();
    minEnvyDiffFile.close();
    minBundlePriceFile.close();
    EFMaxValuationFile.close();
    EFMaxBundlePriceFile.close();
    minBundleValuationFile.close();
    minAndEFMaxBundlePriceDiffFile.close();

    system("canberra-gtk-play -f ~/Downloads/pno-cs.wav"); // play sound once the program finishes LOL!
    return 0;
}

