/*
 * search.h
 *
 *  Created on: Oct 18, 2014
 *      Author: Jakob Frielingsdorf
 */

#ifndef SEARCH_16_H_
#define SEARCH_16_H_

#include <stdint.h>
#include <immintrin.h>

#include "../../libssa_datatypes.h"
#include "../../util/minheap.h"

struct s16query {
    unsigned long q_len;

    __m128i ** q_table;
};
typedef struct s16query * p_s16query;

struct s16info { // TODO rename
    __m128i matrix[32];
    __m128i * hearray;
    __m128i * dprofile;

    int q_count;
    p_s16query queries[6];

    unsigned long maxdlen;

    int16_t penalty_gap_open;
    int16_t penalty_gap_extension;
};
typedef struct s16info * p_s16info;

void search16_init_algo( int search_type );

p_s16info search16_init( int16_t penalty_gap_open, int16_t penalty_gap_extension );

void search16_exit( p_s16info s );

void search16_init_query( p_s16info s, int q_count, seq_buffer * queries );

void search_16_sw( p_s16info s, p_db_chunk chunk, p_minheap heap, int query_id );

void search_16_nw( p_s16info s, p_db_chunk chunk, p_minheap heap, int query_id );

void search16_nw_sellers( p_s16info s, p_db_chunk chunk, p_minheap heap, int query_id );

void search_16( p_db_chunk chunk, p_search_data sdp, p_search_result res );

#endif /* SEARCH_16_H_ */