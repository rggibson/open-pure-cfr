/* entries.cpp
 * Richard Gibson, Jul 1, 2013
 * email: richard.g.gibson@gmail.com
 *
 * Contains the remaining Entries procedures that are not implemented in
 * entries.hpp
 *
 * Copyright (C) 2013 by Richard Gibson
 */

/* C / C++ / STL includes */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>

/* C project_acpc_poker includes */
extern "C" {
}

/* Pure CFR includes */
#include "entries.hpp"
#include "constants.hpp"

Entries::Entries( size_t new_num_entries_per_bucket,
		  size_t new_total_num_entries )
  : num_entries_per_bucket( new_num_entries_per_bucket ),
    total_num_entries( new_total_num_entries )
{
}

Entries::~Entries( )
{
}

size_t Entries::get_entry_index( const int bucket, const int64_t soln_idx ) const
{
  return ( num_entries_per_bucket * bucket ) + soln_idx;
}

Entries *new_loaded_entries( const size_t num_entries_per_bucket,
			     const size_t total_num_entries,
			     void **data )
{
  /* First, read the entry type */
  pure_cfr_entry_type_t *type_ptr = ( pure_cfr_entry_type_t * ) ( *data );
  pure_cfr_entry_type_t type = *type_ptr;

  /* Advance the data pointer past the entry type */
  type_ptr += 1;
  ( *data ) = ( void * ) type_ptr;

  /* Load the appropriate type of entries and advance data past the entries */
  Entries *entries = NULL;
  switch( type ) {
  case TYPE_UINT8_T: {
    uint8_t *uint8_t_data = ( uint8_t * ) ( *data );
    entries = new Entries_der<uint8_t>( num_entries_per_bucket, total_num_entries,
					uint8_t_data );
    uint8_t_data += total_num_entries;
    ( *data ) = ( void * ) uint8_t_data;
    break;
  }
    
  case TYPE_INT: {
    int *int_data = ( int * ) ( *data );
    entries = new Entries_der<int>( num_entries_per_bucket, total_num_entries,
				    int_data );
    int_data += total_num_entries;
    ( *data ) = ( void * ) int_data;
    break;
  }

  case TYPE_UINT32_T: {
    uint32_t *uint32_t_data = ( uint32_t * ) ( *data );
    entries = new Entries_der<uint32_t>( num_entries_per_bucket,
					 total_num_entries, uint32_t_data );
    uint32_t_data += total_num_entries;
    ( *data ) = ( void * ) uint32_t_data;
    break;
  }

  case TYPE_UINT64_T: {
    uint64_t *uint64_t_data = ( uint64_t * ) ( *data );
    entries = new Entries_der<uint64_t>( num_entries_per_bucket,
					 total_num_entries, uint64_t_data );
    uint64_t_data += total_num_entries;
    ( *data ) = ( void * ) uint64_t_data;
    break;
  }

  default: {
    fprintf( stderr, "unrecognized entry type [%d]\n", type );
    break;
  }
  }

  return entries;
}
