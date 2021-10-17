#include "message.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

struct client {
	bool online;
	uint32_t ip, port;
	int socket;
	queue<packet> messagesQ;
};

int main(int argc, char** argv) {

#pragma region INIT
    if (argc < 2) {
		usage(argv[0]);
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int portno = atoi(argv[1]);

    int TCPsockfd, UDPsockfd, TMPsockfd;
	struct sockaddr_in serv_addr{};
	int n, ret;

	fd_set read_fds;	// multimea de citire folosita in select()
	int fdmax;			// valoare maxima fd din multimea read_fds

    // se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);

	/*Setare struct sockaddr_in pentru a asculta pe portul respectiv */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

#pragma region TCP
	TCPsockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(TCPsockfd == -1, "TCP socket");

	int enable = 1;
	if (setsockopt(TCPsockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
		perror("reuse port");
		exit(1);
	}
	enable = 1;
	if (setsockopt(TCPsockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) == -1) {
		perror("disable Neagle's algorithm");
		exit(2);
	}


	uint32_t maxTCP = 10;
	ret = bind(TCPsockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind TCP");

	ret = listen(TCPsockfd, maxTCP);
	DIE(ret < 0, "listen TCP");

#pragma endregion
#pragma region UDP
	UDPsockfd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(UDPsockfd < 0, "UDP socket");
	enable = 1;
	if (setsockopt(UDPsockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
		perror("setsocketopt");
		exit(1);
	}
	/* Legare proprietati de socket */
	ret = bind(UDPsockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "bind UDP");
#pragma endregion

	// se adauga cei 3 file descriptori (socketii pe care se asculta conexiuni) in multimea read_fds
	FD_SET(UDPsockfd, &read_fds);
	FD_SET(TCPsockfd, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
	fdmax = max(TCPsockfd, UDPsockfd);

#pragma endregion

	unordered_map<string, unordered_map<string, int>> topicClients;
	unordered_map<string, client> clients;

    while (1) {
		fd_set tmp_fds{};		// multime folosita temporar
        tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret == -1, "select");

		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			// se citeste de la tastatura
			char buffer[200];
			memset(buffer, 0, 200);
			fgets(buffer, 200, stdin);
			if (strstr(buffer, "exit")) {
				packet p{};
				p.m.tip_pachet = EXIT;
				p.length = htonl(TOPICANDTYPE);
				for (auto it = clients.begin(); it != clients.end(); it++) {
					if ((*it).second.online) {
						send((*it).second.socket, &p, TCPCLIENTLEN, 0);
						close((*it).second.socket);
					}
					close(UDPsockfd);
					close(TCPsockfd);
				}

				return 0;
			}
		}

        for (int i = 3; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == TCPsockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					struct sockaddr_in cli_addr;
					socklen_t socklen = sizeof(struct sockaddr_in);
					memset(&cli_addr, 0, socklen);
					TMPsockfd = accept(TCPsockfd, (struct sockaddr *) &cli_addr, &socklen);
					DIE(TMPsockfd < 0, "accept");

					char buf[10]{};
					ret = recv(TMPsockfd, buf, 10, 0);
					DIE(ret == -1, "recv new client ID");

					string id = string(buf);
					id.resize(10);

					auto it = clients.find(id);

					if (it == clients.end()) {
						// se adauga noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(TMPsockfd, &read_fds);
						if (TMPsockfd > fdmax) {
							fdmax = TMPsockfd;
						}

						client c{};
						c.online = true;
						c.ip = cli_addr.sin_addr.s_addr;
						c.port = cli_addr.sin_port;
						c.socket = TMPsockfd;

						string id = string(buf);
						id.resize(10);

						clients.insert(make_pair(id, c));

						if (clients.size() == maxTCP) {
							maxTCP += 10 % INT_MAX;
							ret = listen(TCPsockfd, maxTCP);
							DIE(ret < 0, "listen TCP");
						}

						cout << "New client " << string(buf) << " connected from "
							<< inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << "\n";

						packet p{};
						p.m.tip_pachet = OK;
						p.length = htonl(TOPICANDTYPE);
						send(TMPsockfd, &p, TCPCLIENTLEN, 0);
					} else if(!(*it).second.online) {
						// client vechi cu socket nou
						FD_SET(TMPsockfd, &read_fds);
						if (TMPsockfd > fdmax) {
							fdmax = TMPsockfd;
						}

						clients[(*it).first].socket = TMPsockfd;
						clients[(*it).first].online = true;

						packet p{};
						p.m.tip_pachet = OK;
						p.length = htonl(TOPICANDTYPE);
						send(TMPsockfd, &p, TCPCLIENTLEN, 0);

						cout << "New client " << string(buf) << " connected from "
							<< inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << "\n";

						while (!clients[(*it).first].messagesQ.empty()) {
							packet p = clients[(*it).first].messagesQ.front();
							clients[(*it).first].messagesQ.pop();
							send(TMPsockfd, &p, ntohl(p.length) + HDRLEN, 0);
						}
					} else {
						// client nou cu id deja existent
						packet p{};
						p.m.tip_pachet = EXIT;
						p.length = htonl(TOPICANDTYPE);
						send(TMPsockfd, &p, TCPCLIENTLEN, 0);
						close(TMPsockfd);
						cout << "Client " << string(buf) << " already connected.\n";
					}
				} else if(i == UDPsockfd) {
					// mesaj primit pe socketul pasiv de tip UDP
					packet p{};
					struct sockaddr_in cli_addr;
					socklen_t socklen = sizeof(struct sockaddr_in);
					memset(&cli_addr, 0, socklen);
					ret = recvfrom(UDPsockfd, &p.m, UDPMESSAGESIZE, 0,
										(struct sockaddr*)&cli_addr, &socklen);
					DIE(ret == -1, "recvfrom UDP");
					p.src = cli_addr;
					string topic = string(p.m.topic);
					topic.resize(50);

					uint32_t len = TOPICANDTYPE;
					switch (p.m.tip_pachet)
					{
					case INT:
						len += sizeof(myInteger);
						break;
					case SHORT_REAL:
						len += sizeof(uint16_t);
						break;
					case FLOAT:
						len += sizeof(myFloat);
						break;
					case STRING:
						len += strlen(p.m.value.str);
						break;
					}

					p.length = htonl(len);

					for (auto& it: topicClients[topic]) {
						int sf = it.second;

						auto cl = clients.find(it.first);

						if ((*cl).second.online) {
							send((*cl).second.socket, &p, HDRLEN + len, 0);
						} else if (sf)
							clients[(*cl).first].messagesQ.push(p);
					}
            	} else {
					// s-au primit date pe unul din socketii de client TCP,
					// asa ca serverul trebuie sa le receptioneze
					packet p{};
					ret = recv(i, &p, TCPCLIENTLEN, 0);
					DIE(ret == -1, "read");

					if (ret == 0) {
						 unordered_map<string, client>::iterator it =
						 	find_if(clients.begin(), clients.end(),
							 	[=] (unordered_map<string, client>::value_type c_)
											{ return c_.second.socket == i; });
						client* c = &clients[(*it).first];
						c->online = false;
						close(i);
						FD_CLR(i, &read_fds);
						c->socket = -1;
						cout << "Client " << p.id << " disconnected.\n";
						continue;
					}

					string topic = string(p.m.topic);
					topic.resize(50);

					string id = string(p.id);
					id.resize(10);

					switch (p.m.tip_pachet){
						case EXIT: {
							client* c = &clients[id];
							c->online = false;
							close(i);
							FD_CLR(i, &read_fds);
							c->socket = -1;
							cout << "Client " << p.id << " disconnected.\n";
							continue;
						}
						case SUBSCRIBEREQ:
							topicClients[topic].insert(make_pair(id, 1));
							break;
						case SUBSCRIBEREQNOSF:
							topicClients[topic].insert(make_pair(id, 0));
							break;
						case UNSUBSCRIBEREQ:
							topicClients[topic].erase(id);
					}
				}
			}
        }
    }
}
