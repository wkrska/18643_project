#!/usr/bin/env python3

import os
import sys
import argparse
import hashlib
import random
import statistics


def main(arguments):
    parser = argparse.ArgumentParser(description='Arguments for input and output files')
    parser.add_argument('--infiles', type=str, required=True, help='my name jeff')
    args = parser.parse_args(arguments)

    all_filenames = args.infiles.split('SPLIT')
    stdevs = []
    max_occupancy = 0
    for filename in all_filenames:
        inf = open('/home/18643_team/edwin_stuff/18643_project/cpp_scripts/' + filename, 'r')
        inf_lines = inf.readlines()

        occupancies = []
        for line in inf_lines:
            split_line = line.split()
            occupancy = int(split_line[-1])
            if occupancy > max_occupancy:
                max_occupancy = occupancy
            occupancies.append(occupancy)
        hash_occupancy_stdev = statistics.stdev(occupancies)
        stdevs.append(hash_occupancy_stdev)
        print(filename + ': ' + str(hash_occupancy_stdev))

    curr_sum = 0
    for stdev in stdevs:
        curr_sum += stdev
    avg = curr_sum / len(stdevs)
    print('avg: ' + str(avg))
    print('max_occupancy: ' + str(max_occupancy))


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

