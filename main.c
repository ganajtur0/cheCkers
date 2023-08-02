#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

/* BITS */
#define BIT(x) 		(1<<(x))
#define SETBIT(x,p) 	((x)|(1<<(p)))
#define CLEARBIT(x,p)	((x)&(~(1<<(p))))
#define BITSET(x,p)	((x)&(1<<(p)))
#define TOGGLEBIT(x,p)	((x)^(1<<(p)))

/* ASCII */
#define ISDIGIT(x)	('1' <= x && x <= '8')
#define ISALPHA(x)	( ('a' <= x && x <= 'h') || ('A' <= x && x <= 'H') )
#define ISUPPERCASE(x)  ( x >= 'A' && x <= 'Z' )

/* MOVELIST */
#define MOVELIST_MALLOC ((MoveList *) malloc(sizeof(MoveList)))

/* Cool Struct For Moves */
typedef
struct {
	uint8_t from;
	uint8_t to;
	uint8_t taken[16];
	uint8_t taken_len;
} Move;


/*
	Compares two Move structs for equality
	the "ignore_taken" boolean can be provided to the function for it
	to ignore the ".taken" attribute of both structs
	(meaning it'll only compare the "from" and the "to" attributes)
*/
bool
move_equal(Move m1, Move m2, bool ignore_taken){
	if (!ignore_taken){
		if (m1.taken_len != m2.taken_len)
			return false;
		for (int i = 0; i<m1.taken_len; ++i)
			if (m1.taken[i] != m2.taken[i])
				return false;
	}
	if (m1.from != m2.from || m1.to != m2.to)
		return false;
	return true;
}

/* Computer Science Moment: Storing the Moves in a Linked List */
typedef
struct movelist {
	Move value;
	struct movelist *next;
} MoveList;

/* storing all the relevant game data in one struct */
typedef
struct {
	char board[32];
	MoveList *movelist;
	bool quit;
} GameCtx;

/* used to determine the next square when jumping */
typedef
enum {
	NW,
	NE,
	SW,
	SE,
} DIRECTION;

/*
	utility functions for the linked list
*/
void
movelist_append(MoveList **mvlstptr, Move m){
	if (*mvlstptr == NULL) {
		(*mvlstptr) = MOVELIST_MALLOC;
		(*mvlstptr)->value = m;
		for (int i = 0; i<m.taken_len; ++i)
			((*mvlstptr)->value).taken[i] = m.taken[i];
		(*mvlstptr)->next  = NULL;
	}
	else {
		MoveList *ptr = (*mvlstptr);
		while ( ptr->next != NULL )
			ptr = ptr->next;
		ptr->next = MOVELIST_MALLOC;
		ptr->next->value = m;
		for (int i = 0; i<m.taken_len; ++i)
			ptr->next->value.taken[i] = m.taken[i];
		ptr->next->next  = NULL;
	}
}

void
movelist_free(MoveList **mvlstptr){

	if ( (*mvlstptr) == NULL ) return;

	MoveList *prev = (*mvlstptr);

	do {
		(*mvlstptr) = (*mvlstptr)->next;
		free(prev);
		prev = (*mvlstptr);
	} while ( (*mvlstptr) != NULL );

}

int
movelist_len(MoveList *mvlstptr){

	int len = 0;

	MoveList *tmp = mvlstptr;
	while ( tmp != NULL) {
		len++;
		tmp = tmp->next;
	}

	return len;
	
}

Move
movelist_get(MoveList *mvlstptr, int index){
	MoveList *tmp = mvlstptr;
	for (int i = 0; i<index && tmp->next != NULL; ++i)
		tmp = tmp->next;
	return tmp->value;
}

/*
 	the "ignore_taken" argument in this function call will be provided to
	the "move_equal" function inside of it
 */
bool
movelist_contains(MoveList *mvlstptr, Move m, bool ignore_taken){
	MoveList *tmp = mvlstptr;
	while (tmp != NULL){
		if (move_equal(tmp->value, m, ignore_taken))
			return true;
		tmp = tmp->next;
	}
}

/* ANSI ESCAPE CODES */
#define ANSI_HIGHLIGHT "\033[7m"
#define ANSI_CLEAR     "\033[0m"
#define ANSI_YELLOW    "\033[30;103m"

/*
	__1-__2-__3-__4-
	5-__6-__7-__8-__
	__9-__10__11__12
	13__14__15__16__
	__17__18__19__20
	21__22__23__24__
	__25__26__27__28
	29__30__31__32__
*/

