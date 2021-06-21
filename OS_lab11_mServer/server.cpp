#include "resourses.h"

using namespace std;

const int MAX_FAULT = 20;
HANDLE mutx;

static vector<SOCKET> clients;
static string mainModer = "fanteak";
static SOCKET moderator;
static bool moderOnline = false;

unsigned int __stdcall ClientTread(void* threadParam) {
	SOCKET client = (SOCKET)threadParam;
	clients.push_back(client);

	char buff[512];
	int msgSize;
	string cmd, params;

	bool auth = false;
	string username = "anon";
	bool admin = false;

	NewLog(true, vector<string>{ "SERVER", "Client connected" });
	while (true) {
		while (!(msgSize = recv(client, buff, 512, 0)));
		if (msgSize == SOCKET_ERROR) {
			NewLog(true, vector<string>{ "ERROR", "socket error" });
			break;
		}
		buff[msgSize] = 0;
		cout << buff << endl;
		NewLog(true, vector<string>{ "SERVER", "Message recieved" });
		if (parseCMD(buff, cmd, params)) {
			if (cmd == "QUIT") {
				NewLog(true, vector<string>{ cmd, username });
				if (username == mainModer) {
					moderOnline = false;
				}
				break;
			}
			if (cmd == "AUTH") {
				if (auth) {
					NewLog(true, vector<string>{ cmd, username, "Already logined" });
					strcpy(buff, "ERR You are already logged in.\r\n");
				} else {
					int success = CreateUser(params, username);
					if (success < 0) {
						NewLog(true, vector<string>{ cmd, username, "Invalid command" });
						strcpy(buff, "ERR Can`t recognise command.\r\n");
					}
					else if (success > 0) {
						NewLog(true, vector<string>{ cmd, username, "Username taken" });
						strcpy(buff, "ERR This username is taken.\r\n");
					}
					else {
						NewLog(true, vector<string>{ cmd, username, "Account created" });
						strcpy(buff, "INF User account created successfuly.\r\n");
						auth = true;
					}
				}
				send(client, buff, strlen(buff), 0);
			}
			if (cmd == "LOGIN") {	// LOGIN yuri yuriTheTop
				if (auth) {
					NewLog(true, vector<string>{ cmd, username, "Already logined" });
					strcpy(buff, "ERR You are already logged in.\r\n");
				} else {
					int success = LogInUser(params, username, admin);
					if (success < 0) {
						NewLog(true, vector<string>{ cmd, username, "Invalid command" });
						strcpy(buff, "ERR Can`t recognise command.\r\n");
					}
					else if (success == 1) {
						NewLog(true, vector<string>{ cmd, username, "Wrong password" });
						strcpy(buff, "ERR Wrong password.\r\n");
					}
					else if (success == 2) {
						NewLog(true, vector<string>{ cmd, username, "Wrong username" });
						strcpy(buff, "ERR No such username.\r\n");
					}
					else {
						NewLog(true, vector<string>{ cmd, username, "User authorised" });
						strcpy(buff, "INF Logged in successfuly.\r\n");
						auth = true;
						if (username == mainModer) {
							moderator = client;
							moderOnline = true;
						}
					}
				}
				send(client, buff, strlen(buff), 0);
			}
			if (cmd == "SEND") {
				if (auth) {
					if (GetMsg(params, username, admin)) {
						NewLog(true, vector<string>{ cmd, username, "Message recieved" });
					}
					else {
						NewLog(true, vector<string>{ cmd, username, "User is banned" });
						strcpy(buff, "ERR Bad words exceded.\r\n");
						send(client, buff, strlen(buff), 0);
					}
				}
				else {
					NewLog(true, vector<string>{ cmd, username, "User not logged" });
					strcpy(buff, "ERR You are not logged in.\r\n");
					send(client, buff, strlen(buff), 0);
				}
			}
			if (cmd == "SET") {
				if (admin) {
					string tUser;
					int success = SetFaults(params, tUser);
					if (success == -1) {
						NewLog(true, vector<string>{ cmd, params, username, "Invalid command" });
					} 
					else if (success == -2) {
						NewLog(true, vector<string>{ cmd, tUser, username, "No such user" });
					}
					else if (success == -3) {
						NewLog(true, vector<string>{ cmd, tUser, username, "User is admin" });
					}
					else {
						NewLog(true, vector<string>{ cmd, tUser, "faults set to " + to_string(success) });
					}
				}
				else {
					NewLog(true, vector<string>{ cmd, username, "User is not admin" });
					strcpy(buff, "ERR You do not have rights to do it.\r\n");
					send(client, buff, strlen(buff), 0);
				}
			}
			if (cmd == "FAULT") {
				if (admin) {
					int success = GetFaults(client);
					if (success == -1) {
						NewLog(true, vector<string>{ cmd, username, "Can`t send" });
					}
					else {
						NewLog(true, vector<string>{ cmd, username, "Sent successfully" });
					}
				}
				else {
					NewLog(true, vector<string>{ cmd, username, "User is not admin" });
					strcpy(buff, "ERR You do not have rights to do it.\r\n");
					send(client, buff, strlen(buff), 0);
				}
			}
		}
		else {
			NewLog(true, vector<string>{ buff, username, "Invalid command" });
			strcpy(buff, "ERR Invalid command.\r\n");
			send(client, buff, strlen(buff), 0);
		}
	}
	closesocket(client);
}

