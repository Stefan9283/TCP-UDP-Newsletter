#include <cmath>
#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <cassert>
#include <errno.h>
#include <climits>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <queue>
#include <fstream>

using namespace std;


struct __attribute__((__packed__)) myFloat {
    char semn;
    uint32_t parteIntreaga;
    uint8_t putereNeg10;

};

struct __attribute__((__packed__)) myInteger {
    char semn;
    uint32_t value;
};


struct __attribute__((__packed__)) message {
    char topic[50];
    char tip_pachet;

    union {
        myInteger intreg;
        uint16_t real_scurt;
        myFloat real;
        char str[1501];
    } value;
};

struct __attribute__((__packed__)) packet {
    sockaddr_in src;
    uint32_t length;
    char id[10];
    message m;
};



#define INT 0
#define SHORT_REAL 1
#define FLOAT 2
#define STRING 3

#define SUBSCRIBEREQ 4
#define SUBSCRIBEREQNOSF 5
#define UNSUBSCRIBEREQ 6

#define OK 100
#define EXIT 10

#define SUCCESS 100
#define FAIL 10

#define UDPMESSAGESIZE 1551
#define HDRLEN sizeof(packet) - sizeof(message)  // 30
#define TOPICANDTYPE 51
#define TCPCLIENTLEN HDRLEN + TOPICANDTYPE // 81


packet* createUnSubRequest(char* topic, char id[10]);
packet* createSubRequest(char* topic, int sf, char id[10]);

int DIE(bool val, const char* s, bool shouldExit = true);

void printPacket(packet* m);
void printMessage(packet* m);