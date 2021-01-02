#include "headers.h"
#include<bits/stdc++.h>
#define MSG_SIZE 1024
using namespace std;
void error(const char *msg)
{
    cout<<msg<<endl;
    exit(1);
}
struct file
{
    string file_name;
    //string path;
    string sha_of_chunks;
    int number_of_chucks;
    string file_sha;
    long filesize;
    vector<string> file_clients; //<user>
    vector<string> not_sharing;
};

map<string,string>port_number; //username to port_number map
map<string,vector<string>>user_list_files; // list of all files shared by the user in the network;
map<string,vector<string>>pending_join_requests;//pending requests of each group
map<string,vector<string>>group_list; //list of group and corresponding users 
map<string,vector<string>>user_group; //username and list of groups to which he belongs
vector<string>username; //list of all usernames being used
map<string,map<string,struct file*>>group_files; //group and files shared within that group

bool create_group(string str){
    vector<string> v=split(str,"/");
    //cout<<v[1]<<" "<<v[0]<<endl;
    if(group_list.find(v[1])==group_list.end())
    {
        //cout<<v[1]<<endl<<v[0]<<endl;
        user_group[v[0]].push_back(v[1]);
        group_list[v[1]].push_back(v[0]); /*first user in the list is the admin*/
        return true;
    }        
    
    return false;    
}

bool join_group(string str){
    vector<string> v=split(str,"/");
    /*check if group exists*/
    if(group_list.find(v[1])==group_list.end())
        return false;
    /*check if user is already in the group*/
    if(user_group.find(v[0])!=user_group.end()){
        vector<string>groups=user_group[v[0]];
        if(find(groups.begin(),groups.end(),v[1])!=groups.end())
            return false;
    }

    /*check if user has already requested to join*/
    if(pending_join_requests.find(v[1])!=pending_join_requests.end()){
        vector<string> requests=pending_join_requests[v[1]];
        if(find(requests.begin(),requests.end(),v[0])!=requests.end())
            return true;
    }
    /*adding user to the pending requests*/
    pending_join_requests[v[1]].push_back(v[0]);
    return true;
                 
}

string stop_sharing_file(string userid, string groupid, string filename){
    string str;
    //vector<string>info=split(buffer,"/");
    if(group_files.find(groupid)==group_files.end())
        str="no";
    else if(group_files[groupid].find(filename)==group_files[groupid].end())
        str="no";
    else{
        struct file *file_str;
        vector<string>clients=group_files[groupid][filename]->file_clients;
        if(find(clients.begin(),clients.end(),userid)==clients.end()){
            str="no";
        }
        else{
            str="yes";
            clients.erase(remove(clients.begin(), clients.end(), userid), clients.end());
            group_files[groupid][filename]->file_clients=clients;
            group_files[groupid][filename]->not_sharing.push_back(userid);
            cout<<"stoped sharing "<<filename<<" in the group "<<groupid<<endl;
            // if(clients.size()>0)
            //     group_files[groupid][filename]->file_clients=clients;
            // else 
            // {
            //     group_files[groupid].erase(filename);  //if no user is sharing then remove that file from list
            //     if(group_files[groupid].size()==0)
            //         group_files.erase(groupid);     //if no files are being shared the group remove that from list 
            // } 
        }
    }
    return str;
}

void stop_sharing_group(string userid,string groupid){
    if(user_list_files.find(userid)!=user_list_files.end()){
        vector<string>files=user_list_files[userid];
        for(int i=0;i<files.size();i++)
            stop_sharing_file(userid,groupid,files[i]);
    }

}

void stop_sharing_network(string userid)
{
    if(user_list_files.find(userid)!=user_list_files.end() && user_group.find(userid)!=user_group.end()){
        vector<string>groups=user_group[userid];
        for(int i=0;i<groups.size();i++){
            stop_sharing_group(userid,groups[i]);
        }
    }
}

bool group_exist(string groupid){
    if(group_list.find(groupid)!=group_list.end())
        return true;
    return false;
}

