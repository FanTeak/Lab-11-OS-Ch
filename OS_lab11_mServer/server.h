#ifndef MY_SERVER_HANDLER
#define MY_SERVER_HANDLER

#include <string>

unsigned int __stdcall ClientTread(void*);
bool parseCMD(char*, std::string&, std::string&);

int CreateUser(std::string, std::string&);
int LogInUser(std::string, std::string&, bool&);

int GetFaults(SOCKET&);
int SetFaults(std::string, std::string&);

bool GetMsg(std::string, std::string&, bool);
bool sendToModer(std::string);
void msgDistr(std::string);

void NewLog(bool, std::vector<std::string>);
#endif

