
//Include the library LedControl
#include "LedControl.h"
#include "chess_constants.h"



/* Define the pins */
#define MUX_ONE 11
#define MUX_TWO 1
#define MUX_THREE 13

#define DMUX_ONE 5
#define DMUX_TWO 6
#define DMUX_THREE 7

#define HALLIN 4


/* Arrays for the sensor matrix states */
boolean current_state[8][8] = { {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0} };
boolean delta_state[8][8]   = { {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0} };
boolean led_matrix[8][8]    = { {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0} };

/* Delta threshold for recognizing a real delta occuring */
#define delta_threshold 2
  
  
  
/*   -----------------------------------------
*    IMPLEMENTATION OF CHESS LOGIC BEGINS HERE
*/  

/* Board representation in 0x88 */
byte board[128] = {0};
byte move_buffer[128] = {0};
byte temp_move_buffer[128] = {0};


/*
 * Function: cm_initBoard
 * --------------------------------------------------
 * Creates an initial placement of the pieces
 * in the global board array, regarding chess rules.
 */ 
 
void cm_initBoard(){
  for(int i = 1; i < 128; i++) board[i] = NO_PIECE;
  // Set up white pieces
  board[0] = W_ROOK;  board[1] = W_KNIGHT; board[2] = W_BISHOP; 
  board[3] = W_QUEEN; board[4] = W_KING;   board[5] = W_BISHOP;
  board[6] = W_KNIGHT;board[7] = W_ROOK;
  for(int i = 0; i < 8; i++) board[16+i] = W_PAWN;   // 16 start of second row in 0x88
  // Set up black pieces
  board[112] = B_ROOK;  board[113] = B_KNIGHT; board[114] = B_BISHOP; 
  board[115] = B_QUEEN; board[116] = B_KING;   board[117] = B_BISHOP;
  board[118] = B_KNIGHT;board[119] = B_ROOK;
  for(int i = 0; i < 8; i++) board[96+i] = B_PAWN;   // 96 start of seventh row in 0x88
}


/*
 * Function: makeMove
 * --------------------------------------------------
 * Moves the piece from one field to another. Take 
 * to be sure that the pieces exists, the function 
 * does not check that. Stores essential information
 * (captured, move itself, etc...) in global buffers.
 *
 * from:  0x88 coordinates in the global board array 
 *        defining the figure that has to be moved.
 * to:    0x88 coordinates  in the global board array 
 *        defining where the figure in from is moved to.
 */ 
 
void cm_makeMove(int from, int to){
  from_buffer = from;
  to_buffer = to;
  byte piece_from = board[from];
  byte piece_to   = board[to];
  board[from] = NO_PIECE;
  board[to]   = piece_from;
  captured_buffer = piece_to;  
}

/*
 * Function: unmakeMove
 * --------------------------------------------------
 * Makes the former move undone in the global board
 * array.
 * The function does not ensure if there is a move in
 * the buffers, undefined behavior if not!
 */
 
void cm_unmakeMove(){
  byte piece = board[to_buffer];
  board[to_buffer] = captured_buffer;
  board[from_buffer] = piece;
  captured_buffer = NO_PIECE;
  from_buffer = -1;
  to_buffer   = -1;
}

/*
 * Function: cm_getKingIndex
 * --------------------------------------------------
 * Finds the king of the given color and returns its
 * index in the global board.
 *
 * color:   Color of the searched king 
 *          0 = white, 1 = black
 * 
 * returns: 0x88 coordinate of the king on the board.
 *          -1 if no king present.
 */

int cm_getKingIndex(int color){
  byte king = color ? B_KING : W_KING;
  int index;
  for(int i = 0; i < 64; i++){
    index = i + (i & ~7); 
    if(board[index] == king) return index;  
  }
  return -1;
}

/*
 * Function: cm_isLegalMove
 * --------------------------------------------------
 * Returns if a move is legit. Therefore, it makes a 
 * move on the board, checks if legit (king in check?)
 * and unmakes the move.
 *
 * from:     0x88 coordinates in the global board array 
 *           defining the figure that has to be moved.
 * to:       0x88 coordinates  in the global board array 
 *           defining where the figure in from is moved to.
 * 
 * returns:  Return 1 if the move is legit, 0 if not
 */

