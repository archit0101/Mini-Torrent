#include <stdlib.h>
#include <stdio.h>
#include<bits/stdc++.h>
#include <ctype.h>          
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include<math.h>
#include<cstring>
#include <unistd.h>
#include <openssl/sha.h>
#include <thread>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#define BUFF_SIZE 10240
#define BACK 2500
using namespace std;
string TR1ip;
string TR2ip;
string TR1port;
string TR2port;
vector<thread> threadVector;
int threadCount;
enum req { view_request, accept_req, creat_user, join_g,create_g, upload_f, share_f_detail,seeder_l,leave_g, list_g,list_f,lin,lout};
map<string,string>userReg;
map<string,set<string> >GroupInfo;
map<pair<string,string>,set<string> >seederlist;
map<pair<string,string>,int > FileSizemap;
map<string,string>FileMap;
map<string,set<string> >GroupAndFile;
map<string,string>groupIdandAdmin;
map<pair<string,string>,set<string>>requests;
vector<string>split(string s,char del);
void  create_user(int newsocketdes,string user_id,string pass);
void  create_group(int newsocketdes,string group_id,string user_id);
void view_requests(int clientdes,string group_id,string user_id);
void  list_files(int newsocketdes,string group_id);
void  join_group(int newsocketdes,string group_id,string user_id);
void accept_request(int mysocketdes,string group_id,string admin_id,string user_id);
void  leave_group(int newsocketdes,string group_id,string user_id);
void  list_groups(int newsocketdes);
void  login(int newsocketdes,string user_id,string pass,string IP_Port);
void  logout(int newsocketdes,string IPport,string group_id);
void upload_file(int newsocketdes,string group_id,string FileId,string IPport);
void share_file_details(int newsocketdes,string group_id,string FileId,string IPport);
void seeder_list(int newsocketdes,string group_id,string FileId);
void Request(int newsocketdes,vector<string> &requestarray);
void serverequest(int newsocketdes,string ip,int port);
int connect();
vector<string>split(string s,char del){
  stringstream ss(s);
  vector<string>a;
  string temp;
  while(getline(ss,temp,del)){
    a.push_back(temp);
  }
  return a;
}

void  create_user(int newsocketdes,string user_id,string pass){
	if(userReg.find(user_id)!=userReg.end()){
	  char status[]="0";
	  send(newsocketdes,status,sizeof(status),0);
	}
	else{
		char status[]="1";
		userReg[user_id]=pass;
	    send(newsocketdes,status,sizeof(status),0);

	}
}

