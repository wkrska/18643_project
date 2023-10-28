#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <stdlib.h>

struct table_data_t {
  char *arr;
  int xdim;
  int ydim;
};

table_data_t parse_table_file(char *filename, int xdim, int ydim) {
  std::cout << filename << " xdim: " << xdim << ", ydim: " << ydim << std::endl;
  char *arr = (char *) malloc(xdim * ydim * sizeof(char));

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
