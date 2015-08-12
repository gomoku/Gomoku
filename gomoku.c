  1 /* gomoku - 5 in a row game             Author: ? */
  2 
  3 /* This program plays a very old Japanese game called GO-MOKU,
  4    perhaps better known as  5-in-line.   The game is played on
  5    a board with 19 x 19 squares, and the object of the game is
  6    to get 5 stones in a row.
  7 */
  8 
  9 #include <sys/types.h>
 10 #include <curses.h>
 11 #include <ctype.h>
 12 #include <stdlib.h>
 13 #include <unistd.h>
 14 
 15 /* Size of the board */
 16 #define SIZE 19
 17 
 18 /* Importance of attack (1..16) */
 19 #define AttackFactor 4
 20 
 21 /* Value of having 0, 1,2,3,4 or 5 pieces in line */
 22 int Weight[7] = {0, 0, 4, 20, 100, 500, 0};
 23 
 24 #define Null 0
 25 #define Horiz 1
 26 #define DownLeft 2
 27 #define DownRight 3
 28 #define Vert 4
 29 
 30 /* The two players */
 31 #define Empty 0
 32 #define Cross 1
 33 #define Nought 2
 34 
 35 char PieceChar[Nought + 1] = {' ', 'X', ''};
 36 
 37 int Board[SIZE + 1][SIZE + 1];/* The board */
 38 int Player;                     /* The player whose move is next */
 39 int TotalLines;                 /* The number of Empty lines left */
 40 int GameWon;                    /* Set if one of the players has won */
 41 
 42 int Line[4][SIZE + 1][SIZE + 1][Nought + 1];
 43 
 44 /* Value of each square for each player */
 45 int Value[SIZE + 1][SIZE + 1][Nought + 1];
 46 
 47 int X, Y;                       /* Move coordinates */
 48 char Command;                   /* Command from keyboard */
 49 int AutoPlay = FALSE;           /* The program plays against itself */
 50 
 51 _PROTOTYPE(void Initialize, (void));
 52 _PROTOTYPE(int Abort, (char *s));
 53 _PROTOTYPE(void WriteLetters, (void));
 54 _PROTOTYPE(void WriteLine, (int j, int *s));
 55 _PROTOTYPE(void WriteBoard, (int N, int *Top, int *Middle, int *Bottom));
 56 _PROTOTYPE(void SetUpScreen, (void));
 57 _PROTOTYPE(void GotoSquare, (int x, int y));
 58 _PROTOTYPE(void PrintMove, (int Piece, int X, int Y));
 59 _PROTOTYPE(void ClearMove, (void));
 60 _PROTOTYPE(void PrintMsg, (char *Str));
 61 _PROTOTYPE(void ClearMsg, (void));
 62 _PROTOTYPE(void WriteCommand, (char *S));
 63 _PROTOTYPE(void ResetGame, (int FirstGame));
 64 _PROTOTYPE(int OpponentColor, (int Player));
 65 _PROTOTYPE(void BlinkRow, (int X, int Y, int Dx, int Dy, int Piece));
 66 _PROTOTYPE(void BlinkWinner, (int Piece, int X, int Y, int WinningLine));
 67 _PROTOTYPE(int Random, (int x));
 68 _PROTOTYPE(void Add, (int *Num));
 69 _PROTOTYPE(void Update, (int Lin[], int Valu[], int Opponent));
 70 _PROTOTYPE(void MakeMove, (int X, int Y));
 71 _PROTOTYPE(int GameOver, (void));
 72 _PROTOTYPE(void FindMove, (int *X, int *Y));
 73 _PROTOTYPE(char GetChar, (void));
 74 _PROTOTYPE(void ReadCommand, (int X, int Y, char *Command));
 75 _PROTOTYPE(void InterpretCommand, (int Command));
 76 _PROTOTYPE(void PlayerMove, (void));
 77 _PROTOTYPE(void ProgramMove, (void));
 78 _PROTOTYPE(int main, (void));
 79 
 80 /* Set terminal to raw mode. */
 81 void Initialize()
 82 {
 83   srand(getpid() + 13);         /* Initialize the random seed with our pid */
 84   initscr();
 85   raw();
 86   noecho();
 87   clear();
 88 }
 89 
 90 /* Reset terminal and exit from the program. */
 91 int Abort(s)
 92 char *s;
 93 {
 94   move(LINES - 1, 0);
 95   refresh();
 96   endwin();
 97   exit(0);
 98 }
 99 
