#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdbool.h>

#include "ttt.h"


TTTS g_ttts;


int open_listener(char *service, int queue_size)
{
    struct addrinfo hint, *info_list, *info;
    int error, sock;

    // initialize hints
    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family   = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags    = AI_PASSIVE;

    // obtain information for listening socket
    error = getaddrinfo(NULL, service, &hint, &info_list);
    if (error) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    // attempt to create socket
    for (info = info_list; info != NULL; info = info->ai_next) {
        sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

        // if we could not create the socket, try the next method
        if (sock == -1) continue;

        // bind socket to requested port
        error = bind(sock, info->ai_addr, info->ai_addrlen);
        if (error) {
            close(sock);
            continue;
        }

        // enable listening for incoming connection requests
        error = listen(sock, queue_size);
        if (error) {
            close(sock);
            continue;
        }

        // if we got this far, we have opened the socket
        break;
    }

    freeaddrinfo(info_list);

    // info will be NULL if no method succeeded
    if (info == NULL) {
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    return sock;
}

void destroyOneGame(TTTS *ttts, int gameIndex)
{
    printf("destroyOneGame gameIndex %d\n", gameIndex);
    Game *game = &ttts->games[gameIndex];
    // delete X player
    for (int i = 0; i < ttts->waitingNum; i++) {
        if (0 == strcmp(game->X.name, ttts->waitingPlayer[i].name)) {
            printf("delete X player fd(%d)\n", game->X.sock);
            close(game->X.sock);
            FD_CLR(game->X.sock, &ttts->rd_fds);
            // 1 2 3 4 5 6      6
            // 1 2 4 5 6 6      5
            for (int j = i; j < ttts->waitingNum-1; j++) {
                ttts->waitingPlayer[j] = ttts->waitingPlayer[j+1];
            }
            ttts->waitingNum--;
        }
    }

    // delete O player
    for (int i = 0; i < ttts->waitingNum; i++) {
        if (0 == strcmp(game->O.name, ttts->waitingPlayer[i].name)) {
            printf("delete O player fd(%d)\n", game->O.sock);
            close(game->O.sock);
            FD_CLR(game->O.sock, &ttts->rd_fds);
            // 1 2 3 4 5 6      6
            // 1 2 4 5 6 6      5
            for (int j = i; j < ttts->waitingNum-1; j++) {
                ttts->waitingPlayer[j] = ttts->waitingPlayer[j+1];
            }
            ttts->waitingNum--;
        }
    }

    // delete game
    for (int i = gameIndex; i < ttts->gamesNum-1; i++) {
        ttts->games[i] = ttts->games[i+1];
    }
    ttts->gamesNum--;
}

void AddWaitingPlayer(TTTS *ttts,int fd)
{

    
    for(int i = 0; i < ttts->waitingNum; i++){
    	if(fd == ttts->waitingPlayer[i].sock){
	     return;
	}
    }



    //printf("im here %d, waitingNum(%d)\n",__LINE__, ttts->waitingNum); 
    ttts->waitingPlayer = realloc(ttts->waitingPlayer, (ttts->waitingNum+1)*sizeof(Player));
    //printf("im here %d\n",__LINE__); 
    //memset(ttts->waitingPlayer[ttts->waitingNum].name, 0, sizeof(ttts->waitingPlayer[ttts->waitingNum].name));
    memset(&ttts->waitingPlayer[ttts->waitingNum], 0, sizeof(ttts->waitingPlayer[ttts->waitingNum]));
    ttts->waitingPlayer[ttts->waitingNum].sock = fd;
    ttts->waitingPlayer[ttts->waitingNum].playing = false;
    ttts->waitingPlayer[ttts->waitingNum].step = -1;
    // ttts->waitingPlayer[ttts->waitingNum].isHere = false;
    ttts->waitingNum++;
    return;
}

void DelWaitingPlayer(TTTS *ttts,int fd)
{
    // check player is in game
    for (int i = 0; i < ttts->gamesNum; i++) {
        if (ttts->games[i].X.sock == fd || ttts->games[i].O.sock == fd) {
            destroyOneGame(ttts, i);
            break;
        }
    }

    // delete player
    for (int i = 0; i < ttts->waitingNum; i++) {
        if (ttts->waitingPlayer[i].sock == fd) {
            for (int j = i; j < ttts->waitingNum-1; j++) {
                ttts->waitingPlayer[j] = ttts->waitingPlayer[j+1];
            }
            ttts->waitingNum--;
            break;
        }
    }

}

Cmd parseCmd(TTTS *ttts, int indexOfPlayer, char *cmdStr)
{
    // printf("parse cmdStr:(%s)\n", cmdStr);
    Cmd cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = INVALID_CMD;

    char *bar = strchr(cmdStr, '|');
    if (bar == NULL) {
        cmd.type = INVALID_CMD;
        snprintf(cmd.reason, sizeof(cmd.reason), "INVL|18|!invalid command.|");
        int n = write(ttts->waitingPlayer[indexOfPlayer].sock, cmd.reason, strlen(cmd.reason));
        if (n < 0) {
            printf("response INVL error");
        }
        return cmd;
    }
    *bar = '\0';

    char *l = bar+1;
    
    if (0 == strcmp(cmdStr, "PLAY")) {
        cmd.type = PLAY;
    } else if (0 == strcmp(cmdStr, "MOVE")) {
        cmd.type = MOVE;
    } else if (0 == strcmp(cmdStr, "RSGN")) {
        cmd.type = RSGN;
    } else if (0 == strcmp(cmdStr, "DRAW")) {
        cmd.type = DRAW;
    } else {
        cmd.type = INVALID_CMD;
    }

    // MOVE|6|X|2,2|
    bar = strchr(l, '|');
    if (bar == NULL) {
        cmd.type = INVALID_CMD;
        snprintf(cmd.reason, sizeof(cmd.reason), "INVL|18|!invalid command.|");
        int n = write(ttts->waitingPlayer[indexOfPlayer].sock, cmd.reason, strlen(cmd.reason));
        if (n < 0) {
            printf("response INVL error");
        }
        return cmd;
    }
    *bar = '\0';
    int len = atoi(l);
    if (len > 0) {
        char *p1 = bar+1;
        // printf("p1:%s\n", p1);
        bar = strchr(p1, '|');
        if (!bar) {
            printf("cannot find a bar\n");
            cmd.type = INVALID_CMD;
            snprintf(cmd.reason, sizeof(cmd.reason), "INVL|18|!invalid command.|");
            int n = write(ttts->waitingPlayer[indexOfPlayer].sock, cmd.reason, strlen(cmd.reason));
            if (n < 0) {
                printf("response INVL error");
            }
            return cmd;
        } else { 
        	if (bar-p1+1 == len) {  // only have p1
        	    strncpy(cmd.param1, p1, bar-p1);
        	} else {    // have p2
        	    // get p1
        	    strncpy(cmd.param1, p1, bar-p1);

        	    char *p2 = bar+1;
        	    bar = strchr(p2, '|');
        	    if (!bar) {
        	        printf("cannot find a bar\n");
        	        cmd.type = INVALID_CMD;
                    snprintf(cmd.reason, sizeof(cmd.reason), "INVL|18|!invalid command.|");
                    int n = write(ttts->waitingPlayer[indexOfPlayer].sock, cmd.reason, strlen(cmd.reason));
                    if (n < 0) {
                        printf("response INVL error");
                    }
                    return cmd;
        	    } else {
                    if (bar-p1+1 == len) {  // only have p1 and p2
                        strncpy(cmd.param2, p2, bar-p2);
                    } else {    // have p3
                        // get p2
                        strncpy(cmd.param2, p2, bar-p2);

                        char *p3 = bar+1;
                        if (!bar)
                        {
                            cmd.type = INVALID_CMD;
                            snprintf(cmd.reason, sizeof(cmd.reason), "INVL|18|!invalid command.|");
                            int n = write(ttts->waitingPlayer[indexOfPlayer].sock, cmd.reason, strlen(cmd.reason));
                            if (n < 0) {
                                printf("response INVL error");
                            }
                            return cmd;
                        }
                        
                        bar = strchr(p3, '|');
                        
                        if (bar-p1+1 != len) {
                            printf("cannot find a bar\n");
                            cmd.type = INVALID_CMD;
                        } else {
                            strncpy(cmd.param3, p3, bar-p3);
                        }
                    }
		        }
    		}
    	}
    }	
    
    
    snprintf(cmd.reason, sizeof(cmd.reason), "INVL|18|!invalid command.|");

    cmd.indexOfPlayer = indexOfPlayer;

    switch (cmd.type) {
        case PLAY:
	        // printf("waitingNum(%d), param1(%s)\n", ttts->waitingNum, cmd.param1);
		//  only param1    but this will never be used fr......
            if (strlen(cmd.param2) != 0 || strlen(cmd.param3) != 0)
            {
                cmd.type = INVALID_CMD;
                printf("invalid cmd PLAY");
            }
            
            strncpy(ttts->waitingPlayer[cmd.indexOfPlayer].name, cmd.param1, sizeof(ttts->waitingPlayer[cmd.indexOfPlayer].name));
            break;
        case MOVE:
            cmd.gameIndex = -1;
            for (int i = 0; i < ttts->gamesNum; i++) {
                if (ttts->waitingPlayer[indexOfPlayer].sock == ttts->games[i].X.sock
                    || ttts->waitingPlayer[indexOfPlayer].sock == ttts->games[i].O.sock) {
                    cmd.gameIndex = i;
                    break;
                }
            }
            if (cmd.gameIndex == -1) {
                printf("error move, cannot find the correct game for this player move\n");
                cmd.type = INVALID_CMD;
            }
	        //check para1 yes param2 yes param3 no
            if (strlen(cmd.param3) != 0)
            {
                cmd.type = INVALID_CMD;
                printf("invalid cmd MOVE");
            }
	    printf("cmd.param1(%s)\n", cmd.param1);
	    if(ttts->waitingPlayer[indexOfPlayer].isX && cmd.param1[0] == 'O'){
	    	cmd.type = INVALID_CMD;
		break;
	    }
	    if (!ttts->waitingPlayer[indexOfPlayer].isX && cmd.param1[0] == 'X'){
	   	cmd.type = INVALID_CMD;
		break;
	    }
	    if (strlen(cmd.param2) != 3 || cmd.param2[1] != ',') {
                printf("illegal move position\n");
                cmd.type = INVALID_CMD;
            } else {
                cmd.pos.row = atoi(cmd.param2);
                cmd.pos.column = atoi(&cmd.param2[2]);
            }

            // check turn
            if (ttts->waitingPlayer[indexOfPlayer].isX && ttts->games[cmd.gameIndex].turn != 0) {
                cmd.type = INVALID_CMD;
                snprintf(cmd.reason, sizeof(cmd.reason), "INVL|22|!it is not your turn.|");
            }

            if (!ttts->waitingPlayer[indexOfPlayer].isX && ttts->games[cmd.gameIndex].turn != 1) {
                cmd.type = INVALID_CMD;
                snprintf(cmd.reason, sizeof(cmd.reason), "INVL|22|!it is not your turn.|");
            }
            break;
        case RSGN:
	    // check params  should I also check error move below????????
        if (strlen(cmd.param1) != 0 || strlen(cmd.param2) != 0 || strlen(cmd.param3) != 0)
        {
            printf("invalid cmd RSGN");
            cmd.type = INVALID_CMD;
            // memset(&cmd, 0, sizeof(cmd));
        }
        
        case DRAW: // same with
            cmd.gameIndex = -1;
            for (int i = 0; i < ttts->gamesNum; i++) {
                if (ttts->waitingPlayer[indexOfPlayer].sock == ttts->games[i].X.sock
                    || ttts->waitingPlayer[indexOfPlayer].sock == ttts->games[i].O.sock) {
                    cmd.gameIndex = i;
                    break;
                }
            }
            if (cmd.gameIndex == -1) {
                printf("error move, cannot find the correct game for this player move\n");
            	cmd.type = INVALID_CMD;
            }
	    // check params
        if (strlen(cmd.param2) != 0 || strlen(cmd.param3) != 0)
        {
            printf("invalid cmd DRAW");
            cmd.type = INVALID_CMD;
            // memset(&cmd, 0, sizeof(cmd));
        }
            break;
        default:
            printf("what cmd?? default type!! WRONG!!!!!\n");
    }

    if (cmd.type == INVALID_CMD) {
        int n = write(ttts->waitingPlayer[indexOfPlayer].sock, cmd.reason, strlen(cmd.reason));
        if (n < 0) {
            printf("response INVL error");
        }
    }

    // printf("parse cmd length(%d) param1:(%s), param2(%s), param3(%s), gameIndex(%d)\n", len, cmd.param1, cmd.param2, cmd.param3, cmd.gameIndex);

    return cmd;
}

bool equals(char c1, char c2, char c3, char c4)
{
    return (c1 == c2 && c2 == c3 && c3 == c4) ? true : false;
}

bool checkResult(Game *games)
{
    // bool win = false;
    // X wins
    // 3 in a row
    
    if (equals(games->board[0], games->board[1], games->board[2], 'X') ||  
    equals(games->board[3], games->board[4], games->board[5], 'X') || 
    equals(games->board[6], games->board[7], games->board[8], 'X') || 
    equals(games->board[0], games->board[3], games->board[6], 'X') ||  
    equals(games->board[1], games->board[4], games->board[7], 'X') || 
    equals(games->board[2], games->board[5], games->board[8], 'X') ||
    equals(games->board[0], games->board[4], games->board[8], 'X') ||
    equals(games->board[2], games->board[4], games->board[6], 'X'))
    {
        games->result = 1;
        return true;
    }    
    
    // O wins
    if (equals(games->board[0], games->board[1], games->board[2], 'O') ||  
    equals(games->board[3], games->board[4], games->board[5], 'O') || 
    equals(games->board[6], games->board[7], games->board[8], 'O') || 
    equals(games->board[0], games->board[3], games->board[6], 'O') ||  
    equals(games->board[1], games->board[4], games->board[7], 'O') || 
    equals(games->board[2], games->board[5], games->board[8], 'O') ||
    equals(games->board[0], games->board[4], games->board[8], 'O') ||
    equals(games->board[2], games->board[4], games->board[6], 'O'))
    {
        games->result = 2;
        return true;
    }
    bool full = true;
    // draw by grid full with no one wins
    for (int i = 0; i < 9; i++)
    {
        if (games->board[i] == '.')
        {
            full = false;
            break;
        }
    }
    // grid really full and no one wins
    if (full)
    {
        games->result = 3;
        return true;
    }


    // nothing    
    return false;
}

int Play(TTTS *ttts, Cmd cmd)
{
    //
    // only after pairing
    // it's ok but waitPlayer will increase, fix it. 
    // DONE! for loop at the top of the AddWaitingPlayer().

    // bool here = false;
    if (ttts->waitingPlayer[cmd.indexOfPlayer].playing) 
    {
        //send INVL
        printf("already in the game with name: %s\n", ttts->waitingPlayer[cmd.indexOfPlayer].name);
        int n = write(ttts->waitingPlayer[cmd.indexOfPlayer].sock, "INVL|30|!you are already in the game.|", sizeof("INVL|30|!you are already in the game.|")-1);
        if (n < 0) {
            printf("response INVL error");
        }
        return -1;
    }
    
    
    // check name is too long
    if (strlen(cmd.param1) > 64) {
        printf("play cmd over max name len\n");
        int n = write(ttts->waitingPlayer[cmd.indexOfPlayer].sock, "INVL|18|!name is too long|", sizeof("INVL|18|!name is too long|")-1);
        if (n < 0) {
            printf("response INVL error");
        }
        return -1;
    }

    // check name already existed
    for (int i = 0; i < ttts->waitingNum; i++) {
        if (i != cmd.indexOfPlayer) {
            if (0 == strcmp(cmd.param1, ttts->waitingPlayer[i].name)) {
                printf("player %s existed\n", cmd.param1);
                int n = write(ttts->waitingPlayer[cmd.indexOfPlayer].sock, "INVL|13|!name exists|", sizeof("INVL|13|!name exists|")-1);
                if (n < 0) {
                    printf("response INVL error");
                }
                // delete the current player.
                // for (int j = i; j < ttts->waitingNum-1; j++) {
                //     ttts->waitingPlayer[i] = ttts->waitingPlayer[i+1];
                // }
                // ttts->waitingNum--;
                return -1;
            }
        }
    }

    // valid name
    //snprintf(ttts->waitingPlayer[cmd.indexOfPlayer].name, sizeof(ttts->waitingPlayer[cmd.indexOfPlayer].name), "%s", cmd.param1);
    strncpy(ttts->waitingPlayer[cmd.indexOfPlayer].name, cmd.param1, sizeof(ttts->waitingPlayer[cmd.indexOfPlayer].name) - 1);

    // send wait
    printf("send WAIT(WAIT|0|)\n");
    int n = write(ttts->waitingPlayer[cmd.indexOfPlayer].sock, "WAIT|0|", strlen("WAIT|0|"));
    // printf("ttts->waitingPlayer[cmd.indexOfPlayer].sock(%d), n(%d)\n", ttts->waitingPlayer[cmd.indexOfPlayer].sock, n);
    if (n <= 0) {
        printf("response WAIT error");
        return -1;
    }

    // pair for the current player
    int pairIndex = -1;
    // printf("ttts->waitingNum(%d), cmd.indexOfPlayer(%d)\n", ttts->waitingNum, cmd.indexOfPlayer);
    for (int i = 0; i < ttts->waitingNum; i++) {
        // printf("ttts->waitingPlayer[%d].playing(%d), ttts->waitingPlayer[%d].name(%s)\n", i, ttts->waitingPlayer[i].playing, i, ttts->waitingPlayer[i].name);
        if (i != cmd.indexOfPlayer) {
            if (!ttts->waitingPlayer[i].playing && strlen(ttts->waitingPlayer[i].name) > 0) {
                pairIndex = i;
                break;
            }
        }
    }
    if (pairIndex != -1) {
        ttts->waitingPlayer[pairIndex].playing = true;
        ttts->waitingPlayer[cmd.indexOfPlayer].playing = true;
        ttts->games = realloc(ttts->games, (ttts->gamesNum+1)*sizeof(Game));
        ttts->gamesNum++;

        memset(&ttts->games[ttts->gamesNum-1], 0, sizeof(Game));

        ttts->waitingPlayer[pairIndex].isX = true;
        ttts->games[ttts->gamesNum-1].X = ttts->waitingPlayer[pairIndex];
        ttts->waitingPlayer[cmd.indexOfPlayer].isX = false;
        ttts->games[ttts->gamesNum-1].O = ttts->waitingPlayer[cmd.indexOfPlayer];
        
        for (int i = 0; i < sizeof(ttts->games[ttts->gamesNum-1].board); i++) {
            ttts->games[ttts->gamesNum-1].board[i] = '.';
        }
        
        char begin[256];

        // send BEGIN X
        snprintf(begin, sizeof(begin), "BEGN|%lu|X|%s|", 3+strlen(ttts->games[ttts->gamesNum-1].O.name), ttts->games[ttts->gamesNum-1].O.name);
        printf("send to X BEGIN (%s)\n", begin);
        n = write(ttts->games[ttts->gamesNum-1].X.sock, begin, strlen(begin));
        if (n <= 0) {
            printf("response BEGIN error");
            return -1;
        }

        // send BEGIN O
        snprintf(begin, sizeof(begin), "BEGN|%lu|O|%s|", 3+strlen(ttts->games[ttts->gamesNum-1].X.name), ttts->games[ttts->gamesNum-1].X.name);
        printf("send to O BEGIN (%s)\n", begin);
        n = write(ttts->games[ttts->gamesNum-1].O.sock, begin, strlen(begin));
        if (n <= 0) {
            printf("response BEGIN error");
            return -1;
        }

        // set turn to X
        ttts->games[ttts->gamesNum-1].turn = 0;
    }

    return 0;
}

int Move(TTTS *ttts, Cmd cmd)
{
    // return value for write()
    int n;
    
    //parse cmd -> parse in real row and col
    // check out of bound in parse()   -------- no, check here cuz want to send INVL together
    // check turn correctness in parse()  ---- for extra, shouldn't check turn. In other word, check partial turn.
    int position = (cmd.pos.row - 1) * 3 + (cmd.pos.column - 1);
    char invalid[256];

    

    //check out of bounds
    if (cmd.pos.row < 1 || cmd.pos.row > 3 || cmd.pos.column < 1 || cmd.pos.column > 3)
    {
        snprintf(invalid, sizeof(invalid), "INVL|15|Out of bounds.|");
        printf("send to O INVL (%s)\n", invalid);
        n = write(ttts->waitingPlayer[cmd.indexOfPlayer].sock, invalid, strlen(invalid));
        if (n <= 0) {
            printf("response INVL error");
        }
        return -1;
    }
    

    //check position occupied
    if (ttts->games[cmd.gameIndex].board[position] != '.')
    {
        snprintf(invalid, sizeof(invalid), "INVL|24|That space is occupied.|");
        printf("send to player INVL (%s)\n", invalid);
        n = write(ttts->waitingPlayer[cmd.indexOfPlayer].sock, invalid, strlen(invalid));
        if (n <= 0) {
            printf("response INVL error");
        }
        return -1;
    } else {
        // valid move so far
        //set board
        if (0 == strcmp(cmd.param1, "X"))
        {
            ttts->games[cmd.gameIndex].board[position] = 'X';
        } else {
            ttts->games[cmd.gameIndex].board[position] = 'O';
        }

        //check result W L D
        bool end = checkResult(&ttts->games[cmd.gameIndex]);
        char over[256];

        if(end){
            if (ttts->games[cmd.gameIndex].result == 1)
            {
                // send OVER to X
                
                snprintf(over, sizeof(over), "OVER|%lu|W|%s has completed a line.|", 
                                strlen(ttts->games[cmd.gameIndex].X.name) + 25,
                                ttts->games[cmd.gameIndex].X.name);

                printf("send OVER to X (%s)\n", over);
                n = write(ttts->games[cmd.gameIndex].X.sock, over, strlen(over));
                if (n <= 0) {
                    printf("response OVER error");
                    return -1;
                }
                // send OVER to O
                
                snprintf(over, sizeof(over), "OVER|%lu|L|%s has completed a line.|", 
                            strlen(ttts->games[cmd.gameIndex].X.name) + 25,
                            ttts->games[cmd.gameIndex].X.name);
                
                printf("send OVER to O (%s)\n", over);
                n = write(ttts->games[cmd.gameIndex].O.sock, over, strlen(over));
                if (n <= 0) {
                    printf("response OVER error");
                    return -1;
                }

            } else if (ttts->games[cmd.gameIndex].result == 2)
            {
                // send OVER to X

                snprintf(over, sizeof(over), "OVER|%lu|L|%s has completed a line.|", 
                            strlen(ttts->games[cmd.gameIndex].O.name) + 25,
                            ttts->games[cmd.gameIndex].O.name);

                printf("send OVER to X (%s)\n", over);
                n = write(ttts->games[cmd.gameIndex].X.sock, over, strlen(over));
                if (n <= 0) {
                    printf("response OVER error");
                    return -1;
                }
                // send OVER to O

                snprintf(over, sizeof(over), "OVER|%lu|W|%s has completed a line.|", 
                            strlen(ttts->games[cmd.gameIndex].O.name) + 25,
                            ttts->games[cmd.gameIndex].O.name);
                
                printf("send OVER to O (%s)\n", over);
                n = write(ttts->games[cmd.gameIndex].O.sock, over, strlen(over));
                if (n <= 0) {
                    printf("response OVER error");
                    return -1;
                }
            } else {
                // draw grid full

                // send OVER to X
                snprintf(over, sizeof(over), "OVER|20|D|the grid is full.|");
                printf("send OVER to X (%s)\n", over);
                n = write(ttts->games[cmd.gameIndex].X.sock, over, strlen(over));
                if (n <= 0) {
                    printf("response OVER error");
                    return -1;
                }
                // send OVER to O
                snprintf(over, sizeof(over), "OVER|20|D|the grid is full.|");
                printf("send OVER to O (%s)\n", over);
                n = write(ttts->games[cmd.gameIndex].O.sock, over, strlen(over));
                if (n <= 0) {
                    printf("response OVER error");
                    return -1;
                }
            }

            destroyOneGame(ttts, cmd.gameIndex);
            return 0;
        }



        char MOVD[512];
        // send MOVD X
        snprintf(MOVD, sizeof(MOVD), "MOVD|16|%s|%d,%d|%c%c%c%c%c%c%c%c%c|", cmd.param1, cmd.pos.row, cmd.pos.column,
	    ttts->games[cmd.gameIndex].board[0],ttts->games[cmd.gameIndex].board[1],ttts->games[cmd.gameIndex].board[2],ttts->games[cmd.gameIndex].board[3],ttts->games[cmd.gameIndex].board[4],
            ttts->games[cmd.gameIndex].board[5],ttts->games[cmd.gameIndex].board[6],ttts->games[cmd.gameIndex].board[7],ttts->games[cmd.gameIndex].board[8]);		
        printf("send to X MOVD (%s)\n", MOVD);
        n = write(ttts->games[cmd.gameIndex].X.sock, MOVD, strlen(MOVD));
        if (n <= 0) {
            printf("response MOVD error");
            return -1;
        }

        // send MOVD O  
        // snprintf(MOVD, sizeof(MOVD), "MOVD|O|%d,%d||", cmd.pos.row, cmd.pos.column);
        printf("send to O MOVD (%s)\n", MOVD);
        n = write(ttts->games[cmd.gameIndex].O.sock, MOVD, strlen(MOVD));
        if (n <= 0) {
            printf("response MOVD error");
            return -1;
        }
        
        // set turn
        if (0 == strcmp(cmd.param1, "X"))
        {
            ttts->games[cmd.gameIndex].turn = 1;
        } else{
            ttts->games[cmd.gameIndex].turn = 0;
        }
    }

    ttts->games[cmd.gameIndex].draw = 0;
    return 0;
}

int Draw(TTTS *ttts, Cmd cmd)
{
    int ret = 0;
    Game *games = &ttts->games[cmd.gameIndex];
    char response[256];
    int n;
    switch (games->draw) {
        case 0: // none draw
            if (0 == strcmp(cmd.param1, "S")) {
                snprintf(response, sizeof(response), "DRAW|2|S|");
                if (ttts->waitingPlayer[cmd.indexOfPlayer].isX) {
                    games->draw = 1;    // X draw request
                    printf("send to player O (%s)\n", response);
                    n = write(games->O.sock, response, strlen(response));
                    if (n <= 0) {
                        printf("response DRAW error");
                        ret = -1;
                    }
                } else {
                    games->draw = 2;    // O draw request
                    printf("send to player X (%s)\n", response);
                    n = write(games->X.sock, response, strlen(response));
                    if (n <= 0) {
                        printf("response DRAW error");
                        ret = -1;
                    }
                }
            } else {
                snprintf(response, sizeof(response), "INVL|15|!invalid draw.|");
                printf("send to player INVL (%s)\n", response);
                n = write(ttts->waitingPlayer[cmd.indexOfPlayer].sock, response, strlen(response));
                if (n <= 0) {
                    printf("response INVL error");
                    ret = -1;
                }
            }
            break;
        case 1:
            if (!ttts->waitingPlayer[cmd.indexOfPlayer].isX) {
                if (0 == strcmp(cmd.param1, "A")) {
                    snprintf(response, sizeof(response), "OVER|2|D|");
                    printf("send to player O (%s)\n", response);
                    n = write(games->O.sock, response, strlen(response));
                    if (n <= 0) {
                        printf("response DRAW error");
                    }
                    printf("send to player X (%s)\n", response);
                    n = write(games->X.sock, response, strlen(response));
                    if (n <= 0) {
                        printf("response DRAW error");
                    }
                    games->draw = 3;
                    destroyOneGame(ttts, cmd.gameIndex);
                } else if (0 == strcmp(cmd.param1, "R")) {
                    snprintf(response, sizeof(response), "DRAW|2|R|");
                    printf("send to player X (%s)\n", response);
                    n = write(games->X.sock, response, strlen(response));
                    if (n <= 0) {
                        printf("response DRAW error");
                        ret = -1;
                    }
                    games->draw = 0;
                } else {
                    // invl
                    snprintf(response, sizeof(response), "INVL|15|!invalid draw.|");
                    printf("send to player INVL (%s)\n", response);
                    n = write(ttts->waitingPlayer[cmd.indexOfPlayer].sock, response, strlen(response));
                    if (n <= 0) {
                        printf("response INVL error");
                    }
                    ret = -1;
                }
            } else {
                snprintf(response, sizeof(response), "INVL|15|!invalid draw.|");
                printf("send to player INVL (%s)\n", response);
                n = write(ttts->waitingPlayer[cmd.indexOfPlayer].sock, response, strlen(response));
                if (n <= 0) {
                    printf("response INVL error");
                }
                ret = -1;
            }
            break;
        case 2:
            if (ttts->waitingPlayer[cmd.indexOfPlayer].isX) {
                if (0 == strcmp(cmd.param1, "A")) {
                    snprintf(response, sizeof(response), "OVER|2|D|");
                    printf("send to player O (%s)\n", response);
                    n = write(games->O.sock, response, strlen(response));
                    if (n <= 0) {
                        printf("response DRAW error");
                    }
                    printf("send to player X (%s)\n", response);
                    n = write(games->X.sock, response, strlen(response));
                    if (n <= 0) {
                        printf("response DRAW error");
                        ret = -1;
                    }
                    games->draw = 3;
                    destroyOneGame(ttts, cmd.gameIndex);
                } else if (0 == strcmp(cmd.param1, "R")) {
                    snprintf(response, sizeof(response), "DRAW|2|R|");
                    printf("send to player O (%s)\n", response);
                    n = write(games->O.sock, response, strlen(response));
                    if (n <= 0) {
                        printf("response DRAW error");
                        ret = -1;
                    }
                    games->draw = 0;
                } else {
                    // invl
                    snprintf(response, sizeof(response), "INVL|15|!invalid draw.|");
                    printf("send to player INVL (%s)\n", response);
                    n = write(ttts->waitingPlayer[cmd.indexOfPlayer].sock, response, strlen(response));
                    if (n <= 0) {
                        printf("response INVL error");
                    }
                    ret = -1;
                }
            } else {
                snprintf(response, sizeof(response), "INVL|15|!invalid draw.|");
                printf("send to player INVL (%s)\n", response);
                n = write(ttts->waitingPlayer[cmd.indexOfPlayer].sock, response, strlen(response));
                if (n <= 0) {
                    printf("response INVL error");
                }
                ret = -1;
            }
            break;
        case 3:
            printf("%d: should not reach here\n", __LINE__);
            ret = -1;
            break;
        default:
            printf("%d: draw default\n", __LINE__);
            ret = -1;
    }

    return ret;
}

int Resign(TTTS *ttts, Cmd cmd)
{
    char over[256];
    int n;
    Game *games = &ttts->games[cmd.gameIndex];

    if (ttts->waitingPlayer[cmd.indexOfPlayer].isX) {
        snprintf(over, sizeof(over), "OVER|%lu|W|%s has resigned.|", strlen(games->X.name) + 17, games->X.name);
        printf("send to O OVER (%s)\n", over);
        // Send OVER W to O
        n = write(games->O.sock, over, strlen(over));
        if (n <= 0) {
            printf("response OVER error");
            return -1;
        }

        // Send OVER L to X
        snprintf(over, sizeof(over), "OVER|%lu|L|%s has resigned.|", strlen(games->X.name) + 17, games->X.name);
        printf("send to X OVER (%s)\n", over);
        
        n = write(games->X.sock, over, strlen(over));
        if (n <= 0) {
            printf("response OVER error");
            return -1;
        }
    } else {
        snprintf(over, sizeof(over), "OVER|%lu|L|%s has resigned.|", strlen(games->O.name) + 17, games->O.name);
        printf("send to O OVER (%s)\n", over);
        // Send OVER W to O
        n = write(games->O.sock, over, strlen(over));
        if (n <= 0) {
            printf("response OVER error");
            return -1;
        }

        // Send OVER L to X
        snprintf(over, sizeof(over), "OVER|%lu|W|%s has resigned.|", strlen(games->O.name) + 17, games->O.name);
        printf("send to X OVER (%s)\n", over);
    
        n = write(games->X.sock, over, strlen(over));
        if (n <= 0) {
            printf("response OVER error");
            return -1;
        }
    }

    destroyOneGame(ttts, cmd.gameIndex);

    return 0;
}

int processCmd(TTTS *ttts, int indexOfPlayer, char *cmdStr)
{
    
    int result;
    Cmd cmd = parseCmd(ttts, indexOfPlayer, cmdStr);
    // if (cmd.type != RSGN && ttts->waitingPlayer[indexOfPlayer].paramLen == 0)
    // {
    //     /* code */
    // }
    
    switch(cmd.type) {
        case PLAY:
            result = Play(ttts, cmd);
            break;
        case MOVE:
            result = Move(ttts, cmd);
            break;
        case RSGN:
            result = Resign(ttts, cmd);
            break;
        case DRAW:
            result = Draw(ttts, cmd);
            break;
        default:
            result = -1;
            fprintf(stderr, "unknown cmd %d\n", cmd.type);
    }

    return result;
}

int start_server(TTTS *ttts)
{
    ttts->sock = open_listener(ttts->port, 8);
    //printf("ttts->sock(%d)\n", ttts->sock);
    if (ttts->sock < 0) {
        fprintf(stderr, "open_listener failed\n");
        return -1;
    }
    printf("server start!\n");

    
    fd_set fds;

    FD_ZERO(&fds);
    FD_ZERO(&ttts->rd_fds);

    FD_SET(ttts->sock, &ttts->rd_fds);
    int max_fd = ttts->sock;

    //char cmd[256];


    time_t now;
    int n;

    while (1) {
        fds = ttts->rd_fds;

        struct timeval tv = {0, 100 * 1000};
        int num = select(max_fd+1, &fds, NULL, NULL, &tv);
        if (num < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }
        // timeout check
        if (num == 0)
        {
            now = time(NULL);
            for (size_t i = 0; i < ttts->waitingNum; i++)
            {
                if (ttts->waitingPlayer[i].step != -1 && now - ttts->waitingPlayer[i].time_start >= 3)
                {
                    printf("presentation-level error, timeout.\n");
                    //write INVL
                    n = write(ttts->waitingPlayer[i].sock, "INVL|27|!presentation-level error.|", sizeof("INVL|27|!presentation-level error.|"));
                    if (n < 0) {
                        printf("response INVL error");
                    }
                    // printf("play(%s), step(%d), cmd(%s)\n", ttts->waitingPlayer[i].name, ttts->waitingPlayer[i].step, ttts->waitingPlayer[i].cmd);
                    DelWaitingPlayer(ttts, ttts->waitingPlayer[i].sock);
                }
            }
        }
        
        
        // printf("num = %d\n", num);
        if (FD_ISSET(ttts->sock, &fds)) {
            struct sockaddr_storage remote_host;
            socklen_t remote_host_len = sizeof(remote_host);
            int s = accept(ttts->sock, (struct sockaddr *)&remote_host, &remote_host_len);
            if (s < 0) {
                perror("accept");
                continue;
            }
            printf("accept (%d) num(%d) ok\n", s, num);
            FD_SET(s, &ttts->rd_fds);
            if (max_fd < s) {
                max_fd = s;
            }

            AddWaitingPlayer(ttts, s);

            if (num - 1 <= 0) {
                continue;
            }
        }

        // check which player is readable
	
	
    // char disconnect[256];
	for (int i = 0; i < ttts->waitingNum; i++) {
            if (FD_ISSET(ttts->waitingPlayer[i].sock, &fds)) {
                // memset(cmd, 0, sizeof(cmd));
		        // printf("play(%s), step(%d), cmd(%s)\n", ttts->waitingPlayer[i].name, ttts->waitingPlayer[i].step, ttts->waitingPlayer[i].cmd);
                if (ttts->waitingPlayer[i].step == -1)
                {
                    ttts->waitingPlayer[i].step = 0;
                    ttts->waitingPlayer[i].time_start = time(NULL);
                }
                
                
                if (ttts->waitingPlayer[i].step == 0)
                {
                    n = read(ttts->waitingPlayer[i].sock, ttts->waitingPlayer[i].cmd + ttts->waitingPlayer[i].cmdIndex, 5 - ttts->waitingPlayer[i].cmdIndex);
                    if (n <= 0) {
                        printf("presentation-level error.\n");
                        //write INVL
                        n = write(ttts->waitingPlayer[i].sock, "INVL|27|!presentation-level error.|", sizeof("INVL|27|!presentation-level error.|"));
                        if (n < 0) {
                            printf("response INVL error");
                        }
                        // printf("play(%s), step(%d), cmd(%s)\n", ttts->waitingPlayer[i].name, ttts->waitingPlayer[i].step, ttts->waitingPlayer[i].cmd);
                        DelWaitingPlayer(ttts, ttts->waitingPlayer[i].sock);
                        printf("read failed n(%d)\n", n);
                        continue;
                    }
                    ttts->waitingPlayer[i].cmdIndex += n;
                    if (ttts->waitingPlayer[i].cmdIndex == 5)
                    {
                        if (ttts->waitingPlayer[i].cmd[4] != '|')
                        {
                            printf("presentation-level error.\n");
                            //write INVL
                            n = write(ttts->waitingPlayer[i].sock, "INVL|27|!presentation-level error.|", sizeof("INVL|27|!presentation-level error.|"));
                            if (n < 0) {
                                printf("response INVL error");
                            }
                            // printf("play(%s), step(%d), cmd(%s)\n", ttts->waitingPlayer[i].name, ttts->waitingPlayer[i].step, ttts->waitingPlayer[i].cmd);
                            DelWaitingPlayer(ttts, ttts->waitingPlayer[i].sock);
                            continue;
                        }
                        
                        ttts->waitingPlayer[i].step = 1;
                    }

                    // printf("play(%s), step(%d), cmd(%s)\n", ttts->waitingPlayer[i].name, ttts->waitingPlayer[i].step, ttts->waitingPlayer[i].cmd);
                    
                } else if(ttts->waitingPlayer[i].step == 1){
                    n = read(ttts->waitingPlayer[i].sock, ttts->waitingPlayer[i].cmd + ttts->waitingPlayer[i].cmdIndex, 1);
                    if (n <= 0) {
                        DelWaitingPlayer(ttts, ttts->waitingPlayer[i].sock);
                        
                        printf("fd_clear %d\n", ttts->waitingPlayer[i].sock);
                        // FD_CLR(ttts->waitingPlayer[i].sock, &rd_fds);
                        // close(ttts->waitingPlayer[i].sock);
                        printf("read failed n(%d)\n", n);
                        continue;
                    }
                    

                    ttts->waitingPlayer[i].cmdIndex++;

                    if (ttts->waitingPlayer[i].cmd[ttts->waitingPlayer[i].cmdIndex - 1] != '|' &&
                        ('0' > ttts->waitingPlayer[i].cmd[ttts->waitingPlayer[i].cmdIndex - 1] || 
                         '9' < ttts->waitingPlayer[i].cmd[ttts->waitingPlayer[i].cmdIndex - 1]))
                    {
                        printf("presentation-level error.\n");
                        //write INVL
                        n = write(ttts->waitingPlayer[i].sock, "INVL|27|!presentation-level error.|", sizeof("INVL|27|!presentation-level error.|"));
                        if (n < 0) {
                            printf("response INVL error");
                        }
                        // printf("play(%s), step(%d), cmd(%s)\n", ttts->waitingPlayer[i].name, ttts->waitingPlayer[i].step, ttts->waitingPlayer[i].cmd);
                        DelWaitingPlayer(ttts, ttts->waitingPlayer[i].sock);
                        continue;
                    }
                    
                    

                    if (ttts->waitingPlayer[i].cmd[ttts->waitingPlayer[i].cmdIndex - 1] == '|')
                    {
                        ttts->waitingPlayer[i].step = 2;
                        ttts->waitingPlayer[i].paramLen = atoi(ttts->waitingPlayer[i].cmd + 5);
                        //MOVE||123
                        if (ttts->waitingPlayer[i].cmdIndex <= 6)
                        {
                            printf("presentation-level error, length is missing.\n");
                            //write INVL
                            n = write(ttts->waitingPlayer[i].sock, "INVL|27|!presentation-level error.|", sizeof("INVL|27|!presentation-level error.|"));
                            if (n < 0) {
                                printf("response INVL error");
                            }
                            // printf("play(%s), step(%d), cmd(%s)\n", ttts->waitingPlayer[i].name, ttts->waitingPlayer[i].step, ttts->waitingPlayer[i].cmd);
                            DelWaitingPlayer(ttts, ttts->waitingPlayer[i].sock);
                            continue;
                        }
                        

			            if (ttts->waitingPlayer[i].paramLen == 0)
                        {
                            ttts->waitingPlayer[i].step = 3;
                            // printf("play(%s), step(%d), cmd(%s)\n", ttts->waitingPlayer[i].name, ttts->waitingPlayer[i].step, ttts->waitingPlayer[i].cmd);
                            // ttts->waitingPlayer[i].paramLen = atoi(ttts->waitingPlayer[i].cmd + 5);
                            processCmd(ttts, i, ttts->waitingPlayer[i].cmd);
                            ttts->waitingPlayer[i].step = -1;
                            memset(ttts->waitingPlayer[i].cmd, 0,sizeof(ttts->waitingPlayer[i].cmd));
                            ttts->waitingPlayer[i].paramLen = 0;
                            ttts->waitingPlayer[i].cmdIndex = 0;
                            ttts->waitingPlayer[i].readParamLen = 0;
                        }
                    } else {
                        //MOVE|5555X
                        if (ttts->waitingPlayer[i].cmdIndex >= 9)
                        {
                            printf("presentation-level error, length is too long.\n");
                            //write INVL
                            n = write(ttts->waitingPlayer[i].sock, "INVL|27|!presentation-level error.|", sizeof("INVL|27|!presentation-level error.|"));
                            if (n < 0) {
                                printf("response INVL error");
                            }
                            // printf("play(%s), step(%d), cmd(%s)\n", ttts->waitingPlayer[i].name, ttts->waitingPlayer[i].step, ttts->waitingPlayer[i].cmd);
                            DelWaitingPlayer(ttts, ttts->waitingPlayer[i].sock);
                            continue;
                        }
                        
                    }





                    //1234|abcdef|

                    
                    //
                    //            (249)
                    // e.g., 12345 678910
                    //         ^        ^
                //seem as protocol| cannot stop here
                    // if (ttts->waitingPlayer[i].cmdIndex)
                    // {
                    //     /* code */
                    // }
                    
                    

                    // printf("play(%s), step(%d), cmd(%s)\n", ttts->waitingPlayer[i].name, ttts->waitingPlayer[i].step, ttts->waitingPlayer[i].cmd);

                } else if(ttts->waitingPlayer[i].step == 2){
                    n = read(ttts->waitingPlayer[i].sock, ttts->waitingPlayer[i].cmd + ttts->waitingPlayer[i].cmdIndex, ttts->waitingPlayer[i].paramLen);
                    if (n <= 0) {
                        DelWaitingPlayer(ttts, ttts->waitingPlayer[i].sock);
                        
                        // printf("fd_clear %d\n", ttts->waitingPlayer[i].sock);
                        // FD_CLR(ttts->waitingPlayer[i].sock, &rd_fds);
                        // close(ttts->waitingPlayer[i].sock);
                        printf("read failed n(%d)\n", n);
                        continue;
                    }
                    ttts->waitingPlayer[i].cmdIndex += n;
                    ttts->waitingPlayer[i].readParamLen += n;
                    if (ttts->waitingPlayer[i].readParamLen == ttts->waitingPlayer[i].paramLen)
                    {
			        // what if i read longer than this cmd. But another one is read some bytes by this read. 
                    // It didn't terminate just send INVL and reread from client. use the return value in processCmd(-1 normal INVL -2 kill program)
			        // how about just kill it. whatever zhan bao or not! 
            		//MOVE|2|X|2,2| how to ????????????
                        // 
                        if (ttts->waitingPlayer[i].cmd[ttts->waitingPlayer[i].cmdIndex - 1] != '|')
                        {
                            printf("presentation-level error.\n");
                            //write INVL
                            n = write(ttts->waitingPlayer[i].sock, "INVL|27|!presentation-level error.|", sizeof("INVL|27|!presentation-level error.|"));
                            if (n < 0) {
                                printf("response INVL error");
                            }
                            // printf("play(%s), step(%d), cmd(%s)\n", ttts->waitingPlayer[i].name, ttts->waitingPlayer[i].step, ttts->waitingPlayer[i].cmd);
                            DelWaitingPlayer(ttts, ttts->waitingPlayer[i].sock);
                            continue;
                        }else{
                            ttts->waitingPlayer[i].step = 3;
                            // printf("play(%s), step(%d), cmd(%s)\n", ttts->waitingPlayer[i].name, ttts->waitingPlayer[i].step, ttts->waitingPlayer[i].cmd);
                            // ttts->waitingPlayer[i].paramLen = atoi(ttts->waitingPlayer[i].cmd + 5);
                            processCmd(ttts, i, ttts->waitingPlayer[i].cmd);
                            ttts->waitingPlayer[i].step = -1;
                            memset(ttts->waitingPlayer[i].cmd, 0 ,sizeof(ttts->waitingPlayer[i].cmd));
                            ttts->waitingPlayer[i].paramLen = 0;
                            ttts->waitingPlayer[i].cmdIndex = 0;
                            ttts->waitingPlayer[i].readParamLen = 0;
                        }
                    }
                    // else{
                    //     // THIS APPROACH IS NOT CORRECT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    //     // wrong param number situation  e.g., MOVE|7|X|2,2| (MOVE|6|X|2,2|  <- correct form)
                    //     printf("presentation-level error.\n");
                    //     //write INVL
                        
                    //     n = write(ttts->waitingPlayer[i].sock, "INVL|27|!presentation-level error.|", sizeof("INVL|27|!presentation-level error.|"));
                    //     if (n < 0) {
                    //         printf("response INVL error");
                    //     }
                    //     DelWaitingPlayer(ttts, ttts->waitingPlayer[i].sock);
                    // }
                }
                
                // n = read(ttts->waitingPlayer[i].sock, cmd, sizeof());
                // if (n <= 0) {
                //     DelWaitingPlayer(ttts, ttts->waitingPlayer[i].sock);
                //     printf("fd_clear %d\n", ttts->waitingPlayer[i].sock);
                //     // FD_CLR(ttts->waitingPlayer[i].sock, &rd_fds);
                //     // close(ttts->waitingPlayer[i].sock);
                //     printf("read failed n(%d)\n", n);
                //     continue;
                // }
                // processCmd(ttts, i, cmd);
                
                if (num - 1 <= 0) {
                    break;
                }
            }
        }

    }

    return 0;
}



int main(int argc, char *argv[])
{
    memset(&g_ttts, 0, sizeof(g_ttts));

    if (argc != 2) {
        printf("usage: %s port\n", argv[0]);
        return 1;
    }

    g_ttts.port = argv[1];
    if (atoi(g_ttts.port) == 0) {
        printf("usage: %s port\n", argv[0]);
        return 1;
    }

    start_server(&g_ttts);
    
    return 0;
}
