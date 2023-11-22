# Tic-Tac-Toe On-line test plan

## Qiyun Chen

* First of all, for each test case, it includes three parts. First part, the input commands I used for this test. Second, the log or received message from terminal. Third, the analysis for this test.
I use my ttt.c as my test code, which will transit inputs from STDIN to the socket and pass them to server. Thus, client will interact with server. And ttt will receive messages from server. Then, write in STDOUT as outputs.  

## 1. connection success and server receive PLAY, send WAIT, BEGN correctly

* input  

        player1: test001  
        player2: test002

* output

----------

* player1

        connect success sock 3
        Please input your name to begin this game:
        test001
        send play:(PLAY|8|test001|)
        WAIT|0|
        send play OK!
        BEGN|10|X|test002|

* player2

        connect success sock 3
        Please input your name to begin this game:
        test002
        send play:(PLAY|8|test002|)
        WAIT|0|
        send play OK!
        BEGN|10|O|test001|

* Server

        server start!
        accept (4) num(1) ok
        send WAIT(WAIT|0|)
        accept (5) num(1) ok
        send WAIT(WAIT|0|)
        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)

* Analysis

        By reading the log in server we test that connection correctly build. Server accept 2 sockets from player 1 and 2. Each time when server receive PLAY, i will send WAIT to client, which tests correctly by reading client's outputs. And after two player connect to server, it finds a pair to start a game. Then server sends BEGN to both side and assign their role. Also enemy's name.
        P.S., there is a prompt in client ui "Please input your name to begin this game:", which will let user only input their name and then it will be parsed to PLAY protocol and send to server.

## 2. one player wins also include MOVE

* input

        player1: test001, MOVE|6|X|1,2|, MOVE|6|X|2,2|, MOVE|6|X|3,2|
        player2: test002, MOVE|6|O|1,1|, MOVE|6|O|2,1|

* output

----------

* player1

        BEGN|10|X|test002|
        MOVE|6|X|1,2|
        MOVD|16|X|1,2|.X.......|
        MOVD|16|O|1,1|OX.......|
        MOVE|6|X|2,2|
        MOVD|16|X|2,2|OX..X....|
        MOVD|16|O|2,1|OX.OX....|
        MOVE|6|X|3,2|
        OVER|32|W|test001 has completed a line.|

* player2

        BEGN|10|O|test001|
        MOVD|16|X|1,2|.X.......|
        MOVE|6|O|1,1|
        MOVD|16|O|1,1|OX.......|
        MOVD|16|X|2,2|OX..X....|
        MOVE|6|O|2,1|
        MOVD|16|O|2,1|OX.OX....|
        OVER|32|L|test001 has completed a line.|

* Server

        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)
        send to X MOVD (MOVD|16|X|1,2|.X.......|)
        send to O MOVD (MOVD|16|X|1,2|.X.......|)
        send to X MOVD (MOVD|16|O|1,1|OX.......|)
        send to O MOVD (MOVD|16|O|1,1|OX.......|)
        send to X MOVD (MOVD|16|X|2,2|OX..X....|)
        send to O MOVD (MOVD|16|X|2,2|OX..X....|)
        send to X MOVD (MOVD|16|O|2,1|OX.OX....|)
        send to O MOVD (MOVD|16|O|2,1|OX.OX....|)
        send OVER to X (OVER|32|W|test001 has completed a line.|)
        send OVER to O (OVER|32|L|test001 has completed a line.|)
        destroyOneGame gameIndex 0
        delete X player fd(4)
        delete O player fd(5)

* Analysis

        By reading the log from server we test that MOVE and MOVD and OVER are received and sent correctly. Also, the functionality of the game tests correctly. After the game players are deleted correctly.

## 3. draw by full grid

* input

        player1: test001, MOVE|6|X|2,2|, MOVE|6|X|1,3|, MOVE|6|X|2,1|, MOVE|6|X|3,2|, MOVE|6|X|3,3|
        player2: test002, MOVE|6|O|1,1|, MOVE|6|O|3,1|, MOVE|6|O|2,3|, MOVE|6|O|1,2|

* output

----------