100 /* Set up the screen ----------------------------------------------- */
101 
102 /* Write the letters */
103 void WriteLetters()
104 {
105   int i;
106 
107   addch(' ');
108   addch(' ');
109   for (i = 1; i <= SIZE; i++) printw(" %c", 'A' + i - 1);
110   addch('\n');
111 }
112 
113 /* Write one line of the board */
114 void WriteLine(j, s)
115 int j;
116 int *s;
117 {
118   int i;
119 
120   printw("%2d ", j);
121   addch(s[0]);
122   for (i = 2; i <= SIZE - 1; i++) {
123         addch(s[1]);
124         addch(s[2]);
125   }
126   addch(s[1]);
127   addch(s[3]);
128   printw(" %-2d\n", j);
129 }
130 
131 /* Print the Empty board and the border */
132 void WriteBoard(N, Top, Middle, Bottom)
133 int N;
134 int *Top, *Middle, *Bottom;
135 {
136   int j;
137 
138   move(1, 0);
139   WriteLetters();
140   WriteLine(N, Top);
141   for (j = N - 1; j >= 2; j--) WriteLine(j, Middle);
142   WriteLine(1, Bottom);
143   WriteLetters();
144 }
145 
146 /* Sets up the screen with an Empty board */
147 void SetUpScreen()
148 {
149   int top[4], middle[4], bottom[4];
150 
151   top[0] = ACS_ULCORNER;
152   top[1] = ACS_HLINE;
153   top[2] = ACS_TTEE;
154   top[3] = ACS_URCORNER;
155 
156   middle[0] = ACS_LTEE;
157   middle[1] = ACS_HLINE;
158   middle[2] = ACS_PLUS;
159   middle[3] = ACS_RTEE;
160 
161   bottom[0] = ACS_LLCORNER;
162   bottom[1] = ACS_HLINE;
163   bottom[2] = ACS_BTEE;
164   bottom[3] = ACS_LRCORNER;
165 
166   WriteBoard(SIZE, top, middle, bottom);
167 }
168 
169 /* Show moves ----------------------------------------------- */
170 
171 void GotoSquare(x, y)
172 int x, y;
173 {
174   move(SIZE + 2 - y, 1 + x * 2);
175 }
176 
177 /* Prints a move */
178 void PrintMove(Piece, X, Y)
179 int Piece;
180 int X, Y;
181 {
182   move(22, 49);
183   printw("%c %c %d", PieceChar[Piece], 'A' + X - 1, Y);
184   clrtoeol();
185   GotoSquare(X, Y);
186   addch(PieceChar[Piece]);
187   GotoSquare(X, Y);
188   refresh();
189 }
190 
191 /* Clears the line where a move is displayed */
192 void ClearMove()
193 {
194   move(22, 49);
195   clrtoeol();
196 }
197 
198 /* Message handling ---------------------------------------------- */
199 
200 /* Prints a message */
201 void PrintMsg(Str)
202 char *Str;
203 {
204   mvprintw(23, 1, "%s", Str);
205 }
206 
207 /* Clears the message about the winner */
208 void ClearMsg()
209 {
210   move(23, 1);
211   clrtoeol();
212 }
213 
214 /* Highlights the first letter of S */
215 void WriteCommand(S)
216 char *S;
217 {
218   standout();
219   addch(*S);
220   standend();
221   printw("%s", S + 1);
222 }
223 
224 /* Display the board ----------------------------------------------- */
225 
226 /* Resets global variables to start a new game */
227 void ResetGame(FirstGame)
228 int FirstGame;
229 {
230   int I, J;
231   int C, D;
232 
233   SetUpScreen();
234   if (FirstGame) {
235         move(1, 49);
236         addstr("G O M O K U");
237         move(3, 49);
238         WriteCommand("Newgame    ");
239         WriteCommand("Quit ");
240         move(5, 49);
241         WriteCommand("Auto");
242         move(7, 49);
243         WriteCommand("Play");
244         move(9, 49);
245         WriteCommand("Hint");
246         move(14, 60);
247         WriteCommand("Left, ");
248         WriteCommand("Right, ");
249         move(16, 60);
250         WriteCommand("Up, ");
251         WriteCommand("Down");
252         move(18, 60);
253         standout();
254         addstr("SPACE");
255         move(20, 49);
256         WriteCommand(" NOTE: Use Num Lock & arrows");
257         standend();
258         mvaddstr(14, 49, "7  8  9");
259         mvaddch(15, 52, ACS_UARROW);
260         mvaddch(16, 49, '4');
261         addch(ACS_LARROW);
262         mvaddch(16, 54, ACS_RARROW);
263         addch('6');
264         mvaddch(17, 52, ACS_DARROW);
265         mvaddstr(18, 49, "1  2  3");
266         FirstGame = FALSE;
267   } else {
268         ClearMsg();
269         ClearMove();
270   }
271 
272   /* Clear tables */
273   for (I = 1; I <= SIZE; I++) for (J = 1; J <= SIZE; J++) {
274                 Board[I][J] = Empty;
275                 for (C = Cross; C <= Nought; C++) {
276                         Value[I][J][C] = 0;
277                         for (D = 0; D <= 3; D++) Line[D][I][J][C] = 0;
278                 }
279         }
280 
281   /* Cross starts */
282   Player = Cross;
283   /* Total number of lines */
284   TotalLines = 2 * 2 * (SIZE * (SIZE - 4) + (SIZE - 4) * (SIZE - 4));
285   GameWon = FALSE;
286 }
287 
288 int OpponentColor(Player)
289 int Player;
290 {
291   if (Player == Cross)
292         return Nought;
293   else
294         return Cross;
295 }
296 
297 /* Blink the row of 5 stones */
298 void BlinkRow(X, Y, Dx, Dy, Piece)
299 int X, Y, Dx, Dy, Piece;
300 {
301   int I;
302 
303   attron(A_BLINK);
304   for (I = 1; I <= 5; I++) {
305         GotoSquare(X, Y);
306         addch(PieceChar[Piece]);
307         X = X - Dx;
308         Y = Y - Dy;
309   }
310   attroff(A_BLINK);
311 }
312 
313 /* Prints the 5 winning stones in blinking color */
314 void BlinkWinner(Piece, X, Y, WinningLine)
315 int Piece, X, Y, WinningLine;
316 {
317   /* Used to store the position of the winning move */
318   int XHold, YHold;
319   /* Change in X and Y */
320   int Dx, Dy;
321 
322   /* Display winning move */
323   PrintMove(Piece, X, Y);
324   /* Preserve winning position */
325   XHold = X;
326   YHold = Y;
327   switch (WinningLine) {
328       case Horiz:
329         {
330                 Dx = 1;
331                 Dy = 0;
332                 break;
333         }
334 
335       case DownLeft:
336         {
337                 Dx = 1;
338                 Dy = 1;
339                 break;
340         }
341 
342       case Vert:
343         {
344                 Dx = 0;
345                 Dy = 1;
346                 break;
347         }
348 
349       case DownRight:
350         {
351                 Dx = -1;
352                 Dy = 1;
353                 break;
354         }
355   }
356 
357   /* Go to topmost, leftmost */
358   while (Board[X + Dx][Y + Dy] != Empty && Board[X + Dx][Y + Dy] == Piece) {
359         X = X + Dx;
360         Y = Y + Dy;
361   }
362   BlinkRow(X, Y, Dx, Dy, Piece);
363   /* Restore winning position */
364   X = XHold;
365   Y = YHold;
366   /* Go back to winning square */
367   GotoSquare(X, Y);
368 }
369 
370 /* Functions for playing a game -------------------------------- */
371 
372 int Random(x)
373 int x;
374 {
375   return((rand() / 19) % x);
376 }
377 
378 /* Adds one to the number of pieces in a line */
379 void Add(Num)
380 int *Num;
381 {
382   /* Adds one to the number.     */
383   *Num = *Num + 1;
384   /* If it is the first piece in the line, then the opponent cannot use
385    * it any more.  */
386   if (*Num == 1) TotalLines = TotalLines - 1;
387   /* The game is won if there are 5 in line. */
388   if (*Num == 5) GameWon = TRUE;
389 }
390 
391 /* Updates the value of a square for each player, taking into
392    account that player has placed an extra piece in the square.
393    The value of a square in a usable line is Weight[Lin[Player]+1]
394    where Lin[Player] is the number of pieces already placed
395 in the line */
396 void Update(Lin, Valu, Opponent)
397 int Lin[];
398 int Valu[];
399 int Opponent;
400 {
401   /* If the opponent has no pieces in the line, then simply update the
402    * value for player */
403   if (Lin[Opponent] == 0)
404         Valu[Player] += Weight[Lin[Player] + 1] - Weight[Lin[Player]];
405   else
406         /* If it is the first piece in the line, then the line is
407          * spoiled for the opponent */
408   if (Lin[Player] == 1) Valu[Opponent] -= Weight[Lin[Opponent] + 1];
409 }
410 
411 /* Performs the move X,Y for player, and updates the global variables
412 (Board, Line, Value, Player, GameWon, TotalLines and the screen) */
413 void MakeMove(X, Y)
414 int X, Y;
415 {
416   int Opponent;
417   int X1, Y1;
418   int K, L, WinningLine;
419 
420   WinningLine = Null;
421   Opponent = OpponentColor(Player);
422   GameWon = FALSE;
423 
424   /* Each square of the board is part of 20 different lines. The adds
425    * one to the number of pieces in each of these lines. Then it
426    * updates the value for each of the 5 squares in each of the 20
427    * lines. Finally Board is updated, and the move is printed on the
428    * screen. */
429 
430   /* Horizontal lines, from left to right */
431   for (K = 0; K <= 4; K++) {
432         X1 = X - K;             /* Calculate starting point */
433         Y1 = Y;
434         if ((1 <= X1) && (X1 <= SIZE - 4)) {    /* Check starting point */
435                 Add(&Line[0][X1][Y1][Player]);  /* Add one to line */
436                 if (GameWon && (WinningLine == Null))   /* Save winning line */
437                         WinningLine = Horiz;
438                 for (L = 0; L <= 4; L++)        /* Update value for the
439                                                  * 5 squares in the line */
440                         Update(Line[0][X1][Y1], Value[X1 + L][Y1], Opponent);
441         }
442   }
443 
444   for (K = 0; K <= 4; K++) {    /* Diagonal lines, from lower left to
445                                  * upper right */
446         X1 = X - K;
447         Y1 = Y - K;
448         if ((1 <= X1) && (X1 <= SIZE - 4) &&
449             (1 <= Y1) && (Y1 <= SIZE - 4)) {
450                 Add(&Line[1][X1][Y1][Player]);
451                 if (GameWon && (WinningLine == Null))   /* Save winning line */
452                         WinningLine = DownLeft;
453                 for (L = 0; L <= 4; L++)
454                         Update(Line[1][X1][Y1], Value[X1 + L][Y1 + L], Opponent);
455         }
456   }                             /* for */
457 
458   for (K = 0; K <= 4; K++) {    /* Diagonal lines, down right to upper left */
459         X1 = X + K;
460         Y1 = Y - K;
461         if ((5 <= X1) && (X1 <= SIZE) &&
462             (1 <= Y1) && (Y1 <= SIZE - 4)) {
463                 Add(&Line[3][X1][Y1][Player]);
464                 if (GameWon && (WinningLine == Null))   /* Save winning line */
465                         WinningLine = DownRight;
466                 for (L = 0; L <= 4; L++)
467                         Update(Line[3][X1][Y1], Value[X1 - L][Y1 + L], Opponent);
468         }
469   }                             /* for */
470 
471   for (K = 0; K <= 4; K++) {    /* Vertical lines, from down to up */
472         X1 = X;
473         Y1 = Y - K;
474         if ((1 <= Y1) && (Y1 <= SIZE - 4)) {
475                 Add(&Line[2][X1][Y1][Player]);
476                 if (GameWon && (WinningLine == Null))   /* Save winning line */
477                         WinningLine = Vert;
478                 for (L = 0; L <= 4; L++)
479                         Update(Line[2][X1][Y1], Value[X1][Y1 + L], Opponent);
480         }
481   }
482 
483   Board[X][Y] = Player;         /* Place piece in board */
484   if (GameWon)
485         BlinkWinner(Player, X, Y, WinningLine);
486   else
487         PrintMove(Player, X, Y);/* Print move on screen */
488   Player = Opponent;            /* The opponent is next to move */
489 }
490 
491 int GameOver()
492 /* A game is over if one of the players have
493 won, or if there are no more Empty lines */
494 {
495   return(GameWon || (TotalLines <= 0));
496 }
497 
498 /* Finds a move X,Y for player, simply by picking the one with the
499 highest value */
500 void FindMove(X, Y)
501 int *X, *Y;
502 {
503   int Opponent;
504   int I, J;
505   int Max, Valu;
506 
507   Opponent = OpponentColor(Player);
508   Max = -10000;
509   /* If no square has a high value then pick the one in the middle */
510   *X = (SIZE + 1) / 2;
511   *Y = (SIZE + 1) / 2;
512   if (Board[*X][*Y] == Empty) Max = 4;
513   /* The evaluation for a square is simply the value of the square for
514    * the player (attack points) plus the value for the opponent
515    * (defense points). Attack is more important than defense, since it
516    * is better to get 5 in line yourself than to prevent the op- ponent
517    * from getting it. */
518 
519   /* For all Empty squares */
520   for (I = 1; I <= SIZE; I++) for (J = 1; J <= SIZE; J++)
521                 if (Board[I][J] == Empty) {
522                         /* Calculate evaluation */
523                         Valu = Value[I][J][Player] * (16 + AttackFactor) / 16 + Value[I][J][Opponent] + Random(4);
524                         /* Pick move with highest value */
525                         if (Valu > Max) {
526                                 *X = I;
527                                 *Y = J;
528                                 Max = Valu;
529                         }
530                 }
531 }
532 
533 char GetChar()
534 /* Get a character from the keyboard */
535 {
536   int c;
537 
538   c = getch();
539   if (c < 0) abort();
540   if (c == '\033') {    /* arrow key */
541         if ((c = getch()) == '[') {
542                 c = getch();
543                 switch (c) {
544                 case 'A': c = 'U'; break;
545                 case 'B': c = 'D'; break;
546                 case 'C': c = 'R'; break;
547                 case 'D': c = 'L'; break;
548                 default:
549                         c = '?';
550                         break;
551                 }
552         }
553         else
554                 c = '?';
555   }
556   if (islower(c))
557         return toupper(c);
558   else
559         return c;
560 }
561 
562 /* Reads in a valid command character */
563 void ReadCommand(X, Y, Command)
564 int X, Y;
565 char *Command;
566 {
567   int ValidCommand;
568 
569   do {
570         ValidCommand = TRUE;
571         GotoSquare(X, Y);       /* Goto square */
572         refresh();
573         *Command = GetChar();   /* Read from keyboard */
574         switch (*Command) {
575             case '\n':          /* '\n', '\r' or space means place a */
576             case '\r':
577             case ' ':
578                 *Command = 'E';
579                 break;          /* stone at the cursor position  */
580 
581             case 'L':
582             case 'R':
583             case 'U':
584             case 'D':
585             case '7':
586             case '9':
587             case '1':
588             case '3':
589             case 'N':
590             case 'Q':
591             case 'A':
592             case 'P':
593             case 'H':
594                 break;
595 
596             case '8':   *Command = 'U'; break;
597             case '2':   *Command = 'D'; break;
598             case '4':   *Command = 'L'; break;
599             case '6':   *Command = 'R'; break;
600             default:
601                 {
602                         if (GameOver())
603                                 *Command = 'P';
604                         else
605                                 ValidCommand = FALSE;
606                         break;
607                 }
608         }
609   } while (!ValidCommand);
610 }
611 
612 void InterpretCommand(Command)
613 char Command;
614 {
615   int Temp;
616 
617   switch (Command) {
618       case 'N':{                /* Start new game */
619                 ResetGame(FALSE);       /* ResetGame but only redraw
620                                          * the board */
621                 X = (SIZE + 1) / 2;
622                 Y = X;
623                 break;
624         }
625       case 'H':
626         FindMove(&X, &Y);
627         break;                  /* Give the user a hint */
628       case 'L':
629         X = (X + SIZE - 2) % SIZE + 1;
630         break;                  /* Left  */
631       case 'R':
632         X = X % SIZE + 1;
633         break;                  /* Right */
634       case 'D':
635         Y = (Y + SIZE - 2) % SIZE + 1;
636         break;                  /* Down  */
637       case 'U':
638         Y = Y % SIZE + 1;
639         break;                  /* Up    */
640       case '7':{
641                 if ((X == 1) || (Y == SIZE)) {  /* Move diagonally    *//* t
642                                                  * owards upper left */
643                         Temp = X;
644                         X = Y;
645                         Y = Temp;
646                 } else {
647                         X = X - 1;
648                         Y = Y + 1;
649                 }
650                 break;
651         }
652       case '9':{                /* Move diagonally    */
653                 if (X == SIZE) {/* toward upper right */
654                         X = (SIZE - Y) + 1;
655                         Y = 1;
656                 } else if (Y == SIZE) {
657                         Y = (SIZE - X) + 1;
658                         X = 1;
659                 } else {
660                         X = X + 1;
661                         Y = Y + 1;
662                 }
663                 break;
664         }
665       case '1':{                /* Move diagonally   */
666                 if (Y == 1) {   /* toward lower left */
667                         Y = (SIZE - X) + 1;
668                         X = SIZE;
669                 } else if (X == 1) {
670                         X = (SIZE - Y) + 1;
671                         Y = SIZE;
672                 } else {
673                         X = X - 1;
674                         Y = Y - 1;
675                 }
676                 break;
677         }
678       case '3':{                /* Move diagonally    */
679                 if ((X == SIZE) || (Y == 1)) {  /* toward lower right */
680                         Temp = X;
681                         X = Y;
682                         Y = Temp;
683                 } else {
684                         X = X + 1;
685                         Y = Y - 1;
686                 }
687                 break;
688         }
689       case 'A':
690         AutoPlay = TRUE;
691         break;                  /* Auto play mode */
692   }                             /* case */
693 }                               /* InterpretCommand */
694 
695 void PlayerMove()
696 /* Enter and make a move */
697 {
698   if (Board[X][Y] == Empty) {
699         MakeMove(X, Y);
700         if (GameWon) PrintMsg("Congratulations, You won!");
701         Command = 'P';
702   }
703   refresh();
704 }                               /* PlayerMove */
705 
706 void ProgramMove()
707 /* Find and perform programs move */
708 {
709   do {
710         if (GameOver()) {
711                 AutoPlay = FALSE;
712                 if ((Command != 'Q') && (!GameWon)) PrintMsg("Tie game!");
713         } else {
714                 FindMove(&X, &Y);
715                 MakeMove(X, Y);
716                 if (GameWon) PrintMsg("I won!");
717         }
718         refresh();
719   } while (AutoPlay);
720 }
721 
722 int main()
723 {
724   Initialize();
725   ResetGame(TRUE);              /* ResetGame and draw the entire screen */
726   refresh();
727   X = (SIZE + 1) / 2;           /* Set starting position to */
728   Y = X;                        /* the middle of the board  */
729   do {
730         ReadCommand(X, Y, &Command);
731         if (GameOver())
732                 if (Command != 'Q') Command = 'N';
733         InterpretCommand(Command);
734         if (Command == 'E') PlayerMove();
735         if (Command == 'P' || Command == 'A') ProgramMove();
736   } while (Command != 'Q');
737   Abort("Good bye!");
738   return(0);
739 }
740 