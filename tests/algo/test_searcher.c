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

#include "../tests.h"

#include "../../src/util/util.h"
#include "../../src/libssa.h"
#include "../../src/util/minheap.h"
#include "../../src/matrices.h"
#include "../../src/cpu_config.h"
#include "../../src/query.h"
#include "../../src/algo/searcher.h"
#include "../../src/algo/gap_costs.h"
#include "../../src/db_adapter.h"

static p_search_result setup_searcher_test( int bit_width, int search_type, char * query_string, char * db_file,
        size_t hit_count, int symtype, int strands ) {
    init_symbol_translation( symtype, strands, 3, 3 );
    mat_init_constant_scoring( 1, -1 );
    p_query query = query_read_from_string( query_string );

    s_init( search_type, bit_width, query );

    ssa_db_init( concat( "./tests/testdata/", db_file ) );

    gapO = -1;
    gapE = -1;

    adp_init( hit_count );

    p_search_result res = s_search( &hit_count );

    minheap_sort( res->heap );

    query_free( query );

    return res;
}

static void exit_searcher_test( p_search_result res ) {
    s_free( res );
    adp_exit();
    mat_free();

    reset_compute_capability();
}

static void do_searcher_test_simple( int bit_width, int search_type, long score ) {
    p_search_result res = setup_searcher_test( bit_width, search_type, "AT", "short_db.fas", 1, NUCLEOTIDE,
    FORWARD_STRAND );

    ck_assert_int_eq( 1, res->chunk_count );
    ck_assert_int_eq( 1, res->seq_count );

    ck_assert_int_eq( 1, res->heap->alloc );
    ck_assert_int_eq( 1, res->heap->count );

    elem_t e = res->heap->array[0];
    ck_assert_int_eq( 0, e.db_id );
    ck_assert_int_eq( 0, e.query_id );

    ck_assert_int_eq( score, e.score );

    exit_searcher_test( res );
}

START_TEST (test_searcher_simple_sw_64)
    {
        do_searcher_test_simple( BIT_WIDTH_64, SMITH_WATERMAN, 2 );
    }END_TEST

START_TEST (test_searcher_simple_nw_64)
    {
        do_searcher_test_simple( BIT_WIDTH_64, NEEDLEMAN_WUNSCH, -2 );
    }END_TEST

START_TEST (test_searcher_simple_sw_16)
    {
        set_max_compute_capability( COMPUTE_ON_SSE2 );

        do_searcher_test_simple( BIT_WIDTH_16, SMITH_WATERMAN, 2 );

        set_max_compute_capability( COMPUTE_ON_AVX2 );

        do_searcher_test_simple( BIT_WIDTH_16, SMITH_WATERMAN, 2 );
    }END_TEST

START_TEST (test_searcher_simple_nw_16)
    {
        set_max_compute_capability( COMPUTE_ON_SSE2 );

        do_searcher_test_simple( BIT_WIDTH_16, NEEDLEMAN_WUNSCH, -2 );

        set_max_compute_capability( COMPUTE_ON_AVX2 );

        do_searcher_test_simple( BIT_WIDTH_16, NEEDLEMAN_WUNSCH, -2 );
    }END_TEST

START_TEST (test_searcher_simple_sw_8)
    {
        set_max_compute_capability( COMPUTE_ON_SSE41 );

        do_searcher_test_simple( BIT_WIDTH_8, SMITH_WATERMAN, 2 );

        set_max_compute_capability( COMPUTE_ON_AVX2 );

        do_searcher_test_simple( BIT_WIDTH_8, SMITH_WATERMAN, 2 );
    }END_TEST

START_TEST (test_searcher_simple_nw_8)
    {
        set_max_compute_capability( COMPUTE_ON_SSE41 );

        do_searcher_test_simple( BIT_WIDTH_8, NEEDLEMAN_WUNSCH, -2 );

        set_max_compute_capability( COMPUTE_ON_AVX2 );

        do_searcher_test_simple( BIT_WIDTH_8, NEEDLEMAN_WUNSCH, -2 );
    }END_TEST

