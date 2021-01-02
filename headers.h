#include<stdio.h>
#include<sys/types.h>  // datatypes
#include<sys/socket.h> //socket
#include<netinet/in.h> //structures
#include<stdlib.h>
#include<netdb.h>
#include<string.h>
#include<unistd.h>
#include<errno.h> //perror
#include<pthread.h>
#include<bits/stdc++.h>
using namespace std;
struct user_file
{
    string filename;
    string full_path;
    long filesize;
    int *bit_vector;
    vector<string> chunk_sha;
    int number_chunks;
    //int sharing;
};
struct user
{
string username;
string password;
string ip_address;
string port;
//map<string,pair<string,string>> files_info;
//map<string,int*> bit_vector;
int login;
//vector<string> groupid;
//vector<struct user_file*>files;
};
struct file_download{
    vector<pair<string,string>> complete;  //<file_name,groupid>
    vector<pair<string,string>> downloading;
};
map<string,struct file_download*>download_list;
map<string,map<string,struct user_file*>>user_files_info;

map<string,vector<pair<string,string>>>pending_request;
map<string,vector<string>>pending_group_requests;
vector<string> split(string s, string delim)
{
    string token;
    size_t loc=0;
    vector<string>res;
    while ((loc = s.find(delim)) != string::npos) {
        token = s.substr(0, loc);
        res.push_back(token);
        s.erase(0, loc + delim.length());
    }
    res.push_back(s);
    return res;
}
// map<string,map<string,pair<string,string>>>user_file_info;
// map<string,map<string,int*>>bit_vector;