bool not_in_group(string userid, string groupid){
    if(group_exist(groupid)){
        vector<string>users =group_list[groupid];
        if(find(users.begin(),users.end(),userid)==users.end())
            return true;
     }
    return false;
}

bool file_in_group(string groupid, string filename){
    if(group_exist(groupid)){
        if(group_files.find(groupid)==group_files.end())
            return false;
        if(group_files[groupid].find(filename)==group_files[groupid].end())
            return false;
        return true;
    }
    return false;
}

void share_file_in_group(string userid,string groupid,string filename){
    if(file_in_group(groupid,filename)){
        struct file* temp=group_files[groupid][filename];
        vector<string>users=temp->not_sharing;
        if(users.size()>0){
            if(find(users.begin(),users.end(),userid)!=users.end())
            {
                users.erase(remove(users.begin(), users.end(), userid), users.end());
                group_files[groupid][filename]->not_sharing=users;
                group_files[groupid][filename]->file_clients.push_back(userid);
                cout<<"started sharing file "<<filename<<" in group "<<groupid<<endl;
            }
        }
    }
}

void share_file_in_network(string userid,string filename){
    if(!group_files.empty()){
        for(auto it=group_files.begin();it!=group_files.end();it++){
            share_file_in_group(userid,it->first,filename);
        }
    }
}

void share_files(string userid){
    if(!user_list_files.empty() && user_list_files.find(userid)!=user_list_files.end()){
        vector<string>files=user_list_files[userid];
        if(!files.empty()){
            for(int i=0;i<files.size();i++){
                share_file_in_network(userid,files[i]);
            }
        }
    }
}

bool is_file_shared_in_group(string groupid, string filename){
    if(file_in_group(groupid,filename)){
        struct file* temp=group_files[groupid][filename];
        if(temp->file_clients.size()>0)
            return true;
    }
    return false;
}

