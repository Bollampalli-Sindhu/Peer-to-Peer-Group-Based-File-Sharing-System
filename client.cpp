#include "headers.h"
#include<bits/stdc++.h>
#include<openssl/sha.h>
#define MSG_SIZE 100000
#define CHUNK_SIZE 20000
pthread_mutex_t lock1;
using namespace std;
struct information{
    string filename;
    string port_num;
};
struct user *user1=(struct user*)malloc(sizeof(struct user));
struct file_download *download_info= new file_download;
void error(const char *msg)
{
    cout<<msg<<endl;
    exit(1);
}
// bool is_file_with_user(string filename){
//     string userid=user1->username;
//     if(user_files_info.find(userid)==user_files_info.end())
//         return false;
    

// }
// string sha1(string msg){

// }
string calculate_sha(string file){
    FILE *fd = fopen(file.c_str(),"r");
    if(!fd){
        perror("error in opening file");
        exit(1);
    }
    unsigned char buffer[CHUNK_SIZE];
    string file_hash;
    int n;
    unsigned char hash[SHA_DIGEST_LENGTH];
  
        while((n=fread((void*)buffer,1,CHUNK_SIZE,fd))>0){
            SHA1(buffer,n, hash);
            ostringstream oss;
            for(int i = 0; i < SHA_DIGEST_LENGTH; ++i) 
                oss << std::hex << std::setw(2) << std::setfill('0') << +hash[i];

            //string str =oss.str();
            file_hash +=oss.str()+"/";
        }
        //cout<<file_hash<<endl;
    
    return file_hash;

}

string get_filehash(string hash){
    vector<string>res=split(hash,"/");
    string file_hash="";
    for(int j=0;j<res.size()-1;j++){
       //cout<<res[j]<<endl;
       file_hash+=res[j].substr(0,20);
    }
    return file_hash;
}

long get_filesize(string file)
{
    long res;
    FILE *fd=fopen(file.c_str(),"rb");
    fseek(fd,0,SEEK_END);
    res=ftell(fd);
	fclose(fd);

    return res;
}

