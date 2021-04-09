#include "myheader.h"
#define BUFF_SIZE 524288
#define BUFF 10240
#define BACK 2500
enum req { view_request, accept_req, creat_user, join_g,create_g, upload_f, share_f_detail,download_f,leave_g, list_g,list_f,lin,lout,exiti};
using namespace std;


string TR1port;
unordered_map<string,string>FileId_PathMap;
string serverip;
string user_id;
string group;
vector<thread> Thread_Vector;
int threadCount;
sem_t m;
string serverport;
unordered_map<string,set<int> >AvailableChunkInfoPerFileBasis;
string TR1ip;
unordered_map<string,int>download_status;
string TR2ip;
bool islogedin=false;
string TR2port;
//functions:
vector<string>split(string s,char del);
void server_side();
void process_request(int clientdes,string ip,int port);
int connect();
void create_user(string user_id ,string pass);
void send_the_packet_vector(int clientdes,string FileId);
void get_the_particular_packet(int clientdes,string FileId,string packetNos);
void login(string user ,string pass);
void create_group(string group_id);
void join_group(string group_id);
void list_files(string group_id);
int connect_peer(string IPport);
void leave_group(string group_id);
void list_groups();
void logout();
void downloadPiece(vector<int>packetList,string IPport,string FileId,string downloadFilepath);
vector<int> query_chunk(string IPport,string FileId);
void download_file(string group_id,string FileId,string downloadFilepath);
void share_file_details(string group_id,string FileId);
req string_to_enum(string request);


