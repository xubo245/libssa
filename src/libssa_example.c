/*
 Copyright (C) 2014-2015 Jakob Frielingsdorf

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as
 published by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 Contact: Jakob Frielingsdorf <jfrielingsdorf@gmail.com>
 */

#include "libssa.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

/*
 * Using UniProtKB/Swiss-Prot database.
 *
 * Can be downloaded from here: http://www.uniprot.org/downloads
 *
 * Run it with:
 * ./libssa_example -N 1 -O -3 -E -1 -M BLOSUM50 -c 10 -i tests/testdata/O74807.fasta -d tests/testdata/uniprot_sprot.fasta -t SW -b 16 -s AVX2
 */

#define SSE2 0
#define SSE41 1
#define AVX2 2

#define SW 0
#define NW 1

static void print_help() {
    printf("Usage: ./libssa_example [options]\n");
    printf("Commandline options:\n");

    printf("  -N             Number of used threads\n");
    printf("  -O             Gap open costs (unsigned)\n");
    printf("  -E             Gap extension costs (unsigned)\n");
    printf("  -M             Scoring matrix name (BLOSUM(45|50|62|80|90)|PAM(30|70|250))\n");
    printf("  -i             Query file (FASTA format)\n");
    printf("  -d             Database file (FASTA format)\n");
    printf("  -c             Number of returned hits\n");
    printf("  -t             SW oder NW for local or global alignment\n");
    printf("  -b             Bit-width for the main search (in case of an overflow a fallback to the next bigger bit-width is used\n");
    printf("  -s             AVX2|SSE41|SSE2 decides which SIMD implementation is used (SSE2 does not support local alignments)\n");
    printf("\n");

    exit(EXIT_FAILURE);
}

static void read_options( int argc, char ** argv, uint8_t * gapO, uint8_t * gapE, int * search_type, int * bit_width,
        int * simd, p_query * query, size_t * hit_count ) {
    int c;

    if( argc == 1 ) {
        print_help();
    }

    while( (c = getopt( argc, argv, "N:O:E:M:i:d:c:t:b:s:" )) != -1 ) {
        switch( c ) {
        case 'N':
            set_thread_count( atoi( optarg ) );
            break;
        case 'O':
            *gapO = atoi( optarg );
            break;
        case 'E':
            *gapE = atoi( optarg );
            break;
        case 'M':
            init_score_matrix( MATRIX_BUILDIN, optarg );
            break;
        case 'i':
            *query = init_sequence_fasta( READ_FROM_FILE, optarg );
            break;
        case 'd':
            init_db( optarg );
            break;
        case 't':
            if( strcmp( optarg, "SW" ) == 0 ) {
                *search_type = SW;
            }
            else if( strcmp( optarg, "NW" ) == 0 ) {
                *search_type = NW;
            }
            break;
        case 'b':
            *bit_width = atoi( optarg );
            break;
        case 'c':
            *hit_count = atoi( optarg );
            break;
        case 's':
            if( strcmp( optarg, "AVX2" ) == 0 ) {
                *simd = AVX2;
            }
            else if( strcmp( optarg, "SSE41" ) == 0 ) {
                *simd = SSE41;
            }
            else if( strcmp( optarg, "SSE2" ) == 0 ) {
                *simd = SSE2;
            }
            break;
        default:
            print_help();
            break;
        }
    }

    if( *bit_width == -1 ) {
        fprintf( stderr, "Unknown bit width, try --help.\n" );
        exit( EXIT_FAILURE );
    }

    if( optind > argc ) {
        fprintf( stderr, "Error getting program options, try --help.\n" );
        exit( EXIT_FAILURE );
    }
}

static void print_alignment( p_alignment_list alist ) {
    printf( "Nr of alignments: %ld\n", alist->len );

    printf( "(Score, DB-ID), " );
    for( size_t i = 0; i < alist->len; i++ ) {
        p_alignment a = alist->alignments[i];

        printf( "(%ld, %ld), ", a->score, a->db_seq.ID );
    }
    printf( "\n" );
}

int main( int argc, char**argv ) {
    p_query query = 0;
    uint8_t gapO = 0, gapE = 0;
    int search_type = -1;
    int bit_width = -1;
    int simd = -1;
    size_t hit_count = 0;

    set_output_mode( OUTPUT_INFO );

    set_chunk_size( 1000 );

    init_symbol_translation( AMINOACID, FORWARD_STRAND, 3, 3 ); // TODO currently not configurable

    read_options( argc, argv, &gapO, &gapE, &search_type, &bit_width, &simd, &query, &hit_count );
    init_gap_penalties( gapO, gapE );

    if( simd == AVX2 ) {
        set_simd_compute_mode( COMPUTE_ON_AVX2 );
    }
    else if( simd == SSE41 ) {
        set_simd_compute_mode( COMPUTE_ON_SSE41 );
    }
    else if( simd == SSE2 ) {
        set_simd_compute_mode( COMPUTE_ON_SSE2 );
    }
    else {
        fprintf( stderr, "Unknown SIMD option, try --help.\n" );
        exit( EXIT_FAILURE );
    }

    p_alignment_list alist;
    if( search_type == SW ) {
        alist = sw_align( query, hit_count, bit_width, COMPUTE_SCORE );
    }
    else if( search_type == NW ) {
        alist = nw_align( query, hit_count, bit_width, COMPUTE_SCORE );
    }
    else {
        fprintf( stderr, "Unknown search type, try --help.\n" );
        exit( EXIT_FAILURE );
    }

    print_alignment( alist );

    free_alignment( alist );

    free_sequence( query );

    ssa_exit();

    return 0;
}