void view_requests(int clientdes,string group_id,string user_id){
    if(requests.find({group_id,user_id})!=requests.end()){
        set<string>s=requests[{group_id,user_id}];
        string pending_requests = "";
        for(auto it=s.begin();it!=s.end();it++){
        if(it==s.begin())
        {
            pending_requests+=(*it);
        }
        else{
            pending_requests+=";";
            pending_requests+=(*it);
        }
	}
	send(clientdes,pending_requests.c_str(),strlen(pending_requests.c_str()),0);
	close(clientdes);
    }
}
void  list_files(int newsocketdes,string group_id){
	set<string>s=GroupAndFile[group_id];
	set<string>:: iterator it;
	string ans="";
	for(it=s.begin();it!=s.end();it++){
       if(it==s.begin()){
       	ans+=(*it);
       }
       else{
       	ans+=";";
       	ans+=(*it);
       }
	}
	send(newsocketdes,ans.c_str(),strlen(ans.c_str()),0);
	close(newsocketdes);
}
void  join_group(int newsocketdes,string group_id,string user_id){
	if(GroupInfo.find(group_id)!=GroupInfo.end()){
      string admin=groupIdandAdmin[group_id];
      requests[{group_id,admin}].insert(user_id);
	  char  status[]="1";
	  GroupInfo[group_id].insert(user_id);
	  send(newsocketdes,status,sizeof(status),0);
	}
	else{
		char  status[]="0";
	    send(newsocketdes,status,sizeof(status),0);
	}
	close(newsocketdes);
}
void accept_request(int mysocketdes,string group_id,string admin_id,string user_id){
    string admin=groupIdandAdmin[group_id];
    if(admin==admin_id){
        if(requests[{group_id,admin}].find(user_id)!=requests[{group_id,admin}].end()){
            GroupInfo[group_id].insert(user_id);
            requests[{group_id,admin}].erase(user_id);
            char status[]="1";
            send(mysocketdes,status,sizeof(status),0);
        }
        else{
            char status[]="2";
            send(mysocketdes,status,sizeof(status),0);
        }
    }
    else{
        char status[]="0";
        send(mysocketdes,status,sizeof(status),0);
    }
    close(mysocketdes);
}
void  leave_group(int newsocketdes,string group_id,string user_id){
	if(GroupInfo.find(group_id)!=GroupInfo.end()){
	  if(GroupInfo[group_id].find(user_id)!=GroupInfo[group_id].end()){
	  char  status[]="1";
	  GroupInfo[group_id].erase(user_id);
	  send(newsocketdes,status,sizeof(status),0);
	 }
	else{
		char  status[]="0";
	    send(newsocketdes,status,sizeof(status),0);
	}
    }
    else{
		char  status[]="0";
	    send(newsocketdes,status,sizeof(status),0);
	}
	close(newsocketdes);
}
void  create_group(int newsocketdes,string group_id,string user_id){
	if(GroupInfo.find(group_id)!=GroupInfo.end()){
	  char status[]="0";
	  send(newsocketdes,status,sizeof(status),0);
	}
	else{
		char status[]="1";
		groupIdandAdmin[group_id]=user_id;
        GroupInfo[group_id].insert(user_id);
        requests[{group_id,user_id}].insert("");
	    send(newsocketdes,status,sizeof(status),0);
    }
	close(newsocketdes);
}
int connect(){
    int socketdes;
 	int newsocketdes;
 	int val;
 	socklen_t size;
 	struct sockaddr_in myaddr;
 	struct sockaddr_in otheraddr;
 	if((socketdes=socket(AF_INET,SOCK_STREAM,0))<0){
 		perror("Failed to obtained the socket descriptor");
        return -1;
	}
    myaddr.sin_family=AF_INET;
    myaddr.sin_port=htons(stoi(TR1port));
    inet_pton(AF_INET,TR1ip.c_str() , &myaddr.sin_addr); 

    bzero(&(myaddr.sin_zero),8);

    if(bind(socketdes,(struct sockaddr *)&myaddr,sizeof(struct sockaddr))==-1){
        perror("Failed to obtain bind");
        return -1;
    }
    if(listen(socketdes,BACK)==-1){
        perror("Error!! backlog overflow");
        return -1;

    }
    size=sizeof(struct sockaddr);
    while((newsocketdes=accept(socketdes,(struct sockaddr *)&otheraddr,&size))!=-1){
        string ip=inet_ntoa(otheraddr.sin_addr);
        int port=(ntohs(otheraddr.sin_port));
        cout<<"ip="<<ip<<"port"<<port<<endl;
        threadVector.push_back(thread(serverequest,newsocketdes,ip,port));
        size=sizeof(struct sockaddr);
    }
    vector<thread>:: iterator it;
    for(it=threadVector.begin();it!=threadVector.end();it++){
            if(it->joinable()) 
                it->join();
        }
    return 0;
}
void  list_groups(int newsocketdes){
	string info="";
	for(auto &i: GroupInfo){
      info+=i.first;
      info+=";";
	}
	send(newsocketdes,info.c_str(),strlen(info.c_str()),0);
	close(newsocketdes);
}
void  login(int newsocketdes,string user_id,string pass,string IP_Port)
{
	if(userReg.find(user_id)!=userReg.end()&&(userReg[user_id]==pass)){
	  char  status[]="1";
	  
	  send(newsocketdes,status,sizeof(status),0);
	}
	else{
		char  status[]="0";
	    send(newsocketdes,status,sizeof(status),0);

	}
	close(newsocketdes);
	
}


