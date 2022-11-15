#include <bits/stdc++.h>
#include <pcg_random.hpp>
#include "output.h"
#define EPS 0.0001f
using namespace std;

int main() 
{
    // Define Inputs 
    bool DEBUG = true;                    // DEBUG Mode ON - true / OFF - false
    int samples = 1000, iteration = 0;      // number of samples to run the code for
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
    ofstream logfile;                          // logs the entire output
    ofstream sampleFile;                       // logs the sample
    ofstream minEnvyDiff_File;                 // tracks the minimum of [d_i(X_h) - d_i(X_i) + max(d_ij)] over all i.j 
    ofstream minBundlePrice_File;              // tracks the minimum bundle price over iterations
    ofstream EFMaxValuation_File;              // tracks di(Xi) - di_max = disutility of least spender after removing highest disutility chore
    ofstream EFMaxBundlePrice_File;            // tracks EFMax bundle price over iterations
    ofstream nashEFMaxWelfare_File;            // tracks product of EFMax Valuations of all agents ~ Nash Welfare (if > 0)
    ofstream minBundleValuation_File;          // tracks the disutility/valuation of the minimum bundle
    ofstream EFMaxPlusMinValuation_File;       // tracks d_i(Xi) - d_i_max + d_i_min of least spender 
    ofstream minAndEFMaxBundlePriceDiff_File;  // tracks the difference between the minimum bundle price and the EFMax bundle Price
    ofstream LSValAndBSEFMaxValDiff_wrtBS_File; // tracks d_BS(X_LS) - d_BS(X_BS\g) after every transfer path completes

    myExcel.open("./Logs/ExcelLog.txt", std::ios_base::app);
    logfile.open("./Logs/Log.txt");
    sampleFile.open("./Logs/Samples.txt");
    minEnvyDiff_File.open("./Logs/MinEnvyDiff.txt");
    minBundlePrice_File.open("./Logs/MinBundlePrice.txt");
    EFMaxValuation_File.open("./Logs/EFMaxValuation.txt");
    EFMaxBundlePrice_File.open("./Logs/EFMaxBundlePrice.txt");
    nashEFMaxWelfare_File.open("./Logs/nashEFMaxWelfare_File.txt");
    minBundleValuation_File.open("./Logs/MinBundleValuation.txt");
    EFMaxPlusMinValuation_File.open("./Logs/EFMaxPlusMinValuation_File.txt");
    minAndEFMaxBundlePriceDiff_File.open("./Logs/MinAndEFMaxBundlePriceDiff.txt");
    LSValAndBSEFMaxValDiff_wrtBS_File.open("./Logs/LSValAndBSEFMaxValDiff_wrtBS.txt");
    
    // setting file headers
    logfile << "Iteration" << " " << "Agents" << " " << "Items" << " " << "Duration" << " " << "Price_Rise_Steps" << " " << "Transfer_Steps" << endl;
    minEnvyDiff_File.precision(1);
    EFMaxValuation_File.precision(1);
    nashEFMaxWelfare_File.precision(1);
    EFMaxPlusMinValuation_File.precision(1);
    minAndEFMaxBundlePriceDiff_File.precision(1);

    // run until number of samples
    while(iteration < samples) {

        (DEBUG)?(cout << "Working on Sample Number " << iteration << endl):(cout << "" << endl);

        // Get starting timepoint
        auto start = std::chrono::high_resolution_clock::now();

        // Uniform RNG for determining number of agents and items
        pcg32 rng(iteration);
        std::uniform_int_distribution<int> uniform_dist_agent(2, 20);
        std::uniform_int_distribution<int> uniform_dist_item(1, 50);

        // define inputs - initialize n - agents (iterator-> i), m - items (iterator-> j)
        int n = uniform_dist_agent(rng);
        int m = uniform_dist_item(rng);
        int priceRiseStepsCount = 0;
        int transferStepsCount = 0;

        // initialize and generate the sample
        vector<AgentNodes> agents(n);
        vector<ItemNodes> items(m);
        unordered_map<int, long double> valuationMap;                    // stores LS and their corresponding metrics to track
        unordered_map<string, vector<long double> > customValuationMap;     
        unordered_map<int, long double> afterReceivingItemValuationMap;  // stores LS and their corresponding valuations after directly receiving an item
        double minBundlePrice = numeric_limits<double>::max();

        // generate the sample
        generateSample(iteration, dist_type, parameters, agents, items, sampleFile);

        // populate MBB ratio/items, bundle price for every agent, an initial allocation and least spender's spending (pass by reference)
        populateInstance(agents, items, minBundlePrice);

        // find Least Spenders given minimum bundle price and estimate EFMax Bundle Price
        vector<int> leastSpenders = findLeastSpenders(agents, minBundlePrice);
        double EFMaxBundlePrice = findEFMaxBundlePrice(agents, items);
        vector<int> bigSpenders = findBigSpenders(agents, items, EFMaxBundlePrice);

        // LOGS
        cout << "Number of Agents: " << n << "; Number of Items: " << m << endl;
        printUtilityMap(agents.size(), items.size(), agents, items);

        myExcel << endl << "Sample-" <<  iteration << endl;
        logfile << iteration << " " << n << " " << m << " ";
        minEnvyDiff_File << iteration << " ";
        minBundlePrice_File << iteration << " ";
        EFMaxValuation_File << iteration << " ";
        EFMaxBundlePrice_File << iteration << " ";
        nashEFMaxWelfare_File << iteration << " ";
        minBundleValuation_File << iteration << " ";
        EFMaxPlusMinValuation_File << iteration << " ";

        DEBUG?(cout << "Least Spenders" << " -> "):(cout << "");
        DEBUG?(printIntVector(leastSpenders)):(printIntVector({}));
        DEBUG?(printAgentAllocationMBB(agents, items)):(printIntVector({}));
        DEBUG?(printRevisedPrices(items)):(printIntVector({}));
        DEBUG?(cout << endl):(cout<< "");
        DEBUG?(cout << "Least Spenders Bundle Price: " << minBundlePrice << endl):(cout<< "");
        DEBUG?(cout << "Big Spender EFMax Bundle Price: " << EFMaxBundlePrice << endl):(cout<< "");


        // 1.-> Do BFS with Least Spender as source to find path violator----------------------------------------------------------------------------------------1.
        while( (minBundlePrice < EFMaxBundlePrice) && doubleIsEqual(minBundlePrice, EFMaxBundlePrice, EPS)==false ) {

            cout << "----> Allocation not currently pEF1" << endl;

            // 2.-> finding alternating paths from LS to path violater --------------------------------------------------------2.
              
            int LS = -1;
            int BS = -1;
            int prevLS = -1;
            int prevBS = -1;
            int path_found = 1; //denotes if path was found in the coming step or not
            unordered_set<int> leastSpenderComponentAgents, leastSpenderComponentItems; 

            while(1) {

                queue<Nodes*> q;
                vector<int> visitedAgent(n,0), visitedItem(m,0);
                vector<int> predAgentToItem(m,-1), predItemToAgent(n,0); //predAgent = preceding agent to an item
                int LSToBSAgent = -1;

                if(path_found) {
                    // recompute LS and BS and check if allocation is pEF1 is the path was found or exchange occured
                    minBundlePrice = findMinBundlePrice(agents);
                    EFMaxBundlePrice = findEFMaxBundlePrice(agents, items);
                    if( doubleIsGreaterOrEqual(minBundlePrice, EFMaxBundlePrice, EPS) ) {
                        cout << "Allocation - EF1+fPO"; 
                        break;
                    }

                    leastSpenders = findLeastSpenders(agents, minBundlePrice);
                    bigSpenders = findBigSpenders(agents, items, EFMaxBundlePrice);
                    leastSpenderComponentAgents.clear();
                    leastSpenderComponentItems.clear();
                    prevLS = LS;
                    prevBS = BS;
                    LS = leastSpenders[0];
                    BS = bigSpenders[0];

                    // LOGS
                    DEBUG?(cout << "Big Spenders EFMax Bundle Price: " << EFMaxBundlePrice << endl):(cout<< "");
                    DEBUG?(cout << "Least Spenders Bundle Price: " << minBundlePrice << endl):(cout<< "");
                    DEBUG?(cout << "Least Spenders -> "):(cout<< "");
                    DEBUG?(printIntVector(leastSpenders)):(printIntVector({}));
                    DEBUG?(cout << "Big Spenders -> "):(cout<< "");
                    DEBUG?(printIntVector(bigSpenders)):(printIntVector({}));

                    minBundlePrice_File << minBundlePrice << " ";
                    EFMaxBundlePrice_File << EFMaxBundlePrice << " "; 
                }
                else {
                    // else if no path was found but there exists multiple lease spenders move to next LS
                    leastSpenders.erase(leastSpenders.begin());
                }   

                // LOG
                double minBundleValuation = findBundleValuation(LS, LS, agents);
                minEnvyDiff_File << std::fixed << findMinEnvyDiff(agents) << " ";
                minBundleValuation_File << minBundleValuation << " " << LS << " ; ";
                EFMaxValuation_File << findEFMaxValuation(agents, items, LS) << LS << ";";
                EFMaxPlusMinValuation_File << findEFMaxPlusMinValuation(agents, items, LS) << LS << " ";
                minAndEFMaxBundlePriceDiff_File << std::fixed << (EFMaxBundlePrice - minBundlePrice) << " ";
                nashEFMaxWelfare_File << std::fixed << findNashEFMaxWelfare(agents, items) << " ";
                DEBUG?(cout << "Least Spenders " << LS << "'s Valuation " << minBundleValuation << endl):(cout << "");
                DEBUG?(cout << "Big Spenders " << BS << "'s Valuation " << findBundleValuation(BS, BS, agents) << endl):(cout << "");
                // generateExcel(agents, items, myExcel);

                
                
                // insert the metric to check for monotonicity in a map
                // long double metric = (long double) (findBundleValuation(LS, BS, agents));
                // if(valuationMap.find(LS)==valuationMap.end()) {
                //     valuationMap.insert({LS, metric});
                // } 
                // else {
                //     long double prevValuation = valuationMap.at(LS);
                //     if( doubleIsGreaterOrEqual(prevValuation, metric, EPS)==false ) {
                //         // if previous value of metric was less than current, update it 
                //         valuationMap.at(LS) = metric;
                //     }
                //     else if( doubleIsGreater(prevValuation, metric, EPS) && prevLS!=LS) {
                //         // else if the metric value has strictly decreased from it previous value when LS was least spender, then exit
                //         cout << "Exited: PREV_METRIC_AFTER_LS_AGAIN_GREATER, prev: " << prevValuation << " now: " << metric << endl;
                //         goto GOTO_EXIT;
                //         // if(is_EF1_fPO(agents, items)==true) {
                //         //     cout << "CHECK1: LS_TO_BS=1 & EF1=1" << endl;
                //         //     // goto GOTO_NEXT;
                //         // }
                //         // else {
                //         //     cout << "CHECK1: LS_TO_BS=1 & EF1=0" << endl;
                //         //     // goto GOTO_EXIT;
                //         // }
                //     }
                // }

                // check if any of the past Least Spenders have become the Big Spenders
                // for(unordered_map<int, long double>::iterator it = valuationMap.begin(); it!=valuationMap.end(); it++) {
                //     if( doubleIsEqual(EFMaxBundlePrice, findEFMaxBundlePrice(agents, items, it->first), EPS)==true ) {
                //         cout << "Exited: PREV_LS_BECOMES_BS Agent " << it->first << " becomes Big Spender. Bbundle price: " << findEFMaxBundlePrice(agents, items, it->first) << endl;
                //         LSToBSAgent = it->first;

                //         if(is_EF1_fPO(agents, items)==false) cout << "EF1 condition not satisfied" << endl;
                //         else cout << "EF1 satisfied" << endl;
                //     }
                // }

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
                            if( path_found==1 && doubleIsGreaterOrEqual(minBundlePrice, (agents[i].bundlePrice - item->price), EPS)==false ) {
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

                if(prevLS!=LS && prevLS!=-1) {
                    // log data when any least spender repeats
                    double LSVal = findBundleValuation(LS, LS, agents);
                    double BSVal = findBundleValuation(BS, LS, agents);
                    string id = to_string(LS) + "-" + to_string(BS);
                    if(customValuationMap.find(id)==customValuationMap.end()) {
                        customValuationMap.insert({id, {LSVal, BSVal}});
                    }
                    else {
                        if(doubleIsGreaterOrEqual(LSVal, customValuationMap[id][0], EPS)==false) {
                            double LSValDiff = customValuationMap[id][0] - LSVal;
                            double BSValDiff = customValuationMap[id][1] - BSVal;
                            cout << "\u0394LS: " << LSValDiff << " \u0394BS: " << BSValDiff << endl;
                            if(doubleIsGreater(BSValDiff, abs(LSValDiff), EPS)) cout <<  "Good" << endl;
                            else cout << "BAD" << endl;
                        }
                        else {
                            customValuationMap[id][0] = LSVal;
                            customValuationMap[id][1] = BSVal;

                        }
                    }
                    long double metric = (long double) (findBundleValuation(LS, LS, agents));
                    if(valuationMap.find(LS)!=valuationMap.end()) {
                        generateExcel(agents, items, myExcel);
                    }
                    else {
                        valuationMap.insert({LS, 0});
                    }
                    // valuationMap[prevLS] = findBundleValuation(prevLS, prevLS, agents) - agents[prevLS].itemUtilityMap[agents[prevLS].allocationItems.back()->index];
                    // long double metric = (long double) findEFMaxValuation(agents, items, BS) - findBundleValuation(LS, BS, agents);
                    // int status = checkMetricMonotonicityWhenSameAgentbecomesLS("increasing", LS, valuationMap, metric, agents, items);
                    // if(status==2) goto GOTO_NEXT;
                    // else if(status==0) goto GOTO_EXIT;
                }



                // if LS becomes BS and no path was found, check if it was EF1, if yes, continue to next sample, else exit 
                // if(LSToBSAgent!=-1 && pathViolater==-1) {
                //     if(is_EF1_fPO(agents, items)==true) {
                //         cout << "CHECK1: LS_TO_BS=1 & EF1=1" << endl;
                //         // goto GOTO_NEXT;
                //     }
                //     else {
                //         cout << "CHECK1: LS_TO_BS=1 & EF1=0" << endl;
                //         // goto GOTO_EXIT;
                //     }
                // }

                // transfer item to pred[itemViolater] from pathViolater and update bundle prices if a path violater was found
                if(pathViolater!=-1) {

                    // see if the pathviolator was the new BS - If not, check EF1, else continue
                    // if(LSToBSAgent!=-1 && pathViolater!=LSToBSAgent) {
                    //     cout << "EXIT:LS_TO_BS - LS turned BS was not path violator" << endl;
                    //     if(is_EF1_fPO(agents, items)==true) {
                    //         cout << "CHECK2: LS_TO_BS=1 & EF1=1" << endl;
                    //         // goto GOTO_NEXT;
                    //     }
                    //     else {
                    //         cout << "CHECK2: LS_TO_BS=1 & EF1=0" << endl;
                    //         // goto GOTO_EXIT;
                    //     }
                    // }
                    
                    // perform transfer, update bundles and graph
                    transferItem(itemViolater, pathViolater, predAgentToItem[itemViolater], agents, items);

                    // // if the LS directly receives an item, log it
                    // if(predAgentToItem[itemViolater]==LS) {
                    //     // generateExcel(agents, items, myExcel);
                    //     long double metric = findBundleValuation(LS, LS, agents);
                    //     if(afterReceivingItemValuationMap.find(LS)==afterReceivingItemValuationMap.end()) {
                    //         afterReceivingItemValuationMap.insert({LS, metric});
                    //     }
                    //     else {
                    //         long double prevValuation = afterReceivingItemValuationMap.at(LS);
                    //         if(prevValuation < metric && (abs(prevValuation - metric)< EPS)==false) {
                    //             valuationMap.at(LS) = metric;
                    //         }
                    //         // if the metric value has strictly decreased from it previous value when LS was least spender, then exit
                    //         else if(prevValuation > metric && (abs(prevValuation - metric) < EPS)==false) {
                    //             cout << "Exited: PREV_AFTER_RECEIVING_ITEM_PROOF not satisfied prev: " << prevValuation << " now: " << metric << endl;
                    //             // return 0;
                    //           }
                    //     }
                    // }

                    transferStepsCount++;
                    // printAgentAllocationMBB(agents, items);
                    path_found = 1;
                }
                else if(q.empty()) {

                    // if pEF1 condition satisfied, come out of the loop and return the allocation
                    if( doubleIsGreaterOrEqual(minBundlePrice, EFMaxBundlePrice, EPS) ) {
                        break;
                    }
                    
                    // generateExcel(agents, items, myExcel);

                    // Add items allocated to least spender also in the Component
                    for(int i:leastSpenderComponentAgents) {
                        for(ItemNodes* item: agents[i].allocationItems) {
                            leastSpenderComponentItems.insert(item->index);
                        }
                    }

                    // compute alpha 1, alpha 2 and beta
                    double alpha1 = computeAlpha1(leastSpenderComponentAgents, leastSpenderComponentItems, agents, items);
                    double alpha2 = computeAlpha2(leastSpenderComponentAgents, agents, minBundlePrice);
                    double beta = fmax(alpha1, 0);
                    
                    // raise the prices of all items in the Least Spender component
                    updateItemPrices(leastSpenderComponentItems, items, beta);

                    // update bundles of LS component Agents
                    updateAgentBundles(leastSpenderComponentAgents, leastSpenderComponentItems, agents, items, beta);

                    // LOG
                    DEBUG?(cout << "Big Spenders EFMax Bundle Price: " << EFMaxBundlePrice << endl):(cout<< "");
                    DEBUG?(cout << "Least Spenders Bundle Price: " << minBundlePrice << endl):(cout<< "");
                    DEBUG?(cout << "----> No alternating path from LS agent " << LS << " -> Decreasing Prices" << endl):(cout<< "");
                    
                    DEBUG?(cout << "LS Component: Agents -> "):(cout<< "");
                    DEBUG?(printIntSet(leastSpenderComponentAgents)):(printIntVector({}));
                    DEBUG?(cout << "LS Component: Items -> "):(cout<< "");
                    DEBUG?(printIntSet(leastSpenderComponentItems)):(printIntVector({}));

                    cout << "Beta value is " << beta << endl;
                    DEBUG?(cout << "Alpha 1 -> " << alpha1 << "; Alpha 2 -> " << alpha2 << endl):(cout<< "");
                    DEBUG?(cout << "----> Decreasing Price of LS Component by beta = " << beta << endl):(cout<< "");
                    DEBUG?(printRevisedPrices(items)):(printIntVector({}));
                    DEBUG?(printAgentAllocationMBB(agents, items)):(printIntVector({}));

                    path_found = 1;
                    priceRiseStepsCount++;
                }
            }
            // -------------------------------------------------------------------------------------------------------------2.
            
            // redefine minimum Price Bundle and EFMax Bundle Price
            minBundlePrice = findMinBundlePrice(agents);
            EFMaxBundlePrice = findEFMaxBundlePrice(agents, items);
        }
        // -----------------------------------------------------------------------------------------------------------------------------------------------------1.

        // capturing total runtime
        // auto stop = std::chrono::high_resolution_clock::now();
        // auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        // LOG
        myExcel << endl;
        logfile << "" /*duration.count()*/ << " " << priceRiseStepsCount << " " << transferStepsCount << endl;
        minEnvyDiff_File << std::fixed << findMinEnvyDiff(agents) << endl;
        minBundlePrice_File << endl;
        EFMaxValuation_File << endl;
        EFMaxBundlePrice_File << endl;
        minBundleValuation_File << endl;
        minAndEFMaxBundlePriceDiff_File << (EFMaxBundlePrice - minBundlePrice) << endl;
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

        GOTO_NEXT:
        iteration++;
    }

    // closing log files
    myExcel.close();
    logfile.close();
    sampleFile.close();
    minEnvyDiff_File.close();
    minBundlePrice_File.close();
    EFMaxValuation_File.close();
    EFMaxBundlePrice_File.close();
    minBundleValuation_File.close();
    minAndEFMaxBundlePriceDiff_File.close();

    GOTO_EXIT:
    system("canberra-gtk-play -f ~/Downloads/pno-cs.wav"); // play sound once the program finishes LOL!
    return 0;
}