static p_search_result init_translate_test( int bit_width, int search_type, char * query, char * db_file,
        size_t hit_count, int symtype, int strands ) {
    p_search_result res = setup_searcher_test( bit_width, search_type, query, db_file, hit_count, symtype, strands );

    ck_assert_int_eq( hit_count, res->heap->count );

    return res;
}

static void do_multiple_sequence_test( int bit_width, int search_type, int result[16] ) {
    p_search_result res = init_translate_test( bit_width, search_type, "ATGCAAATTT", "short_nuc_db.fas", 8, NUCLEOTIDE,
    FORWARD_STRAND );

    p_minheap heap = res->heap;

    /*
     * Result contains tuple of the score and the database ID
     */
    for( int i = 0; i < 16; i += 2 ) {
        ck_assert_int_eq( result[i], heap->array[i / 2].score );
        ck_assert_int_eq( result[i + 1], heap->array[i / 2].db_id );
    }

    exit_searcher_test( res );
}

START_TEST (test_searcher_multiple_sequences_nw_64)
    {
        int result[16] = { -1, 0, -3, 3, -4, 7, -4, 6, -4, 4, -5, 2, -6, 5, -6, 1 };

        do_multiple_sequence_test( BIT_WIDTH_64, NEEDLEMAN_WUNSCH, result );
    }END_TEST

START_TEST (test_searcher_multiple_sequences_sw_64)
    {
        int result[16] = { 4, 3, 4, 1, 4, 0, 3, 7, 3, 5, 3, 4, 3, 2, 2, 6 };

        do_multiple_sequence_test( BIT_WIDTH_64, SMITH_WATERMAN, result );
    }END_TEST

START_TEST (test_searcher_multiple_sequences_nw_16)
    {
        set_max_compute_capability( COMPUTE_ON_SSE2 );

        int result[16] = { -1, 0, -3, 3, -4, 7, -4, 6, -4, 4, -5, 2, -6, 5, -6, 1 };

        set_max_compute_capability( COMPUTE_ON_SSE41 );
        do_multiple_sequence_test( BIT_WIDTH_16, NEEDLEMAN_WUNSCH, result );

        set_max_compute_capability( COMPUTE_ON_AVX2 );
        do_multiple_sequence_test( BIT_WIDTH_16, NEEDLEMAN_WUNSCH, result );
    }END_TEST

START_TEST (test_searcher_multiple_sequences_sw_16)
    {
        set_max_compute_capability( COMPUTE_ON_SSE2 );

        int result[16] = { 4, 3, 4, 1, 4, 0, 3, 7, 3, 5, 3, 4, 3, 2, 2, 6 };

        set_max_compute_capability( COMPUTE_ON_SSE41 );
        do_multiple_sequence_test( BIT_WIDTH_16, SMITH_WATERMAN, result );

        set_max_compute_capability( COMPUTE_ON_AVX2 );
        do_multiple_sequence_test( BIT_WIDTH_16, SMITH_WATERMAN, result );
    }END_TEST

START_TEST (test_searcher_multiple_sequences_nw_8)
    {
        set_max_compute_capability( COMPUTE_ON_SSE41 );

        int result[16] = { -1, 0, -3, 3, -4, 7, -4, 6, -4, 4, -5, 2, -6, 5, -6, 1 };

        set_max_compute_capability( COMPUTE_ON_SSE41 );
        do_multiple_sequence_test( BIT_WIDTH_8, NEEDLEMAN_WUNSCH, result );

        set_max_compute_capability( COMPUTE_ON_AVX2 );
        do_multiple_sequence_test( BIT_WIDTH_8, NEEDLEMAN_WUNSCH, result );
    }END_TEST

