#include <iostream>
#include <string>
#include <map>
#include <utility>
#include <vector>

using namespace std;

/**
 * Giving Higher priority to
 * higher numbered process
 * Each group has the highest number 
 * process as its leader
 * It waits for 2*TM+TP(see readme!) before declaring timeout on each id
 * node id's start from 1-n(considering n processes)
 */

long long TM,TP;

/**
 * Simulate the network(Assume and handle failures randomly make node sleep for some time doing nothing)
 * for this generate a random number between 0-100 if number <10 make the node sleep
 */

double failueProbability = 0.1;

/**
 * Randomly set it to be 4 sec(maybe anything)
 */

long long sleepingTime = 4;

/**
 * Store ID of network with IP,port
 */

#define pair<string,int> ps

std::map<int,ps> map;


/**
 * Keeping track of nodeid of current node and total number of nodes
 */
myId = -1;
N_nodes = -1;

class invitation
{
private:
	int leader = -1;
public:
	std::vector<int> group; //only if needed!
	invitation(int nodeId);
	~invitation();
	void sendMessage(int id);
	void recieveMessage();
	void merge();
	void checkIfLeaderIsAlive();
	void announceLeaderChange();
	
};

invitation::invitation(int nodeId){
	leader = nodeId;
}

void invitation::announceLeaderChange(){
	/**
	 * create socket and send the message to the whole group telling them their leader
	 * after that clear the nodes under it mind all messages should again have a probability of being sent
	 */
	group.clear();
}

void invitation::checkIfLeaderIsAlive(){
	/**
	 * if no message is recieved from the leader from some given time
	 * assume leader not alive(assume time to be 3*(2*TM+Tp) similar to 3RTT)
	 * anounce itself to be leader
	 * keep a lock on each function
	 */
	group.clear();
	group.push_back(myId);
	leader = myId;
}

void invitation::mergeGroups(int _leader,string s){
	/**
	 *still check the fact that more than two leaders respond with higher priority
	 *check what to do in such a case
	 *If leader not equal to myId
	 */
	if(myId == _leader){
		/**
		 * Update the group by converting string to vector
		 */
	}
	else{
		/**
		 * in this case inform the group mates of leader change
		 * and update leader
		 */
		leader = _leader;
		sendMessage(leaderChange);
		group.clear();
	}
}

int main(int argc, char const *argv[])
{
	if (argc != 3){
		cout<<"Error Enter node ID,Number of nodes as command line Args"<<endl;
		exit(1);
	}
	myId = atoi(argv[1]);
	N_nodes = atoi(argv[2]);
	invitation node(myId);
	return 0;
}