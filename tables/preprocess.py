#!/usr/bin/env python3

import os
import sys
import argparse
import hashlib
import random


def rand_unique(unique_keys):
    rand_try = str(random.randrange(0, 2147483647))
    if rand_try in unique_keys:
        return rand_unique(unique_keys)
    else:
        return rand_try


def main(arguments):
    parser = argparse.ArgumentParser(description='Arguments for input and output files')
    parser.add_argument('--infile', type=str, required=True, help='.tbl file in 18643_project/tables to preprocess')
    parser.add_argument('--outfile', type=str, required=True, help='.txt file in 18643_project/tables to store preprocessing output')
    parser.add_argument('--parse-stage', type=str, required=True, help='specify whether to parse tpch-gen output or to generate first table with unique join column')
    parser.add_argument('--join-column', type=str, required=False, help='if parsing unique stage, specify join column integer for unique join selection')
    parser.add_argument('--num-rows', type=int, required=False, help='if parsing unique stage, specify number of columns to generate for unique join selection')
    args = parser.parse_args(arguments)

    inf = open(args.infile, 'r')
    outf = open(args.outfile, 'w')
    inf_lines = inf.readlines()
    outf_lines = []

    if args.parse_stage == 'tpch':
        # Leave header line as-is, except for replacing '|' separators with spaces
        outf_lines.append(inf_lines[0].replace('|', ' ').strip() + '\n')

        # Hash items below the header line
        for line in inf_lines[1:]:
            newline_removed = line.strip('\n')
            line_split = newline_removed.split('|')
            line_hashes = []
            for string in line_split[1:-1]:
                string_hash = str(int(hashlib.sha256(string.encode('utf-8')).hexdigest()[:7], 16))
                line_hashes.append(string_hash)
            hashes = ''
            for individual_hash in line_hashes:
                hashes += (individual_hash + ' ')
            hash_line = line_split[0] + ' ' + hashes.strip() + '\n'
            outf_lines.append(hash_line)
        outf.writelines(outf_lines)

    elif args.parse_stage == 'unique':
        # Parse header line w/ column names
        col_index = -1
        curr_index = 1
        for column_name in inf_lines[0].split()[1:]:
            if not args.join_column:
                raise ValueError('need --join-column arg for unique col table')
            if column_name == args.join_column:
                col_index = curr_index
            curr_index += 1
        if col_index == -1:
            raise ValueError('invalid column name provided')

        # Finding unique keys
        unique_col_vals = {}
        for line in inf_lines[1:]:
            line_split = line.split()
            join_col_val = line_split[col_index]
            if join_col_val in unique_col_vals.keys():
                unique_col_vals[join_col_val] += 1
            else:
                unique_col_vals[join_col_val] = 1
        unique_keys = list(unique_col_vals.keys())
        random.shuffle(unique_keys)

        # Generating new tables
        outf_lines.append(str(0) + ' ' + args.join_column + '\n')
        if not args.num_rows:
            raise ValueError('need --num-rows arg for unique col table')
        next_keys = []
        for i in range(args.num_rows):
            if len(unique_keys) > i:
                next_key = unique_keys[i]
            else:
                next_key = rand_unique(unique_keys)
            next_keys.append(next_key)
        random.shuffle(next_keys)
        for i in range(len(next_keys)):
            next_line = str(i+1) + ' ' + next_keys[i] + '\n'
            outf_lines.append(next_line)

        outf.writelines(outf_lines)
    else:
        raise ValueError('must specify tpch or unique for table preprocessing')


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
