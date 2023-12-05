#include <cstdint>

uint32_t djb2(uint32_t *val, uint32_t len) {
  uint32_t hash = 5381;

  for (uint32_t i = 0; i < len; i++) {
    hash = ((hash << 5) + hash) + val[i];
  }

  return hash;
}