int cm_isLegalMove(int from, int to){
  int color = board[from] >> 1 & 1;
  int otherColor = color ? 0 : 1;
  cm_makeMove(from, to);
  int index = cm_getKingIndex(color);
  int result = !cm_isAttacked(index, otherColor);
  cm_unmakeMove();
  return result;
}

/*
 * Function: cm_filterLegalMoves
 * --------------------------------------------------
 * Takes a buffer of psuedo-legal moves and an index 
 * of a piece and checks the moves to be legal. 
 * Illegal moves get deleted from the moves array.
 *
 * index:    0x88 coordinates of the piece
 * buffer:   Buffer of pseudo-legal moves, may
 *           containing check violations etc. Moves
 *           are directly delete from this pointer.
 */

void cm_filterLegalMoves(int index, byte * buffer){
  int _index;
  for(int i = 0; i < 64; i++){
    _index = i + (i & ~7); 
    if(buffer[_index])
        if(!cm_isLegalMove(index, _index)) buffer[_index] = 0;
  }
}

/*
 * Function: cm_isAttacked
 * --------------------------------------------------
 * Checks if a given square is attacked by a 
 * specified color. This method uses a precalculated
 * attack array, defining which pieces can 
 * attack the unique 0x88 square to square difference.
 * The lookup avoids generation of unnecessary pseudo-
 * legal moves. Operates on the global board array.
 *
 * index:     0x88 coordinates of the square
 * byColor:   Color of the attacker.
 *            0 = white, 1 = black
 *
 * returns:   int defining if it is attackable or not.
 *            1 = attackable, 0 = not attackable.
 */

int cm_isAttacked(int index, int byColor){
  int _index, diff, res;
  for(int i = 0; i < 64; i++){
    _index = i + (i & ~7);   // coordinate transformation to 0x88
    byte piece = board[_index];
    byte piece_type = piece >> 2;
    if((piece >> 1 & 1) == byColor){  
      int diff = index - _index + 128;   
      switch( attack_array[diff] ) {
        case ATTACK_KQR:
          if(piece_type == KING || piece_type == QUEEN || piece_type == ROOK)
            res = cm_canReach(_index, index);
          break;
        case ATTACK_QR:
          if(piece_type == QUEEN || piece_type == ROOK)
            res = cm_canReach(_index, index);
          break;
        case ATTACK_KQBwP:
          if(piece_type == KING || piece_type == QUEEN || piece_type == BISHOP || piece == W_PAWN)
            res = cm_canReach(_index, index);
          break;
        case ATTACK_KQBbP:
          if(piece_type == KING || piece_type == QUEEN || piece_type == BISHOP || piece == B_PAWN)
            res = cm_canReach(_index, index);
          break;
        case ATTACK_QB:
          if(piece_type == BISHOP || piece_type == QUEEN) 
            res = cm_canReach(_index, index);
          break;
        case ATTACK_N:
          if(piece_type == KNIGHT)
            res = cm_canReach(_index, index);
          break;
        default:
          res = 0;
      }
      if(res == 1) return 1;
    }
  }
  return 0;
}

/*
 * Function: cm_canReach
 * --------------------------------------------------
 * Determine if a piece on a specific square can 
 * reach a target square in a pseudo-legal manner.
 * The function calls the generation of moves with
 * a temporal buffer.
 *
 * index:     0x88 coordinates of the piece
 * target:    0x88 coordinates of the target square
 *
 * returns:   int defining if the given piece can 
 *            reach the target or not.
 *            1 = reachable, 0 = not reachable.
 */

int cm_canReach(int index, int target){
  cm_pseudoLegalMoves(index, temp_move_buffer);
  return temp_move_buffer[target]; 
}

/*
 * Function: cm_pseudoLegalMoves
 * --------------------------------------------------
 * Generates the pseudo-legal moves of the piece at
 * index. Operates on the global board array and writes
 * the results in a buffer. Calls specific subroutines 
 * for PAWNS, KINGS and the rest.
 *
 * index:     0x88 coordinates of the piece
 * buffer:    Buffer defining the movements. Each 
 *            field in the 128 byte array is either
 *            marked 1 for a possible move target or
 *            0 for not reachable.
 */

void cm_pseudoLegalMoves(int index, byte * buffer){
  memset(buffer,0, 128 * sizeof(byte));
  byte piece = board[index];
  byte piece_type = piece >> 2;
  if(piece_type == PAWN){
    cm_generatePseudoLegalPawnMoves(piece, index, buffer);
  } else if (piece_type == KING) {
    // DO KING STUFF HERE
  } else { // everything but a pawn and king
    cm_generatePseudoLegalMoves(piece, index, buffer);
  }
}

