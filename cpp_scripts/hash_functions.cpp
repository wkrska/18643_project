#include <cstdint>

// This one is also meant to work on strings
uint32_t djb2(uint32_t *val, uint32_t len) {
  uint32_t hash = 5381;

  for (uint32_t i = 0; i < len; i++) {
    hash = ((hash << 5) + hash) + val[i];
  }

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
  return val;
}

uint32_t hash32shift(uint32_t val) {
  int sval;
  sval = ~((int)val) + (((int)val) << 15);
  //val = ~val + (val << 15);
  val = ((uint32_t)sval) ^ (((uint32_t)sval) >> 12);
  //val = val ^ (val >>> 12);
  sval = ((int)val) + (((int)val) << 2);
  //val = val + (val << 2);
  sval = ((uint32_t)sval) ^ (((uint32_t)sval) >> 4);
  //val = val ^ (val >>> 4);
  sval = sval * 2057;
  //val = val * 2057;
  val = ((uint32_t)sval) ^ (((uint32_t)sval) >> 16);
  //val = val ^ (val >>> 16);
  return val; 
}