START_TEST (test_searcher_multiple_sequences_sw_8)
    {
        set_max_compute_capability( COMPUTE_ON_SSE41 );

        int result[16] = { 4, 3, 4, 1, 4, 0, 3, 7, 3, 5, 3, 4, 3, 2, 2, 6 };

        set_max_compute_capability( COMPUTE_ON_SSE41 );
        do_multiple_sequence_test( BIT_WIDTH_8, SMITH_WATERMAN, result );

        set_max_compute_capability( COMPUTE_ON_AVX2 );
        do_multiple_sequence_test( BIT_WIDTH_8, SMITH_WATERMAN, result );
    }END_TEST

static void test_result( p_search_result res, int * result, int result_len ) {
    minheap_sort( res->heap );

    ck_assert_int_eq( result_len / 2, res->heap->count );

    p_minheap heap = res->heap;

    /*
     * Result contains tuple of the score and the database ID
     */
    for( int i = 0; i < result_len; i += 2 ) { // TODO test query ID as well!!
        ck_assert_int_eq( result[i], heap->array[i / 2].score );
        ck_assert_int_eq( result[i + 1], heap->array[i / 2].db_id );
    }

    exit_searcher_test( res );
}

START_TEST (test_searcher_translate_nw_64)
    {
        int result[12] = { -109, 0, -109, 0, -110, 0, -110, 0, -110, 0, -113, 0 };

        p_search_result res = init_translate_test( BIT_WIDTH_64, NEEDLEMAN_WUNSCH,
                "ATGCCCAAAATTGACTTGAATAGGGGCGTGAGGAGGGGTTTTCAATACTATTTTTTG", "short_AA.fas", 6,
                TRANS_QUERY,
                BOTH_STRANDS );

        test_result( res, result, 12 );
    }END_TEST

START_TEST (test_searcher_translate_sw_64)
    {
        int result[12] = { 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 1, 0 };

        p_search_result res = init_translate_test( BIT_WIDTH_64, SMITH_WATERMAN,
                "ATGCCCAAAATTGACTTGAATAGGGGCGTGAGGAGGGGTTTTCAATACTATTTTTTG", "short_AA.fas", 6,
                TRANS_QUERY,
                BOTH_STRANDS );

        test_result( res, result, 12 );
    }END_TEST

START_TEST (test_searcher_translate_nw_16)
    {
        set_max_compute_capability( COMPUTE_ON_SSE2 );

        int result[12] = { -109, 0, -109, 0, -110, 0, -110, 0, -110, 0, -113, 0 };

        p_search_result res = init_translate_test( BIT_WIDTH_16, NEEDLEMAN_WUNSCH,
                "ATGCCCAAAATTGACTTGAATAGGGGCGTGAGGAGGGGTTTTCAATACTATTTTTTG", "short_AA.fas", 6,
                TRANS_QUERY,
                BOTH_STRANDS );

        test_result( res, result, 12 );
    }END_TEST

START_TEST (test_searcher_translate_sw_16)
    {
        set_max_compute_capability( COMPUTE_ON_SSE2 );

        int result[12] = { 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 1, 0 };

        p_search_result res = init_translate_test( BIT_WIDTH_16, SMITH_WATERMAN,
                "ATGCCCAAAATTGACTTGAATAGGGGCGTGAGGAGGGGTTTTCAATACTATTTTTTG", "short_AA.fas", 6,
                TRANS_QUERY,
                BOTH_STRANDS );

        test_result( res, result, 12 );
    }END_TEST

START_TEST (test_searcher_translate_nw_8)
    {
        set_max_compute_capability( COMPUTE_ON_SSE41 );

        int result[12] = { -109, 0, -109, 0, -110, 0, -110, 0, -110, 0, -113, 0 };

        p_search_result res = init_translate_test( BIT_WIDTH_8, NEEDLEMAN_WUNSCH,
                "ATGCCCAAAATTGACTTGAATAGGGGCGTGAGGAGGGGTTTTCAATACTATTTTTTG", "short_AA.fas", 6,
                TRANS_QUERY,
                BOTH_STRANDS );

        test_result( res, result, 12 );
    }END_TEST

