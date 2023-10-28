//------------------------------------------------------------------------------
//
// kernel:  exp1a
//
// Purpose: Determine memory bandwidth by reading row-by-row
//

// Update values when needed!!
#define DATA_DIM 4096
//#define DATA_DIM 128

extern "C" {
void krnl_exp1a(const int *in,        // Read-Only Matrix
                int *out) {
  int num00 = 852129405;
  int num01 = 1297230771;
  int num02 = 721781807;
  int num03 = 701130504;
  int num04 = -1877593432;
  int num05 = -686465853;
  int num06 = 1758295511;
  int num07 = -1973247524;
  int num08 = 933706032;
  int num09 = 2056139590;
  int num10 = -1538942814;
  int num11 = 773576297;
  int num12 = 1336751546;
  int num13 = -327623126;
  int num14 = 192869803;
  int num15 = -2046359899;

  int i_index = 0;

  for (int i = 0; i < DATA_DIM; i++) {
    for (int j = 0; j < DATA_DIM; j+=16) {
      num00 ^= in[i_index+j];
      num01 ^= in[i_index+j+1];
      num02 ^= in[i_index+j+2];
      num03 ^= in[i_index+j+3];
      num04 ^= in[i_index+j+4];
      num05 ^= in[i_index+j+5];
      num06 ^= in[i_index+j+6];
      num07 ^= in[i_index+j+7];
      num08 ^= in[i_index+j+8];
      num09 ^= in[i_index+j+9];
      num10 ^= in[i_index+j+10];
      num11 ^= in[i_index+j+11];
      num12 ^= in[i_index+j+12];
      num13 ^= in[i_index+j+13];
      num14 ^= in[i_index+j+14];
      num15 ^= in[i_index+j+15];
    }
    i_index += DATA_DIM;
  }

  out[0] = num00^num01^num02^num03^num04^num05^num06^num07^num08^num09^num10^num11^num12^num13^num14^num15;
}
}
