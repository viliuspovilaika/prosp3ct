#include <fstream>
#include <iostream>
#include <cstring>
#include <list>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <cctype>
#include <filesystem>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define bHOST	"www.bing.com"
#define	bPORT	"443"
#define	bSurl	"https://www.bing.com/search?q=<QUERY>"

#define	MTU	1500

#define	VERS	"1.1"
#define DESC	"prosp3ct - a blazing fast Bing based OSINT engine"

using namespace std;

string fArg = "";

void Initialize()
{
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();
}

string Graphical(string input)
{
	string output = "";
	for (unsigned int c = 0; c < input.size(); c++)
	{
		if (isgraph(input[c]))
			output += input[c];
	}
	return output;
}

string Transmit(char *sendBuff)
{
	char sendBuf[MTU];
	strcpy(sendBuf, sendBuff);
	char recvBuf[MTU];
	BIO * bio;
	SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
	SSL *ssl;
	bio = BIO_new_ssl_connect(ctx);
	BIO_get_ssl(bio, &ssl);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	BIO_set_conn_hostname(bio, bHOST);
	BIO_set_conn_port(bio, bPORT);
	if (bio == NULL)
		return "";
	if (BIO_do_connect(bio) <= 0)
		return "";
	if (BIO_write(bio, sendBuf, sizeof(sendBuf)) <= 0)
		return "";
	int x = 1;
	string fullData = "";
	while (x > 0)
	{
		memset(&recvBuf[0], 0, sizeof(recvBuf));
		x = BIO_read(bio, recvBuf, MTU);
		fullData += recvBuf;
	}
	SSL_CTX_free(ctx);
	BIO_reset(bio);
	BIO_free_all(bio);
	return fullData;
}

string replaceSubstring(string data, const char *replaceFrom, const char *replaceTo, bool once = false)
{
	string fixed = data;
	while (fixed.find(replaceFrom) != string::npos)
	{
		fixed = fixed.replace(fixed.find(replaceFrom), strlen(replaceFrom), replaceTo);
		if (once)
			break;
	}
	return fixed;
}