string trim(const string& str) {
	int first = str.find_first_not_of(' ');
	if (string::npos == first) {
		return str;
	}
	int last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

bool parseCMD(char* str, std::string& cmd, std::string& params) {
	int n;
	string tmp = str;
	tmp = trim(tmp);
	if ((n = tmp.find(' ')) == string::npos) {
		std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
		if ((tmp != "QUIT") && (tmp != "FAULT")) {
			return false;
		}
		cmd = tmp;
		return true;
	}
	cmd = tmp.substr(0, n);
	params = tmp.substr(n + 1, tmp.length());
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

	cmd = trim(cmd);
	params = trim(params);
	if ((cmd != "AUTH") && (cmd != "FILE") && (cmd != "LOGIN") && (cmd != "SET") && (cmd != "SEND"))
		return false;
	return true;
}

bool SplitString(string str, string& first, string& second) {
	int n;
	if ((n = str.find(' ')) == -1) {
		return false;
	}
	first = str.substr(0, n);
	second = str.substr(n + 1, str.length());
	second = trim(second);
	std::transform(first.begin(), first.end(), first.begin(), ::tolower);
	return true;
}

int CreateUser(string str, std::string& user) {
	string login, password;
	if (!SplitString(str, login, password)) {
		return -1;
	}
	Json::Value userList;
	ifstream fin;
	fin.open("users.json");
	fin >> userList;
	fin.close();
	int n = userList.get("number", 0).asInt();
	for (int i = 0; i < n; ++i) {
		if (login == userList.get("idList", 0)[i]["login"].asString()) {
			return 1;
		}
	}
	userList["number"] = n + 1;
	userList["idList"][n]["login"] = login;
	userList["idList"][n]["passw"] = password;
	userList["idList"][n]["fault"] = 0;
	userList["idList"][n]["admin"] = false;
	ofstream fout;
	fout.open("users.json");
	fout << userList;
	fout.close();
	user = login;
	return 0;
}

int LogInUser(string str, string& user, bool& admin) {
	string login, password;
	if (!SplitString(str, login, password)) {
		return -1;
	}
	Json::Value userList;
	ifstream fin;

	WaitForSingleObject(mutx, INFINITE);
	fin.open("users.json");
	fin >> userList;
	fin.close();
	ReleaseMutex(mutx);

	int n = userList.get("number", 0).asInt();
	for (int i = 0; i < n; ++i) {
		if (login == userList.get("idList", 0)[i]["login"].asString()) {
			if (password == userList.get("idList", 0)[i]["passw"].asString()) {
				user = login;
				if (userList.get("idList", 0)[i]["admin"].asBool()) {
					admin = true;
				}
				return 0;
			}
			else {
				return 1;
			}
		}
	}
	return 2;
}

int GetUserFault(string login) {
	Json::Value userList;
	ifstream fin;
	fin.open("users.json");
	fin >> userList;
	fin.close();
	int n = userList.get("number", 0).asInt();
	for (int i = 0; i < n; ++i) {
		if (login == userList.get("idList", 0)[i]["login"].asString()) {
			 return userList.get("idList", 0)[i]["fault"].asInt();
		}
	}
	return -1;
}

bool GetMsg(string str, string& user, bool admin) {
	if (GetUserFault(user) >= MAX_FAULT) {
		return false;
	}
	string out = "";
	if (admin) {
		out = "<M> ";
	}
	out += user + " : " + str;
	
	if (moderOnline) {
		if (sendToModer(out)) {
			return true;
		}
	}
	msgDistr(out);
	return true;
}

bool sendToModer(string str) {
	str = "EDI " + str;
	const int n = 512;
	if (str.length() > n - 1) {
		str[n - 1] = 0;
	}
	char buff[n];
	strcpy(buff, str.c_str());
	int success = send(moderator, buff, strlen(buff), 0);
	if (success == SOCKET_ERROR) {
		moderOnline = false;
		closesocket(moderator);
		return false;
	}
	return true;
}
//SET <M> fanteak : let`s play csgo
//SET yuri 1 let`s play ****

int SetFaults(string param, string& tUser) {
	string user, text;
	if (!SplitString(param, user, text)) {
		cout << "first split" << endl;
		return -1;		// Wrong command
	}
	tUser = user;
	if (user == "<m>") {
		msgDistr(param);
		return -3;
	}
	string msg, numStr;
	if (!SplitString(text, numStr, msg)) {
		cout << "second split" << endl;
		return -1;
	}
	int n;
	try {
		n = stoi(numStr);
	} 
	catch (...) {
		return -1;
	}
	if (n < 0) {
		return -1;
	}

	Json::Value userList;
	ifstream fin;
	fin.open("users.json");
	fin >> userList;
	fin.close();
	int faults = -1;

	int number = userList.get("number", 0).asInt();
	int i = 0;
	for (i = 0; i < number; ++i) {
		if (user == userList.get("idList", 0)[i]["login"].asString()) {
			faults = userList.get("idList", 0)[i]["fault"].asInt();
			break;
		}
	}
	if (faults < 0) {
		return -2;		// No such user
	}
	faults += n;
	userList["idList"][i]["fault"] = faults;

	msgDistr(user + " : " + msg + "\n");
	return faults;
}

string nowTime() {
	time_t now = time(0);
	tm* gmtm = gmtime(&now);
	stringstream out;
	out << gmtm->tm_mday << "-";
	out << 1 + gmtm->tm_mon << "-";
	out << 1900 + gmtm->tm_year << " ";
	out << gmtm->tm_hour << ":";
	out << gmtm->tm_min << ":";
	out << gmtm->tm_sec;
	return out.str();
}

void msgDistr(string str) {
	ofstream fout;
	WaitForSingleObject(mutx, INFINITE);
	fout.open("chatLog.txt", fstream::in | fstream::out | fstream::app);
	fout << str;
	fout.close();
	ReleaseMutex(mutx);

	str = "MSG " + str + '\n';
	const int n = 512;
	if (str.length() > n-1) {
		str[n - 1] = 0;
	}
	char buff[n];
	strcpy(buff, str.c_str());
	for (int i = 0; i < clients.size(); ++i) {
		send(clients[i], buff, strlen(buff), 0);
	}
}

//FAULT
//FREZ 3 zumori 4|fanteak 5|yuri 213
int GetFaults(SOCKET& admin) {
	Json::Value userList;
	ifstream fin;

	WaitForSingleObject(mutx, INFINITE);
	fin.open("users.json");
	fin >> userList;
	fin.close();
	ReleaseMutex(mutx);

	string out = "FREZ ";
	int n = userList.get("number", 0).asInt();

	//out += to_string(n) + " ";
	for (int i = 0; i < n; ++i) {
		out += userList.get("idList", 0)[i]["login"].asString() + " "
			+ to_string(userList.get("idList", 0)[i]["fault"].asInt()) + " ";
	}

	const int max = 512;
	char buff[max];
	strcpy(buff, out.c_str());
	int success = send(admin, buff, strlen(buff), 0);
	if (success == SOCKET_ERROR) {
		return -1;
	}
	return 0;
}

void NewLog(bool time, std::vector<std::string> logParam) {
	ofstream fout;
	stringstream ss;
	if (time) {
		ss << "[" << nowTime() << "]: ";
	}
	for (int i = 0; i < logParam.size() - 1; ++i) {
		ss << logParam[i] << " | ";
	}
	ss << logParam[logParam.size() - 1];
	ss << endl;

	WaitForSingleObject(mutx, INFINITE);
	fout.open("serverLog.txt", fstream::in | fstream::out | fstream::app);
	cout << ss.str();
	fout << ss.str();
	fout.close();
	ReleaseMutex(mutx);
}
