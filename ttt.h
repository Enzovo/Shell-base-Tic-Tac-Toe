#include <stdbool.h>
#include <time.h>

typedef struct {
    char name[64];
    int sock;
    bool playing;
    bool isX;
    char cmd[256]; // single cmd
    int cmdIndex;
    int paramLen;  // PLAY|8| <- this number  
    int step;  // -1: free status 0: type  1: len 2:param 3: done   the steps read from sock
    int readParamLen;   // the len has been read from cmd
    time_t time_start;    //when parsing the first character
    // bool isHere; //for double play
} Player;

typedef struct 
{
    int row;
    int column;
} Position;


typedef struct {
    Player X;
    Player O;
    char board[9];
    int turn;       // 0: X, 1:O
    int result;     // 0: playing, 1: X wins, 2: O wins 3: draw grid full 4: draw from X 5: draw from O
    int draw;       // 0: none draw, 1: X draw request, 2: O draw request 3: over
    // Position pos;
} Game;

typedef struct {
    Game *games;
    int gamesNum;
    Player *waitingPlayer;
    int waitingNum;
    char* port;
    int sock;
    fd_set rd_fds;
} TTTS;

enum CMD {
    PLAY,
    WAIT,
    BEGN,
    MOVE,
    MOVD,
    INVL,
    RSGN,
    DRAW,
    OVER,
    INVALID_CMD
};

typedef struct {
    int type;
    int indexOfPlayer;
    char param1[256];
    char param2[256];
    char param3[256];
    Position pos;
    int gameIndex;
    char reason[128];
} Cmd;

//static const char* CmdName[] = {"PLAY", "WAIT", "BEGN", "MOVE", "MOVD", "INVL", "RSGN", "DRAW", "OVER"};