/*
	fills up a 4 byte array with the squares adjacent to "square"
	in NW NE SW SE order.
	if there is no square to be found in a direction, it's value will be set to 0
*/
void
adjacent_squares(uint8_t square, uint8_t squares[4], bool isking){
	// even rows
	if ( ! ( ( square/4 + ( (square%4) > 0 ) ) % 2 ) ){
		// edge
		if ( square % 4 == 1 ){
			squares[0] = 0;
			squares[1] = square - 4;
			squares[2] = 0;
			squares[3] = isking ? square + 4 : 0;
			return;
		}
		squares[0] = square - 5 > 0 ? square - 5 : 0;
		squares[1] = square - 4 > 0 ? square - 4 : 0;
		squares[2] = ( ( square + 3 <= 32 && isking ) ) ? square + 3 : 0;
		squares[3] = ( ( square + 4 <= 32 && isking ) ) ? square + 4 : 0;
	}
	// odd rows
	else {
		// edge
		if ( square % 4 == 0 ){
			squares[0] = square - 4;
			squares[1] = 0;
			squares[2] = isking ? square + 4 : 0;
			squares[3] = 0;
			return;
		}
		squares[0] = square - 4 > 0 ? square - 4 : 0;
		squares[1] = square - 3 > 0 ? square - 3 : 0;
		squares[2] = ( ( square + 4 <= 32 && isking ) ) ? square + 4 : 0;
		squares[3] = ( ( square + 5 <= 32 && isking ) ) ? square + 5 : 0;
	}
}

/*
	returns the index of the square opposite to the one
	given in the "square" argument
	the "d" argument is used to determine which direction we
	want to find the opposing square
	"isking" argument speaks for itself
*/
uint8_t
opposing_square(uint8_t square, DIRECTION d, bool isking) {
	// even rows
	if ( ! ( ( square/4 + ( (square%4) > 0 ) ) % 2 ) ){
		// edge
		if ( square % 4 == 1 ){
			switch (d) {
			case NE:
				return square - 4;
			case SE:
				return isking ? square + 4 : 0;
			default:
				return 0;
			}
		}
		switch (d) {
		case NW:
			return square - 5 > 0 ? square - 5 : 0;
		case NE:
			return square - 4 > 0 ? square - 4 : 0;
		case SW:
			return ( ( square + 3 <= 32 && isking ) ) ? square + 3 : 0;
		case SE:
			return ( ( square + 4 <= 32 && isking ) ) ? square + 4 : 0;
		// unreachable
		default:
			return 0;
		}
	}
	// odd rows
	else {
		// edge
		if ( square % 4 == 0 ){
			switch (d) {
			case NW:
				return square - 4;
			case SW:
				return isking ? square + 4 : 0;
			default:
				return 0;
			}
		}
		switch (d) {
		case NW:
			return square - 4 > 0 ? square - 4 : 0;
		case NE:
			return square - 3 > 0 ? square - 3 : 0;
		case SW:
			return ( ( square + 4 <= 32 && isking ) ) ? square + 4 : 0;
		case SE:
			return ( ( square + 5 <= 32 && isking ) ) ? square + 5 : 0;
		// unreachable
		default:
			return 0;
		}
	}
}

/*
	turns the index of the square to coordinate values
*/
void
square_to_coord(uint8_t square, char *col, uint8_t *row){

	int i = 0, j = 0, k = 1;
	bool l = true;

	for (i = 0; i<8; ++i){
	for (j = 0; j<8; ++j){

		if (!l) {
			if (k == square){
				*row = 8-i;
				*col = 'a' + j;
				return;
			}
			k++;
		}

		l = !l;
	}

	if ( !(j%2) )
		l = !l;
	}
}

/*
	turns a pair of coordinates to the index of the square
*/
uint8_t
coord_to_square(char col, uint8_t row){

	int i = 0, j = 0, k = 1;
	bool l = true;

	for (i = 0; i<8; ++i){
	for (j = 0; j<8; ++j){

		if (col == 'a' + j && row == 8-i)
			return k;

		if (!l) k++;

		l = !l;
	}

	if ( !(j%2) ) l = !l;

	}
}

/*
	prints the current state of the board (with the pieces)
	with ascii characters using ANSI escape sequences to
	represent the colors of the squares
*/
void
print_board(char board[32]){
	int i = 0, j = 0, k = 0;
	bool l = true;
	for (i = 0; i<8; ++i){
		putc('0' + (8-i), stdout);
		for (j = 0; j<8; ++j){
			
			if (l) printf(ANSI_HIGHLIGHT " " ANSI_CLEAR);
			else putc(board[k++], stdout);

			l = !l;
		}

		if ( !(j%2) )
			l = !l;

		putc('\n', stdout);
	}
	printf(" abcdefgh\n");
}

