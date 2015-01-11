
//Include the library LedControl
#include "LedControl.h"

LedControl lc=LedControl(10,8,9,1);


/* Define the pins */
#define MUX_ONE 11
#define MUX_TWO 1
#define MUX_THREE 13

#define DMUX_ONE 5
#define DMUX_TWO 6
#define DMUX_THREE 7

#define HALLIN 4


/* Arrays for the sensor matrix states */
boolean current_state[8][8] = { {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}};
boolean delta_state[8][8]   = { {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}};
boolean led_matrix[8][8]    = { {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}};

/* Delta threshold for recognizing a real delta occuring */
#define delta_threshold 2
  
  
  
/*   -----------------------------------------
*    IMPLEMENTATION OF CHESS LOGIC BEGINS HERE
*/  

/* Board representation in 0x88 */
byte board[128] = {0};
byte move_buffer[128] = {0};
byte temp_move_buffer[128] = {0};


// Define directions

#define NORTH  16
#define NN    ( NORTH + NORTH )
#define SOUTH -16
#define SS    ( SOUTH + SOUTH )
#define EAST    1
#define WEST   -1
#define NE     17
#define SW    -17
#define NW     15
#define SE    -15
  

  
// Constants for pieces, while last bit stands for its color
// ENCODING:    
//              pppcs
// 
// p: Piece type, 3 bits
// c: Color, 0 = white, 1 = black
// s: Sliding, 0 none-sliding, 1 sliding


const byte W_PAWN   = B00000;
const byte W_ROOK   = B00101;
const byte W_KNIGHT = B01000;
const byte W_BISHOP = B01101;
const byte W_KING   = B10000;
const byte W_QUEEN  = B10101;
const byte B_PAWN   = B00010;
const byte B_ROOK   = B00111;
const byte B_KNIGHT = B01010;
const byte B_QUEEN  = B10111;
const byte B_BISHOP = B01111;
const byte B_KING   = B10010;
const byte NO_PIECE = B11111;

// General piece types:

const byte PAWN   = B000;
const byte ROOK   = B001;
const byte KNIGHT = B010;
const byte BISHOP = B011;
const byte KING   = B100;
const byte QUEEN  = B101;

// Buffers for undoing moves
byte captured_buffer = NO_PIECE;
int  from_buffer    = -1;
int  to_buffer      = -1;

// Move deltas. Define the index-deltas for different figures
// Index of move vector defined by the peace bit representation

const int move_deltas[6][8] = {
  { 0 },                                        // pawn: Handled differently
  { WEST, SOUTH, EAST,  NORTH, 0, 0, 0, 0},     // rook
  {-18, -33, -31, -14,  18,  33,  31,  14},     // knight very special movements
  {SE, SW, NW, NE, 0, 0, 0, 0},                 // bishop
  {SE, SW, NW, NE, WEST, SOUTH, EAST, NORTH},   // king
  {SE, SW, NW, NE, WEST, SOUTH, EAST, NORTH}    // queen
};

// Attack vectors
// Taken from http://mediocrechess.blogspot.de/2006/12/guide-attacked-squares.html
// Jonathan offer a great overview of the 0x88 representation.

const int ATTACK_NONE  = 0;
const int ATTACK_KQR   = 1;
const int ATTACK_QR    = 2;
const int ATTACK_KQBwP = 3;
const int ATTACK_KQBbP = 4;
const int ATTACK_QB    = 5;
const int ATTACK_N     = 6;

const int attack_array[257] =
  {0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,2,0,0,0,   
   0,0,0,5,0,0,5,0,0,0,0,0,2,0,0,0,0,0,5,0,  
   0,0,0,5,0,0,0,0,2,0,0,0,0,5,0,0,0,0,0,0, 
   5,0,0,0,2,0,0,0,5,0,0,0,0,0,0,0,0,5,0,0,
   2,0,0,5,0,0,0,0,0,0,0,0,0,0,5,6,2,6,5,0,   
   0,0,0,0,0,0,0,0,0,0,6,4,1,4,6,0,0,0,0,0,  
   0,2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,0,0,0,0,     
   0,0,6,3,1,3,6,0,0,0,0,0,0,0,0,0,0,0,5,6,    
   2,6,5,0,0,0,0,0,0,0,0,0,0,5,0,0,2,0,0,5,   
   0,0,0,0,0,0,0,0,5,0,0,0,2,0,0,0,5,0,0,0,     
   0,0,0,5,0,0,0,0,2,0,0,0,0,5,0,0,0,0,5,0,    
   0,0,0,0,2,0,0,0,0,0,5,0,0,5,0,0,0,0,0,0,   
   2,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0 };