START_TEST (test_searcher_translate_sw_8)
    {
        set_max_compute_capability( COMPUTE_ON_SSE41 );

        int result[12] = { 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 1, 0 };

        p_search_result res = init_translate_test( BIT_WIDTH_8, SMITH_WATERMAN,
                "ATGCCCAAAATTGACTTGAATAGGGGCGTGAGGAGGGGTTTTCAATACTATTTTTTG", "short_AA.fas", 6,
                TRANS_QUERY,
                BOTH_STRANDS );

        test_result( res, result, 12 );
    }END_TEST

START_TEST (test_searcher_AA_nw_64)
    {
        int result[12] = { -87, 0 };

        p_search_result res = init_translate_test( BIT_WIDTH_64, NEEDLEMAN_WUNSCH,
                "HPEVYILIIPGFGIISHVVSTYSKKPVFGEISMVYAMASIGLLGFLVWSHHMYIVGLDADTRAYFTSATMIIAIPTGIKI", "short_AA.fas", 1,
                AMINOACID,
                FORWARD_STRAND );

        test_result( res, result, 2 );
    }END_TEST

START_TEST (test_searcher_AA_sw_64)
    {
        int result[12] = { 3, 0 };

        p_search_result res = init_translate_test( BIT_WIDTH_64, SMITH_WATERMAN,
                "HPEVYILIIPGFGIISHVVSTYSKKPVFGEISMVYAMASIGLLGFLVWSHHMYIVGLDADTRAYFTSATMIIAIPTGIKI", "short_AA.fas", 1,
                AMINOACID,
                FORWARD_STRAND );

        test_result( res, result, 2 );
    }END_TEST

START_TEST (test_searcher_AA_nw_16)
    {
        set_max_compute_capability( COMPUTE_ON_SSE2 );

        int result[12] = { -87, 0 };

        p_search_result res = init_translate_test( BIT_WIDTH_16, NEEDLEMAN_WUNSCH,
                "HPEVYILIIPGFGIISHVVSTYSKKPVFGEISMVYAMASIGLLGFLVWSHHMYIVGLDADTRAYFTSATMIIAIPTGIKI", "short_AA.fas", 1,
                AMINOACID,
                FORWARD_STRAND );

        test_result( res, result, 2 );
    }END_TEST

START_TEST (test_searcher_AA_sw_16)
    {
        set_max_compute_capability( COMPUTE_ON_SSE2 );

        int result[12] = { 3, 0 };

        p_search_result res = init_translate_test( BIT_WIDTH_16, SMITH_WATERMAN,
                "HPEVYILIIPGFGIISHVVSTYSKKPVFGEISMVYAMASIGLLGFLVWSHHMYIVGLDADTRAYFTSATMIIAIPTGIKI", "short_AA.fas", 1,
                AMINOACID,
                FORWARD_STRAND );

        test_result( res, result, 2 );
    }END_TEST

START_TEST (test_searcher_AA_nw_8)
    {
        set_max_compute_capability( COMPUTE_ON_SSE41 );

        int result[12] = { -87, 0 };

        p_search_result res = init_translate_test( BIT_WIDTH_8, NEEDLEMAN_WUNSCH,
                "HPEVYILIIPGFGIISHVVSTYSKKPVFGEISMVYAMASIGLLGFLVWSHHMYIVGLDADTRAYFTSATMIIAIPTGIKI", "short_AA.fas", 1,
                AMINOACID,
                FORWARD_STRAND );

        test_result( res, result, 2 );
    }END_TEST

START_TEST (test_searcher_AA_sw_8)
    {
        set_max_compute_capability( COMPUTE_ON_SSE41 );

        int result[12] = { 3, 0 };

        p_search_result res = init_translate_test( BIT_WIDTH_8, SMITH_WATERMAN,
                "HPEVYILIIPGFGIISHVVSTYSKKPVFGEISMVYAMASIGLLGFLVWSHHMYIVGLDADTRAYFTSATMIIAIPTGIKI", "short_AA.fas", 1,
                AMINOACID,
                FORWARD_STRAND );

        test_result( res, result, 2 );
    }END_TEST

