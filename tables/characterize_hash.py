#!/usr/bin/env python3

import os
import sys
import argparse
import hashlib
import random
import statistics


def main(arguments):
    parser = argparse.ArgumentParser(description='Arguments for input and output files')
    parser.add_argument('--infile', type=str, required=True, help='.tbl file in 18643_project/tables to preprocess')
    args = parser.parse_args(arguments)

    inf = open(args.infile, 'r')
    inf_lines = inf.readlines()

    occupancies = []
    for line in inf_lines:
        split_line = line.split()
        occupancy = int(split_line[-1])
        occupancies.append(occupancy)
    hash_occupancy_stdev = statistics.stdev(occupancies)
    print(args.infile + ': ' + str(hash_occupancy_stdev))


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

