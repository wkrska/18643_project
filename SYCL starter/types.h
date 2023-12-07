#ifndef TYPES_H
#define TYPES_H

// James loves to redefine EVERY SINGLE TYPE EVER no idea why, but I didn't feel like replacing all of them in the code so this is here
#define BITSINBYTE (8)
#define BYTEMASK ((1 << BITSINBYTE) - 1)
#define LGBITSINBYTE (3)

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

typedef unsigned int AggKey;
typedef unsigned int AggVal;

#define TUPLEN (4)

struct Tuple {
  BOOL valid;
  AggKey key;
  AggVal val;
};

//
// Tuple padded to 16-bytes
//
struct TuplePadded16 {
  UINT valid;
  AggKey key;
  AggVal val;
  UINT pad;
};

// left this here so it does't break
typedef unsigned short AggTblIdx;

#endif