* player1

        BEGN|10|X|test002|
        MOVE|6|X|2,2|
        MOVD|16|X|2,2|....X....|
        MOVD|16|O|1,1|O...X....|
        MOVE|6|X|1,3|
        MOVD|16|X|1,3|O.X.X....|
        MOVD|16|O|3,1|O.X.X.O..|
        MOVE|6|X|2,1|
        MOVD|16|X|2,1|O.XXX.O..|
        MOVD|16|O|2,3|O.XXXOO..|
        MOVE|6|X|3,2|
        MOVD|16|X|3,2|O.XXXOOX.|
        MOVD|16|O|1,2|OOXXXOOX.|
        MOVE|6|X|3,3|
        OVER|20|D|the grid is full.|

* player2
        BEGN|10|O|test001|
        MOVD|16|X|2,2|....X....|
        MOVE|6|O|1,1|
        MOVD|16|O|1,1|O...X....|
        MOVD|16|X|1,3|O.X.X....|
        MOVE|6|O|3,1|
        MOVD|16|O|3,1|O.X.X.O..|
        MOVD|16|X|2,1|O.XXX.O..|
        MOVE|6|O|2,3|
        MOVD|16|O|2,3|O.XXXOO..|
        MOVD|16|X|3,2|O.XXXOOX.|
        MOVE|6|O|1,2|
        MOVD|16|O|1,2|OOXXXOOX.|
        OVER|20|D|the grid is full.|

* Server

        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)
        send to X MOVD (MOVD|16|X|2,2|....X....|)
        send to O MOVD (MOVD|16|X|2,2|....X....|)
        send to X MOVD (MOVD|16|O|1,1|O...X....|)
        send to O MOVD (MOVD|16|O|1,1|O...X....|)
        send to X MOVD (MOVD|16|X|1,3|O.X.X....|)
        send to O MOVD (MOVD|16|X|1,3|O.X.X....|)
        send to X MOVD (MOVD|16|O|3,1|O.X.X.O..|)
        send to O MOVD (MOVD|16|O|3,1|O.X.X.O..|)
        send to X MOVD (MOVD|16|X|2,1|O.XXX.O..|)
        send to O MOVD (MOVD|16|X|2,1|O.XXX.O..|)
        send to X MOVD (MOVD|16|O|2,3|O.XXXOO..|)
        send to O MOVD (MOVD|16|O|2,3|O.XXXOO..|)
        send to X MOVD (MOVD|16|X|3,2|O.XXXOOX.|)
        send to O MOVD (MOVD|16|X|3,2|O.XXXOOX.|)
        send to X MOVD (MOVD|16|O|1,2|OOXXXOOX.|)
        send to O MOVD (MOVD|16|O|1,2|OOXXXOOX.|)
        send OVER to X (OVER|20|D|the grid is full.|)
        send OVER to O (OVER|20|D|the grid is full.|)
        destroyOneGame gameIndex 0
        delete X player fd(4)
        delete O player fd(5)

* Analysis

        By reading the logs and the outputs, we test the functionality that draw with full grid correctly.

## 4. DRAW

* input

        player1:test001, DRAW|2|A|, DRAW|2|R|, DRAW|2|S|
        player2:test002, DRAW|2|R|, DRAW|2|A|

* output

----------

* player1

        BEGN|10|X|test002|
        DRAW|2|A|
        INVL|15|!invalid draw.|
        DRAW|2|R|
        INVL|15|!invalid draw.|
        DRAW|2|S|
        DRAW|2|R|
        DRAW|2|S|
        OVER|2|D|

* player2

        BEGN|10|O|test001|
        DRAW|2|S|
        DRAW|2|R|
        DRAW|2|S|
        DRAW|2|A|
        OVER|2|D|

* Server

        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)
        send to player INVL (INVL|15|!invalid draw.|)
        send to player INVL (INVL|15|!invalid draw.|)
        send to player O (DRAW|2|S|)
        send to player X (DRAW|2|R|)
        send to player O (DRAW|2|S|)
        send to player O (OVER|2|D|)
        send to player X (OVER|2|D|)
        destroyOneGame gameIndex 0
        delete X player fd(4)
        delete O player fd(5)

