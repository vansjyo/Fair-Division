// #include <bits/stdc++.h>
#include "output.h"
#define EPS 0.0001f
using namespace std;

int main() 
{

    // initialize n - agents (iterator-> i), m - items (iterator-> j)
    int samples = 5, iteration = 0;
    while(iteration < samples) {
        int n, m;
        string fileName = "utilities" + to_string(iteration) + ".txt";
        ifstream agentUtilities(fileName);
        agentUtilities >> n >> m;
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
                agentUtilities >> agents[i].itemUtilityMap[j];
                items[j].price = fmax(items[j].price, agents[i].itemUtilityMap[j]);
            };
            items[j].index = j;
        }

        // populate MBB ratio, MBB items, bundle price for every agent, an initial allocation and least spender's spending
        float minBundlePrice = numeric_limits<float>::max();
        for(int j = 0; j < m; j++) {
            int allocated_flag = 0;
            for(int i = 0; i < n; i++) {
                if(floatIsEqual(items[j].price, agents[i].itemUtilityMap[j], EPS)) {
                    agents[i].MBBItems.push_back(&items[j]);
                    if(allocated_flag==0) {
                        agents[i].allocationItems.push_back(&items[j]);
                        agents[i].bundlePrice+=items[j].price;
                        items[j].allocatedAgent = i;
                    }
                    allocated_flag = 1;
                }    
                minBundlePrice = (j==m-1)?fmin(minBundlePrice, agents[i].bundlePrice):minBundlePrice;
            }
        }

        // print Least Spenders given minimum bundle price
        vector<int> leastSpenders = findLeastSpenders(agents, minBundlePrice);
        cout << "Least Spenders" << " -> ";
        printIntVector(leastSpenders);

        // print inital allocation
        printAgentAllocationMBB(agents);
        printRevisedPrices(items);
        cout << endl;

        // estimate EFMaxBundlePrice
        float EFMaxBundlePrice = findEFMaxBundlePrice(agents, items);
        cout << "Least Spenders Bundle Price: " << minBundlePrice << endl;
        cout << "Big Spender EFMax Bundle Price: " << EFMaxBundlePrice << endl;


        // 1.-> Do BFS with Least Spender as source to find path violator----------------------------------------------------------------------------------------1.
        while( (minBundlePrice < EFMaxBundlePrice) && floatIsEqual(minBundlePrice, EFMaxBundlePrice, EPS)==false ) {

            cout << "----> Allocation not currently pEF1" << endl;

            // 2.-> finding alternating paths from LS to path violater --------------------------------------------------------2.
            
            int path_found = 1; //denotes if path was found in the coming step or not
            unordered_set<int> leastSpenderComponentAgents, leastSpenderComponentItems; 
            while(1) {
                queue<Nodes*> q;
                vector<int> visitedAgent(n,0), visitedItem(m,0);
                vector<int> predAgentToItem(m,-1), predItemToAgent(n,0); //predAgent = preceding agent to an item
                // int alternate_flag = 0; //denoted whether to find an MBB item or aloctaed agent

                // revise Least Spenders if path was found or exchange occured
                if(path_found) {
                    minBundlePrice = findMinBundlePrice(agents);
                    cout << "Least Spenders Bundle Price: " << minBundlePrice << endl;
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
                    // alternate floag = 0 -> search for MBB items
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
                        // alternate_flag = 1;
                    }
                    // alternate floag = 1 -> search for agent to which item is allocated
                    else if(q.front()->type=="ItemNode") {
                        ItemNodes* item = dynamic_cast<ItemNodes*>(q.front());
                        // int temp = q.front().first;
                        q.pop();
                        int i = item->allocatedAgent;
                        if(visitedAgent[i]==0) {
                            predItemToAgent[i] = item->index;
                            q.push(&agents[i]);
                            visitedAgent[i] = 1;
                            leastSpenderComponentAgents.insert(i);
                            if( minBundlePrice < (agents[i].bundlePrice - item->price) ) {
                                cout << "----> Path Violator Found" << endl;
                                cout << "Path Violater -> Agent - " << i << "; Item - " << item->index << endl; 
                                pathViolater = i;
                                itemViolater = item->index;
                                leastSpenderComponentItems.insert(item->index);
                                break;
                            }
                        }
                        
                        // alternate_flag = 0;
                    }
                }
                // ------------------------------------------------------------------------------- 3.

                // transfer item to pred[itemViolater] from pathViolater and update bundle prices if a path violater is found
                if(pathViolater!=-1) {
                    transferItem(itemViolater, pathViolater, predAgentToItem[itemViolater], agents, items);
                    path_found = 1;
                }
                // check if no path found and there exists another least spender.
                else if(q.empty() && leastSpenders.size()>1) {
                    cout << "----> No alternating path from LS agent " << LS  << " -> Trying next LS" << endl; 
                    path_found = 0;
                }
                else if(q.empty()) {

                    // estimate EFMaxBundlePrice
                    EFMaxBundlePrice = findEFMaxBundlePrice(agents, items);
                    cout << "Big Spenders EFMax Bundle Price: " << EFMaxBundlePrice << endl;
                    cout << "Least Spenders Bundle Price: " << minBundlePrice << endl;

                    // if pEF1 condition satisfied, come out of the loop and return the allocation
                    if((minBundlePrice > EFMaxBundlePrice) && floatIsEqual(minBundlePrice, EFMaxBundlePrice, EPS)==false ) {
                        break;
                    }
                    // else increase price of all items in LS component
                    else {

                        cout << "----> No alternating path from LS agent " << LS << " -> Increasing Prices" << endl;

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
                        float alpha1 = computeAlpha1(leastSpenderComponentAgents, leastSpenderComponentItems, agents, items);
                        float alpha2 = computeAlpha2(leastSpenderComponentAgents, agents, minBundlePrice);
                        float beta = fmin(alpha1, alpha2);
                        cout << "Alpha 1 -> " << alpha1 << "; Alpha 2 -> " << alpha2 << endl;
                        cout << "----> Increasing Price of LS Component by beta = " << beta << endl;
                        
                        // raise the prices of all items in the Least Spender component
                        updateItemPrices(leastSpenderComponentItems, items, beta);

                        // update bundles of LS component Agents
                        updateAgentBundles(leastSpenderComponentAgents, leastSpenderComponentItems, agents, items, beta);

                        printRevisedPrices(items);
                    }
                    path_found = 1;
                    printAgentAllocationMBB(agents);
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

    iteration++;
    }

    return 0;
}