string MakeQuery(string query, int page=0)
{
	char sendBuf[MTU];
	query = replaceSubstring(query, " ", "+");
	sprintf(sendBuf, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", bSurl, bHOST);
	strcpy(sendBuf, replaceSubstring(sendBuf, "<QUERY>", query.c_str()).c_str());
	if (page > 0)
	{
		string pageStr = query + "&first=" + to_string(page*10+1);
		strcpy(sendBuf, replaceSubstring(sendBuf, query.c_str(), pageStr.c_str(), true).c_str());
	}
	return Transmit(sendBuf);
}

list<string> GetQueryResults(string query, int page=0)
{
	string queryResult = MakeQuery(query, page);
	string regex = "<h2><a href=\"";
	list<string> results;
	while (queryResult.find(regex) != string::npos)
	{
		queryResult = queryResult.substr(queryResult.find(regex)+regex.size(), queryResult.size());
		string cResult = queryResult.substr(regex.size(), queryResult.size());
		cResult = queryResult.substr(0, queryResult.find("\""));
		cResult = Graphical(cResult);
		results.push_back(cResult);
	}
	return results;
}

void PrintUsage(bool desc=false)
{
	if (desc)
		cout << endl << "\t" << DESC << endl;
	cout << endl << "\tUsage:\t" << fArg << " <-q string>/<-i string> [-p int] [-o string] [-s string] [-vh]" << endl << endl;
	cout << "\t\t-q <query>\tscrape for a single query" << endl;
	cout << "\t\t-i <file>\tload queries from a file" << endl;
	cout << "\t\t-p <pages>\tpages per query (default: 1)" << endl;
	cout << "\t\t-o <file>\toutput to file" << endl;
	cout << "\t\t-s <site>\tspecify a target site for the queries" << endl;
	cout << "\t\t-v\t\tverbose when writing to file" << endl;
	cout << "\t\t-h\t\tprint this help menu" << endl << endl;
}

bool isNumber(string s)
{
	return !s.empty() && find_if(s.begin(), s.end(), [](unsigned char c) { return !isdigit(c); }) == s.end();
}

void MissingArg(string flag, string arg)
{
	cout << endl << "\tMissing argument for flag " << flag << ", usage: " << flag << " " << arg << endl;
}

string GetTime()
{
	auto timen = chrono::system_clock::now();
	time_t timet = chrono::system_clock::to_time_t(timen);
	return (string)ctime(&timet);
}

int main(int argc, char *argv[])
{
	fArg = argv[0];
	string site = "";
	string query = "";
	string outFile = "";
	bool isFile = false;
	bool verbose = false;
	unsigned int pages = 1;
	if (argc == 1)
	{
		PrintUsage(true);
		return 0;
	}
	for (int c = 1; c < argc; c++)
	{
		if (strcmp(argv[c],"-h") == 0)
		{
			PrintUsage(true);
			return 0;
		}
	}
	for (int c = 1; c < argc; c++)
	{
		if (strcmp(argv[c],"-q") == 0)
		{
			if (c+1 != argc)
			{
				query = argv[c+1];
			}
			else
			{
				MissingArg(argv[c], "<search query>");
				PrintUsage();
				return 0;
			}
			c++;
			isFile = false;
		}
		else if (strcmp(argv[c],"-i") == 0)
		{
			if (c+1 != argc)
			{
				query = argv[c+1];
				if (!filesystem::exists(query))
				{
					cout << endl << "\tQuery input file not found, usage: -i <query file location>" << endl;
					PrintUsage();
					return 0;
				}
			}
			else
			{
				MissingArg(argv[c], "<query file location>");
				PrintUsage();
				return 0;
			}
			c++;
			isFile = true;
		}
		else if (strcmp(argv[c],"-p") == 0)
		{
			if (c+1 != argc)
			{
				string TestBuf = argv[c+1];
			}
			else
			{
				MissingArg(argv[c], "<number of pages>");
				PrintUsage();
				return 0;
			}
			if (isNumber(argv[c+1]))
			{
				pages = stoi(argv[c+1]);
				if (pages < 1)
				{
					cout << endl << "\tPage count must be a positive number, usage: -p <number of pages>" << endl;
					PrintUsage();
					return 0;
				}
			}
			else
			{
				cout << endl << "\tFlag -p takes integer as input, usage: -p <number of pages>" << endl;
				PrintUsage();
				return 0;
			}
			c++;
		}
		else if (strcmp(argv[c],"-o") == 0)
		{
			if (c+1 != argc)
			{
				outFile = argv[c+1];
			}
			else
			{
				MissingArg(argv[c], "<output file location>");
				PrintUsage();
				return 0;
			}
			c++;	
		}
		else if (strcmp(argv[c],"-s") == 0)
		{
			if (c+1 != argc)
			{
				site = argv[c+1];
			}
			else
			{
				MissingArg(argv[c], "<site name>");
				PrintUsage();
				return 0;
			}
			c++;
		}
		else if (strcmp(argv[c],"-v") == 0)
			verbose = true;
		else
		{
			if (((string)argv[c]).substr(0, 1) == "-")
				cout << endl << "\tUnrecognized flag \"" << argv[c] << "\"" << endl;
			else
				cout << endl << "\tUnexpected argument \"" << argv[c] << "\"" << endl;
			PrintUsage();
			return 0;
		}
	}
	if (query == "")
	{
		cout << endl << "\tNo queries specified. Did you forget the -q/-i flag?"  << endl;
		PrintUsage();
		return 0;
	}
	// Init stage
	cout << endl << "\tprosp3ct " << VERS << " starting at " << GetTime() << endl;
	list<string> queries;
	ifstream queryFstream;
	queryFstream.open(query);
	for (string line; getline(queryFstream, line);)
	{
		queries.push_back(line);
	}
	queryFstream.close();
	cout << "     Settings:" << endl;
	if (site != "")
		cout << "  Site: " << site << endl;
	if (isFile)
		cout << "  Search queries loaded: " << queries.size() << endl;
	else
	{
		queries.push_back(query);
		cout << "  Search query: " << query << endl;
	}
	cout << "  Pages per query: " << pages << endl;
	bool output = false;
	if (outFile != "")
	{
		cout << "  Output file: " << outFile << endl;
		output = true;
	}
	if (!output)
		cout << endl;
	else
	{
		if (verbose)
			cout << endl;
	}
	Initialize();
	list<string>::iterator queryIt;
	ofstream outputFstream;
	if (output)
	{
		outputFstream.open(outFile);
	}
	for (queryIt = queries.begin(); queryIt != queries.end(); queryIt++)
	{
		for (unsigned int c = 0; c < pages; c++)
		{
			list<string> results;
			if (site == "")
				results = GetQueryResults((*queryIt), c);
			else
				results = GetQueryResults(("site:" + site + "+" + (*queryIt)), c);
			list<string>::iterator rit;
			for (rit = results.begin(); rit != results.end(); rit++)
			{
				if (output)
				{
					outputFstream << (*rit) << endl;
					if (verbose)
						cout << (*rit) << endl;
				}
				else
					cout << (*rit) << endl;
			}
		}
	}
	outputFstream.close();
	cout << endl << "\tFinished at " << GetTime() << endl;
	return 0;
}
