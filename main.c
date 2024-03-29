// Note to self:
// create a program using sokol (bit of a gui) that we can use to edit checkersboards
// modify MoveList : give it **head**

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/* ASCII */
#define ISDIGIT(x)	('1' <= x && x <= '8')
#define ISALPHA(x)	( ('a' <= x && x <= 'h') || ('A' <= x && x <= 'H') )
#define ISUPPERCASE(x)  ( x >= 'A' && x <= 'Z' )

/* GAME MACROS */

#ifdef _WIN32
	#define RANDINT(MAX) rand() % MAX + 1
#else
#ifdef __linux__
	#define RANDINT(MAX) rand() % MAX + 1
#endif
#endif

/*
	returns the index of the square opposite to the one
	given in the "square" argument
	the "d" argument is used to determine which direction we
	want to find the opposing square
*/
#define OPPOSING_SQUARE(S, D) ADJ_SQUARES[ (ADJ_SQUARES[S-1][D] - 1) ][D]

/* copies a 4 "item" array into another */
#define FOURBYTECOPY(FROM, TO) do { for (int i = 0; i<4; ++i) { TO[i] = FROM[i]; } } while (false);

/* GAME CONSTANTS */

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
	a 4 byte array with the squares adjacent to the square "index"+1
	in NW NE SW SE order.
	if there is no square to be found in a direction, it is set to 0
*/
const uint8_t ADJ_SQUARES[32][4] = {
		{ 0, 0, 5, 6 },     // 1
		{ 0, 0, 6, 7 },     // 2
		{ 0, 0, 7, 8 },     // 3
		{ 0, 0, 8, 0 },     // 4
		{ 0, 1, 0, 9 },     // 5
		{ 1, 2, 9, 10 },    // 6
		{ 2, 3, 10, 11 },   // 7
		{ 3, 4, 11, 12 },   // 8
		{ 5, 6, 13, 14 },   // 9
		{ 6, 7, 14, 15 },   // 10
		{ 7, 8, 15, 16 },   // 11
		{ 8, 0, 16, 0 },    // 12
		{ 0, 9, 0, 17 },    // 13
		{ 9, 10, 17, 18 },  // 14
		{ 10, 11, 18, 19 }, // 15
		{ 11, 12, 19, 20 }, // 16
		{ 13, 14, 21, 22 }, // 17
		{ 14, 15, 22, 23 }, // 18
		{ 15, 16, 23, 24 }, // 19
		{ 16, 0, 24, 0 },   // 20
		{ 0, 17, 0, 25 },   // 21
		{ 17, 18, 25, 26 }, // 22
		{ 18, 19, 26, 27 }, // 23
		{ 19, 20, 27, 28 }, // 24
		{ 21, 22, 29, 30 }, // 25
		{ 22, 23, 30, 31 }, // 26
		{ 23, 24, 31, 32 }, // 27
		{ 24, 0, 32, 0 },   // 28
		{ 0, 25, 0, 0 },    // 29
		{ 25, 26, 0, 0 },   // 30
		{ 26, 27, 0, 0 },   // 31
		{ 27, 28, 0, 0 }    // 32
};


/* ANSI ESCAPE CODES */
#define ANSI_HIGHLIGHT "\033[7m"
#define ANSI_CLEAR     "\033[0m"
#define ANSI_RED	   "\033[30;41m"
#define ANSI_YELLOW    "\033[30;103m"

/* used to determine the next square when jumping */
typedef
enum {
	NW,
	NE,
	SW,
	SE,
} DIRECTION;

/* MOVELIST */
#define MOVELIST_MALLOC ((MoveList *) malloc(sizeof(MoveList)))

/* Cool Struct For Moves */
typedef
struct {
	uint8_t from;
	uint8_t to;
	uint8_t taken[16];
	uint8_t taken_len;
	DIRECTION direction;
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

/* Copies one Move into the other */
void
move_copy(Move m_from, Move *m_to){
	m_to->from = m_from.from;
	m_to->to   = m_from.to;
	m_to->taken_len = m_from.taken_len;
	for (int i = 0; i<m_from.taken_len; ++i)
		(m_to->taken)[i] = (m_from.taken)[i];
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
	bool player;
	char last_turn;
	MoveList *movelist;
	MoveList *available_moves;
	int movelist_len;
	int available_moves_len;
	bool quit;
	uint8_t selected_piece;
} GameCtx;

/*
	utility functions for the linked list
*/
// add an element to the end of the list
void
movelist_append(MoveList **mvlstptr, Move m, int *length){
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
	*length += 1;
}

// pop an element off the end of the list
Move
movelist_pop(MoveList **mvlstptr, bool *success) {

	Move m;

	if (mvlstptr == NULL) {
		*success = false;
		return m;
	}

	MoveList *prev = *mvlstptr;
	MoveList *iter = *mvlstptr;

	while (iter->next != NULL) {
		prev = iter;
		iter = iter->next;
	}

	m = iter->value;
	free(iter);

	// the list was only one element long
	if (iter == prev) *mvlstptr = NULL;

	else prev->next = NULL; 

	*success = true;

	return m;

}

MoveList *
movelist_last(MoveList **mvlstptr) {

	MoveList *iter = *mvlstptr;

	while (iter->next != NULL)
		iter = iter->next;
	
	return iter;
}

// free all the memory acquired by the list
void
movelist_free(MoveList **mvlstptr, int *length){

	if ( (*mvlstptr) == NULL ) return;

	MoveList *prev = (*mvlstptr);

	do {
		(*mvlstptr) = (*mvlstptr)->next;
		free(prev);
		prev = (*mvlstptr);
	} while ( (*mvlstptr) != NULL );

	*length = 0;
	*mvlstptr = NULL;

}

// returns the length of the list
int
movelist_length(MoveList *mvlstptr){

	int len = 0;

	MoveList *tmp = mvlstptr;
	while ( tmp != NULL) {
		len++;
		tmp = tmp->next;
	}

	return len;
	
}

// returns the Move from the list at position "index" (0 indexed)
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
	return false;
}

