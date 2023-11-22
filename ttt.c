#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

bool request = false;
bool response = false;


int connect_inet(char *host, char *service)
{
    struct addrinfo hints, *info_list, *info;
    int sock, error;

    // look up remote host
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;  // in practice, this means give us IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // indicate we want a streaming socket

    error = getaddrinfo(host, service, &hints, &info_list);
    if (error) {
        fprintf(stderr, "error looking up %s:%s: %s\n", host, service, gai_strerror(error));
        return -1;
    }

    for (info = info_list; info != NULL; info = info->ai_next) {
        sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (sock < 0) continue;

        error = connect(sock, info->ai_addr, info->ai_addrlen);
        if (error) {
            close(sock);
            continue;
        }

        break;
    }
    freeaddrinfo(info_list);

    if (info == NULL) {
        fprintf(stderr, "Unable to connect to %s:%s\n", host, service);
        return -1;
    }

    return sock;
}

#define BUFLEN 512

void *recvMessage(void *arg)
{
    int *s = (int *)arg;
    char buf[BUFLEN];
    // int bytes;
    while (1) {
        // scanf("%[^\n]", buf);
        scanf("%s", buf);
        // printf("get(%s)\n", buf);
        if (strlen(buf) <= 0) {
            continue;
        }
        int byte = write(*s, buf, strlen(buf));
        if (byte <= 0)
        {
            perror("write thread");
            break;
        }
    }
    return NULL;
}

void ProcessResponse(char* buf, int len)
{
    printf("%s\n", buf);
}

int playCmd(int sock)
{
    // input name
    char name[BUFLEN];
    char buf[BUFLEN * 2];
    char c;
    while (1) {
        printf("Please input your name to begin this game:\n");
        //fflush(stdin);
        scanf("%[^\n]", name);
	    while((c = getchar()) != '\n' && c != EOF);
        // printf("get name:(%s)\n", name);

        snprintf(buf, sizeof(buf), "PLAY|%lu|%s|", strlen(name)+1, name);

        printf("send play:(%s)\n", buf);

        // sent PLAY
        int byte = write(sock, buf, strlen(buf));
        if (byte <= 0)
        {
            perror("write !!!");
            return -1;
        }

	    memset(buf, 0, sizeof(buf));

        byte = read(sock, buf, sizeof(buf));
        if (byte <= 0) {
            perror("read !!!");
            return -1;
        }
	    ProcessResponse(buf, byte);
	    // printf("readbuf(%s), byte(%d)\n", buf, byte);
        if (strncmp(buf, "WAIT", 4) == 0) {
            printf("send play OK!\n");
            break;
        }
    }
    
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: ttt domain port\n");
        return -1;
    }

    int sock, bytes;
    char buf[BUFLEN];
    memset(buf, 0, sizeof(buf));

    sock = connect_inet(argv[1], argv[2]);
    if (sock < 0) {
        exit(EXIT_FAILURE);
    }

    printf("connect success sock %d\n", sock);

    if (playCmd(sock)) {
        return -1;
    }

    pthread_t tid;
    int res = pthread_create(&tid, NULL, recvMessage, &sock);
    if (res) {
        fprintf(stderr, "pthread_create error");
        return -1;
    }

    memset(buf, 0, sizeof(buf));

    while ((bytes = read(sock, buf, BUFLEN)) > 0) {
        // TODO parse message
        if (bytes <= 0) {
            fprintf(stderr, "read error");
            break;
        }

        ProcessResponse(buf, bytes);

        memset(buf, 0, sizeof(buf));
        
        
    }

    close(sock);
    
    
    

}