* Analysis

        By reading the outputs and logs, we test one player sends DRAW in a incorrect form. E.g., sending DRAW A or R without get a DRAW S. And we also test that send DRAW S and reply by DRAW R or DRAW A. By reading the logs and outputs in client, we know that server receive DRAW and send DRAW and handle DRAW then send OVER are correct.

## 5. invalid DRAW

Shown as above.

## 6. RSGN

input:
------------------

player1: test001, RSGN|0|
player2: test002
------------------

output:
------------------

player1:
        BEGN|10|X|test002|
        RSGN|0|
        OVER|24|L|test001 has resigned.|

player2:
        BEGN|10|O|test001|
        OVER|24|W|test001 has resigned.|
Server:
        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)
        send to O OVER (OVER|24|W|test001 has resigned.|)
        send to X OVER (OVER|24|L|test001 has resigned.|)
        destroyOneGame gameIndex 0
        delete X player fd(4)
        delete O player fd(5)
------------------

Analysis:
------------------

By reading outputs and logs, we know that RSGN tests correctly.
------------------

7. game-level error

input:
------------------

player1: test001, MOVE|6|X|2,2|
player2: test002, MOVE|6|O|2,2|, MOVE|6|O|5,5|, MOVE|6|O|1,1|
------------------

output:
------------------

player1:
        BEGN|10|X|test002|
        MOVE|6|X|2,2|
        MOVD|16|X|2,2|....X....|
        MOVD|16|O|1,1|O...X....|

player2:
        BEGN|10|O|test001|
        MOVD|16|X|2,2|....X....|
        MOVE|6|O|2,2|
        INVL|24|That space is occupied.|
        MOVE|6|O|5,5|
        INVL|15|Out of bounds.|
        MOVE|6|O|1,1|
        MOVD|16|O|1,1|O...X....|

Server:
        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)
        send to X MOVD (MOVD|16|X|2,2|....X....|)
        send to O MOVD (MOVD|16|X|2,2|....X....|)
        send to player INVL (INVL|24|That space is occupied.|)
        send to O INVL (INVL|15|Out of bounds.|)
        send to X MOVD (MOVD|16|O|1,1|O...X....|)
        send to O MOVD (MOVD|16|O|1,1|O...X....|)
------------------

Analysis:
------------------

For game-level errors, I consider two situations. One is when the MOVE out of the bound, another one is MOVE to a grid occupied. By reading the logs and outputs, we know that game-level errors are caught correctly. And server can still receive protocol, which means game isn't shuted down.
------------------

8. application-level error

input:
------------------

player1: test001, PLAY|8|test003|, MOVD|16|X|2,2|....X....|, OVER|0|, WAIT|0|
player2: test002,
------------------

output:
------------------

player1:
        BEGN|10|X|test002|
        PLAY|8|test003|
        INVL|30|!you are already in the game.|
        MOVD|16|X|2,2|....X....|
        INVL|18|!invalid command.|
        OVER|0|
        INVL|18|!invalid command.|
        WAIT|0|
        INVL|18|!invalid command.|

player2:
        BEGN|10|O|test001|

Server:
        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)
        already in the game with name: test003
        what cmd?? default type!! WRONG!!!!!
        unknown cmd 9
        what cmd?? default type!! WRONG!!!!!
        unknown cmd 9
        what cmd?? default type!! WRONG!!!!!
        unknown cmd 9
------------------

Analysis:
------------------

For application-level errors, I first test player sends PLAY again. And this will be caught if this player is already in the game. Second, if client sends other protocols that are not valid(beside PLAY, MOVE, DRAW, RSGN). My program will catch this error and send INVL protocol back. By reading logs and outputs, we know that application-level errors are tested correctly.
------------------

9. presentation-level error

test number smaller than real length
input:
------------------

player1: test001, MOVE|5|X|2,2|
player2: test002
------------------

output:
------------------

player1:
        BEGN|10|X|test002|
        MOVE|5|X|2,2|
        INVL|27|!presentation-level error.|
        qc104@ilab4:~/Desktop/test_socket/p3$

player2:
        BEGN|10|O|test001|
        qc104@ilab4:~/Desktop/test_socket/p3$

Server:
        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)
        presentation-level error.
        destroyOneGame gameIndex 0
        delete X player fd(4)
        delete O player fd(5)