/* 
   writes into an array of booleans of length 4.
   (NW, NE, SW, SE)
   true means the piece is allowed to move in that direction
*/
void
direction_filter(GameCtx *gmctx, uint8_t square, bool filter[4]){
	char piece = (gmctx->board)[square-1];
	
	if (ISUPPERCASE(piece))
		for (int i = 0; i<4; ++i) filter[i]= true;
	else if (piece == 'b'){
		filter[0] = false;
		filter[1] = false;
		filter[2] = true;
		filter[3] = true;
	}
	else {
		filter[0] = true;
		filter[1] = true;
		filter[2] = false;
		filter[3] = false;
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
				else {
					MoveList *current = mvlstptr;
					bool flag = false;
					while (current != NULL && !flag) {

					for ( int t = 0; t<current->value.taken_len; ++t ) {
						if (current->value.taken[t] == m.to) {
							printf(ANSI_RED);
							flag = true;
							break;
						}
					}
					current = current->next;
					
					}
				}
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

void
available_moves(GameCtx *gmctx, uint8_t square, bool jumping, bool must_take){

	uint8_t adj_squares[4];
	bool dir_filter[4];

	FOURBYTECOPY(ADJ_SQUARES[square-1], adj_squares);

	if (jumping)
		// when jumping, the value of square is irrelevant
		// we should rather get the last jump's "from" field
		direction_filter(gmctx, movelist_last(&(gmctx->available_moves))->value.from, dir_filter);
	else
		direction_filter(gmctx, square, dir_filter);


	for (int i = 0; i<4; ++i){

		uint8_t neighbor = adj_squares[i];

		// not allowed to go that way
		if (!(dir_filter[i])) continue;

		if (jumping && neighbor == 0) {
			// reached end
			MoveList* lst_mv = movelist_last(&(gmctx->available_moves));
			lst_mv->value.to = square;
			if (i == 3)
				return;
		}

		else if (!jumping && (gmctx->board)[neighbor-1] == ' '){

			if (must_take) continue;

			// just add a simple move
			Move m;
			m.taken_len = 0;
			m.from = square;
			m.to = neighbor;
			m.direction = i;
			movelist_append( &(gmctx->available_moves), m, &(gmctx->available_moves_len));
		}

		else if (jumping && ( (gmctx->board)[neighbor-1] == ' '
						   || (gmctx->board)[neighbor-1] == (gmctx->board)[square-1] ) ) {
			// cannot jump anymore
			MoveList* lst_mv = movelist_last(&(gmctx->available_moves));
			lst_mv->value.to = square;
			if (i == 3)
				return;
		}

		else if ((gmctx->board)[neighbor-1] != (gmctx->board)[square-1]){

			uint8_t opp_sqr = OPPOSING_SQUARE(square, i);

			if ((gmctx->board)[opp_sqr-1] == ' '){

				// everything is set for a good jump

				if (!jumping) {
					// create new move object the first time
					Move m;
					m.taken_len = 0;
					m.from = square;
					m.direction = i;
					m.taken[m.taken_len++] = neighbor;
					movelist_append( &(gmctx->available_moves), m, &(gmctx->available_moves_len));
				}

				else {
					// every next call we just add to the list of taken pieces
					MoveList* lst_mv = movelist_last(&(gmctx->available_moves));
					lst_mv->value.taken[lst_mv->value.taken_len++] = neighbor;
				}

				available_moves(gmctx, opp_sqr, true, must_take);

				if (jumping)
					// otherwise we still need to check other directions
					return;
			}

			else if (jumping) {
				// we couldn't finish the jump
				MoveList* lst_mv = movelist_last(&(gmctx->available_moves));
				lst_mv->value.to = square;
				if (i == 3)
					return;
			} 
		}
	}
}

// checks all pieces of "color" and checks if
// there are any moves that result in taking with
// setting the must_take argument of available_moves true
bool
taking_available(GameCtx *ctx, char color) {

	for ( uint8_t i = 0; i<32; ++i) {
		if ( ( ctx->board[i] | 32 ) == color ) {

			movelist_free(&(ctx->available_moves), &(ctx->available_moves_len));
			available_moves(ctx, i+1, false, true);

			if (ctx->available_moves_len > 0) return true;
		}
	}
	
	return false;
}

// if there is no piece, returns false
// otherwise computes the available moves and stores them in the context
bool
select_piece(GameCtx *gmctx, uint8_t square) {

	char color = (gmctx->board)[square-1];

	if ( color == ' ' ) return false;

	gmctx->selected_piece = square;
	bool has_to_take = taking_available(gmctx, color|32);
	available_moves(gmctx, square, false, has_to_take);

	return true;
}

// this saves the move
// then executes it (if it's correct)
bool
do_move(GameCtx *gmctx, Move m){

	if ( !select_piece(gmctx, m.from)    ||
		 gmctx->selected_piece != m.from ||
		 !movelist_contains(gmctx->available_moves, m, true) )
		return false;

	movelist_append(&(gmctx->movelist), m, &(gmctx->movelist_len));

	(gmctx->board)[m.to - 1] = (gmctx->board)[m.from - 1];
	(gmctx->board)[m.from - 1] = ' ';

	for (int i = 0; i<m.taken_len; ++i)
		(gmctx->board)[m.taken[i] - 1] = ' ';

	return true;
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
		movelist_free(&(gmctx->movelist), &(gmctx->movelist_len));
		gmctx->quit = true;
		move.from = 0;
		return move;
	}

	if (cmd[0] == 's'){
		if (ISALPHA(cmd[1]) && ISDIGIT(cmd[2])){

			char buffer[3];
			uint8_t square = coord_to_square(cmd[1], cmd[2]-'0');

			if (!select_piece(gmctx, square))
				printf("You cannot select that square!\n");

			else {

				printf("The available moves for the piece on square %d are: ", square);
				movelist_print( gmctx->available_moves );
				print_board_with_moves( gmctx->board, square, gmctx->available_moves );

			}
			
		}
		else if (ISDIGIT(cmd[1]) && ISDIGIT(cmd[2])){
			char buffer[3];
			buffer[0] = cmd[1];
			buffer[1] = cmd[2];
			buffer[2] = '\n';
			int result = atoi(buffer);
			if (result >= 1 && result <= 32) {
				uint8_t square = (uint8_t)result;

				if (!select_piece(gmctx, square))
					printf("You cannot select that square!\n");

				else {

					printf("The available moves for the piece on square %d are: ", square);
					movelist_print( gmctx->available_moves );
					print_board_with_moves( gmctx->board, square, gmctx->available_moves );

				}
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

		char buffer[3];
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

// AI
void
ai_random_move( GameCtx *ctx ) {

	printf("AI move debug\n");

	int ran_i, j;
	MoveList *mvptr;

	// choose random piece
	bool chosen = false;
	while (!chosen) {
		ran_i = RANDINT(12);
		uint8_t square;
		j = 0;
		for ( int i = 0; i<32; ++i) {
			if ( ctx->board[i] == 'b' || ctx->board[i] == 'B' ) {
				j++;
				if (j == ran_i) {
					square = i+1;
					break;
				}
			}
		}
		select_piece(ctx, square);
		printf("Selected piece on square %d\n", square);

		if (ctx->available_moves_len > 0) chosen = true;

		
	}

	mvptr = ctx->available_moves;

	ran_i = RANDINT(ctx->available_moves_len);

	j = 0;
	while (mvptr->next != NULL && j<ran_i) {
		mvptr = mvptr->next;
		j++;
	}

	printf("chosen move: ");
	move_print(mvptr->value);
	putchar('\n');
	do_move(ctx, mvptr->value);
	movelist_append(&(ctx->movelist), mvptr->value, &(ctx->movelist_len));
}

int
main(int argc, char *argv[]){

	#ifdef _WIN32
		srand((unsigned int)time(NULL));
	#else
	#ifdef __linux__
		srand((unsigned int)time(NULL));
	#endif
	#endif

	GameCtx gmctx;

	uint8_t neighbors[4];
	setup_board(gmctx.board);
	char cmd[8];
	
	gmctx.movelist = NULL;
	gmctx.movelist_len = 0;
	gmctx.available_moves = NULL;
	gmctx.available_moves_len = 0;

	gmctx.selected_piece = 0;

	gmctx.player = true;

	while (!gmctx.quit){

		if (gmctx.player) {
			print_board(gmctx.board);
			putc('>',stdout);
			scanf("%7[^\n]", cmd);
			getchar();
			bool error;
			Move move = parse_cmd(&gmctx, cmd, &error);
			if (!error && move.from){

				bool valid_move = do_move(&gmctx, move);

				if (!valid_move)
					printf("That move is invalid!\n");
				else {

					movelist_append( &(gmctx.movelist), move, &(gmctx.movelist_len) );
					gmctx.player = false;

				}
			}
			memset(cmd, 0, sizeof cmd);
		}
		else {
			ai_random_move(&gmctx);
			gmctx.player = true;
		}
	}

	movelist_print(gmctx.movelist);
	movelist_free(&(gmctx.movelist), &(gmctx.movelist_len));

	return 0;
}