static p_search_result setup_BLOSUM62_test( int bit_width, int search_type, size_t hit_count ) {
    init_symbol_translation( AMINOACID, FORWARD_STRAND, 3, 3 );
    mat_init_buildin( BLOSUM62 );

    p_query query = query_read_from_string(
            "HPEVYILIIPGFGIISHVVSTYSKKPVFGEISMVYAMASIGLLGFLVWSHHMYIVGLDADTRAYFTSATMIIAIPTGIKI" );

    s_init( search_type, bit_width, query );

    ssa_db_init( concat( "./tests/testdata/", "short_AA.fas" ) );

    gapO = -1;
    gapE = -1;

    adp_init( hit_count );

    p_search_result res = s_search( &hit_count );

    minheap_sort( res->heap );

    query_free( query );

    ck_assert_int_eq( hit_count, res->heap->count );

    return res;
}

START_TEST (test_searcher_AA_BLOSUM_sw_64)
    {
        int result[12] = { 103, 0 };

        p_search_result res = setup_BLOSUM62_test( BIT_WIDTH_64, SMITH_WATERMAN, 1 );

        test_result( res, result, 2 );
    }END_TEST

START_TEST (test_searcher_AA_BLOSUM_nw_64)
    {
        int result[12] = { 82, 0 };

        p_search_result res = setup_BLOSUM62_test( BIT_WIDTH_64, NEEDLEMAN_WUNSCH, 1 );

        test_result( res, result, 2 );
    }END_TEST

START_TEST (test_searcher_AA_BLOSUM_sw_16)
    {
        set_max_compute_capability( COMPUTE_ON_SSE2 );

        int result[12] = { 103, 0 };

        p_search_result res = setup_BLOSUM62_test( BIT_WIDTH_16, SMITH_WATERMAN, 1 );

        test_result( res, result, 2 );
    }END_TEST

START_TEST (test_searcher_AA_BLOSUM_nw_16)
    {
        set_max_compute_capability( COMPUTE_ON_SSE2 );

        int result[12] = { 82, 0 };

        p_search_result res = setup_BLOSUM62_test( BIT_WIDTH_16, NEEDLEMAN_WUNSCH, 1 );

        test_result( res, result, 2 );
    }END_TEST

START_TEST (test_searcher_AA_BLOSUM_sw_8)
    {
        set_max_compute_capability( COMPUTE_ON_SSE41 );

        int result[12] = { 103, 0 };

        p_search_result res = setup_BLOSUM62_test( BIT_WIDTH_8, SMITH_WATERMAN, 1 );

        test_result( res, result, 2 );
    }END_TEST

START_TEST (test_searcher_AA_BLOSUM_sw_8_avx)
    {
        set_max_compute_capability( COMPUTE_ON_AVX2 );

        int result[12] = { 103, 0 };

        p_search_result res = setup_BLOSUM62_test( BIT_WIDTH_8, SMITH_WATERMAN, 1 );

        test_result( res, result, 2 );
    }END_TEST

START_TEST (test_searcher_AA_BLOSUM_nw_8)
    {
        set_max_compute_capability( COMPUTE_ON_SSE41 );

        int result[2] = { 82, 0 };

        p_search_result res = setup_BLOSUM62_test( BIT_WIDTH_8, NEEDLEMAN_WUNSCH, 1 );

        test_result( res, result, 2 );
    }END_TEST