/*
 * Function: cm_generatePseudoLegalMoves
 * --------------------------------------------------
 * Generates the pseudo-legal moves of the piece at
 * index. Specialized routine for QUEEN, ROOK, KNIGHT,
 * BISHOP. Operates on global board array.
 *
 * piece:     byte describing the piece. Important 
 *            for color and sliding attribute. Only
 *            Q,R,K,B are allowed, undefined behavior 
 *            else.
 * index:     0x88 coordinates of the piece
 * buffer:    Buffer defining the movements. Each 
 *            field in the 128 byte array is either
 *            marked 1 for a possible move target or
 *            0 for not reachable.
 */

void cm_generatePseudoLegalMoves(byte piece, int index, byte * buffer){
  const int * md = move_deltas[piece >> 2];
  byte sliding = piece & B1;
  byte color   = piece & B10;
  int tmp_index = index;

  for(int i = 0; i < 8; i++){
    int delta = md[i];
    if(delta == 0) break;
    tmp_index = index + delta;
    while((tmp_index & 0x88) == 0){
      byte target = board[tmp_index];
      if(target != NO_PIECE){
        if((target & B10) != color){   buffer[tmp_index] = 1; }
        break;
      }
      buffer[tmp_index] = 1;
      if(!sliding) break;
      tmp_index += delta;
    }
  }    
}


/*
 * Function: cm_generatePseudoLegalPawnMoves
 * --------------------------------------------------
 * Generates the pseudo-legal moves for pawns. 
 * Calls straight and capture subroutines.
 * Operates on global board array.
 *
 * piece:     byte describing the piece. Important 
 *            for color. Every passed piece is handled
 *            as a pawn.
 * index:     0x88 coordinates of the piece
 * buffer:    Buffer defining the movements. Each 
 *            field in the 128 byte array is either
 *            marked 1 for a possible move target or
 *            0 for not reachable.
 */

void cm_generatePseudoLegalPawnMoves(byte piece, int index, byte * buffer){
  byte color = piece >> 1 & 1;
  // Get the direction of the pieces!
  int dir = color ? -1 : 1;
  int starting_row = color ? 6 : 1;
  cm_straightPawnMove(dir, index, starting_row, buffer);
  cm_capturePawnMove(dir, index, color, buffer);
}


/*
 * Function: cm_straightPawnMove
 * --------------------------------------------------
 * Generates pseudo-legal straight pawn moves. 
 * Handles two field move from initial position.
 * Every incomming piece is handled as a pawn.
 * Operates on global board array.
 *
 * dir:       Defining direction, 1 = white (to top)
 *            -1 = black (to bottom).
 * starting_row: defines the row of initial position
 * index:     0x88 coordinate of the pawn
 * buffer:    Buffer defining the movements. Each 
 *            field in the 128 byte array is either
 *            marked 1 for a possible move target or
 *            0 for not reachable.
 */

void cm_straightPawnMove(int dir, int index, int starting_row, byte * buffer){
  if(board[index + (dir * NORTH)] == NO_PIECE){
    buffer[index + (dir * NORTH)] = 1;
    if(index / 16 == starting_row  && board[index + (dir * NN)] == NO_PIECE){
          buffer[index + (dir * NN)] = 1;
    }
  }
}

/*
 * Function: cm_capturePawnMove
 * --------------------------------------------------
 * Generates pseudo-legal pawn captures. 
 * Every incomming piece is handled as a pawn.
 * Operates on global board array.
 *
 * dir:       Defining direction, 1 = white (to top)
 *            -1 = black (to bottom).
 * index:     0x88 coordinate of the pawn
 * color:     defines the color of the pawn (w=0,b=1)
 * buffer:    Buffer defining the movements. Each 
 *            field in the 128 byte array is either
 *            marked 1 for a possible move target or
 *            0 for not reachable.
 */

void cm_capturePawnMove(int dir, int index, int color, byte * buffer){
  int  attack_index = index + (dir * NW);
  byte attack_piece = board[attack_index];
  if(attack_piece != NO_PIECE && (attack_piece & B10) != color)
    buffer[attack_index] = 1; 
  attack_index = index + (dir * NE);
  attack_piece = board[attack_index];
  if(attack_piece != NO_PIECE && (attack_piece & B10) != color)
    buffer[attack_index] = 1; 
}


