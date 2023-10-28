#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <stdlib.h>

struct table_data_t {
  int *arr;
  int xdim;
  int ydim;
};

void print_table_data(table_data_t t) {
  for (int i = 0; i < t.ydim; i++) {
    for (int j = 0; j < t.xdim; j++) {
      printf("%3d ", t.arr[i * t.xdim + j]);
    }
    printf("\n");
  }
  printf("\n");
}

table_data_t parse_table_file(char *filename, int xdim, int ydim) {
  std::cout << filename << " xdim: " << xdim << ", ydim: " << ydim << std::endl;
  int *arr = (int *) malloc(xdim * ydim * sizeof(int));
  std::fstream fileStream;
  fileStream.open(filename);
  int holder;
  for (int i = 0; i < ydim; i++) {
    for (int j = 0; j < xdim; j++) {
      fileStream >> holder;
      arr[i * xdim + j] = holder;
    }
  }
  fileStream.close();

  table_data_t table;
  table.arr = arr;
  table.xdim = xdim;
  table.ydim = ydim;

  return table;
}
 
int main(int argc, char *argv[])
{
  if (argc != 10) {
    std::cout << "ARGS ERROR" << std::endl;
    std::cout << "./main <t1_filename> <t1_xdim> <t1_ydim> <t2_filename> <t2_xdim> <t2_ydim> <t3_filename> <t3_xdim> <t3_ydim>" << std::endl;
    return 1;
  }

  int t1_xdim = strtol(argv[2], nullptr, 0);
  int t1_ydim = strtol(argv[3], nullptr, 0);
  table_data_t t1_data = parse_table_file(argv[1], t1_xdim, t1_ydim);
  std::cout << "TABLE 1" << std::endl;
  print_table_data(t1_data);

  int t2_xdim = strtol(argv[5], nullptr, 0);
  int t2_ydim = strtol(argv[6], nullptr, 0);
  table_data_t t2_data = parse_table_file(argv[4], t2_xdim, t2_ydim);
  std::cout << "TABLE 2" << std::endl;
  print_table_data(t2_data);
//  char **t1_arr = 

  //std::cout << "argc == " << argc << '\n';

  //for (int ndx{}; ndx != argc; ++ndx)
  //    std::cout << "argv[" << ndx << "] == " << argv[ndx] << '\n';
  //std::cout << "argv[" << argc << "] == "
  //          << static_cast<void*>(argv[argc]) << '\n';

  /*...*/

  //return argc == 3 ? EXIT_SUCCESS : EXIT_FAILURE; // optional return value
  return 0;
}