vector<string>split(string s,char del){
  stringstream ss(s);
  vector<string>request;
  string temp;
  while(getline(ss,temp,del)){
    request.push_back(temp);
  }
  return request;
}
int connect(){
    int mysocketdes;
    struct sockaddr_in trakeraddr;
    if((mysocketdes=socket(AF_INET,SOCK_STREAM,0))==-1){
        perror("Cannot Obtain socket descriptor! Retry!");
        exit(1);
    }
    trakeraddr.sin_family=AF_INET;
    trakeraddr.sin_port=htons(stoi(TR1port));
    inet_pton(AF_INET,TR1ip.c_str() , &trakeraddr.sin_addr); 
    bzero(&(trakeraddr.sin_zero),8);
    if(connect(mysocketdes,(struct sockaddr *)&trakeraddr,sizeof(struct sockaddr))==-1){
        perror("Connect failed");
        exit(1);
    }
    return mysocketdes;
}
void create_user(string user_id ,string pass){
   string token="create_user";
   token+=";"+user_id+";"+pass;
   int mysocketdes;
   mysocketdes=connect();
   send(mysocketdes,token.c_str(),strlen(token.c_str()),0);
   char bit[1];
   recv(mysocketdes,bit,sizeof(bit),0);
   int status=bit[0]-'0';
   if(status==1){
   	cout<<"Username created succesfully"<<endl;
    cout<<"=============================="<<endl;
   }
   else{
   	cout<<"Username already exists!!!Please enter valid username!!"<<endl;
   	cout<<"=============================="<<endl;
   }
}  
int connect_peer(string IPport){
   vector<string>IPcred=split(IPport,':');
   int socketdes;
   struct sockaddr_in trakeraddr;
   if((socketdes=socket(AF_INET,SOCK_STREAM,0))==-1){
      perror("Failed to obtain socket descriptor");
      exit(1);
   }
   trakeraddr.sin_family=AF_INET;
   trakeraddr.sin_port=htons(stoi(IPcred[1]));
   inet_pton(AF_INET,IPcred[0].c_str() , &trakeraddr.sin_addr); 
   bzero(&(trakeraddr.sin_zero),8);
   if(connect(socketdes,(struct sockaddr *)&trakeraddr,sizeof(struct sockaddr))==-1)
   {
      perror("Connect failed");
      exit(1);
   }
   return socketdes;
}
vector<int> query_chunk(string IPport,string FileId)
{
   string token="send_the_packet_vector";
   token+=";"+FileId;
   int socketdes;
   char buffer[BUFF];
   socketdes=connect_peer(IPport);
   send(socketdes,token.c_str(),strlen(token.c_str()),0);
   recv(socketdes,buffer,sizeof(buffer),0);
   vector<string>PacketVector=split(buffer,';');
   vector<int>Packet;
   int i=0;
   while(i<PacketVector.size()){
     Packet.push_back(stoi(PacketVector[i]));
     i++;
   }
   return Packet;
}
void downloadPiece(vector<int>packetList,string IPport,string FileId,string downloadFilepath){
    cout<<"Downloading the following packetList from IPport="<<IPport<<endl;
    cout<<"printing the packetlist"<<endl;
    cout<<"=============================="<<endl;
    for(int i=0;i<packetList.size();i++){
        cout<<packetList[i]<<" ";
    }
    cout<<endl;
    int socketdes;
    char buffer[BUFF];
    struct sockaddr_in trakeraddr;
    for(int i=0;i<packetList.size();i++){
    socketdes=connect_peer(IPport);
    string token="get_the_particular_packet";
    token+=";"+FileId+";"+to_string(packetList[i]);
    send(socketdes,token.c_str(),strlen(token.c_str()),0);
    bzero(buffer,sizeof(buffer));
    int val;
    int size=BUFF_SIZE;
    
    sem_wait(&m);
    char ans[BUFF_SIZE];
    int count=0;
        int sum=0;
        fstream in;
        in.open(downloadFilepath.c_str(),ios::out|ios::in|ios::binary);        
        in.seekp(packetList[i]*BUFF_SIZE,ios::beg);
        cout<<"download file in piece in progress "<<"for chunk "<<packetList[i]<<endl;

    while((val=recv(socketdes,buffer,BUFF,0))>0&&size>0){
        in.write(buffer,val);        
        bzero(buffer,sizeof(buffer));
        sum=sum+val;
        size=size-val;
        val=0;
    }  
    in.close();
    AvailableChunkInfoPerFileBasis[FileId].insert(packetList[i]);
    sem_post(&m);    
    close(socketdes);
    }
}
void share_file_details(string group_id,string FileId){
   int socketdes=connect();
   string token="share_file_details";
   token+=";"+group_id+";"+FileId+";"+serverip+":"+serverport;
   send(socketdes,token.c_str(),strlen(token.c_str()),0);
}