/*
	prints the current state of the board, and
	also highlights the squares (with a pretty yellow color) that the currently selected
	piece (given by the "square" argument) can move to
*/
void
print_board_with_moves(char board[32], uint8_t square, MoveList *mvlstptr){

	Move m;
	m.from = square;
	int i = 0, j = 0, k = 0;
	bool l = true;
	for (i = 0; i<8; ++i){
		putc('0' + (8-i), stdout);
		for (j = 0; j<8; ++j){
			
			if (l) printf(ANSI_HIGHLIGHT " " ANSI_CLEAR);
			else {
				m.to = coord_to_square('a' + j, 8-i);
				if (movelist_contains(mvlstptr, m, true))
					printf(ANSI_YELLOW);
				putc(board[k++], stdout);
				printf(ANSI_CLEAR);
			}

			l = !l;
		}

		if ( !(j%2) )
			l = !l;

		putc('\n', stdout);
	}
	printf(" abcdefgh\n");
}

/*
	these printing functions may very well be useful for logging
*/
void
move_print(Move m) {
	char from_col;
	uint8_t from_row;
	square_to_coord(m.from, &from_col, &from_row);
	char to_col;
	uint8_t to_row;
	square_to_coord(m.to, &to_col, &to_row);
	printf("%c%d-%c%d", from_col, from_row, to_col, to_row);
}

void
movelist_print(MoveList *mvlstptr) {
	MoveList *ptr = mvlstptr;
	while (ptr != NULL){
		move_print(ptr->value);
		printf("; ");
		ptr = ptr->next;
	}
	putc('\n', stdout);
}

/*
	here's the meat: the --MOVES--
*/
void
available_moves(char board[32], uint8_t square, MoveList **mvlstptr){
	uint8_t adj_squares[4];
	adjacent_squares(square, adj_squares, ISUPPERCASE(board[square]));
	Move m;
	for (int i = 0; i<4; ++i){
		if (adj_squares[i]){
			if (board[adj_squares[i]-1] == ' '){
				m.from = square;
				m.to = adj_squares[i];
				m.taken_len = 0;
				movelist_append(mvlstptr, m);
			}
			else if (board[adj_squares[i]-1] != board[square-1]){
				m.from = square;
				uint8_t opp_sqr = opposing_square(adj_squares[i], i, ISUPPERCASE(board[square-1]));
				printf("%d\n", opp_sqr);
				m.to = opp_sqr;
				m.taken_len = 1;
				m.taken[m.taken_len-1] = adj_squares[i];
				movelist_append(mvlstptr, m);
			}
		}
	}
}

void
do_move(char board[32], Move m){
	board[m.to - 1] = board[m.from - 1];
	board[m.from - 1] = ' ';
}

/*
	set the board up for a new game
*/
void
setup_board(char board[32]){
	int i;
	for (i = 0; i<12; ++i)
		board[i] = 'b';
	for (i = 12; i<20; ++i)
		board[i] = ' ';
	for (i = 20; i<32; ++i)
		board[i] = 'w';
	putc('\n', stdout);
}

