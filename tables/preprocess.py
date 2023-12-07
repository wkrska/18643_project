#!/usr/bin/env python3

import os
import sys
import argparse
import hashlib


def main(arguments):
    parser = argparse.ArgumentParser(description='Arguments for input and output files')
    parser.add_argument('--infile', type=str, required=True, help='.tbl file in 18643_project/tables to preprocess')
    parser.add_argument('--outfile', type=str, required=True, help='.txt file in 18643_project/tables to store preprocessing output')
    args = parser.parse_args(arguments)

    inf = open(args.infile, 'r')
    outf = open(args.outfile, 'w')
    inf_lines = inf.readlines()
    outf_lines = []

    # Leave header line as-is, except for replacing '|' separators with spaces
    outf_lines.append(inf_lines[0].replace('|', ' ').strip() + '\n')

    # Hash items below the header line
    for line in inf_lines[1:]:
        newline_removed = line.strip('\n')
        line_split = newline_removed.split('|')
        line_hashes = []
        for string in line_split[1:-1]:
            string_hash = str(int(hashlib.sha256(string.encode('utf-8')).hexdigest()[:8], 16))
            line_hashes.append(string_hash)
        hashes = ''
        for individual_hash in line_hashes:
            hashes += (individual_hash + ' ')
        hash_line = line_split[0] + ' ' + hashes.strip() + '\n'
        outf_lines.append(hash_line)

    outf.writelines(outf_lines)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
