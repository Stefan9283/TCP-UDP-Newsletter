#include "message.h"

#define BUFLEN 200

void usage(char *file)
{
	fprintf(stderr, "Usage: %s client_id server_address server_port\n", file);
	exit(0);
}

int main(int argc, char** argv) {
#pragma region PREP
    if (argc < 3) {
		usage(argv[0]);
	}

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int sockfd, n, ret;
    struct sockaddr_in serv_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");
	serv_addr.sin_port = htons(atoi(argv[3]));

    int enable = 1;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) == -1) {
		perror("disable Neagle's algorithm");
		exit(2);
	}

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

    DIE(strlen(argv[1]) > 10, "ID is too long");
    n = send(sockfd, argv[1], 10, 0);
	DIE(n < 0, "connect");

    {
        packet p{};
        ret = recv(sockfd, &p, sizeof(packet), 0);
        DIE(ret < 0, "recv");
        if (p.m.tip_pachet != OK) {
            close(sockfd);
            exit(0);
        }
    }



    fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	FD_SET(sockfd, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
	int fdmax = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

    char buffer[BUFLEN];
    #pragma endregion

    while (true) {
	    tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

        #pragma region HANDLE STDIN
		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			// se citeste de la tastatura
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN, stdin);

            if (strstr(buffer, "unsubscribe")) {
                if (DIE(strlen(buffer) <= strlen("unsubscribe"), "Invalid unsubscribe input", false) == FAIL)
                    goto receive_from_server;
                packet* p = createUnSubRequest(buffer + strlen("unsubscribe "), argv[1]);
                n = send(sockfd, p, TCPCLIENTLEN, 0);
                delete p;
                DIE(n < 0, "send");
                cout << "Unsubscribed from topic.\n";
            } else if (strstr(buffer, "subscribe")) {
                char *topic, *sfchr;
                int sf;
                packet* p;

                topic = strtok (buffer + strlen("subscribe "), " ");

                if (DIE(topic == nullptr, "No subscription topic", false) == FAIL ||
                    DIE(strlen(topic) > 50, "Topic length is over 50 chars", false) == FAIL)
                    goto receive_from_server;

                sfchr = strtok (NULL, " ");

                if (DIE(sfchr == nullptr, "No subscription sf", false) == FAIL ||
                    DIE(!isdigit(sfchr[0]), "sf should be a number", false) == FAIL)
                    goto receive_from_server;

                sf = atoi(sfchr);

                if (DIE(!(sf != 1 || sf != 0), "Invalid sf value", false) == FAIL)
                    goto receive_from_server;

                p = createSubRequest(topic, sf, argv[1]);
                n = send(sockfd, p, TCPCLIENTLEN, 0);
                delete p;
                DIE(n < 0, "send");
                cout << "Subscribed to topic.\n";
            } else if (strstr(buffer, "exit")) {
                packet p{};
                p.m.tip_pachet = EXIT,
                p.length = htonl(51);

                strcpy(p.id, argv[1]);
                n = send(sockfd, &p, TCPCLIENTLEN, 0);
                break;
            };
		}
        #pragma endregion

        #pragma region HANDLE RECV
        receive_from_server:
		if (FD_ISSET(sockfd, &tmp_fds)) {
			packet p{};

            int r;

            r = recv(sockfd, &p, HDRLEN, 0);
			DIE(r < 0, "recv");

			r = recv(sockfd, &p.m, ntohl(p.length), 0);
			DIE(r < 0, "recv");

            if (p.m.tip_pachet == EXIT) break;
            printMessage(&p);
        }
        #pragma endregion
    }

    close(sockfd);
}
