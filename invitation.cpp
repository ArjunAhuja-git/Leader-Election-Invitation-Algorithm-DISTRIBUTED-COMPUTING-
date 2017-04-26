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

struct sockaddr_in server,cli_addr;
socklen_t clilen;
char server_reply[10000];

std::mutex foo,bar,failure;

char* currentTime(time_t curtime)
{
    struct tm* ptime = localtime(&curtime);
    char *currentTime = new char[80];
    strftime(currentTime, 80, "%T", ptime);
    return currentTime;
}

std::vector<int> graph[1000];

thread a[20];
bool called[20];
bool isDownVar = false;
time_t isDownTime = time(NULL);
int downTime = 20; //10 secs failure

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
	map<PSI,int> psitoi;
	int timeout = 1;
	bool ingroup[1000];
public:
	map<int,PSI> itopsi;
	std::vector<int> group; //only if needed!
	queue<int> incomingQueue[30];
	/**
	 * Expecting 1000 nodes in the end.
	 */
	bool isRecieved[20][1000];
	string RecievedString[20][1000];
	time_t isSent[20][1000];
	invitation(int nodeId);
	//0
	void AreYouCoord(int nodeId);
	//1
	void AreYouCoordReply(int incomingId);
	//2
	void AreYouThereQues();
	//3
	void AreYouThereAns(int nodeId);
	//4
	void SendInviteMessage(int nodeId);
	//5
	void IwantToJoin(int nodeId);
	//6
	void IwantToJoinReply(int nodeId);
	//7
	void AskCoordinatorToAcceptSender();
	//8
	void CoordinatorToSender();
	//9
	void Recovery();
	//10
	void merge(int nodeId);
	//11
	string createMessage(string s,string op){
		return op+"|"+s+"|"+to_string(nodeId);
	}
	//12
	void sendMessage(string s,int incomingId){
		if(isDownVar) return;
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
	    close(sockfd);
	    return;
	}
	//13
	string checkInstream(int s,int node){
		if(isRecieved[s][node]){
			isRecieved[s][node] = 0;
			if(RecievedString[s][node] == ""){
				return "anyMessage";
			}
			else{
				return RecievedString[s][node];
			}
		}
		else{
			return "";
		}
	}
	//14
	void keepACheckOnCoordinator();
	//15
	void recieveFunction();
	//16
	void sendFunction();
	//17
	void keepSendingMessagesToOtherNodesToSeeIfTheyAreCoord(int n);
	//18
	void onRecieveingSendInviteMessasge(int incomingId,string message);
	//19
	void mainFunction(int nodes);
};

/**
 * Basic Initialisations
 */

invitation::invitation(int nodeId){
	leader = nodeId;
	status = 1;
	counter = 1;
	this->nodeId = nodeId;
	group.push_back(nodeId);
	for (int i = 0; i < 20; ++i)
	{
		called[i] = false;
		for (int j = 0; j < 1000; ++j)
		{
			isRecieved[i][j] = false;
			RecievedString[i][j] = "";
			/**
			 * Assuming at max maxwill arive after 100 secs after failue
			 */
			isSent[i][j] = time(NULL)-100;
		}
	}
}

/**
 *
 */

void invitation::AreYouCoord(int toAskNodeId){
	string s = createMessage("","0");
	printf("Time:%s --> Are you a Coord message sent from Node%d to Node%d\n",currentTime(time(NULL)),nodeId,toAskNodeId);
	sendMessage(s,toAskNodeId);
	isSent[1][toAskNodeId] = time(NULL);
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
	foo.lock();
	printf("Time:%s --> Are you a Coord message Reply sent from Node%d to Node%d message:%s\n",currentTime(time(NULL)),nodeId,incomingId,s.c_str());
	sendMessage(s,incomingId);
	foo.unlock();
}

/**
 * Asking the coordinator Periodically if it is there
 * TODO://TRY INCORPORATING INTO WHILE LOOP OR KEEP THIS IN A INFINITE THREAD
 */

