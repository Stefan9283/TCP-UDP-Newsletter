#include "message.h"

packet* createSubRequest(char* topic, int sf, char id[10]) {
    packet *p = new packet;
    memset(p, 0, sizeof(packet));
    p->m.tip_pachet = sf == 1 ? SUBSCRIBEREQ : SUBSCRIBEREQNOSF;
    p->length = TCPCLIENTLEN;
    memcpy(p->m.topic, topic, strlen(topic));
    memcpy(p->id, id, strlen(id));
    return p;
}
packet* createUnSubRequest(char* topic, char id[10]) {
    packet *p = new packet;
    memset(p, 0, sizeof(packet));
    p->m.tip_pachet = UNSUBSCRIBEREQ;
    p->length = TCPCLIENTLEN;
    memcpy(p->m.topic, topic, strlen(topic));
    memcpy(p->id, id, strlen(id));
    return p;
}

int DIE(bool val, const char* s, bool shouldExit) {
    if (val) {
        perror(s);
        if (shouldExit)
            exit(0);
    }
    return val * FAIL + !val * SUCCESS;
}


float toFloat(myFloat f) {
    float unsignVal = (float)(ntohl(f.parteIntreaga) / (double)pow(10, (int)f.putereNeg10));
    if (f.semn == 1)
        return - unsignVal;
    else return unsignVal;
}
int toInteger(myInteger i) {
    if (!i.semn)
        return ntohl(i.value);
    else return - ntohl(i.value);
}


void printPacket(packet* m) {
    cout << htonl(m->length) << " ";
    printMessage(m);
}

void printMessage(packet* p) {
    message* m = &p->m;
    cout << inet_ntoa(p->src.sin_addr) << ":" << ntohs(p->src.sin_port) << " - " << m->topic << " - ";

    switch (m->tip_pachet) {
            case INT:
                cout << "INT - " << toInteger(m->value.intreg) << "\n";
                break;
            case SHORT_REAL:
                printf("SHORT_REAL - %.2f\n", ntohs(m->value.real_scurt) / 100.0f);
                break;
            case FLOAT:
                printf("FLOAT - %f\n", toFloat(m->value.real));
                break;
            case STRING:
                cout << "STRING - " << m->value.str << "\n";
                break;
            default:
                cout << (int)m->tip_pachet << "\n";
            }
}