void* handleConnection(void *arg){
        int new_socket= *((int*)arg),valread;
        char buffer[MSG_SIZE] = {0}; 
        char *hello=new char [MSG_SIZE]; 
        while(1){
            
            bzero(buffer,MSG_SIZE);
            if((valread = recv( new_socket , buffer, MSG_SIZE,0))<0)
                error("error in reading"); 
           //printf("%s\n",buffer ); 
            if(strcmp(buffer,"create_user")==0){
                strcpy(hello,"dummy");
                send(new_socket , hello , strlen(hello) , 0 );

                /*receive username to check availability */
                bzero(buffer,MSG_SIZE);
                if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                    error("error in reading");
                vector<string>list1=split(buffer,"/");
                //cout<<list1[0]<<"\n";
                string user1=list1[0],str="no";
                
                // if(username.empty() || find(username.begin(),username.end(),user1)==username.end()){
                //     str="yes";
                //     username.push_back(user1);
                // }
                    
                if(username.empty())
                {
                    str="yes";
                    username.push_back(user1);
                }
                else if(find(username.begin(),username.end(),user1)==username.end()){
                    str="yes";
                    username.push_back(user1);
                }
                if(str=="yes"){
                    port_number[user1]=list1[1];
                    cout<<"user with port num "<<port_number[user1]<<" is created"<<endl;
                }
                send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );

            }
            
            else if(strcmp(buffer,"login")==0){
                strcpy(hello,"dummy");
                send(new_socket , hello , strlen(hello) , 0 );

                /*receive username*/
                bzero(buffer,MSG_SIZE);
                if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                    error("error in reading");
                string str=buffer;
                share_files(str);
                
                /*dummy data*/
                send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );

            }
            
            else if(strcmp(buffer,"create_group")==0){
                strcpy(hello,"dummy");
                send(new_socket , hello , strlen(hello) , 0 );

                /*recieve userid and group id*/
                bzero(buffer,MSG_SIZE);
                if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                    error("error in reading");
                if(create_group(buffer)){
                    strcpy(hello,"yes");
                    send(new_socket , hello , strlen(hello) , 0 );
                }
                else{
                    strcpy(hello,"no");
                    send(new_socket , hello , strlen(hello) , 0 );
                }
            }
            
            else if(strcmp(buffer,"join_group")==0){
                strcpy(hello,"dummy");
                send(new_socket , hello , strlen(hello) , 0 );

                /*recieve user_id and group id*/
                bzero(buffer,MSG_SIZE);
                if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                    error("error in reading");
                if(join_group(buffer)){
                    strcpy(hello,"yes");
                    send(new_socket , hello , strlen(hello) , 0 );
                }
                else{
                    strcpy(hello,"no");
                    send(new_socket , hello , strlen(hello) , 0 );
                }
            }
            
            else if(strcmp(buffer,"list_groups")==0){
                string str;
                if(group_list.size()>0){
                    for(auto it = group_list.begin();it!=group_list.end();it++)
                        str+="/"+it->first;
                }
                else    
                    str="no";
                send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
            }
            
            else if(strcmp(buffer,"requests")==0){
                strcpy(hello,"dummy");
                send(new_socket , hello , strlen(hello) , 0 );
                string str;
                /*recieve group id*/
                bzero(buffer,MSG_SIZE);
                if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                    error("error in reading");
                if(pending_join_requests.find(buffer)==pending_join_requests.end())
                    str="no";
                else{
                    vector<string>requests=pending_join_requests[buffer];
                    for(int i=0;i<requests.size();i++)
                        str+=" "+requests[i];
                }
                send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
            }
            
            else if(strcmp(buffer,"accept_request")==0){
                strcpy(hello,"dummy");
                send(new_socket , hello , strlen(hello) , 0 );
                /*recieve owner_id and group id and user_id*/
                bzero(buffer,MSG_SIZE);
                if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                    error("error in reading");
                string str;
                vector<string> info=split(buffer,"/");
                //cout<<info[0]<<" "<<info[1]<<" "<<info[2];
                if(group_list.find(info[1])==group_list.end())
                    str="no_group";
                else if(pending_join_requests.find(info[1])==pending_join_requests.end())
                    str="no_request";
                else if(group_list[info[1]][0]!=info[0])
                    str="no_owner";
                else{
                    vector<string>requests = pending_join_requests[info[1]];
                    if(find(requests.begin(),requests.end(),info[2])==requests.end())
                        str="no_request";
                    else 
                    {
                        str="yes";
                        group_list[info[1]].push_back(info[2]);
                        user_group[info[2]].push_back(info[1]);
                        requests.erase(remove(requests.begin(), requests.end(), info[2]), requests.end());
                        pending_join_requests[info[1]]=requests;
                    }

                }
                send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
            }

            else if(strcmp(buffer,"leave_group")==0){
                strcpy(hello,"dummy");
                send(new_socket , hello , strlen(hello) , 0 );

                /*recieve user_id and group id*/
                bzero(buffer,MSG_SIZE);
                if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                    error("error in reading");

                string str="yes";
                vector<string>info=split(buffer,"/");
                if(not_in_group(info[0],info[1]))
                    str="no";
                else 
                   { 
                       stop_sharing_group(info[0],info[1]);
                       vector<string>users=group_list[info[1]];
                       users.erase(remove(users.begin(), users.end(), info[0]), users.end());
                       if(users.size()>0)
                            group_list[info[1]]=users;
                        else
                            group_list.erase(info[1]);
                        
                   }
                send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );

            }
           
            else if(strcmp(buffer,"upload_file")==0){
                strcpy(hello,"dummy");
                send(new_socket , hello , strlen(hello) , 0 );
               
                /*recieve userid, groupid,filename*/
                bzero(buffer,MSG_SIZE);
                if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                    error("error in reading");
                
                string str;
                vector<string>info=split(buffer,"/");
                cout<<"uploading file\n";
                //cout<<"userid: "<<info[0]<<"  groupid: "<<info[1]<<"  file: "<<info[2]<<endl;
                //check if file present in the group
                if(user_list_files.find(info[0])==user_list_files.end()||find(user_list_files[info[0]].begin(),user_list_files[info[0]].end(),info[2])==user_list_files[info[0]].end()){
                    user_list_files[info[0]].push_back(info[2]);
                }
                if(!group_exist(info[1])){
                    str="no_group";
                    cout<<"no group"<<endl;
                    send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
                }
                else if(not_in_group(info[0],info[1]))
                {    
                    str="can't";
                    cout<<info[1]<<" is not found";
                    send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
                }
                else if(file_in_group(info[1],info[2])){
                    struct file *file_str;
                    vector<string>clients=group_files[info[1]][info[2]]->file_clients;
                    vector<string>stop_sharing=group_files[info[1]][info[2]]->not_sharing;
                    str="no";
                    if(find(clients.begin(),clients.end(),info[0])==clients.end())
                        group_files[info[1]][info[2]]->file_clients.push_back(info[0]);

                    /*also check if the user is present in not_sharing list. if yes remove */
                    if(!stop_sharing.empty() && find(stop_sharing.begin(),stop_sharing.end(),info[0])!=stop_sharing.end())
                       stop_sharing.erase(remove(stop_sharing.begin(), stop_sharing.end(), info[0]), stop_sharing.end()); 
                    group_files[info[1]][info[2]]->not_sharing=stop_sharing;
                       
                    send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
                }
                else if(!file_in_group(info[1],info[2]))
                {    
                    str="yes";
                    send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
                    bzero(buffer,MSG_SIZE);
                    if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                        error("error in reading");
                    vector<string> arg = split(buffer,";");
                    //cout<<"file_size: "<<arg[0]<<endl<<"no of chunks: "<<arg[1]<<endl<<"chunksha: "<<arg[2]<<endl<<"file_hash: "<<arg[3]<<endl;
                    struct file *temp=new file;
                    temp->file_name=info[2];
                    temp->file_clients.push_back(info[0]);
                    temp->filesize=atoi(arg[0].c_str());
                    temp->number_of_chucks=atoi(arg[1].c_str());
                    temp->sha_of_chunks=arg[2];
                    temp->file_sha=arg[3];
                    group_files[info[1]][info[2]]=temp;
                    cout<<"new file "<<info[2]<<" has been uploaded\n\n";
                    send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
                }
                
            }

            else if(strcmp(buffer,"stop_share")==0){
                strcpy(hello,"dummy");
                send(new_socket , hello , strlen(hello) , 0 );                
                
                /*recieve userid, groupid,filename*/
                bzero(buffer,MSG_SIZE);
                if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                    error("error in reading");
                
                string str;
                vector<string>info=split(buffer,"/");
                str=stop_sharing_file(info[0],info[1],info[2]);
                send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );

                // if(group_files.find(info[1])==group_files.end())
                //     str="no";
                // else if(group_files[info[1]].find(info[2])==group_files[info[1]].end())
                //     str="no";
                // else{
                //     struct file *file_str;
                //     vector<string>clients=group_files[info[1]][info[2]]->file_clients;
                //     if(find(clients.begin(),clients.end(),info[0])==clients.end()){
                //         str="no";
                //     }
                //     else{
                //         str="yes";
                //         clients.erase(remove(clients.begin(), clients.end(), info[0]), clients.end());
                //     }
                // }
            }
            
            else if(strcmp(buffer,"list_files")==0){
                strcpy(hello,"dummy");
                send(new_socket , hello , strlen(hello) , 0 );
               
                /*recieve groupid*/
                bzero(buffer,MSG_SIZE);
                if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                    error("error in reading");
                
                string str;
                if(group_files.find(buffer)!=group_files.end())
                {
                    map<string,struct file*> mp=group_files[buffer];
                    if(mp.size()>0)
                    {
                        for(auto it=mp.begin();it!=mp.end();it++)
                        {
                            struct file*temp=it->second;
                            if(temp->file_clients.size()>0)
                                str+=" "+it->first;
                            
                        }
                    }
                }
                if(str.empty())
                    str="no";
                send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );  
            }

            else if(strcmp(buffer,"download_file")==0){
                strcpy(hello,"dummy");
                send(new_socket , hello , strlen(hello) , 0 );

                /*recieve userid, groupid,filename*/
                bzero(buffer,MSG_SIZE);
                if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                    error("error in reading");
                
                vector<string>info=split(buffer,"/");
                string str;

                if(!group_exist(info[1]))
                    str="no_group";
                else if(not_in_group(info[0],info[1]))
                    str="can't";
                else if(!(is_file_shared_in_group(info[1],info[2])))
                    str="no_file";
                else{
                    struct file* temp=group_files[info[1]][info[2]];
                    str=to_string(temp->filesize);
                    str+= ";" +to_string(temp->number_of_chucks);
                    str+=";"+temp->file_sha;
                    str+=";"+temp->sha_of_chunks;

                    send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
                    /*read dummy data*/
                    bzero(buffer,MSG_SIZE);
                    if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                        error("error in reading");
                    
                    //send info about file users
                    str="";
                    vector<string>peers=group_files[info[1]][info[2]]->file_clients;
                    for(int i=0;i<peers.size();i++)
                        str+="-"+port_number[peers[i]];
                    //cout<<str<<endl;
                    
                    /*add the downloader to file users so that other peers can download from this peer */
                    if(find(peers.begin(),peers.end(),info[0])==peers.end())
                        group_files[info[1]][info[2]]->file_clients.push_back(info[0]);  
                
                }
                send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
            }

            else if(strcmp(buffer,"logout")==0){
                strcpy(hello,"dummy");
                send(new_socket , hello , strlen(hello) , 0 );

                /*receive username to stop sharing his files in the network*/
                bzero(buffer,MSG_SIZE);
                if((valread = read( new_socket , buffer, MSG_SIZE))<0)
                    error("error in reading");
                string str=buffer;
                stop_sharing_network(str);
                /*dummy data*/
                send(new_socket , str.c_str() , strlen(str.c_str()) , 0 );
            }

            else if(strcmp(buffer,"exit")==0)
                break;
            else{
                //printf("%s\n",buffer ); 
                strcpy(hello,"Invalid command");
                send(new_socket , hello , strlen(hello) , 0 ); 
            }
            
            
        }
        close(new_socket);
        
        delete [] hello;
    return NULL;
}
int main(int argc, char const *argv[]) 
{ 
    int server_fd, new_socket, valread,n=12,port; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    char buffer[1024] = {0}; 
    string line,ip_address;
    pthread_t tid;
    if(argc!=3){
        cout<<"execute the program this way --> /a.out tracker_info.txt tracker_no\n";
    }
       
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
        error("socket failed"); 
        
    
       
    // Forcefully attaching socket to the port 8080 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
        error("setsockopt failed"); 
        
    ifstream fd(argv[1]);
    if(getline(fd,line))
        ip_address=line;
    else 
        error("File doesn't exists or File doesn't contain IP address");
    if(getline(fd,line) && !line.empty())   
        port=atoi(line.c_str());
    else 
        error("File doesn't contain port number");
    
    fd.close();
    
    //int port= atoi(portno.c_str());
    
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( port ); 
       
    // // Forcefully attaching socket to the given port 
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) 
        error("bind failed"); 
        
    if (listen(server_fd,n) < 0) //socket can connect to 10 clients at a time
        error("listen failed"); 
    //cout<<"server running at port:"<<port<<endl; 
    vector<pthread_t> tids; 
    while(1){
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) 
            error("accept"); 
        
        pthread_t tid1;
        tids.push_back(tid1);
        int size=tids.size();
        pthread_create(&tids[size-1],NULL,handleConnection,&new_socket);
        //pthread_create(&tid,NULL,handleConnection,&new_socket);
        
    }
    for(int i=0;i<tids.size();i++)
        pthread_join(tids[i],NULL);
    //pthread_join(tid,NULL);
    close(server_fd);
    return 0; 
} 