void init_board(){
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



void makeMove(int from, int to){
  from_buffer = from;
  to_buffer = to;
  byte piece_from = board[from];
  byte piece_to   = board[to];
  board[from] = NO_PIECE;
  board[to]   = piece_from;
  captured_buffer = piece_to;  
}

void unmakeMove(){
  byte piece = board[to_buffer];
  board[to_buffer] = captured_buffer;
  board[from_buffer] = piece;
  captured_buffer = NO_PIECE;
  from_buffer = -1;
  to_buffer   = -1;
}


int getKingIndex(int color){
  byte king = color ? B_KING : W_KING;
  int index;
  for(int i = 0; i < 64; i++){
    index = i + (i & ~7); 
    if(board[index] == king) return index;  
  }
}

int isLegitMove(int from, int to){
  int color = board[from] >> 1 & 1;
  int otherColor = color ? 0 : 1;
  makeMove(from, to);
  int index = getKingIndex(color);
  int result = !isAttacked(index, otherColor);
  unmakeMove();
  return result;
}


int filterLegitMoves(int index, byte * buffer){
  int _index;
  for(int i = 0; i < 64; i++){
    _index = i + (i & ~7); 
    if(buffer[_index])
        if(!isLegitMove(index, _index)) buffer[_index] = 0;
  }
}


int isAttacked(int index, int byColor){
  int _index, diff, res;

  for(int i = 0; i < 64; i++){
    _index = i + (i & ~7); 
    byte piece = board[_index];
    byte piece_type = piece >> 2;
    if((piece >> 1 & 1) == byColor){  
      int diff = index - _index + 128;   
      switch( attack_array[diff] ) {
        case ATTACK_KQR:
          if(piece_type == KING || piece_type == QUEEN || piece_type == ROOK)
            res = canReach(_index, index);
          break;
        case ATTACK_QR:
          if(piece_type == QUEEN || piece_type == ROOK)
            res = canReach(_index, index);
          break;
        case ATTACK_KQBwP:
          if(piece_type == KING || piece_type == QUEEN || piece_type == BISHOP || piece == W_PAWN)
            res = canReach(_index, index);
          break;
        case ATTACK_KQBbP:
          if(piece_type == KING || piece_type == QUEEN || piece_type == BISHOP || piece == B_PAWN)
            res = canReach(_index, index);
          break;
        case ATTACK_QB:
          if(piece_type == BISHOP || piece_type == QUEEN) 
            res = canReach(_index, index);
          break;
        case ATTACK_N:
          if(piece_type == KNIGHT)
            res = canReach(_index, index);
          break;
        default:
          res = 0;
      }
      if(res == 1) return 1;
    }
  }
  return 0;
}

int canReach(int index, int target){
  memset(temp_move_buffer,0, 128 * sizeof(byte));
  byte piece = board[index];
  byte piece_type = piece >> 2;

  if(piece_type == PAWN){
    generatePseudoLegalPawnMoves(piece, index, temp_move_buffer);
  } else if (piece_type == KING) {
    // DO KING STUFF HERE
  } else { // everything but a pawn and king
    generatePseudoLegalMoves(board[index], index, temp_move_buffer );
    return temp_move_buffer[target]; 
  }
  return 0;
}

void pseudoLegalMoves(int index, byte * buffer){
  memset(buffer,0, 128 * sizeof(byte));
  byte piece = board[index];
  byte piece_type = piece >> 2;
  if(piece_type == PAWN){
    generatePseudoLegalPawnMoves(piece, index, buffer);
  } else if (piece_type == KING) {
    // DO KING STUFF HERE
  } else { // everything but a pawn and king
    generatePseudoLegalMoves(piece, index, buffer);
  }
}

void generatePseudoLegalMoves(byte piece, int index, byte * buffer){
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

void generatePseudoLegalPawnMoves(byte piece, int index, byte * buffer){
  byte color = piece >> 1 & 1;
  // Get the direction of the pieces!
  int dir = color ? -1 : 1;
  int starting_row = color ? 6 : 1;
  straightPawnMove(dir, index, starting_row, buffer);
  capturePawnMove(dir, index, color, buffer);
}

void straightPawnMove(int dir, int index, int starting_row, byte * buffer){
  if(board[index + (dir * NORTH)] == NO_PIECE){
    buffer[index + (dir * NORTH)] = 1;
    if(index / 16 == starting_row  && board[index + (dir * NN)] == NO_PIECE){
          buffer[index + (dir * NN)] = 1;
    }
  }
}

void capturePawnMove(int dir, int index, int color, byte * buffer){
  int  attack_index = index + (dir * NW);
  byte attack_piece = board[attack_index];
  if(attack_piece != NO_PIECE && (attack_piece & B10) != color)
    buffer[attack_index] = 1; 
  attack_index = index + (dir * NE);
  attack_piece = board[attack_index];
  if(attack_piece != NO_PIECE && (attack_piece & B10) != color)
    buffer[attack_index] = 1; 
}



/*
*     TECHNICAL RELATED STUFF
*/

void setup() {
  pinMode(MUX_ONE   , OUTPUT);  
  pinMode(MUX_TWO   , OUTPUT);  
  pinMode(MUX_THREE , OUTPUT);  
  pinMode(DMUX_ONE  , OUTPUT);  
  pinMode(DMUX_TWO  , OUTPUT);  
  pinMode(DMUX_THREE, OUTPUT); 
  Serial.begin(9600);      // open the serial port at 9600 bps:    
  
  pinMode(HALLIN, INPUT_PULLUP); 
  lc.setLed(0, 0, 0, true);

  lc.shutdown(0,false);
  lc.setIntensity(0,15);
  lc.clearDisplay(0);
  
//  initLights();
  init_board();

  board[83] = W_KNIGHT;
  pseudoLegalMoves(98, move_buffer);
  filterLegitMoves(98, move_buffer);
  
  moveBufferToLED();

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

/* Cool initial light effects */
void initLights(){
  int myDelay = 20;
  for(int row=0;row<8;row++) {
    for(int col=0;col<8;col++) {
      lc.setLed(0,row,col,true);
      delay(myDelay);     
    }
  }
  
  for(int i = 0; i < 16; i++){lc.setIntensity(0,15 - i); delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,i);      delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,15 - i); delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,i);      delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,15 - i); delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,i);      delay(myDelay);}
  
  for(int row=0;row<8;row++) {
      for(int col=0;col<8;col++) {
        lc.setLed(0,row,col,false);
        delay(myDelay);
      }
    }
}

void loop() {
    readSensors();
    processDeltaArray();
    doTheFancyLights();
}
