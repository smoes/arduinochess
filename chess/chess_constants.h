

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

