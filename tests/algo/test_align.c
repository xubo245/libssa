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

#include <string.h>

#include "../../src/algo/align.h"
#include "../../src/algo/searcher.h"
#include "../../src/matrices.h"
#include "../../src/util/util_sequence.h"
#include "../../src/util/util.h"
#include "../../src/algo/gap_costs.h"

static sequence_t a_seq;
static sequence_t b_seq;

static void setup_align( char * seq_a, char * seq_b, size_t len_a, size_t len_b ) {
    gapO = -1;
    gapE = -1;
    mat_init_constant_scoring( 1, -1 );

    us_init_translation( 3, 3 );

    fill_translated_sequence( &a_seq, seq_a, len_a );
    fill_translated_sequence( &b_seq, seq_b, len_b );
}

static void teardown_align() {
    mat_free();
}

START_TEST (test_find_region_for_local)
    {
        setup_align( "AATC", "AT", 4, 2 );

        region_t region = find_region_for_local( a_seq, b_seq );

        ck_assert_int_eq( 1, region.a_begin );
        ck_assert_int_eq( 2, region.a_end );
        ck_assert_int_eq( 0, region.b_begin );
        ck_assert_int_eq( 1, region.b_end );

        teardown_align();
    }END_TEST

START_TEST (test_find_region_for_global)
    {
        setup_align( "AATC", "AT", 4, 2 );

        region_t region = init_region_for_global( a_seq, b_seq );

        ck_assert_int_eq( 0, region.a_begin );
        ck_assert_int_eq( 3, region.a_end );
        ck_assert_int_eq( 0, region.b_begin );
        ck_assert_int_eq( 1, region.b_end );

        teardown_align();
    }END_TEST

void addAlignTC( Suite *s ) {
    TCase *tc_core = tcase_create( "align" );
    tcase_add_test( tc_core, test_find_region_for_local );
    tcase_add_test( tc_core, test_find_region_for_global );

    suite_add_tcase( s, tc_core );
}