void download_file(string group_id,string FileId,string downloadFilepath){
    if(group==group_id){   
    int socketdes=connect();
    string token="seeder_list";
    char buffer[BUFF];
    vector<thread>FileDownloadThread;
    token=token+";"+group_id+";"+FileId;
    send(socketdes,token.c_str(),strlen(token.c_str()),0);
    recv(socketdes,buffer,sizeof(buffer),0);
    vector<string>IPport=split(buffer,';');
    int fileSize=stoi(IPport[0]);
    cout<<"printing the seeder list "<<endl;
    int i=0;
    while(i<IPport.size()){
        cout<<IPport[i]<<" ";
        i++;
    }
    cout<<endl;
    string ss="";
    i=0;
    while(i<fileSize){
        ss+='\0';
        i++;
    }
    fstream in(downloadFilepath,ios::out|ios::binary);
    in.write(ss.c_str(),strlen(ss.c_str()));  
    in.close();
    vector<int>temp;
    int NumberOfPacket=((fileSize+(BUFF_SIZE-1))/BUFF_SIZE);
    cout<<"no of packets is "<<NumberOfPacket<<endl;   
    FileId_PathMap[FileId]=downloadFilepath;
    vector<vector<int>> ListofSeederWithChunk;
    i=1;
    while(i<IPport.size()){
        temp=query_chunk(IPport[i],FileId);
        cout<<"printing chunk details for each seeder"<<endl;
        int j=0;
        while(j<temp.size()){
            cout<<temp[j]<<" ";
            j++;
        }
        cout<<endl;
        ListofSeederWithChunk.push_back(temp);
        i++;
    }
    cout<<"Printing the List of Seeder With Chunk"<<endl;
    for(int x=0;x<ListofSeederWithChunk.size();x++){
        int j=0;
        while (j<ListofSeederWithChunk[x].size()){
        cout<<ListofSeederWithChunk[x][j]<<" ";
        j++;
        }
        cout<<endl;
    }
    cout<<endl;
    int seederCount=IPport.size()-1;
    vector<int>PacketArray;
    vector<vector<int>>seedersWithPacketMap(seederCount);

    i=0;
    while(i<NumberOfPacket){
        PacketArray.clear();
        for(int j=0;j<ListofSeederWithChunk.size();j++){
            if(ListofSeederWithChunk[j].begin(),ListofSeederWithChunk[j].end(),1){
                PacketArray.push_back(j);
            }
        }
        int randomLocation=rand()%PacketArray.size();
        seedersWithPacketMap[PacketArray[randomLocation]].push_back(i);
        i++;
    }
    cout<<"printing seeder with packet map"<<endl;
    i=0;
    while(i<seedersWithPacketMap.size()){
        for (int j=0;j<seedersWithPacketMap[i].size();j++){
        cout<<seedersWithPacketMap[i][j]<<" ";
        }
        i++;
        cout<<endl;
    }
    thread upload(share_file_details,group_id,FileId);
    upload.detach();
    i=0;
    while(i<seedersWithPacketMap.size()){
        FileDownloadThread.push_back(thread(downloadPiece,seedersWithPacketMap[i],IPport[i+1],FileId,downloadFilepath));
        i++;
    }
    for(auto it=FileDownloadThread.begin();it!=FileDownloadThread.end();it++){
        if(it->joinable()) 
            it->join();
    }

    }
    else{
    cout<<"To Download the file, Please join the group: "<<group_id<<endl;
    }
}
void login(string user ,string pass){
   string token="login";
   token+=";"+user+";"+pass;
   int mysocketdes;
   mysocketdes=connect();
   send(mysocketdes,token.c_str(),strlen(token.c_str()),0);
   char bit[1];
   recv(mysocketdes,bit,sizeof(bit),0);
   int status=bit[0]-'0';
   if(status==1){
   	islogedin=true;
   	user_id=user;
   	cout<<"Login Succesfull!!!!"<<endl;
    cout<<"=============================="<<endl;
   }
   else{
   	cout<<"Wrong Credentials! Please Try Logging in again!!"<<endl;
    cout<<"=============================="<<endl;
   }
}
void create_group(string group_id){
   string token="create_group";
   token+=";"+group_id+";"+user_id;
   int mysocketdes;
   mysocketdes=connect();
   send(mysocketdes,token.c_str(),strlen(token.c_str()),0);
   char bit[1];
   recv(mysocketdes,bit,sizeof(bit),0);
   int status=bit[0]-'0';
   if(status==1){
   	cout<<"Group created succesfully"<<endl;
    cout<<"=============================="<<endl;  
   }
   else{
   	cout<<"Group with the entered groupId already exists!! Try Again!"<<endl;
    cout<<"=============================="<<endl;
   }
}
void join_group(string group_id){
   string token="join_group";
   token+=";"+group_id+";"+user_id;
   int mysocketdes;
   mysocketdes=connect();
   send(mysocketdes,token.c_str(),strlen(token.c_str()),0);
   char bit[1];
   recv(mysocketdes,bit,sizeof(bit),0);
   int status=bit[0]-'0';
   if(status==1){
      group=group_id;
   	cout<<"succesfully joined to group"<<endl;
    cout<<"=============================="<<endl;
   }
   else{
   	cout<<"Group does not exists!! Please enter a valid Groupid!"<<endl;
    cout<<"=============================="<<endl;
   }
}
void list_files(string group_id){
   string token="list_files";
   token+=";"+group_id;
   char buffer[BUFF];
   int mysocketdes;
   mysocketdes=connect();
   send(mysocketdes,token.c_str(),strlen(token.c_str()),0);
   recv(mysocketdes,buffer,sizeof(buffer),0);
   string ans=buffer;
   vector<string>arr=split(ans,';');
   for(int i=0;i<arr.size();i++){
      cout<<arr[i]<<" ";
   }
   cout<<endl<<"=============================="<<endl;
}

