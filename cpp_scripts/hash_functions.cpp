#include <cstdint>

// Rough estimate of computational complexity of different hash functions (based on Intel Skylake latencies)
// bitwise                    1
// add/sub (addsd, subsd)     4
// multiply (mulsd)           4
// divide (divsd)         13-14
// sqrt (sqrtpd)          15-16
//
// modulo          : 1 x bitwise (modulo power of 2)                    = 1
// div             : 2 x bitwise (modulo power of 2 and div power of 2) = 2
// djb2            : 1 x bitwise + 2 * add/sub                          = 9
// magicinthash    : 6 x bitwise + 2 * multiply                         = 14
// hash32shiftmult : 8 x bitwise + 1 * add/sub + 1 * multiply           = 16
// hash32shift     : 9 x bitwise + 2 * add/sub + 1 * multiply           = 21
// jenkins32hash   : 10 x bitwise + 7 * add/sub                         = 38

// This one is also meant to work on strings
uint32_t djb2(uint32_t val) {
  uint32_t hash = 5381;

  //for (uint32_t i = 0; i < len; i++) {
  //  hash = ((hash << 5) + hash) + val[i];
  //}
  hash = ((hash << 5) + hash) + val;

  return hash;
}

// This one doesn't make sense to do on a single int, just returns same value
uint32_t sbdm(uint32_t *val, uint32_t len) {
  uint32_t hash = 0;

  for (uint32_t i = 0; i < len; i++) {
    hash = val[i] + (hash << 6) + (hash << 16) - hash;
  }

  return hash;
}

uint32_t magic_int_hash(uint32_t val) {
  val = ((val >> 16) ^ val) * 0x45d9f3b;
  val = ((val >> 16) ^ val) * 0x45d9f3b;
  val = (val >> 16) ^ val;
  int negcheck = (int) val;
  if (negcheck < 0) {
    negcheck = negcheck * -1;
    return (uint32_t) negcheck;
  }
  return val;
}

uint32_t hash32shift(uint32_t val) {
  int sval;
  sval = ~((int)val) + (((int)val) << 15);
  val = ((uint32_t)sval) ^ (((uint32_t)sval) >> 12);
  sval = ((int)val) + (((int)val) << 2);
  sval = ((uint32_t)sval) ^ (((uint32_t)sval) >> 4);
  sval = sval * 2057;
  val = ((uint32_t)sval) ^ (((uint32_t)sval) >> 16);
  int negcheck = (int) val;
  if (negcheck < 0) {
    negcheck = negcheck * -1;
    return (uint32_t) negcheck;
  }
  return val; 
}

uint32_t jenkins32hash(uint32_t val) {
  val = (val+0x7ed55d16) + (val<<12);
  val = (val^0xc761c23c) ^ (val>>19);
  val = (val+0x165667b1) + (val<<5);
  val = (val+0xd3a2646c) ^ (val<<9);
  val = (val+0xfd7046c5) + (val<<3);
  val = (val^0xb55a4f09) ^ (val>>16);
  int negcheck = (int) val;
  if (negcheck < 0) {
    negcheck = negcheck * -1;
    return (uint32_t) negcheck;
  }
  return val;
}

uint32_t hash32shiftmult(uint32_t val) {
  int sval;
  uint32_t c2=0x27d4eb2d; // a prime or an odd constant
  val = (val ^ 61) ^ (val >> 16);
  sval = ((int)val) + (((int)val) << 3);
  val = (uint32_t(sval)) ^ ((uint32_t(sval)) >> 4);
  val = val * c2;
  val = val ^ (val >> 15);
  int negcheck = (int) val;
  if (negcheck < 0) {
    negcheck = negcheck * -1;
    return (uint32_t) negcheck;
  }
  return val;
}
