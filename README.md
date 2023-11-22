# Tic-Tac-Toe On-line README

## Qiyun Chen

## 04/20/2023

    First of all, you can use command "make" to generate two executable file, ttts(server) and ttt(client, if needed).
    Then execute the server first with usage: ./ttts port_number. And execute the client(if needed) with usage: ./ttt domain port_number.

### My project is named "concurrent Tic-Tac-Toe games with interruption"

    It's an online Tic-Tac-Toe multiplayer game server with C language. select(), socket(), etc. methods in C standard library are applied. It will use the command line to start, move and win the game. There are several parts in the instructions, which followed a specific protocol.

    Protocols: 
    • PLAY name
    • WAIT
    • BEGN role name
    • MOVE role position
    • MOVD role position board
    • INVL reason
    • RSGN
    • DRAW message
    • OVER outcome reason 

    Formats:
    Messages are broken into fields. A field consists of one or more characters, followed by a vertical bar.
    The first field is always a four-character code. The second field gives the length of the remaining
    message in bytes, represented as string containing a decimal integer in the range 0–255. This length
    does not include the bar following the size field, so a message with no additional fields, such as
    WAIT, will give its size as 0. (Note that this documentation will generally omit the length field
    when discussing messages for simplicity.) Subsequent fields are variable-length strings.
    Note that a message will always end with a vertical bar. Implementations must use this to detect
    improperly formatted messages.
    The field types are:
    name Arbitrary text representing a player’s name.
    role Either X or O.
    position Two integers (1, 2, or 3) separated by a comma.
    board Nine characters representing the current state of the grid, using a period (.) for unclaimed
    grid cells, and X or O for claimed cells.
    reason Arbitrary text giving why a move was rejected or the game has ended.
    message One of S (suggest), A (accept), or R (reject).
    outcome One of W (win), L (loss), or D (draw).

    Rules:
    Following the classic Tic-Tac-Toe rules.

### My directory has

#### 1. ttts.c

    A .c file that implements the server part of Tic-Tac-Toe On-line. After setting up the socket stuffs(socket() -> bind() -> listen()) and then start the server(accept()). I choose method select() to handle the blocking read() when multiple games happen together. It will select from a fd_set to find a readable fd(socket), which means server can read from multiple sockets at same time without multithreading. And this file also includes games strategy, which will make sure tic-tac-toe on-line has same rule with the real game.(who wins or full grid draw, etc) Also, I handle all the errors that would happen in the game. Such as, game-level errors that try to move in an occupied grid or outside the board. I handle this with sending invalid message back to client with reason why this move is invalid and let the client move again. For application-level errors that a player send an incorrect message or send a message in incorrect timing(move when it is not his/her turn), my program will detect this and send invalid waring with explanation back to the player and let the player send correct one again. For presentation-level errors that an entire wrong message or message with wrong number, I send the invalid warning to player and shut down the game. For this level error, if a player send a wrong number that smaller than real length I will directly detect it and shut the game down. But when player send a wrong number that larger than real length, it is hard to detect due to the block for waiting data. Thus, I set up a timeout in select that will wait for 3 seconds to wait data comes. If not, I will handle it as an malformed message and shut the game down.(due to the network problem, it will be read several times to get a complete message. But wait for 3 seconds is long enough) At last, for session-level errors, when one player lost connection, the select() will detect that socket cannot be read further. Thus, the game is shuted down and a log will record this error in server, which means the server won't be shuted down. For these errors details will be shown more in testplan file.   

#### 2. ttt.h

    A header file that contains structs will be used in ttts.c file. E.g., Player Games ttts, etc.

#### 3. ttt.c

    A .c file that implements the client part of Tic-Tac-Toe On-line. I force the input by player to be as same as the protocol, which means my client will transit(write()) the entire cmd parsed to socket after connecting with server. For the special protocol PLAY, I handle it separately from other protocols. And, I use multithreading to separate data. Main thread is used to handle the input that read from socket and it will write it to the STDOUT. Work thread is used to get input from STDIN and it will write it to the socket that connect with server.
    And ttt.c is also my test code, I use the  file to interact with server and get the outputs.

#### 4. Makefile

    A Makefile to generate(compile and link) executable files. Generating with "make" command and clean the executable files that generated by "make" with command "make clean". And I add -fsanitize=address as a compile flag to make sure there is no memory leak. Also, I add compile -Wall -Werror to treat every warning as an error.

#### 5. README

    A readme file to describe my project.

#### 6. testplan

    A file includes all my test plans. See detail in testplan. As describe above in (1), I handle all of the 4 kinds of level errors. And my tic-tac-toe game has as same rule as the real game. All of these will be shown inside testplan file.
