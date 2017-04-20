#include <algorithm>
#include <assert.h>
#include <bitset>
#include <complex>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <thread>
#include <time.h>
#include <string>
#include <chrono>
#include <iostream>
#include <string>
#include <map>
#include <utility>
#include <vector>
#include <chrono>
#include <sstream>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits.h>
#include <list>
#include <map>
#include <math.h>
#include <queue>
#include <set>
#include <stack>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <random>
#include <chrono>
#include <netinet/in.h>
#include <thread>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <mutex>

using namespace std;

typedef pair<string,int> PSI;

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
 * Keeping track of nodeid of current node and total number of nodes
 */
int myId = -1;
int N_nodes = -1;

class invitation
{
private:
	int leader = -1;
	/**
	 * Normal = 0/Down = 1/Election = 2/Reorganisation = 3
	 */
	int status;
	int counter;
	int nodeId;
	std::vector<int> neighbours;
	map<int,PSI> itopsi;
	map<PSI,int> psitoi;
	int timeout = 1;
public:
	std::vector<int> group; //only if needed!
	invitation(int nodeId);
	~invitation();
	//0
	void AreYouCoord();
	//1
	void AreYouCoordReply(int incomingId);
	//2
	void AreYouThereQues();
	//3
	void AreYouThereAns();
	//4
	void SendInviteMessage();
	//5
	void AreYouReady();
	//6
	void ReadyReply();
	//7
	void AskCoordinatorToAcceptSender();
	//8
	void CoordinatorToSender();
	//9
	void Recovery();
	//10
	string createMessage(string s,string op){
		return op+"|"+s+"|"+to_string(nodeId);
	}
	//11
	void sendMessage(string s,int incomingId){
		int sockfd = socket(AF_INET , SOCK_STREAM , 0);
		if(sockfd<0){
			printf("Socket Creating Error!\n");
			exit(1);
		}
		struct sockaddr_in server;
	    server.sin_family = AF_INET;
	    server.sin_port = htons(itopsi[incomingId].second);
	    struct hostent *hp;
	    if ((hp = gethostbyname((itopsi[incomingId].first).c_str()))==0) {
	      printf("Invalid or unknown host %s\n",(itopsi[incomingId].first).c_str());
	      close(sockfd);
	      return;
	    }
	    memcpy(&server.sin_addr.s_addr,hp->h_addr,hp->h_length);
	    if (connect(sockfd,(struct sockaddr *)&server, sizeof(server)) < 0)
	    {
	      printf("connect error to Node%d from Node%d\n",incomingId,nodeId);
	      close(sockfd);
	      exit(0);
	    }
	    send(sockfd,s.c_str(),30,0);
	    printf("Node %d send a CoordReply: %s to Node %d\n",nodeId,s.c_str(),incomingId);
	    close(sockfd);
	    return;
	}
};

/**
 * Basic Initialisations
 */

invitation::invitation(int nodeId){
	leader = nodeId;
	status = 1;
	counter = 1;
	this->nodeId = nodeId;
}

/**
 *
 */

void invitation::AreYouCoord(int toAskNodeId){
	sendMessage("0",toAskNodeId);
}

/**
 * Reply to the asker if I am a coordinator or not
 */

void invitation::AreYouCoordReply(int incomingId){
	string s;
	if (leader == nodeId){
		s = createMessage("Yes","1");
	}
	else{
		s = createMessage("No","1");
	}
	sendMessage(s,incomingId);
}

/**
 * Asking the coordinator Periodically if it is there
 */

void invitation::AreYouThereQues(){
	while(true){
		if(nodeId==leader) continue;
		sleep(3*timeout);
		/**
		 * Can try keep a random number here for this eaxcution
		 */
		bool isThere = 0;
		sendMessage("2",leader);
		time_t start = time(NULL);
		while(time(NULL)-start<timeout){
			string s = checkInstream("3",nodeId);
			if(s=="Yes"){
				/**
				 * Good!
				 */
				isThere = 1;
			}
			else if (s == "No"){
				/**
				 * Ignore and break from while loop
				 */
				break;
			}
		}
		if(!isThere){
			/**
			 * Oops leader is not there
			 * Call Recovery immideately
			 */
			Recovery();
		}
		else{
			/**
			 * All good lets continue
			 */
		}
	}
}

/**
 * Replying to a group mate
 */

void invitation::AreYouThereAns(int incomingId){
	if (state != 1 && leader == nodeId){
		string s = createMessage("Yes","3");
	}
	else{
		string s = createMessage("No","3");
	}
	sendMessage(s,incomingId);
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
	node.Recovery();
	while(true){

	}
	return 0;
}