void invitation::AreYouThereQues(){
	if(nodeId==leader) return;
	printf("checking if coordinator is there or not? %d\n",leader);
	/**
	 * Can try keep a random number here for this eaxcution
	 */
	bool isThere = 0;
	string s = createMessage("","2");
	sendMessage(s,leader);
	time_t start = time(NULL);
	isSent[3][leader] = time(NULL);
	while(time(NULL)-start<3*timeout && !isDownVar){
		string s = checkInstream(3,leader);
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
		printf("coordinator not there as no reply recieved within timeout\n");
		Recovery();
	}
	else{
		/**
		 * All good lets continue
		 */
		printf("coordinator replied all is good\n");
		/**
		 * Make isrecieved false and string ""
		 */
		isRecieved[3][nodeId] = false;
		RecievedString[3][nodeId] = "";
	}
}

/**
 * Replying to a group mate
 */

void invitation::AreYouThereAns(int incomingId){
	string s;
	/**
	 * If node is not present in the ingroup category then presume node lost and send no to it
	 */
	if (status != 1 && leader == nodeId && ingroup[incomingId]){
		s = createMessage("Yes","3");
	}
	else{
		s = createMessage("No","3");
	}
	foo.lock();
	sendMessage(s,incomingId);
	printf("Time:%s-->Are you there reply sent to Node%d\n",currentTime(time(NULL)),incomingId);
	foo.unlock();
}

/**
 * As a part of merge send message to other leaders to join me!
 */

void invitation::SendInviteMessage(int nodeId){
	string s = createMessage(to_string(nodeId),"4");
	sendMessage(s,nodeId);
	/**
	 * Keep a track of which all nodes you have sent the message
	 */
	printf("Time:%s --> Node%d invites Node%d to join group\n",currentTime(time(NULL)),this->nodeId,nodeId);
	return;
}

/**
 * Node send I want to join message to proposed leader
 * here nodeId is the proposed leader
 */

void invitation::IwantToJoin(int nodeId){
	foo.lock();
	status = 3;
	string s = createMessage(to_string(leader),"5");
	sendMessage(s,nodeId);
	isSent[6][nodeId] = time(NULL);
	printf("Time:%s --> I want to join your group message sent from Node%d to Node%d\n",currentTime(time(NULL)),this->nodeId,nodeId);
	foo.unlock();
	time_t start = time(NULL);
	bool isThere = 0;
	while(time(NULL)-start<6*timeout && !isDownVar){
		string s = checkInstream(6,nodeId);
		if(s == ""){
			//
		}
		else{
			isThere = 1;
			break;
		}
	}
	if(!isThere){
		/**
		 * Oops new leader is not there
		 * Call Recovery immideately
		 */
		Recovery();
	}
	else{
		/**
		 * As there is reply update the new leader to be nodeId
		 */
		leader = nodeId;
	}
	status = 0;
	return;
}

/**
 * Allow node to join the group
 */

void invitation::IwantToJoinReply(int nodeId){
	/**
	 * Check if I am still the leader or not and my status should not be DOWN
	 * and check if this me
	 */
	if(this->nodeId != leader && status != 1) return;
	/**
	 * Add node to group
	 */
	foo.lock();
	string s = createMessage(to_string(leader),"6");
	sendMessage(s,nodeId);
	printf("Time:%s --> You can join my group Node%d replies to Node%d\n",currentTime(time(NULL)),this->nodeId,nodeId);
	foo.unlock();
	ingroup[nodeId] = true;
	group.push_back(nodeId);
	printf("Node %d added to groupID %d of leader%d\n",nodeId,counter,leader);
	return;
}

/**
 * Go to initial configuration i.e. singleton leader 
 */

