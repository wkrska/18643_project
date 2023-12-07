#ifndef TYPES_H
#define TYPES_H

// James loves to redefine EVERY SINGLE TYPE EVER no idea why, but I didn't feel like replacing all of them in the code so this is here
#define BITSINBYTE (8)
#define BYTEMASK ((1 << BITSINBYTE) - 1)
#define LGBITSINBYTE (3)
#define SECINUSEC (1000000)

typedef unsigned char UBYTE; // a byte
typedef bool BOOL;           // stand-in for Boolean

typedef unsigned short UIDX; // small index for HW loops: 64k is enough
typedef unsigned int UIDXL;  // large index for data loops

typedef unsigned long ULONG;   // 8-byte generic
typedef unsigned short USHORT; // 2-byte generic
typedef unsigned char UCHAR;   // 1-byte generic

typedef int INT;
typedef unsigned int UINT;

// These are the more reasonable things to redefine. 
// You can also make a struct of structs if you want (like make the value a complex number, idk)

typedef unsigned int JoinKey; // Key used in join
typedef unsigned int JoinVal; // Value associated with key
typedef unsigned short HashVal; // Hasshed key
typedef unsigned short HashSel; // Selection for hash method/combo

typedef unsigned int ClockCounter;

struct Tuple {
  BOOL valid;
  JoinKey key;
  JoinVal val;
  BOOL eof;
};

struct HashedTuple {
  BOOL valid;
  JoinKey key;
  JoinVal val;
  HashVal h0;
  HashVal h1;
  BOOL eof;
};

struct MatchedTuple {
  BOOL valid;
  JoinKey key;
  JoinVal val0;
  JoinVal val1;
  BOOL eof;
};

struct HashEntry {
  BOOL full = 0;
  JoinKey key;
  JoinVal val;
  JoinKey tag; // Alternative hash value
};

struct HashBucket {
  HashEntry entries[_SLTS_];
};

#endif