static void test_searcher_overflow_to_64bit( int search_type ) {
    init_constant_scores( 127, -1 );
    init_symbol_translation( AMINOACID, FORWARD_STRAND, 3, 3 );
    p_query query = query_read_from_file( "./tests/testdata/NP_009305.1.fas" );

    s_init( search_type, BIT_WIDTH_8, query );

    ssa_db_init( "./tests/testdata/NP_009305.1.fas" );

    gapO = -1;
    gapE = -1;

    size_t hit_count = 1;
    adp_init( hit_count );

    p_search_result res = s_search( &hit_count );

    minheap_sort( res->heap );

    query_free( query );

    ck_assert_int_eq( hit_count, res->heap->count );

    ck_assert_int_eq( 1, res->overflow_8_bit_count );
    ck_assert_int_eq( 1, res->overflow_16_bit_count );

    int result[2] = { 67818, 0 };

    test_result( res, result, 2 );
}

START_TEST (test_searcher_overflow_to_64bit_sw)
    {
        test_searcher_overflow_to_64bit( SMITH_WATERMAN );
    }END_TEST

START_TEST (test_searcher_overflow_to_64bit_nw)
    {
        test_searcher_overflow_to_64bit( NEEDLEMAN_WUNSCH );
    }END_TEST

START_TEST (test_init_search_data)
    {
        init_symbol_translation( NUCLEOTIDE, FORWARD_STRAND, 3, 3 );
        p_query query = query_read_from_string( "ATGCCCAAGCTGAATAGCGTAGAGGGGTTTTCATCATTTGAGGACGATGTATAA" );
        p_search_data sdp = s_create_searchdata( query );

        // query data
        ck_assert_int_eq( 1, sdp->q_count );

        seq_buffer_t buf = sdp->queries[0];

        ck_assert_str_eq( &query->nt[0].seq, &buf.seq.seq );
        ck_assert_int_eq( query->nt[0].len, buf.seq.len );
        ck_assert_int_eq( 0, buf.frame );
        ck_assert_int_eq( 0, buf.strand );

        ck_assert_int_eq( 0, sdp->queries[1].seq.len );
        ck_assert_int_eq( 0, sdp->queries[2].seq.len );
        ck_assert_int_eq( 0, sdp->queries[3].seq.len );
        ck_assert_int_eq( 0, sdp->queries[4].seq.len );
        ck_assert_int_eq( 0, sdp->queries[5].seq.len );

        ck_assert_int_eq( 54, sdp->maxqlen );

        query_free( query );
    }END_TEST

START_TEST (test_init_search_data2)
    {
        init_symbol_translation( NUCLEOTIDE, COMPLEMENTARY_STRAND, 3, 3 );
        p_query query = query_read_from_string( "ATGCCCAAGCTGAATAGCGTAGAGGGGTTTTCATCATTTGAGGACGATGTATAA" );
        p_search_data sdp = s_create_searchdata( query );

        // query data
        ck_assert_int_eq( 1, sdp->q_count );

        seq_buffer_t buf = sdp->queries[0];
        ck_assert_str_eq( &query->nt[1].seq, &buf.seq.seq );
        ck_assert_int_eq( query->nt[1].len, buf.seq.len );
        ck_assert_int_eq( 0, buf.frame );
        ck_assert_int_eq( 1, buf.strand );

        ck_assert_int_eq( 0, sdp->queries[1].seq.len );
        ck_assert_int_eq( 0, sdp->queries[2].seq.len );
        ck_assert_int_eq( 0, sdp->queries[3].seq.len );
        ck_assert_int_eq( 0, sdp->queries[4].seq.len );
        ck_assert_int_eq( 0, sdp->queries[5].seq.len );

        ck_assert_int_eq( 54, sdp->maxqlen );

        query_free( query );
    }END_TEST