void invitation::Recovery(){
	foo.lock();
	printf("Time:%s -->Node enters recovery mode%d\n",currentTime(time(NULL)),nodeId);
	status = 2;
	group.clear();
	group.push_back(nodeId);
	leader = nodeId;
	for (int i = 0; i < 20; ++i)
	{
		for (int j = 0; j < 1000; ++j)
		{
			isRecieved[i][j] = false;
			RecievedString[i][j] = "";
			ingroup[j] = false;
			isSent[i][j] = 0;
		}
	}
	counter++;
	status = 3;
	status = 0;
	foo.unlock();
}

/**
 * Merge Nodes initially assume no failure
 */

void invitation::merge(int nodeId){
	bar.lock();
	printf("Time:%s-->Merge called by Node%d to add Leader%d\n",currentTime(time(NULL)),this->nodeId,nodeId);
	SendInviteMessage(nodeId);
	//sleep(timeout*5);
	time_t start = time(NULL);
	bool isThere = 0;
	status = 2;
	while(time(NULL)-start<timeout && !isDownVar){
		//Use check in stream to check any existing messages which state I want to join
		if(incomingQueue[5].empty()) continue;
		while(!incomingQueue[5].empty()){
			int t = incomingQueue[5].front();
			incomingQueue[5].pop();
			if(t<this->nodeId){
				IwantToJoinReply(t);
			}
		}
		break;
	}
	status = 3;
	status = 0;
	bar.unlock();
	return;
}

/**
 * A while loop function that initiate is coordinator present (or not)
 */

void invitation::keepACheckOnCoordinator(){
	srand (time(NULL));
	while(true)
	{
		//check after sleeping sometime
		sleep(2*timeout);
		if(nodeId == leader){
			continue;
		}
		if(rand()%2 == 0 && status == 0){
			//initiate check
			bar.lock();
			AreYouThereQues();
			bar.unlock();
		}
	}
}

/**
 * Recieving Function
 */