void* Download(void* arg){
    struct information *temp=(struct information*)arg;
    cout<<temp->filename<<" is being downloaded from "<<temp->port_num<<endl;
    int port = atoi(temp->port_num.c_str());
    string ip_address="127.0.0.1";
    
    if(user1->port!=to_string(port)){
        
        struct user_file *file_str=user_files_info[user1->username][temp->filename];
        vector<string>sha_vector=file_str->chunk_sha;
        string dest_file=file_str->full_path;//cout<<"destination file: "<<dest_file<<endl;
        //long file_size=file_str->filesize;
        //cout<<port;
        //vector<int>bitvector(10);

        /*connect to the peer*/                                                                    
        int sockfd,n,flag=0;
        struct sockaddr_in serv_addr;
        struct hostent *server;
        char buffer[CHUNK_SIZE];
        if((server = gethostbyname(ip_address.c_str()))==NULL)
            error("Error,No such host");
        if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
            error("error opening socket");
        bzero((char*)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char*)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
        serv_addr.sin_port = htons(port);
        if(connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
            error("Error Connecting");
        cout<<"connected to port "<<port<<endl;

        /*send the filename which you want to download*/
        string str=temp->filename;
        send(sockfd,str.c_str(),strlen(str.c_str()),0);

        /*recieve chunk info of the peer*/
        bzero(buffer,sizeof(buffer));
        recv(sockfd,buffer,sizeof(buffer),0);
        str=buffer;
        
        if(str!="no")
        {
            vector<string>vec=split(buffer,"/");
            int chunks[vec.size()];
            
            for(int i=1;i<vec.size();i++){
                chunks[i]=atoi(vec[i].c_str());
                //cout<<"Available chunks with peer "<<port<<chunks[i]<<" ";
                cout<<chunks[i]<<" ";
            }
            cout<<endl;
            FILE* fd;
            //fd=fopen(dest_file.c_str(),"wb");
            //ifstream f(dest_file);
            if((fd=fopen(dest_file.c_str(),"wb"))<0){
                cout<<"file "<<dest_file<<" doesn't exist\n"<<endl;
                str="E";
                send(sockfd,str.c_str(),strlen(str.c_str()),0);
            }
            
            else{
                for(int i=1;i<vec.size();i++){
                    if(file_str->bit_vector[chunks[i]]==1){
                        //already chunk recieved
                        str="N";
                        send(sockfd,str.c_str(),strlen(str.c_str()),0);

                        /*read dummy data*/
                        //bzero(buffer,sizeof(buffer));
                        recv(sockfd,buffer,sizeof(buffer),0);
                    }
                    else{
                        pthread_mutex_lock(&lock1); 
                        if(file_str->bit_vector[chunks[i]]==0){
                            file_str->bit_vector[chunks[i]]=1;
                            flag=1;
                        }
                        pthread_mutex_unlock(&lock1);

                        if(flag){
                            //ask for the required chunk
                        str=to_string(chunks[i]);
                        //cout<<"Asking for chunk_"<<str<<" from "<<port<<endl;
                        send(sockfd,str.c_str(),strlen(str.c_str()),0);

                        /*recieve the chunk*/
                        bzero(buffer,sizeof(buffer));
                        n=recv(sockfd,buffer,CHUNK_SIZE,0);
                        
                        str=buffer;
                        // /*write into the destination file*/
                        cout<<" writing chunk_"<<chunks[i]<<" to "<<dest_file<<" from "<<port<<endl;
                        fseek(fd,(chunks[i])*CHUNK_SIZE,SEEK_SET);
                        //fwrite(buffer,sizeof(char),n,fd);
                        fwrite(str.c_str(),sizeof(char),strlen(str.c_str()),fd);
                        
                        /*update bitvector*/
                        //file_str->bit_vector[chunks[i]]=1;
                        }
                        else{
                            str="N";
                            send(sockfd,str.c_str(),strlen(str.c_str()),0);

                            /*read dummy data*/
                            //bzero(buffer,sizeof(buffer));
                            recv(sockfd,buffer,sizeof(buffer),0);
                        }

                    }
                }
                str="E";
                send(sockfd,str.c_str(),strlen(str.c_str()),0);
            }
            
            fclose(fd);
            //f.close();
        }
    close(sockfd);
    cout<<"Done downloading chunks from "<<port<<endl<<endl;
    }
    return NULL;
}

void* handleConnection(void* arg){
    int new_socket= *((int*)arg),valread;
        char buffer[MSG_SIZE] = {0}; 
        char *hello=new char [MSG_SIZE]; 
        /*get the file name*/
        bzero(buffer,MSG_SIZE);
        if((valread = read( new_socket , buffer, MSG_SIZE))<0)
            error("error in reading");
        string str=buffer;
        struct user_file *file_str=user_files_info[user1->username][str];
        cout<<"file being requested is "<<file_str->filename<<endl;
        int *arr=file_str->bit_vector;
        int n=file_str->number_chunks,size;
        str="";
        vector<int>chunk;
        //cout<<"number of chunks: "<<n<<endl;
        
        for(int i=0;i<n;i++){
            if(arr[i]==1){
                chunk.push_back(i);
                //str+="/"+to_string(i);
            }
                
        }
        cout<<endl;
        if(chunk.size()>0){
            random_shuffle(chunk.begin(), chunk.end());
            for(int i=0;i<chunk.size();i++){
                cout<<chunk[i]<<" ";
                str+="/"+to_string(chunk[i]);
            }
            cout<<endl;
            FILE *fd=fopen(file_str->full_path.c_str(),"rb");
            bzero(buffer,MSG_SIZE);
            send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
            while(1){
                recv(new_socket,buffer,1,0);
                str=buffer;
                if(str=="N"){
                    /*Dont need any chunk. Send dummy data*/
                    send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
                    continue;
                }
                if(str=="E"){
                    break;
                }
                else{
                   /*send the given chunk*/
                    int i=atoi(str.c_str());
                    //cout<<"requested chunk is: "<<i<<endl;
                    fseek(fd,i*CHUNK_SIZE,SEEK_SET);
                    bzero(buffer,MSG_SIZE);
                    size=fread(buffer,sizeof(char),CHUNK_SIZE,fd);
                    //cout<<"Chunk_"<<i<<" is read from source file"<<endl;
                    str=buffer;
                    //buffer[size]='\0';
                    send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
                    //send(new_socket , buffer , size , 0 );
                    bzero(buffer,MSG_SIZE);
                    //cout<<"Chunk_"<<i<<" is sent to dest file"<<endl;
                }
            }
        }
        else{
            str="no";
            send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
        }
        
        close(new_socket);
        
        delete [] hello;
    return NULL;
}

void* server_thread(void *arg)
{
    int port=*(int *)arg;
    int server_fd, new_socket, valread,n=10; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    char buffer[MSG_SIZE]={0};
    pthread_t tid;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
        error("socket failed");
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
        error("setsockopt failed");   

    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) 
        error("bind failed"); 
    if (listen(server_fd,n) < 0) //socket can connect to 10 clients at a time
        error("listen failed"); 

    while(1){
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) 
            error("accept"); 
             
            pthread_create(&tid,NULL,handleConnection,&new_socket);
    } 
    pthread_join(tid,NULL);
    close(server_fd);
    return NULL;
} 
int main(int argc, char *argv[])
{
    int sockfd,n,portno,cli_port;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    string line,ip_address,cli_ipAddr;
    char buffer[MSG_SIZE];
    pthread_t tid;
    string mesg;
    
    //struct user_file *temp1;
    user1->port="NULL";
    if(argc=!3) 
        error("execute the program this way --> /a.out portno <tracker_file_name>\n");
        
    ifstream fd(argv[2]);
    if (fd.is_open()) {
    if(getline(fd,line))
        ip_address=line;
    if(getline(fd,line) && !line.empty())   
        portno=atoi(line.c_str());
    }
    else 
        error("Couldn't open the file");
    fd.close();

    string temp;
    stringstream ss(argv[1]);
    getline(ss, cli_ipAddr, ':');
    getline(ss, temp, ':');
    
    cli_port= atoi(temp.c_str());
    cout<<cli_ipAddr<<" "<<cli_port<<endl;
    pthread_create(&tid,NULL,server_thread,&cli_port);
    if((server = gethostbyname(ip_address.c_str()))==NULL)
        error("Error,No such host");
    
    if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
        error("error opening socket");
    
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(portno);

    if(connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
        error("Error Connecting");
    
    while(1){
        printf(">");
        getline(cin,mesg);
        if(!mesg.empty()){
            vector<string> msg_list=split(mesg," ");
            //printf("%s\n",msg_list[0].c_str());
            
            //create new user
            if(msg_list[0]=="create_user"){
                if(msg_list.size()!=3){
                    cout<<"wrong number of Arguments. Enter username and password\n"<<endl;
                    continue;
                }
                
                if(user1->port == "NULL"){
                    // cout<<"user creating\n";
                    // cout<<cli_ipAddr<<" "<<cli_port<<" "<<msg_list[1]<<" "<<msg_list[2]<<endl;
                    send(sockfd,msg_list[0].c_str(),strlen(msg_list[0].c_str()),0);
                
                    /*read dummy data*/
                    bzero(buffer,sizeof(buffer));
                    recv(sockfd,buffer,sizeof(buffer),0);

                    /*send username to check availability*/
                    string str=msg_list[1]+"/"+to_string(cli_port);
                    send(sockfd,str.c_str(),strlen(str.c_str()),0);

                    bzero(buffer,sizeof(buffer));
                    recv(sockfd,buffer,sizeof(buffer),0);

                    str=buffer;
                    if(str=="yes"){   
                    
                        user1->ip_address=ip_address;
                        user1->port = to_string(cli_port);
                        user1->login=0;
                        user1->username=msg_list[1];
                        user1->password=msg_list[2];
                        download_list[user1->username]=download_info;
                        cout<<"Successfully created account\n"<<endl;
                    }
                    else{
                        cout<<"Username not available\n"<<endl;
                        continue;
                    }
                }else{
                    cout<<"Already account exist\n\n";
                    continue;
                }
            }

            else if(msg_list[0]=="login"){
                if(msg_list.size()!=3){
                    cout<<"wrong number of Arguments. Enter username and password\n\n"<<endl;
                    continue;
                }
                if(user1->port=="NULL")
                {
                    cout<<"Account doesnot exists.\n\n";
                    continue;
                }
                else if(user1->login==1){
                    cout<<"Already logged in\n\n";
                }
                else{
                    if(msg_list[1]==user1->username && msg_list[2]==user1->password){
                        user1->login=1;
                        send(sockfd,msg_list[0].c_str(),strlen(msg_list[0].c_str()),0);

                        /*read dummy data*/
                        recv(sockfd,buffer,sizeof(buffer),0);

                        /*send username to start sharing all the previously shred files*/
                        string str=msg_list[1];
                        send(sockfd,str.c_str(),strlen(str.c_str()),0);

                        /*read dummy data*/
                        recv(sockfd,buffer,sizeof(buffer),0);

                        cout<<"successfully logged in\n\n";
                        continue;
                    }
                    else
                    {
                        cout<<"invalid credentials\n\n";
                        continue;
                    }
                    
                }
            }

            else if(msg_list[0]=="create_group"){
                if(msg_list.size()!=2 || msg_list[1].empty()){
                    cout<<"wrong number of Arguments. Enter groupid\n"<<endl;
                    continue;
                }
                if(user1->port=="NULL"){
                    cout<<"user should have account before creating a group\n\n";
                    continue;
                }
                if(user1->login==0){
                    cout<<"user should log in to create group\n\n";
                    continue;
                }
                else{
                    //cout<<msg_list[0]<<endl;
                    send(sockfd,msg_list[0].c_str(),strlen(msg_list[0].c_str()),0);
                    
                    /*read dummy data*/
                    bzero(buffer,sizeof(buffer));
                    recv(sockfd,buffer,sizeof(buffer),0);
                    
                    /*send userid and group id*/
                    string str=user1->username+"/"+msg_list[1];
                    //cout<<str<<endl<<endl;
                    send(sockfd,str.c_str(),strlen(str.c_str()),0);

                    bzero(buffer,sizeof(buffer));
                    recv(sockfd,buffer,sizeof(buffer),0);
                    if(strcmp(buffer,"yes")==0){
                        cout<<"successfully created\n\n";
                        //user1->groupid.push_back(msg_list[1]);
                        continue;
                    }
                    else
                        cout<<"Failed to create. Group already exists\n\n";
                    
                }
            }

            else if(msg_list[0]=="join_group"){
                if(msg_list.size()!=2 || msg_list[1].empty()){
                    cout<<"wrong number of Arguments. Enter groupid\n"<<endl;
                    continue;
                }
                if(user1->port=="NULL"){
                    cout<<"user should have account before joining a group\n\n";
                    continue;
                }
                if(user1->login==0){
                    cout<<"user should log in to join group\n\n";
                    continue;
                }
                // if(find(user1->groupid.begin(),user1->groupid.end(),msg_list[1])!=user1->groupid.end()){
                //     cout<<"user is already in the group\n\n";
                //     continue;
                // }
                else{
                    send(sockfd,msg_list[0].c_str(),strlen(msg_list[0].c_str()),0);
                    /*read dummy data*/
                    bzero(buffer,sizeof(buffer));
                    recv(sockfd,buffer,sizeof(buffer),0);
                    
                    /*send user_id and group id*/
                    string str=user1->username+"/"+msg_list[1];
                    send(sockfd,str.c_str(),strlen(str.c_str()),0);
                    
                    /*recieve status of the operation*/
                    bzero(buffer,sizeof(buffer));
                    recv(sockfd,buffer,sizeof(buffer),0);
                    
                    if(strcmp(buffer,"yes")==0){
                        cout<<"successfully request sent\n\n";
                        continue;
                    }
                    else
                        cout<<"User is already in the group or Group doesn't exist\n\n";
                }
            }

            else if(msg_list[0]=="list_groups"){
                if(user1->port=="NULL"){
                    cout<<"user should have account to get information\n\n";
                    continue;
                }
                if(user1->login==0){
                    cout<<"user should log in to get information\n\n";
                    continue;
                }
                send(sockfd,msg_list[0].c_str(),strlen(msg_list[0].c_str()),0);
                /*recieve list of groups*/
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);
                string str=buffer;
                if(str=="no"){
                    cout<<"No groups in the network\n"<<endl;
                    continue;
                }
                /*Print list of groups*/
                vector<string>res=split(buffer,"/");
                for(int i=1;i<res.size();i++)
                    cout<<res[i]<<endl;
            }
            
            else if(msg_list[0]=="requests"){
                if(msg_list.size()!=3 ||msg_list[1]!="list_requests" || msg_list[2].empty()) {
                    cout<<"wrong Arguments. Enter groupid\n"<<endl;
                    continue;
                }
                if(user1->port=="NULL"){
                    cout<<"user should have account to request for any information\n\n";
                    continue;
                }
                if(user1->login==0){
                    cout<<"user should log in to get the information\n\n";
                    continue;
                }
                send(sockfd,msg_list[0].c_str(),strlen(msg_list[0].c_str()),0);
                /*recv dummy data*/
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);
                /*send group id*/
                send(sockfd,msg_list[2].c_str(),strlen(msg_list[2].c_str()),0);

                /*recieve list of groups*/
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);
                
                string str=buffer;
                /*Print list of groups*/
                if(str=="no")
                cout<<"No pending requests for this group\n"<<endl;
                else{
                    vector<string>res=split(buffer," ");
                    for(int i=1;i<res.size();i++)
                        cout<<res[i]<<endl;
                }

                
            }
            
            else if(msg_list[0]=="accept_request"){
                if(msg_list.size()!=3 || msg_list[2].empty()){
                    cout<<"wrong number of Arguments. Enter group_id and user_id\n"<<endl;
                    continue;
                }
                if(user1->port=="NULL"){
                    cout<<"user doesn't have an account\n\n";
                    continue;
                }
                if(user1->login==0){
                    cout<<"user is not logged in\n\n";
                    continue;
                }
                send(sockfd,msg_list[0].c_str(),strlen(msg_list[0].c_str()),0);
                /*recv dummy data*/
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);
                /*send my_user_id group id and user_id*/
                string str=user1->username +"/"+ msg_list[1]+"/"+msg_list[2];
                send(sockfd,str.c_str(),strlen(str.c_str()),0);
                
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);
                str=buffer;
                if(str=="yes")
                    cout<<"Success"<<endl<<endl;
                else if(str=="no_group")
                    cout<<msg_list[1]<<" group doesn't exist\n"<<endl;
                else if(str=="no_owner")
                    cout<<"User should be the owner of the group to accept requests.\n"<<endl;
                else if(str=="no_request")
                    cout<<"No request from the user: "<<msg_list[2]<<endl<<endl;
            }
            
            else if(msg_list[0]=="upload_file"){
                if(msg_list.size()!=3 || msg_list[2].empty()){
                    cout<<"wrong number of Arguments. Enter file_path and group_id\n"<<endl;
                    continue;
                }
                ifstream fd(msg_list[1]);
                if(! fd.is_open()){
                    cout<<"file "<<msg_list[1]<<" doesn't exist\n"<<endl;
                    continue;
                }
                vector<string>arg=split(msg_list[1],"/");
                //cout<<"file_name "<<arg[arg.size()-1];
                string filename=arg[arg.size()-1];
                
                long file_size=get_filesize(msg_list[1]);
                
                string file_hash,chunk_sha=calculate_sha(msg_list[1]);
                int number_chunks=split(chunk_sha,"/").size()-1;
                file_hash=get_filehash(chunk_sha);
                // map<string,pair<string,string>> files_info;
                map<string,struct user_file*> file_info;
                struct user_file* file_str = new user_file;
                if(!user_files_info[user1->username].empty())
                {
                    file_info=user_files_info[user1->username];
                    if(!(file_info.size()>0 && file_info.find(filename)!=file_info.end())){
                        
                        file_str->full_path=msg_list[1];
                        file_str->filename=filename;
                        file_str->filesize=file_size;
                        file_str->number_chunks=number_chunks;
                        file_str->bit_vector = new int[number_chunks];
                        file_str->chunk_sha=split(chunk_sha,"/");
                        for(int i=0;i<number_chunks;i++)
                            file_str->bit_vector[i]=1;
                        user_files_info[user1->username][filename]=file_str;
                        //cout<<endl<<"new  "<<user_files_info[user1->username][filename]->filename<<" "<<user_files_info[user1->username][filename]->full_path<<endl;
                    }
                }
                else{
                    file_str->full_path=msg_list[1];
                    file_str->filename=filename;
                    file_str->filesize=file_size;
                    file_str->number_chunks=number_chunks;
                    file_str->chunk_sha=split(chunk_sha,"/");
                    file_str->bit_vector = new int[number_chunks];
                    
                    for(int i=0;i<number_chunks;i++)
                        file_str->bit_vector[i]=1;
                    user_files_info[user1->username][filename]=file_str;
                    //cout<<endl<<"new  "<<user_files_info[user1->username][filename]->filename<<" "<<user_files_info[user1->username][filename]->full_path<<endl;
                }
                /*tell tracker to upload file*/
                send(sockfd,msg_list[0].c_str(),strlen(msg_list[0].c_str()),0);
                
                /*recv dummy data*/
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);
                 
                /*send username,groupid and filename */
                string str=user1->username+"/"+msg_list[2] + "/" + filename;
                send(sockfd,str.c_str(),strlen(str.c_str()),0);
                 
                /*recieve status if the file is already present in the group*/
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);
                str=buffer;
                if(str=="no_group"){
                    cout<<"group doesn't exist\n"<<endl;
                    continue;
                }
                else if(str=="can't"){
                    cout<<"Can't upload file.User do not belong to the group\n"<<endl;
                    continue;
                }
                else if(str=="no"){
                    cout<<"File is uploaded\n"<<endl;
                    continue;
                }
                else if(str=="yes")
                {
                    cout<<"file_size: "<<file_size<<endl<<"no of chunks: "<<number_chunks<<endl;
                    //<<"chunksha: "<<chunk_sha<<endl<<"file_hash: "<<file_hash<<endl;
                    string str1=to_string(file_size)+";"+to_string(number_chunks)+";"+chunk_sha+";"+file_hash;
                    send(sockfd,str1.c_str(),strlen(str1.c_str()),0);
                    bzero(buffer,sizeof(buffer));
                    recv(sockfd,buffer,sizeof(buffer),0);
                    cout<<"file has been successfully uploaded\n"<<endl;
                    
                }
                
            }
           
            else if(msg_list[0]=="leave_group"){
                if(msg_list.size()!=2 || msg_list[1].empty()){
                    cout<<"wrong number of Arguments. Enter groupid\n"<<endl;
                    continue;
                }
                if(user1->port=="NULL"){
                    cout<<"user should have account before joining a group\n\n";
                    continue;
                }
                if(user1->login==0){
                    cout<<"user should log in to join group\n\n";
                    continue;
                }
                
                send(sockfd,msg_list[0].c_str(),strlen(msg_list[0].c_str()),0);
                    
                /*read dummy data*/
                recv(sockfd,buffer,sizeof(buffer),0);

                /*send user_id and group id*/
                string str=user1->username+"/"+msg_list[1];
                send(sockfd,str.c_str(),strlen(str.c_str()),0);

                /*recieve status of the operation*/
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);
                str=buffer;

                if(str=="no"){
                    cout<<"User is already not in the group\n"<<endl;
                    continue;
                }
                cout<<user1->username<<" left the group "<<msg_list[1]<<endl<<endl;

                

            }

            else if(msg_list[0]=="stop_share"){
                if(msg_list.size()!=3 || msg_list[2].empty()){
                    cout<<"wrong number of Arguments. Enter group_id and filename\n"<<endl;
                    continue;
                }
                if(user1->port=="NULL"){
                    cout<<"user doesn't have an account\n\n";
                    continue;
                }
                if(user1->login==0){
                    cout<<"user is not logged in\n\n";
                    continue;
                }

                send(sockfd,msg_list[0].c_str(),strlen(msg_list[0].c_str()),0);
                /*recv dummy data*/
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);
                /*send my_user_id, group id and file_name*/
                string str=user1->username +"/"+ msg_list[1]+"/"+msg_list[2];
                send(sockfd,str.c_str(),strlen(str.c_str()),0);

                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);
                str=buffer;
                if(str=="no")
                    cout<<"You are already not sharing the file\n"<<endl;
                else 
                    cout<<"you have stoped sharing: "<<msg_list[2]<<" in "<<msg_list[1]<<endl<<endl;

            }
            
            else if(msg_list[0]=="list_files"){
                if(msg_list.size()!=2 || msg_list[1].empty()){
                    cout<<"wrong number of Arguments. Enter group_id and filename\n"<<endl;
                    continue;
                }
                if(user1->port=="NULL"){
                    cout<<"user doesn't have an account\n\n";
                    continue;
                }
                if(user1->login==0){
                    cout<<"user is not logged in\n\n";
                    continue;
                }
                send(sockfd,msg_list[0].c_str(),strlen(msg_list[0].c_str()),0);
                
                /*recv dummy data*/
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);

                /*send group id */
                string str=msg_list[1];
                send(sockfd,str.c_str(),strlen(str.c_str()),0);
                
                /*recieve list of files*/
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);
                str=buffer;

                if(str=="no")
                cout<<"No files are shared in the group\n"<<endl;
                else{
                    vector<string>res=split(buffer," ");
                    for(int i=1;i<res.size();i++)
                        cout<<res[i]<<endl;
                    cout<<endl;
                }
            }

            else if(msg_list[0]=="download_file"){
                if(msg_list.size()!=4 || msg_list[3].empty()){
                    cout<<"wrong number of Arguments. Enter group_id and filename and destination path\n"<<endl;
                    continue;
                }
                if(user1->port=="NULL"){
                    cout<<"user doesn't have an account\n\n";
                    continue;
                }
                if(user1->login==0){
                    cout<<"user is not logged in\n\n";
                    continue;
                }
                send(sockfd,msg_list[0].c_str(),strlen(msg_list[0].c_str()),0);

                /*recv dummy data*/
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);

                /*send userid,groupid and filename*/
                string str=user1->username+"/"+msg_list[1]+"/"+msg_list[2];
                send(sockfd,str.c_str(),strlen(str.c_str()),0);

                /*recieve status if i can download the file*/
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);
                str=buffer;

                if(str=="no_group"){
                    cout<<"Failed: "<<msg_list[1]<<" doesn't exist\n"<<endl;
                    continue;
                }
                if(str=="can't"){
                    cout<<"Failed: User doesn't belong to the group\n"<<endl;
                    continue;
                }
                if(str=="no_file"){
                    cout<<"Failed: This file is not being shared in the group\n"<<endl;
                    continue;
                }
                else{
                    struct user_file* file_str = new user_file;
                    vector<string>info=split(str,";");
                    // cout<<atoi(info[0].c_str())<<" "<<atoi(info[1].c_str())<<endl;
                    // cout<<info[2]<<"\n"<<info[3]<<endl;
                    //cout<<file_str->chunk_sha[0]<<endl;
                    file_str->filesize=atoi(info[0].c_str());
                    file_str->chunk_sha=split(info[3],"/");
                    file_str->filename=msg_list[2];
                    file_str->full_path=msg_list[3];
                    
                    int number_chunks=atoi(info[1].c_str());
                    file_str->number_chunks=number_chunks;
                    file_str->bit_vector = new int[number_chunks];
                    for(int i=0;i<number_chunks;i++)
                        file_str->bit_vector[i]=0;
                    user_files_info[user1->username][msg_list[2]]=file_str;
                    
                    /*send dummy data*/
                    send(sockfd,str.c_str(),strlen(str.c_str()),0);
                    
                    /*receive peer info*/
                    bzero(buffer,sizeof(buffer));
                    recv(sockfd,buffer,sizeof(buffer),0);
                    str=buffer;
                    vector<string> peers=split(str,"-");
                    
                    
                    cout<<"list of peers:\n";
                    for(int i=1;i<peers.size();i++)
                        cout<<peers[i]<<" ";
                    cout<<endl;
                    struct information *temp1[peers.size()];
                    // temp1[0]=new information;
                    // temp1[0]->filename=msg_list[2];
                    // temp1[0]->port_num=peers[1];

                    pthread_mutex_init(&lock1, NULL);


                    // struct information *temp=new information;
                    // temp->filename=msg_list[2];
                    // temp->port_num=peers[1];
                    pair<string,string>p=make_pair(msg_list[2],msg_list[1]);
                    download_info->downloading.push_back(p);
                    
                    //cout<<"files downloaded: "<<download_info->downloading[0].first<<endl;
                    pthread_t tid1;
                    pthread_t tid[peers.size()];

                    for(int i=1;i<peers.size();i++){
                        cout<<"creating thread for peer: "<<peers[i]<<endl;
                        temp1[i]=new information;
                        temp1[i]->filename=msg_list[2];
                        temp1[i]->port_num=peers[i];
                        pthread_create(&tid[i], NULL, Download, (void*) temp1[i]);
                    }
                    cout<<endl;
                    //pthread_create(&tid1, NULL, Download, (void*) temp1[0]);
                    for(int i=1;i<peers.size();i++)
                    {
                        //printf("waiting\n\n");
                        pthread_join(tid[i],NULL);
                    }
                    pthread_mutex_destroy(&lock1);
                    //pthread_join(tid1,NULL);
                    //cout<<"thread executed successfully\n\n";
                    for(int i=1;i<peers.size();i++){
                        delete temp1[i];
                        temp1[i]=NULL;
                    }
                    
                    ifstream fd(msg_list[3]);
                    if(! fd.is_open()){
                        cout<<"Error: Download failed\n"<<endl;
                        continue;
                    }

                     //delete temp1[0];
                    string actual_hash=get_filehash(info[3]); 
                    //cout<<"Actual file hash: "<<actual_hash<<endl<<endl;

                    string file_hash=get_filehash(calculate_sha(msg_list[3])); 
                    //cout<<"Downloaded file hash: "<<file_hash<<endl<<endl;
                    
                    
                    vector<pair<string,string>>vec=download_info->downloading;
                    vec.erase(remove(vec.begin(), vec.end(), p), vec.end());
                    download_info->downloading=vec;
                    if(file_hash!=actual_hash){
                        for(int i=0;i<number_chunks;i++)
                            file_str->bit_vector[i]=0;
                        cout<<"Failed: Error while downloading: Please try again\n\n\n";
                    }
                    else
                    { 
                        download_info->complete.push_back(p);
                        cout<<"\n***Download successfull***\n"<<endl;
                    }
                    
                }
                

            }
            
            else if(msg_list[0]=="show_downloads"){
                vector<pair<string,string>>downloading=download_info->downloading;
                vector<pair<string,string>>complete=download_info->complete;
                for(int i=0;i<downloading.size();i++)
                    cout<<"[D] ["<<downloading[i].second<<"] ["<<downloading[i].first<<"]\n";
                for(int i=0;i<complete.size();i++)
                    cout<<"[C] ["<<complete[i].second<<"] ["<<complete[i].first<<"]\n";
                cout<<endl;
                

            }

            else if(msg_list[0]=="logout"){
                if(user1->port=="NULL"||user1->login==0){
                    cout<<"User is not logged in\n\n";
                    continue;
                }
                user1->login=0;
                send(sockfd,msg_list[0].c_str(),strlen(msg_list[0].c_str()),0);
                
                /*recv dummy data*/
                recv(sockfd,buffer,sizeof(buffer),0);

                /*send username to stop sharing files in the network*/
                string str=user1->username;
                send(sockfd,str.c_str(),strlen(str.c_str()),0);

                /*recv dummy data*/
                recv(sockfd,buffer,sizeof(buffer),0);

                cout<<"***Logged out***\n"<<endl;
                
            }

            else if(mesg=="exit")
                break;
            else{
                send(sockfd,mesg.c_str(),strlen(mesg.c_str()),0);
                bzero(buffer,sizeof(buffer));
                recv(sockfd,buffer,sizeof(buffer),0);
                cout<<buffer<<endl;
            }
        }
            
            
        
    }
    pthread_join(tid,NULL);
    //delete [] temp1;
    close(sockfd);
    return 0;
}