START_TEST (test_init_search_data3)
    {
        init_symbol_translation( TRANS_QUERY, FORWARD_STRAND, 3, 3 );
        p_query query = query_read_from_string( "ATGCCCAAGCTGAATAGCGTAGAGGGGTTTTCATCATTTGAGGACGATGTATAA" );
        p_search_data sdp = s_create_searchdata( query );

        // query data
        ck_assert_int_eq( 3, sdp->q_count );

        seq_buffer_t buf = sdp->queries[0];
        ck_assert_str_eq( &query->aa[0].seq, &buf.seq.seq );
        ck_assert_int_eq( query->aa[0].len, buf.seq.len );
        ck_assert_int_eq( 0, buf.frame );
        ck_assert_int_eq( 0, buf.strand );

        buf = sdp->queries[1];
        ck_assert_str_eq( &query->aa[1].seq, &buf.seq.seq );
        ck_assert_int_eq( query->aa[1].len, buf.seq.len );
        ck_assert_int_eq( 1, buf.frame );
        ck_assert_int_eq( 0, buf.strand );

        buf = sdp->queries[2];
        ck_assert_str_eq( &query->aa[2].seq, &buf.seq.seq );
        ck_assert_int_eq( query->aa[2].len, buf.seq.len );
        ck_assert_int_eq( 2, buf.frame );
        ck_assert_int_eq( 0, buf.strand );

        ck_assert_int_eq( 0, sdp->queries[3].seq.len );
        ck_assert_int_eq( 0, sdp->queries[4].seq.len );
        ck_assert_int_eq( 0, sdp->queries[5].seq.len );

        ck_assert_int_eq( 18, sdp->maxqlen );

        query_free( query );
    }END_TEST

START_TEST (test_init_search_data4)
    {
        init_symbol_translation( TRANS_QUERY, BOTH_STRANDS, 3, 3 );
        p_query query = query_read_from_string( "ATGCCCAAGCTGAATAGCGTAGAGGGGTTTTCATCATTTGAGGACGATGTATAA" );
        p_search_data sdp = s_create_searchdata( query );

        // query data
        ck_assert_int_eq( 6, sdp->q_count );

        seq_buffer_t buf = sdp->queries[0];
        ck_assert_str_eq( &query->aa[0].seq, &buf.seq.seq );
        ck_assert_int_eq( query->aa[0].len, buf.seq.len );
        ck_assert_int_eq( 0, buf.frame );
        ck_assert_int_eq( 0, buf.strand );

        buf = sdp->queries[1];
        ck_assert_str_eq( &query->aa[1].seq, &buf.seq.seq );
        ck_assert_int_eq( query->aa[1].len, buf.seq.len );
        ck_assert_int_eq( 1, buf.frame );
        ck_assert_int_eq( 0, buf.strand );

        buf = sdp->queries[2];
        ck_assert_str_eq( &query->aa[2].seq, &buf.seq.seq );
        ck_assert_int_eq( query->aa[2].len, buf.seq.len );
        ck_assert_int_eq( 2, buf.frame );
        ck_assert_int_eq( 0, buf.strand );

        buf = sdp->queries[3];
        ck_assert_str_eq( &query->aa[3].seq, &buf.seq.seq );
        ck_assert_int_eq( query->aa[3].len, buf.seq.len );
        ck_assert_int_eq( 0, buf.frame );
        ck_assert_int_eq( 1, buf.strand );

        buf = sdp->queries[4];
        ck_assert_str_eq( &query->aa[4].seq, &buf.seq.seq );
        ck_assert_int_eq( query->aa[4].len, buf.seq.len );
        ck_assert_int_eq( 1, buf.frame );
        ck_assert_int_eq( 1, buf.strand );

        buf = sdp->queries[5];
        ck_assert_str_eq( &query->aa[5].seq, &buf.seq.seq );
        ck_assert_int_eq( query->aa[5].len, buf.seq.len );
        ck_assert_int_eq( 2, buf.frame );
        ck_assert_int_eq( 1, buf.strand );

        ck_assert_int_eq( 18, sdp->maxqlen );

        query_free( query );
    }END_TEST