void invitation::recieveFunction(){
    int sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
      printf("Could not create socket");
    }
    srand(time(NULL));
    server.sin_family = AF_INET;
    server.sin_port = htons(itopsi[nodeId].second);
    server.sin_addr.s_addr = INADDR_ANY;
    if (::bind(sock, (struct sockaddr *) &server, sizeof(server))<0) {
      printf("Problem binding\n");
      exit(0);
    }
    listen(sock,20);
    while(1){
    	int new_fd = accept(sock, (struct sockaddr *)&cli_addr, &clilen);
        if (new_fd < 0) {
              perror("accept");
              continue;
        }
        char buffero[1000];
        int zn = read(new_fd,buffero,sizeof buffero);
        if(zn<0){
            printf("error reading message\n");
            exit(0);
        }
        string m_recieved = std::string(buffero);

        //find op first
        string op = m_recieved.substr(0,m_recieved.find('|'));
        //left
        m_recieved = m_recieved.substr(m_recieved.find('|')+1);
        string mess = m_recieved.substr(0,m_recieved.find('|'));
        int incomingId = atoi(m_recieved.substr(m_recieved.find('|')+1).c_str());

        /**
         * Simulating failure 1 in 10 chance
         */

        if(rand()%15 == 0 && !isDownVar){
        	cout<<"Simulate Failure start Messages will come but not leave"<<endl;
        	isDownTime = time(NULL);
        	status = 1;
        	isDownVar = true;
        	continue;
        }
        if(isDownVar){
        	/**
        	 * Revival
        	 */
        	if(time(NULL)-isDownTime>downTime){
        		isDownVar = false;
        		/**
        		 * Recover after failure
        		 */
        		Recovery();
        	}
        	else{
        		continue;
        	}
        }
        if(op=="0"){
        	//a incoming node is asking if this node is a coordiantor/leader
        	printf("Time:%s --> Are you a Coord message recieved by Node%d from Node%d\n",currentTime(time(NULL)),nodeId,incomingId);
        	AreYouCoordReply(incomingId);
        }
        else if(op=="1"){
        	/**
        	 * This is the reply to are you coord query!
        	 */
        	printf("Time:%s --> Are you a Coord message reply recieved by Node%d from Node%d message = %s\n",currentTime(time(NULL)),nodeId,incomingId,m_recieved.c_str());
        	if(mess == "Yes" && isSent[1][incomingId]-time(NULL)<6*timeout && status == 0){
        		/**
        		 * TODO:Handle this as another thread
        		 * As this takes time
        		 */
        		if(called[0]){
        			//if thread is called join it first
        			a[0].join();
        		}
        		a[0] = thread(&invitation::merge,this,incomingId);
        		called[0] = true;
        	}
        	else{
        		/**
        		 * Handling refusal ;-) don't worry
        		 */
        		if(isSent[1][incomingId]-time(NULL)<6*timeout){
        			printf("Time:%s --> Are you a Coord message reply recieved by Node%d from Node%d message = %s\n",currentTime(time(NULL)),nodeId,incomingId,m_recieved.c_str());
        		}
        		else{
        			printf("Time:%s --> Are you a Coord message reply recieved by Node%d from Node%d message = %s (assumed lost do not bother)\n",currentTime(time(NULL)),nodeId,incomingId,m_recieved.c_str());
        		}
        		isSent[1][incomingId] = time(NULL)-100;
        	}
        }
        else if(op=="2"){
        	/**
        	 * Send reply to the node to tell it that i am still here
        	 */
        	printf("Time:%s --> Are you there message recieved by Node%d from Node%d message = %s\n",currentTime(time(NULL)),nodeId,incomingId,m_recieved.c_str());
        	AreYouThereAns(incomingId);
        }
        else if(op=="3"){
        	/**
        	 * You have to try these operations simultaneously or oh! wait
        	 * It should be fine I think
        	 * still try to use locks with the checkInstream function
        	 */
        	if(isSent[3][incomingId]-time(NULL)>6*timeout){
        		//Message was not expected or it has taken too long for it to arrive
        		printf("Time:%s --> Are you there reply message recieved by Node%d from Node%d message = %s (assumed lost)\n",currentTime(time(NULL)),nodeId,incomingId,m_recieved.c_str());
        		continue;
        	}
        	isSent[3][incomingId] = time(NULL) - 100;
        	printf("Time:%s --> Are you there reply message recieved by Node%d from Node%d message = %s\n",currentTime(time(NULL)),nodeId,incomingId,m_recieved.c_str());
        	isRecieved[3][incomingId] = true;
        	RecievedString[3][incomingId] = mess;
        }
        else if(op=="4"){
        	printf("Time:%s --> Got infomation from Node%d to join Node%s\n",currentTime(time(NULL)),incomingId,mess.c_str());
        	if (incomingId == leader)
        	{
        		onRecieveingSendInviteMessasge(incomingId,mess);
        	}
        	else{
        		onRecieveingSendInviteMessasge(incomingId,mess);
        	}
        }
        else if(op=="5"){
        	/**
        	 * Just try to send reply
        	 */
        	printf("Time:%s --> I want to join message recieved by Node%d from Node%d message = %s\n",currentTime(time(NULL)),nodeId,incomingId,m_recieved.c_str());
        	incomingQueue[5].push(incomingId);
        	//IwantToJoinReply(incomingId);
        }
        else if(op=="6"){
        	if(isSent[6][incomingId]-time(NULL)>6*timeout){
        		//Message was not expected or it has taken too long for it to arrive ignore message
        		printf("Time:%s-->I want to join reply recieved from incomingId:Node%d message = %s (assumed lost)\n",currentTime(time(NULL)),incomingId,m_recieved.c_str());
        		continue;
        	}
        	isSent[6][incomingId] = time(NULL) - 100;
        	printf("Time:%s-->I want to join reply recieved from incomingId:Node%d message = %s\n",currentTime(time(NULL)),incomingId,m_recieved.c_str());
        	isRecieved[6][incomingId] = true;
        	RecievedString[6][incomingId] = mess;
        	leader = incomingId;
        }
        else{

        }
    }
}