------------------

test number larger than real length
input:
------------------

player1: test001, MOVE|8|X|2,2|
player2: test002
------------------

output:
------------------

player1:
        BEGN|10|X|test002|
        MOVE|8|X|2,2|
        INVL|27|!presentation-level error.|
        qc104@ilab4:~/Desktop/test_socket/p3$

player2:
        BEGN|10|O|test001|
        qc104@ilab4:~/Desktop/test_socket/p3$

Server:
        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)
        presentation-level error, timeout.
        destroyOneGame gameIndex 0
        delete X player fd(4)
        delete O player fd(5)
------------------

test completely wrong message
input:
------------------

player1: test001, this_is_a_message_that_entirely_incorrect
player2: test002
------------------

output:
------------------

player1:
        BEGN|10|X|test002|
        this_is_a_message_that_entirely_incorrect
        INVL|27|!presentation-level error.|
        qc104@ilab4:~/Desktop/test_socket/p3$

player2:
        BEGN|10|O|test001|
        qc104@ilab4:~/Desktop/test_socket/p3$

Server:
        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)
        presentation-level error.
        destroyOneGame gameIndex 0
        delete X player fd(5)
        delete O player fd(4)
------------------

Analysis:
------------------

By reading the logs and outputs above, we know that all of the presentation-level errors are caught by my program. And it will send INVL protocol and shut the game down. For the second test, when number is larger than real length of rest part. The read loop will keep waiting for data. For this situation, I set up a timeout in select, which will wait for 3 seconds. If there is no more data coming, it will stop read(). Then, if the number and the length is not same, I handle it as a presentation-level error.(also should be considered as a presentation-level error) Although, there will have some network problem to delay data, but I think 3 seconds are enough for network sends all the data.
------------------

10. session-level error

input:
------------------

player1: test001, ctrl + C
player2: test002
------------------

output:
------------------

player1:
        BEGN|10|X|test002|
        ^C
        qc104@ilab4:~/Desktop/test_socket/p3$

player2:
        BEGN|10|O|test001|
        qc104@ilab4:~/Desktop/test_socket/p3$

Server:
        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)
        session-level error.
        destroyOneGame gameIndex 0
        delete X player fd(4)
        delete O player fd(5)
        read failed n(36)

------------------

Analysis:
------------------

For session-level errors, I test that if one player send a signal that directly terminate his/her program. The select() will find that socket cannot be read anymore, which can help me to catch this error and report. By reading the logs and outputs, we know that server is not terminated and session-level error is caught by my program. Test correctly.
------------------

11. concurrent game

input:
------------------

player1: test001, MOVE|6|X|2,2|
player2: test002, MOVE|6|O|1,1|
player3: test003, MOVE|6|X|2,2|
player4: test004, MOVE|6|O|1,1|
------------------

output:
------------------

player1:
        BEGN|10|X|test002|
        MOVE|6|X|2,2|
        MOVD|16|X|2,2|....X....|
        MOVD|16|O|1,1|O...X....|

player2:
        BEGN|10|O|test001|
        MOVD|16|X|2,2|....X....|
        MOVE|6|O|1,1|
        MOVD|16|O|1,1|O...X....|

player3:
        BEGN|10|X|test004|
        MOVE|6|X|2,2|
        MOVD|16|X|2,2|....X....|
        MOVD|16|O|1,1|O...X....|

player4:
        BEGN|10|O|test003|
        MOVD|16|X|2,2|....X....|
        MOVE|6|O|1,1|
        MOVD|16|O|1,1|O...X....|

Server:
        server start!
        accept (4) num(1) ok
        send WAIT(WAIT|0|)
        accept (5) num(1) ok
        send WAIT(WAIT|0|)
        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)
        accept (6) num(1) ok
        send WAIT(WAIT|0|)
        accept (7) num(1) ok
        send WAIT(WAIT|0|)
        send to X BEGIN (BEGN|10|X|test004|)
        send to O BEGIN (BEGN|10|O|test003|)
        send to X MOVD (MOVD|16|X|2,2|....X....|)
        send to O MOVD (MOVD|16|X|2,2|....X....|)
        send to X MOVD (MOVD|16|X|2,2|....X....|)
        send to O MOVD (MOVD|16|X|2,2|....X....|)
        send to X MOVD (MOVD|16|O|1,1|O...X....|)
        send to O MOVD (MOVD|16|O|1,1|O...X....|)
        send to X MOVD (MOVD|16|O|1,1|O...X....|)
        send to O MOVD (MOVD|16|O|1,1|O...X....|)