START_TEST (test_init_search_data5)
    {
        init_symbol_translation( AMINOACID, BOTH_STRANDS, 3, 3 );
        p_query query = query_read_from_string( "ATGCCCAAGCTGAATAGCGTAGAGGGGTTTTCATCATTTGAGGACGATGTATAA" );
        p_search_data sdp = s_create_searchdata( query );

        // query data
        ck_assert_int_eq( 1, sdp->q_count );

        seq_buffer_t buf = sdp->queries[0];
        ck_assert_str_eq( &query->aa[0].seq, &buf.seq.seq );
        ck_assert_int_eq( query->aa[0].len, buf.seq.len );
        ck_assert_int_eq( 0, buf.frame );
        ck_assert_int_eq( 0, buf.strand );

        ck_assert_int_eq( 0, sdp->queries[1].seq.len );
        ck_assert_int_eq( 0, sdp->queries[2].seq.len );
        ck_assert_int_eq( 0, sdp->queries[3].seq.len );
        ck_assert_int_eq( 0, sdp->queries[4].seq.len );
        ck_assert_int_eq( 0, sdp->queries[5].seq.len );

        ck_assert_int_eq( 54, sdp->maxqlen );

        query_free( query );
    }END_TEST

void addSearcherTC( Suite *s ) {
    TCase *tc_core = tcase_create( "searcher" );
    tcase_add_test( tc_core, test_searcher_simple_sw_64 );
    tcase_add_test( tc_core, test_searcher_simple_nw_64 );
    tcase_add_test( tc_core, test_searcher_simple_sw_16 );
    tcase_add_test( tc_core, test_searcher_simple_nw_16 );
    tcase_add_test( tc_core, test_searcher_simple_sw_8 );
    tcase_add_test( tc_core, test_searcher_simple_nw_8 );
    tcase_add_test( tc_core, test_searcher_multiple_sequences_nw_64 );
    tcase_add_test( tc_core, test_searcher_multiple_sequences_sw_64 );
    tcase_add_test( tc_core, test_searcher_multiple_sequences_nw_16 );
    tcase_add_test( tc_core, test_searcher_multiple_sequences_sw_16 );
    tcase_add_test( tc_core, test_searcher_multiple_sequences_nw_8 );
    tcase_add_test( tc_core, test_searcher_multiple_sequences_sw_8 );
    tcase_add_test( tc_core, test_searcher_translate_nw_64 );
    tcase_add_test( tc_core, test_searcher_translate_sw_64 );
    tcase_add_test( tc_core, test_searcher_translate_nw_16 );
    tcase_add_test( tc_core, test_searcher_translate_sw_16 );
    tcase_add_test( tc_core, test_searcher_translate_nw_8 );
    tcase_add_test( tc_core, test_searcher_translate_sw_8 );
    tcase_add_test( tc_core, test_searcher_AA_BLOSUM_sw_64 );
    tcase_add_test( tc_core, test_searcher_AA_BLOSUM_nw_64 );
    tcase_add_test( tc_core, test_searcher_AA_BLOSUM_sw_16 );
    tcase_add_test( tc_core, test_searcher_AA_BLOSUM_nw_16 );
    tcase_add_test( tc_core, test_searcher_AA_BLOSUM_sw_8 );
    tcase_add_test( tc_core, test_searcher_AA_BLOSUM_sw_8_avx );
    tcase_add_test( tc_core, test_searcher_AA_BLOSUM_nw_8 );
    tcase_add_test( tc_core, test_searcher_overflow_to_64bit_sw );
    tcase_add_test( tc_core, test_searcher_overflow_to_64bit_nw );
    tcase_add_test( tc_core, test_searcher_AA_nw_64 );
    tcase_add_test( tc_core, test_searcher_AA_sw_64 );
    tcase_add_test( tc_core, test_searcher_AA_nw_16 );
    tcase_add_test( tc_core, test_searcher_AA_sw_16 );
    tcase_add_test( tc_core, test_searcher_AA_nw_8 );
    tcase_add_test( tc_core, test_searcher_AA_sw_8 );
    tcase_add_test( tc_core, test_init_search_data );
    tcase_add_test( tc_core, test_init_search_data2 );
    tcase_add_test( tc_core, test_init_search_data3 );
    tcase_add_test( tc_core, test_init_search_data4 );
    tcase_add_test( tc_core, test_init_search_data5 );

    suite_add_tcase( s, tc_core );
}