/*********************************************************************
****************            LED Control         **********************
*********************************************************************/



LedControl lc=LedControl(10,8,9,1);

/*
 * Function: lc_setupLc
 * --------------------------------------------------
 * Setup the led control module at startup
 */

void lc_setupLc(){    
  lc.setLed(0, 0, 0, true);
  lc.shutdown(0,false);
  lc.setIntensity(0,15);
  lc.clearDisplay(0);
}


/*
 * Function: lc_initLights
 * --------------------------------------------------
 * Play fancy lighteffects. Directly uses the 
 * LedControl library.
 *
 * myDelay:  int defining the delay between steps
 */
 
void lc_initLights(int myDelay){
  for(int row=0;row<8;row++) {
    for(int col=0;col<8;col++) {
      lc.setLed(0,row,col,true);
      delay(myDelay); }}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,15 - i); delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,i);      delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,15 - i); delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,i);      delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,15 - i); delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,i);      delay(myDelay);}
  for(int row=0;row<8;row++) {
      for(int col=0;col<8;col++) {
        lc.setLed(0,row,col,false);
        delay(myDelay);}}
}


/*
*     TECHNICAL RELATED STUFF
*/

void setup() {
  
  setupPins();
  Serial.begin(9600);      // open the serial port at 9600 bps:    

  lc_setupLc();
  lc_initLights(20);
 
  cm_init_board();

  board[83] = W_KNIGHT;
  cm_pseudoLegalMoves(98, move_buffer);
  cm_filterLegalMoves(98, move_buffer);
  
  moveBufferToLED();
}


/*
 * Function: setupPins
 * --------------------------------------------------
 * Set up arduino pins using constants
 */

void setupPins(){
  pinMode(MUX_ONE   , OUTPUT);  
  pinMode(MUX_TWO   , OUTPUT);  
  pinMode(MUX_THREE , OUTPUT);  
  pinMode(DMUX_ONE  , OUTPUT);  
  pinMode(DMUX_TWO  , OUTPUT);  
  pinMode(DMUX_THREE, OUTPUT); 
  pinMode(HALLIN, INPUT_PULLUP); 
}


void moveBufferToLED(){
  // Movebuffer is 0x88, so transform it!
  for(int i = 0; i < 128; i++){
    if(move_buffer[i] == 1){
       led_matrix[i >> 4][i & 7] = 1; }
  }
}

/* All hall sensor reading is done here */
void readSensors(){
  for(int i = 0; i < 8; i++){
    for(int j = 0; j < 8; j++){
  
      /* Write to the mux outputs for choosing a row */
      digitalWrite(MUX_ONE  , (i & 1)   );
      digitalWrite(MUX_TWO  , (i & 3)>>1);
      digitalWrite(MUX_THREE, (i & 7)>>2);
      /* Write to demux for choosing a col */
      digitalWrite(DMUX_ONE  ,(j & 1)   );
      digitalWrite(DMUX_TWO  ,(j & 3)>>1);
      digitalWrite(DMUX_THREE,(j & 7)>>2);
      /* Delay because of reaction time of sensor */
      delay(1);

      /* check the sensors */
      if(!digitalRead(HALLIN)){ // negation because of pull-up resistor
        if(current_state[i][j] != 1){ // Obviously delta happening here
          delta_state[i][j] += 1;
        } 
      } else {
        if(current_state[i][j] != 0){
          delta_state[i][j] += 1;
        }
      }  
    }
  }
}

/* Check the delta array. If a delta threshold is reached, transfer the change
   into the current state */ 
void processDeltaArray(){
  for(int i = 0; i < 8; i++){
    for(int j = 0; j < 8; j++){
      if(delta_state[i][j] >= delta_threshold){
        delta_state[i][j] = 0;
        int current = current_state[i][j];
        current_state[i][j] = current == 1 ? 0 : 1;     
      }
    }
  }
}

/* All LED stuff is handled by this function */
void doTheFancyLights(){
  for(int i = 0; i < 8; i++){
    for(int j = 0; j < 8; j++){
        if(led_matrix[i][j] == 1)
          lc.setLed(0, j, i, true);
        else
          lc.setLed(0, j, i, false); 
    }
  }  
}



void loop() {
    readSensors();
    processDeltaArray();
    doTheFancyLights();
}
