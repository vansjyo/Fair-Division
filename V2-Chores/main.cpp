// #include <bits/stdc++.h>
#include "output.h"
#define EPS 0.00001f
using namespace std;

int main() 
{

    int samples = 10000, iteration = 0;
    while(iteration < samples) {
        cout << "Working on Sample Number " << iteration << endl;

        // initialize n - agents (iterator-> i), m - items (iterator-> j)
        srand(iteration);
        int n = rand() % 10 + 2; 
        int m = rand() % 15 + 1;

        vector<AgentNodes> agents(n);
        vector<ItemNodes> items(m);

        // populate item Utility map and determine price of each item
        for(int j = 0; j < m; j++) {
            ItemNodes *item = new ItemNodes();
            for(int i = 0; i < n; i++) {
                if(j==0) {
                    AgentNodes *agent = new AgentNodes();
                    agents[i] = *agent;
                    agents[i].index = i;
                }
                agents[i].itemUtilityMap.push_back(0);
                int vij = (rand() % 15 + 1)*10000000;
                agents[i].itemUtilityMap[j] = (double) vij;
                items[j].price = fmin(items[j].price, agents[i].itemUtilityMap[j]);
            }
            items[j].index = j;
        }

        // populate MBB ratio for all agents
        for(int i = 0; i < n; i++) {
            double MBB = numeric_limits<double>::max();
            for(int j = 0; j < m; j++) {
                MBB = fmin(MBB, agents[i].itemUtilityMap[j]/items[j].price);
            }
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
        cout << "Least Spenders" << " -> ";
        printIntVector(leastSpenders);

        // print inital allocation
        printAgentAllocationMBB(agents);
        printRevisedPrices(items);
        cout << endl;

        // estimate EFMaxBundlePrice
        double EFMaxBundlePrice = findEFMaxBundlePrice(agents, items);
        cout << "Least Spenders Bundle Price: " << minBundlePrice << endl;
        cout << "Big Spender EFMax Bundle Price: " << EFMaxBundlePrice << endl;


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
                    cout << "Big Spenders EFMax Bundle Price: " << EFMaxBundlePrice << endl;
                    cout << "Least Spenders Bundle Price: " << minBundlePrice << endl;
                    if( (minBundlePrice > EFMaxBundlePrice) || doubleIsEqual(minBundlePrice, EFMaxBundlePrice, EPS)==true ) {
                        break;
                    }
                    leastSpenders = findLeastSpenders(agents, minBundlePrice);
                    cout << "Least Spenders -> ";
                    printIntVector(leastSpenders);
                    leastSpenderComponentAgents.clear();
                    leastSpenderComponentItems.clear();
                }
                // else if no path was found but there exists multiple lease spenders move to next LS
                else 
                    leastSpenders.erase(leastSpenders.begin());

                int LS = leastSpenders[0];
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
                                cout << "Path Violater -> Agent - " << i << "; Item - " << item->index << endl; 
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

                    printAgentAllocationMBB(agents);
                    path_found = 1;
                }
                // check if no path found and there exists another least spender.
                else if(q.empty() && leastSpenders.size()>1) {
                    cout << "----> No alternating path from LS agent " << LS  << " -> Creating component " << leastSpenders[1] << endl; 
                    path_found = 0;
                }
                else if(q.empty()) {

                    // estimate EFMaxBundlePrice
                    EFMaxBundlePrice = findEFMaxBundlePrice(agents, items);
                    cout << "Big Spenders EFMax Bundle Price: " << EFMaxBundlePrice << endl;
                    cout << "Least Spenders Bundle Price: " << minBundlePrice << endl;

                    // if pEF1 condition satisfied, come out of the loop and return the allocation
                    if((minBundlePrice > EFMaxBundlePrice) || doubleIsEqual(minBundlePrice, EFMaxBundlePrice, EPS)==true ) {
                        break;
                    }

                    // else increase price of all items in LS component
                    cout << "----> No alternating path from LS agent " << LS << " -> Decreasing Prices" << endl;

                    // Add items allocated to least spender also in the Component
                    for(int i:leastSpenderComponentAgents) {
                        for(ItemNodes* item: agents[i].allocationItems) {
                            leastSpenderComponentItems.insert(item->index);
                        }
                    }

                    // print the least spender component
                    cout << "LS Component: Agents -> ";
                    printIntSet(leastSpenderComponentAgents);
                    cout << "LS Component: Items -> ";
                    printIntSet(leastSpenderComponentItems);

                    // compute alpha 1, alpha 2 and beta
                    double alpha1 = computeAlpha1(leastSpenderComponentAgents, leastSpenderComponentItems, agents, items);
                    double alpha2 = computeAlpha2(leastSpenderComponentAgents, agents, minBundlePrice);
                    double beta = fmax(alpha1, 0);
                    cout << "Alpha 1 -> " << alpha1 << "; Alpha 2 -> " << alpha2 << endl;
                    cout << "----> Decreasing Price of LS Component by beta = " << beta << endl;
                    
                    // raise the prices of all items in the Least Spender component
                    updateItemPrices(leastSpenderComponentItems, items, beta);

                    // update bundles of LS component Agents
                    updateAgentBundles(leastSpenderComponentAgents, leastSpenderComponentItems, agents, items, beta);

                    printRevisedPrices(items);

                    printAgentAllocationMBB(agents);
                    path_found = 1;
                    
                }
            }
            // -------------------------------------------------------------------------------------------------------------2.
            
            //redefine minimum Price Bundle and EFMax Bundle Price
            minBundlePrice = findMinBundlePrice(agents);
            EFMaxBundlePrice = findEFMaxBundlePrice(agents, items);
        }
        // -----------------------------------------------------------------------------------------------------------------------------------------------------1.

        cout << endl << "------ Allocation is now pEF1+fPO -------" << endl << endl;
        printAgentAllocationMBB(agents);

        // FINAL BRUTE CHECK
        if(is_EF1_fPO(agents, items)==false) {
            cout << "ERROR - ALLOCATION INCORRECT" << endl;
            return 0;
        }
        else {
            cout << "ALLOCATION - CORRECT" << endl;
        }
            
        iteration++;
    }


    

    return 0;
}