void leave_group(string group_id){
   string token="join_group";
   token+=";"+group_id+";"+user_id;
   int mysocketdes;
   mysocketdes=connect();
   send(mysocketdes,token.c_str(),strlen(token.c_str()),0);
   char bit[1];
   recv(mysocketdes,bit,sizeof(bit),0);
   int status=bit[0]-'0';
   if(status==1){
      group="";
      cout<<"succesfully exit from group"<<endl;
      cout<<"=============================="<<endl;
   }
   else{
      cout<<"Group donot exits or user does not belong to the group"<<endl;
      cout<<"enter valid group again"<<endl;
      cout<<"=============================="<<endl;
   }
}
void view_requests(string group_id){
    string token="view_requests";
    token+=";"+group_id+";"+user_id;
    char buffer[BUFF];
    int mysocketdes=connect();
    send(mysocketdes,token.c_str(),strlen(token.c_str()),0);
    recv(mysocketdes,buffer,sizeof(buffer),0);
    string buff=(buffer);
    vector<string>requests=split(buff,';');
    for(auto i:requests)
        cout<<i<<endl;
    cout<<"============================="<<endl;
}
void accept_request(string group_id,string client_id){
    string token="accept_request";
    token+=";"+group_id+";"+user_id+";"+client_id;
    int mysocketdes=connect();
    send(mysocketdes,token.c_str(),strlen(token.c_str()),0);
    char bit[1];
    recv(mysocketdes,bit,sizeof(bit),0);
    int status=bit[0]-'0';
    if(status==1){
      group="";
      cout<<"Member Added to the group Successfully!!!"<<endl;
      cout<<"=============================="<<endl;
    }
    else if(status==2){
      cout<<"No request of joining from the mentioned user_id!!"<<endl;
      cout<<"=============================="<<endl;
    }
    else{
        cout<<"Access Denied!!! You are not an Admin!!"<<endl;
        cout<<"=============================="<<endl;
    }

}
void list_groups(){
   string token="list_groups";
   char buffer[BUFF];
   int mysocketdes;
   mysocketdes=connect();
   send(mysocketdes,token.c_str(),strlen(token.c_str()),0);
   recv(mysocketdes,buffer,sizeof(buffer),0);
   string buff=(buffer);
   vector<string>grouplist=split(buff,';');
   for(auto i : grouplist)
   	    cout<<i<<endl;
   cout<<"=============================="<<endl;
}
void logout(){
   string token="logout";
   token+=";"+serverip+";"+serverport+";"+group;
   int mysocketdes;
   mysocketdes=connect();
   send(mysocketdes,token.c_str(),strlen(token.c_str()),0);
   char ss[1];
   recv(mysocketdes,ss,sizeof(ss),0);
   int status=ss[0]-'0';
   if(status==1){
      islogedin=false;
      user_id="";
      cout<<user_id<<endl;
      cout<<"Successfully logged out!!!"<<endl;
      cout<<"=============================="<<endl;
   }
   else{
      cout<<"Failed to logout!"<<endl;
      cout<<"=============================="<<endl;
   }    
}
void upload_file(string group_id,string FileId)
{
   if(group==group_id){
   int socketdes;
   int newsocketdes;
   char buffer[BUFF];
   struct sockaddr_in trakeraddr;
   if((socketdes=socket(AF_INET,SOCK_STREAM,0))==-1)
   {
      perror("Failed to obtain socket descriptor");
      exit(1);
   }
   trakeraddr.sin_family=AF_INET;
   trakeraddr.sin_port=htons(stoi(TR1port));
   inet_pton(AF_INET,TR1ip.c_str() , &trakeraddr.sin_addr); 
   bzero(&(trakeraddr.sin_zero),8);
   if(connect(socketdes,(struct sockaddr *)&trakeraddr,sizeof(struct sockaddr))==-1){
      perror("Connection failed");
      exit(1);
   }
   FileId_PathMap[FileId]=FileId;
   string token="upload_file";
   token+=";";
   token+=group_id;
   token+=";";
   token+=FileId;
   token+=";";
   token+=serverip;
   token+=":";
   token+=serverport;
   cout<<token<<endl;
   send(socketdes,token.c_str(),strlen(token.c_str()),0);
   bzero(buffer,sizeof(buffer));
   recv(socketdes,buffer,sizeof(buffer),0);
   string jk=buffer;
   int size=stoi(jk);
   int no_of_chunnk=((size+(BUFF_SIZE-1))/BUFF_SIZE);
   cout<<"no_of_chunnk="<<no_of_chunnk<<endl;
   for(int i=0;i<no_of_chunnk;i++)
   {
      AvailableChunkInfoPerFileBasis[FileId].insert(i);
   }
   
 }
 else{
   cout<<"Error you have to join "<<group_id<<"to upload the file"<<endl;
 }
     cout<<"==========================";
}
//=====================SERVER SIDE=======================
void send_the_packet_vector(int clientdes,string FileId){
   string chunkdetails="";
   for(auto it=AvailableChunkInfoPerFileBasis[FileId].begin();it!=AvailableChunkInfoPerFileBasis[FileId].end();it++){
     if(it==AvailableChunkInfoPerFileBasis[FileId].begin()){
       chunkdetails+=to_string(*it);
       it++;
     }
     chunkdetails+=";";
     chunkdetails+=to_string(*it);
    }
   send(clientdes,chunkdetails.c_str(),strlen(chunkdetails.c_str()),0);
   close(clientdes);
}
void get_the_particular_packet(int clientdes,string FileId,string packetNos){
   string Filepath=FileId_PathMap[FileId];
   FILE *fp=fopen(Filepath.c_str(),"rb");
   int val;
   char buffer[BUFF];
   bzero(buffer,BUFF);
   vector<string>ArrayofPacket=split(packetNos,';');
   vector<int>ArrayofPacket_int;
   for(int i=0;i<ArrayofPacket.size();i++){
      ArrayofPacket_int.push_back(stoi(ArrayofPacket[i]));
   }
   for(int i=0;i<ArrayofPacket_int.size();i++){
      fseek(fp,ArrayofPacket_int[i]*BUFF_SIZE,SEEK_SET);
      int size=BUFF_SIZE;
      while((val=fread(buffer,sizeof(char),BUFF,fp))>0&&size>0){
         send(clientdes,buffer,val,0);
         memset ( buffer , '\0', sizeof(buffer));
         size=size-val;
      }
      bzero(buffer,BUFF);
     }
   fclose(fp);
   close(clientdes);
}
void process_request(int clientdes,string ip,int port){
    while(1){
        char buffer[BUFF];
        bzero(buffer,BUFF);
        read(clientdes,buffer,sizeof(buffer));
        string r=buffer;
        vector<string> requestarray=split(r,';');
        string request=requestarray[0];
        if(request=="send_the_packet_vector"){
            string FileId=requestarray[1];
            send_the_packet_vector(clientdes,FileId);
            break;
        }
        else if(request=="get_the_particular_packet"){
            string FileId=requestarray[1];
            string packetNos=requestarray[2];
            get_the_particular_packet(clientdes,FileId,packetNos);
            break;
        }
        else{
            cout<<"Anonymous Request Recieved!! Please Try Again!!"<<endl;
        }

    }
}
void server_side(){
    int serverdes;
    int clientdes;
    int val;
    socklen_t size;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    if((serverdes=socket(AF_INET,SOCK_STREAM,0))<0){
        perror("Cannot Obtain socket descriptor! Retry!");
        exit(1);
    } 
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(stoi(serverport));
    inet_pton(AF_INET,serverip.c_str(),&server_addr.sin_addr);
    bzero(&(server_addr.sin_zero),8);
    if(bind(serverdes,(struct sockaddr *)&server_addr,sizeof(struct sockaddr))==-1){
        perror("Failed to obtain Bind!!");
        exit(1);
    }
    if(listen(serverdes,BACK)==-1){
        perror("error backlog overflow");
        exit(1);
    }
    size=sizeof(struct sockaddr);
    while((clientdes=accept(serverdes,(struct sockaddr *)&client_addr,&size))!=-1){
        string ip=inet_ntoa(client_addr.sin_addr);
        int port=(ntohs(client_addr.sin_port));
        Thread_Vector.push_back(thread(process_request,clientdes,ip,port));
        size=sizeof(struct sockaddr);
    }
    for(auto it=Thread_Vector.begin();it!=Thread_Vector.end();it++){
        if(it->joinable()) //to ensure whether every process has succesfully completed before exiting!!!
            it->join();
    }
    cout<<"Returning from server "<<endl;
     
}
req string_to_enum(string request){
    if(request=="list_requests")return view_request;
    if(request=="accept_request")return accept_req;
    if(request=="create_user")return creat_user;
    if(request=="join_group")return join_g;
    if(request=="create_group")return create_g;
    if(request=="upload_file")return upload_f;
    if(request=="share_file_details")return share_f_detail;
    if(request=="download_file")return download_f;
    if(request=="leave_group")return leave_g;
    if(request=="list_groups")return list_g;
    if(request=="list_files")return list_f;
    if(request=="login")return lin;
    if(request=="logout")return lout;
    if(request=="exit")return exiti;
}
int main(int argc,char** argv){
    sem_init(&m,0,1);
    if(argc!=3){
        cout<<"Please Enter command arguments in the Format IP:PORT tracker_info.txt"<<endl;
        perror("Missing Arguments!!");
        return -1;
    }
    string client_IP_PORT=argv[1];
    string tracker_info_path=argv[2];
    vector<string>IP_PORT=split(client_IP_PORT,':');
    serverip =IP_PORT[0];
    serverport=IP_PORT[1];
    fstream serverfile(tracker_info_path,ios::in);
    vector<string>IP_PORT_TRACKER;
    string temp;
    while(getline(serverfile,temp,'\n')){
        IP_PORT_TRACKER.push_back(temp);
    }
    IP_PORT=split(IP_PORT_TRACKER[0],':');
    TR1ip=IP_PORT[0];
    TR1port=IP_PORT[1];
    IP_PORT=split(IP_PORT_TRACKER[1],':');
    TR2ip=IP_PORT[0];
    TR2port=IP_PORT[1];
    cout<<endl<<endl <<"*************PEER**************"<<endl;
    thread serverthread(server_side);
    serverthread.detach();
    string request;
    while(1){
        cout<<"==================================="<<endl; //enum req { view_request, accept_req, creat_user, join_g,create_g, upload_f, share_f_detail,seeder_l,leave_g, list_g,list_f,lin,lout};
        getline(cin,request);
        cout<<request<<endl;
        vector<string>Requests=split(request,' ');
        string requ=Requests[0];
        req command = string_to_enum(requ);
        switch(command){
        case creat_user:
            if(Requests.size()!=3){
                cout<<"Retry! Please Provide valid Arguments!"<<endl;
            }            
            else{
                Thread_Vector.push_back(thread(create_user,Requests[1],Requests[2]));
            }
            break;
        case create_g:
            if(!islogedin){
                cout<<"Please login first to continue!!!"<<endl;
            }
            else if(Requests.size()!=2){
                cout<<"Error! Missing valid Arguments!"<<endl;
            }
            else{
                Thread_Vector.push_back(thread(create_group,Requests[1]));
            }
            break;
        case lin:
            if(Requests.size()!=3){
                cout<<"Error! Missing or Invalid number of arguments!"<<endl;
            }
            else{
                Thread_Vector.push_back(thread(login,Requests[1],Requests[2]));
            }
            break;
         case join_g:
            if(!islogedin){
                cout<<"Please Login to continue!!!"<<endl;
            }
            if(Requests.size()!=2){
               cout<<"Error! Missing or invalid no. of arguments!!!"<<endl;
            }
            else{
               Thread_Vector.push_back(thread(join_group,Requests[1]));
            }
            break;
         case leave_g:
            if(!islogedin){
                cout<<"Please Login to continue!!!"<<endl;
            }
            if(Requests.size()!=2){
               cout<<"Error! Missing or invalid no. of arguments"<<endl;
            }
            else{
               Thread_Vector.push_back(thread(leave_group,Requests[1]));
            }
            break;
         case list_g:
          if(!islogedin){
            cout<<"Please Login to Continue!!!"<<endl;
          }
          else if(Requests.size()!=1){
                cout<<"Error!! Invalid Number of Arguments!"<<endl;
            }
          else {
            Thread_Vector.push_back(thread(list_groups));
            }
          break;
         case list_f:
          if(!islogedin){
         cout<<"Please Login to Continue!!!"<<endl;
          }
          else{
            if(Requests.size()!=2){
               cout<<"Error!! Invalid Number of Arguments"<<endl;
            }
            else{
               Thread_Vector.push_back(thread(list_files,Requests[1]));
            }
          }
        break;
        case view_request:
          if(!islogedin){
              cout<<"Please Login to Continue!!!"<<endl;
          }
          else{
              if(Requests.size()!=2){
                  cout<<"Error!! Invalid Number of Arguments"<<endl;
              }
              else{
                  Thread_Vector.push_back(thread(view_requests,Requests[1]));
              }
          }
        break;
      case accept_req:
          if(!islogedin){
              cout<<"Please Login to Continue!!!"<<endl;
          }
          else{
              if(Requests.size()!=3){
                  cout<<"Error!! Invalid Number of Arguments"<<endl;
              }
              else{
                  Thread_Vector.push_back(thread(accept_request,Requests[1],Requests[2]));
              }
          }
        break;
      case upload_f:
           if(!islogedin){
            cout<<"Please Login to Continue!!!"<<endl;
          }
           else if(Requests.size()!=3){
               cout<<"Error!!! the valid argument"<<endl;
            }
            else{
               string group_id=Requests[2];//change the order!
               string FileId=Requests[1];
               Thread_Vector.push_back(thread(upload_file,group_id,FileId));
            }
        break;
      case download_f:
            if(!islogedin){
              cout<<"Please Login to Continue!!!"<<endl;
            }
            else if(Requests.size()!=4)
            {
               cout<<"Error!!! the valid argument"<<endl;
            }
            else {
               string group_id=Requests[1];
               string FileId=Requests[2];
               string Filepath=Requests[3];
          Thread_Vector.push_back(thread(download_file,group_id,FileId,Filepath));
            }
        break;
      case lout:
         if(!islogedin){
            cout<<"Please Login to Continue!!!"<<endl;
         }
        else if(Requests.size()!=1){
               cout<<"Error!!! the valid argument"<<endl;
         }
            else
            {
               Thread_Vector.push_back(thread(logout));
            }
        break;
        case exiti:
            cout<<"Good bye"<<endl;
            return 0; 
      }     
    }
 	return 0;
    }