void  logout(int newsocketdes,string IPport,string group_id){
	bool flag=false;
    for(auto it=seederlist.begin();it!=seederlist.end();it++){
    	if(it->second.find(IPport)!=it->second.end()){
    		flag=true;
    		it->second.erase(IPport);
    	}
    }

	if(flag){
	  char  status[]="1";
	  
	  send(newsocketdes,status,sizeof(status),0);
	}
	else{
		char  status[]="0";
	    send(newsocketdes,status,sizeof(status),0);

	}
	close(newsocketdes);
}
void share_file_details(int newsocketdes,string group_id,string FileId,string IPport){
    pair<string,string>s;
	s=make_pair(group_id,FileId);
	seederlist[s].insert(IPport);
	close(newsocketdes);	
}
void seeder_list(int newsocketdes,string group_id,string FileId){
	pair<string,string>s;
	s=make_pair(group_id,FileId);
	set<string>ans;
	ans=seederlist[s];
	int ansb=FileSizemap[s];
	string token=to_string(ansb);
	set<string>::iterator i;
	for(i=ans.begin();i!=ans.end();i++){
      token+=";";
      token+=(*i);
	}
	send(newsocketdes,token.c_str(),strlen(token.c_str()),0);
	close(newsocketdes);
}
req string_to_enum(string request){
    if(request=="view_requests")return view_request;
    if(request=="accept_request")return accept_req;
    if(request=="create_user")return creat_user;
    if(request=="join_group")return join_g;
    if(request=="create_group")return create_g;
    if(request=="upload_file")return upload_f;
    if(request=="share_file_details")return share_f_detail;
    if(request=="seeder_list")return seeder_l;
    if(request=="leave_group")return leave_g;
    if(request=="list_groups")return list_g;
    if(request=="list_files")return list_f;
    if(request=="login")return lin;
    if(request=="logout")return lout;
}
void upload_file(int newsocketdes,string group_id,string FileId,string IPport){
	pair<string,string>s;
	ifstream in(FileId,ios::ate|ios::binary);
	int size=in.tellg();
	in.close();

	s=make_pair(group_id,FileId);
	FileSizemap[s]=(size);
	FileMap[FileId]=FileId;
	seederlist[s].insert(IPport);
	set<string>ans;
	ans=seederlist[s];
	set<string>::iterator i;
	for(i=ans.begin();i!=ans.end();i++)
	{
      cout<<(*i)<<endl;
	}
	string token="";
	token+=to_string(size);

	GroupAndFile[group_id].insert(FileId);
	send(newsocketdes,token.c_str(),strlen(token.c_str()),0);
	close(newsocketdes);
	goto l2;
	l2:
	  cout<<"";
}
void Request(int newsocketdes,vector<string> &requestarray){
    req request=string_to_enum(requestarray[0]);
    string group_id,user_id,admin_id,pass,FileId,IPport;
    switch (request){
    case view_request:
        group_id=requestarray[1];
        user_id=requestarray[2];
        view_requests(newsocketdes,group_id,user_id);
        break;
    
    case accept_req:
        group_id=requestarray[1];
        admin_id=requestarray[2];
        user_id=requestarray[3];
        accept_request(newsocketdes,group_id,admin_id,user_id);
        break;
    
	case creat_user:
	   user_id=requestarray[1];
	   pass=requestarray[2];
       create_user(newsocketdes,user_id,pass);
       break;
	
	case join_g:
      group_id=requestarray[1];
      user_id=requestarray[2];
      join_group(newsocketdes,group_id,user_id);
      break;
	case create_g:
      group_id=requestarray[1];
      user_id=requestarray[2];
      create_group(newsocketdes,group_id,user_id);
	  break;
	case upload_f:
      group_id=requestarray[1];
      FileId=requestarray[2];
      IPport=requestarray[3];
      upload_file(newsocketdes,group_id,FileId,IPport);
      break;
	case share_f_detail:
      group_id=requestarray[1];
      FileId=requestarray[2];
      IPport=requestarray[3];
      share_file_details(newsocketdes,group_id,FileId,IPport);
      break;
	case seeder_l:
      group_id=requestarray[1];
      FileId=requestarray[2];
      seeder_list(newsocketdes,group_id,FileId);
	  break;
	case leave_g:
       group_id=requestarray[1];
       user_id=requestarray[2];
      leave_group(newsocketdes,group_id,user_id);
	   break;
	case list_g:
		list_groups(newsocketdes);
        break;
	case list_f:
      group_id=requestarray[1];
      list_files(newsocketdes,group_id);
	  break;
	case lin:
       user_id=requestarray[1];
       pass=requestarray[2];
       IPport=requestarray[3];
       login(newsocketdes,user_id,pass,IPport);
       break;
	case lout:
       IPport=requestarray[1];
       group_id=requestarray[2];
       logout(newsocketdes,IPport,group_id);
    }
}

void serverequest(int newsocketdes,string ip,int port)
{
	char buffer[BUFF_SIZE];
	bzero(buffer,BUFF_SIZE);
	read(newsocketdes,buffer,BUFF_SIZE);

	string r=buffer;
	vector<string> requestarray=split(r,';');
    Request(newsocketdes,requestarray);
	cout<<"Response sent to the client!!!"<<endl;
}

int main(int argc,char ** argv){
	
	if(argc!=3){
    	cout<<"Oops! Please give command line argument in the format"<<endl;
		perror("Error in command line argument list ");
		return -1;
	}
    string traker_info_path=argv[1];
    int tracker_no=stoi(argv[2]);
    cout<<"tracker_no="<<tracker_no<<endl;
    char buffer[BUFF_SIZE];

   
    fstream serverfilestream(traker_info_path,ios::in);
    vector<string>IPortTrakers;
    string temp;
    while(getline(serverfilestream,temp,'\n')){
    	IPortTrakers.push_back(temp);
    }
    vector<string>IPort;
    IPort=split(IPortTrakers[0],':');
    TR1ip=IPort[0];
 	TR1port=IPort[1];
 	IPort=split(IPortTrakers[1],':');
 	TR2ip=IPort[0];
 	TR2port=IPort[1];
 	
 	int x = connect();
	cout<<"Returning form Tracker main"<<endl;
    return x;
}