void invitation::onRecieveingSendInviteMessasge(int incomingId,string message){
	if (incomingId<myId){
		//refuse
		printf("Request from node%d to join its group is refused\n",incomingId);
	}
	else if(leader == incomingId){
		if(called[2]){
			a[2].join();
		}
		a[2] = thread(&invitation::IwantToJoin,this,atoi(message.c_str()));
		called[2] = true;
	}
	else
	{
		/**
		 * Send message to everyone in the group
		 */
		string s = createMessage(to_string(incomingId),"4");
		for (int i = 0; i < group.size(); ++i)
		{
			/**
			 * Send this message to all members of group
			 * for this I will put nodeId in message
			 */
			if(group[i] == nodeId) continue;
			sendMessage(s,group[i]);
			printf("Time:%s sent message to Node%d assuming its still in my group to join Node%d\n",currentTime(time(NULL)),group[i],incomingId);
		}
		group.clear();
		/**
		 * consider no members in the group left now!
		 */

		/**
		 * Send I want to join reply after recovery mode
		 */
		if(called[3]){
			a[3].join();
		}
		a[3] = thread(&invitation::IwantToJoin,this,incomingId);
		called[3] = true;
	}
}

void invitation::keepSendingMessagesToOtherNodesToSeeIfTheyAreCoord(int N){
	/**
	 * Send message to everyone to see if they want to join group
	 * TODO:Remove the fact that that element is in the group already
	 */
	srand(time(NULL));
	while(1){
		sleep(3);
		if(isDownVar){
        	/**
        	 * Revival
        	 */
        	if(time(NULL)-isDownTime>downTime){
        		isDownVar = false;
        		/**
        		 * Recover after failure
        		 */
        		cout<<"enter Recovery State"<<endl;
        		Recovery();
        	}
        	else{
        		continue;
        	}
        }
		if(rand()%5 == 0 and nodeId == leader && status == 0)
		for (int i = nodeId-1; i > 0; i = i-1)
		{
			AreYouCoord(i);
		}
	}
}

/**
 * Simulating thread
 */
void invitation::mainFunction(int N_nodes){
	thread th[3];
	th[0] = thread(&invitation::recieveFunction,this);
	sleep(10);
	th[1] = thread(&invitation::keepACheckOnCoordinator,this);
	th[2] = thread(&invitation::keepSendingMessagesToOtherNodesToSeeIfTheyAreCoord,this,N_nodes);
	th[0].join();
	th[1].join();
	th[2].join();
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
	string s;
	int a;
	char c;
    /**
    * Ip config id - ip:port
    */
    for (int i = 0; i < N_nodes; ++i)
    {
      std::cin >> a >> c >> s;
      node.itopsi[a] = {s.substr(0,s.find(':')),atoi(s.substr(s.find(':')+1).c_str())};
    }
    /**
     * Topology of the graph
     */
    getline(cin,s);
    for (int i = 0; i < N_nodes; ++i)
    {
      getline(cin,s);
      string s2 = "";
      int id = -1;
      for (int i = 0; i < s.length(); ++i)
      {
          if(s[i] == ' ') {
              if(s2!=""){
                  if(id == -1){
                      id = atoi(s2.c_str());
                  }
                  else{
                      graph[id].push_back(atoi(s2.c_str()));
                  }
                  s2 = "";
              }
              continue;
          }
          s2+=s[i];
      }
      if(s2!=""){
          if(id == -1){
              id = atoi(s2.c_str());
          }
          else{
              graph[id].push_back(atoi(s2.c_str()));
          }
          s2 = "";
      }
    }
	//kee
	node.Recovery();
	node.mainFunction(N_nodes);
	// }
	return 0;
}