------------------

Analysis:
------------------

By reading logs and outputs above, we can see that it's ok we have more than one game at the same time. This test shows concurrence tests correctly.
------------------

12. invalid PLAY
Shown at application-level errors

13. invalid name

input:
------------------

player1: test0000000000000000000000000000000000000000000000000000000000000000000000000, test001
player2: test001, test002
------------------

output:
------------------

player1:
        Please input your name to begin this game:
        test0000000000000000000000000000000000000000000000000000000000000000000000000
        send play:(PLAY|78|test0000000000000000000000000000000000000000000000000000000000000000000000000|)
        INVL|18|!name is too long|
        Please input your name to begin this game:
        test001
        send play:(PLAY|8|test001|)
        WAIT|0|
        send play OK!
        BEGN|10|X|test002|

player2:
        Please input your name to begin this game:
        test001
        send play:(PLAY|8|test001|)
        INVL|13|!name exists|
        Please input your name to begin this game:
        test002
        send play:(PLAY|8|test002|)
        WAIT|0|
        send play OK!
        BEGN|10|O|test001|

Server:
        accept (6) num(3) ok
        play cmd over max name len
        send WAIT(WAIT|0|)
        accept (7) num(3) ok
        player test001 existed
        send WAIT(WAIT|0|)
        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)
------------------

Analysis:
------------------

By reading logs and outputs above, we test when name is too long and name is existed.
------------------

14. interruption for 120 points purpose

input:
------------------

player1: test001, DRAW|2|R|, MOVE|6|X|2,2|, (interrupt)RSGN|0|,
player2: test002, (interrupt)MOVE|6|X|2,2|, (interrupt)MOVE|6|O|2,2|, (interrupt)DRAW|2|A|, (interrupt)DRAW|2|R|, (interrupt)DRAW|2|S|,  
------------------

output:
------------------

player1:
        BEGN|10|X|test002|
        DRAW|2|S|
        DRAW|2|R|
        MOVE|6|X|2,2|
        MOVD|16|X|2,2|....X....|
        RSGN|0|
        OVER|24|L|test001 has resigned.|

player2:
        BEGN|10|O|test001|
        MOVE|6|X|2,2|
        INVL|22|!it is not your turn.|
        MOVE|6|O|2,2|
        INVL|22|!it is not your turn.|
        DRAW|2|A|
        INVL|15|!invalid draw.|
        DRAW|2|R|
        INVL|15|!invalid draw.|
        DRAW|2|S|
        DRAW|2|R|
        MOVD|16|X|2,2|....X....|
        OVER|24|W|test001 has resigned.|

Server:
        send to X BEGIN (BEGN|10|X|test002|)
        send to O BEGIN (BEGN|10|O|test001|)
        unknown cmd 9
        unknown cmd 9
        send to player INVL (INVL|15|!invalid draw.|)
        send to player INVL (INVL|15|!invalid draw.|)
        send to player X (DRAW|2|S|)
        send to player O (DRAW|2|R|)
        send to X MOVD (MOVD|16|X|2,2|....X....|)
        send to O MOVD (MOVD|16|X|2,2|....X....|)
        send to O OVER (OVER|24|W|test001 has resigned.|)
        send to X OVER (OVER|24|L|test001 has resigned.|)
        destroyOneGame gameIndex 0
        delete X player fd(4)
        delete O player fd(5)
------------------

Analysis:
------------------

By reading logs and outputs above, we test that one player can interrupt the game if it is not his/her turn. But they can only interrupt with protocol DRAW and RSGN. And my program correctly handle this interruption, which satisfy the requirement of concurrence game with interruption.
------------------

## 15. OVER

Shown above.

## 16. X goes first

This is shown at interruption part. O cannot move at first.

All the tests above are run in one server, which means this server has not terminated at all. This tests the persistence of my server.