/*
	parsing the commands
	the syntax of a command is as follows:

	- either N[N]-M[M]\0 where 'N[N]' and 'M[M]' represent square numbers -> type 1
	- or aN-bM where 'a' and 'b' represent column letters
		         'N' and 'M' represent row numbers    -> type 2

	left side of the '-' symbol is where we come from
	and the right side is where we go

	writes the index of the erronous character to 'error_index'
	if there is an error, otherwise the value at 'error_index' becomes 0

	who let the dogs out?

	it's also possible to make the program display the available moves for one
	piece (the onepiece is real)

	- sN[N] or saN
	
	and 'q' quits
*/
Move
parse_cmd(GameCtx *gmctx, char cmd[6], bool *error) {
	
	Move move;
	move.taken_len = 0;

	if (cmd[0] == 'q') {
		printf("Quitting\n");
		movelist_free(&(gmctx->movelist));
		gmctx->quit = true;
		move.from = 0;
		return move;
	}

	if (cmd[0] == 's'){
		if (ISALPHA(cmd[1]) && ISDIGIT(cmd[2])){

			char buffer[3];
			uint8_t square = coord_to_square(cmd[1], cmd[2]-'0');

			MoveList *available = NULL;
			available_moves( gmctx->board, square, &available);
			printf("The available moves for the piece on square %d are: ", square);
			movelist_print(available);
			print_board_with_moves( gmctx->board, square, available);
			movelist_free(&available);
			
		}
		else if (ISDIGIT(cmd[1]) && ISDIGIT(cmd[2])){
			char buffer[3];
			buffer[0] = cmd[1];
			buffer[1] = cmd[2];
			buffer[2] = '\n';
			int result = atoi(buffer);
			if (result >= 1 && result <= 32) {
				uint8_t square = (uint8_t)result;
				MoveList *available = NULL;
				available_moves( gmctx->board, square, &available);
				printf("The available moves for the piece on square %d are: ", square);
				movelist_print(available);
				print_board_with_moves( gmctx->board, square, available);
				movelist_free(&available);
			}
		}
		else {
			printf("Error parsing 'show' command!\n");
			*error = true;
		}
		*error = false;
		move.from = 0;
		return move;
	}

	// here we assume the user inteded to input a command of type 1
	if (ISDIGIT(cmd[0])) {

		char buffer_from[3];
		char buffer_to[3];
		
		int i = 0, j = 0;
		do{
			if (!ISDIGIT(cmd[i])){
				printf("\nerror parsing 'from' argument\n");
				*error = true;
				return move;
			}
			buffer_from[j++] = cmd[i++];
		} while (cmd[i] != '-' && cmd[i] != '\0' && j < 2);
		buffer_from[j] = '\0';

		if (cmd[i] != '-'){
			printf("error: invalid 'from' argument\n");
			*error = true;
			return move;
		}

		j = 0; i++;
		do {
			if (!ISDIGIT(cmd[i])){
				printf("\nerror parsing 'to' argument\n");
				*error = true;
				return move;
			}
			buffer_to[j++] = cmd[i++];
		} while (cmd[i] != '\0' && j < 2);
		buffer_to[j] = '\0';

		int from = atoi(buffer_from);
		if ( !(0 < from && from < 33) ){
			printf("\nerror: 'from' not in 1-32 range\n");
			*error = true;
			return move;
		}
		int to = atoi(buffer_to);
		if ( !(0 < to && to < 33) ){
			printf("\nerror: 'to' not in 1-32 range\n");
			*error = true;
			return move;
		}

		move.from = from;
		move.to   = to;

		*error = false;

		return move;
		
	}
	// and here's for parsing type 2
	else {
		if ( !ISALPHA(cmd[0]) ) {
			printf("Invalid 'from' character\n");
			*error = true;
			return move;
		}

		char from_col;
		uint8_t from_row;
		char to_col;
		uint8_t to_row;

		from_col = cmd[0];

		char buffer[2];
		int i = 1, j = 0;
		do {
			if ( !ISDIGIT(cmd[i]) ) {
				printf("error: invalid 'row' value for 'from' argument [%c]\n", cmd[i]);
				*error = true;
				return move;
			}
			buffer[j] = cmd[i++];
		} while ( cmd[i] != '-' && cmd[i] != '\0' && i<2 );
		buffer[2] = '\0';

		if ( cmd[i] != '-' ){
			printf("error parsing 'from' argument\n");
			*error = true;
			return move;
		}

		int atoi_buffer = atoi(buffer);

		if ( !(atoi_buffer>=1 && atoi_buffer<=8) ){
			printf("error: provided 'from' argument's row value is not in range 1-8\n");
			*error = true;
			return move;
		}

		from_row = atoi_buffer;
		i++;
		
		if ( !ISALPHA(cmd[i]) ) {
			printf("error: invalid 'to' character\n");
			*error = true;
			return move;
		}

		to_col = cmd[i++];

		j = 0;
		do {
			if ( !ISDIGIT(cmd[i]) ) {
				printf("error: invalid 'row' value for 'to' argument [%c]\n", cmd[i]);
				*error = true;
				return move;
			}
			buffer[j] = cmd[i++];
		} while ( cmd[i] != '-' && cmd[i] != '\0' && i<2 );
		buffer[2] = '\0';

		atoi_buffer = atoi(buffer);

		if ( !(atoi_buffer>=1 && atoi_buffer<=8) ){
			printf("error: provided 'from' argument's row value is not in range 1-8\n");
			*error = true;
			return move;
		}

		to_row = atoi_buffer;

		move.from = coord_to_square(from_col, from_row);
		move.to = coord_to_square(to_col, to_row);

		return move;
		
	}

}

int
main(int argc, char *argv[]){

	/* 
	   32 bit value to represent the 32 squares used in the game
	   0 - black piece
	   1 - white piece
	*/

	printf("Opposing square to 21 (NW, false): %d\n", opposing_square(21, NW, false) );
	printf("Opposing square to 21 (NE, false): %d\n", opposing_square(21, NE, false) );
	printf("Opposing square to 22 (SW, false): %d\n", opposing_square(22, SW, false) );
	printf("Opposing square to 22 (SW, true): %d\n", opposing_square(22, SW, true) );

	GameCtx gmctx;

	uint8_t neighbors[4];
	setup_board(gmctx.board);
	print_board(gmctx.board);
	char cmd[8];
	
	gmctx.movelist = NULL;

	while (!gmctx.quit){
		putc('>',stdout);
		scanf("%7[^\n]", cmd);
		getchar();
		bool error;
		Move move = parse_cmd(&gmctx, cmd, &error);
		if (move.from){
			movelist_append(&(gmctx.movelist), move);
			do_move(gmctx.board, move);
			print_board(gmctx.board);
		}
		memset(cmd, 0, sizeof cmd);
	}

	movelist_print(gmctx.movelist);
	movelist_free(&(gmctx.movelist));

	return 0;
}
