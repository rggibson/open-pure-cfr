#ifndef __PURE_CFR_ENTRIES_HPP__
#define __PURE_CFR_ENTRIES_HPP__

/* entries.hpp
 * Richard Gibson, Jul 1, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * A class for storing regret and avg strategies of variable type.
 */

/* C / C++ / STL includes */
#include <assert.h>
#include <typeinfo>

/* C project-acpc-poker includes */
extern "C" {
}

/* Pure CFR includes */
#include "constants.hpp"

class Entries {
public:

  Entries( size_t new_num_entries_per_bucket, size_t total_num_entries );
  virtual ~Entries( );

  /* Returns the sum of all pos_values in the returned pos_values array */
  virtual uint64_t get_pos_values( const int bucket,
				   const int64_t soln_idx,
				   const int num_choices,
				   uint64_t *pos_values ) const = 0;
  virtual void update_regret( const int bucket,
			      const int64_t soln_idx,
			      const int num_choices,
			      const int *values,
			      const int retval ) = 0;
  /* Return 0 on success, 1 on overflow */
  virtual int increment_entry( const int bucket, const int64_t soln_idx, const int choice ) = 0;

  /* Return 0 on success, 1 on failure */
  virtual int write( FILE *file ) const = 0;
  virtual int load( FILE *file ) = 0;

  virtual pure_cfr_entry_type_t get_entry_type( ) const = 0;

protected:
  size_t get_entry_index( const int bucket, const int64_t soln_idx ) const;

  const size_t num_entries_per_bucket;
  const size_t total_num_entries;
};

template <typename T>
class Entries_der : public Entries {
public:
  
  Entries_der( size_t new_num_entries_per_bucket,
	       size_t new_total_num_entries,
	       T *loaded_data = NULL );
  virtual ~Entries_der( );

  virtual uint64_t get_pos_values( const int bucket,
				   const int64_t soln_idx,
				   const int num_choices,
				   uint64_t *pos_values ) const;
  virtual void update_regret( const int bucket,
			      const int64_t soln_idx,
			      const int num_choices,
			      const int *values,
			      const int retval );
  virtual int increment_entry( const int bucket,
			       const int64_t soln_idx,
			       const int choice );

  virtual int write( FILE *file ) const;
  virtual int load( FILE *file );

  virtual pure_cfr_entry_type_t get_entry_type( ) const;

  virtual void get_values( const int bucket,
			   const int64_t soln_idx,
			   const int num_choices,
			   T *values ) const;

  virtual void set_values( const int bucket,
			   const int64_t soln_idx,
			   const int num_choices,
			   const T *values );

protected:
  T *entries;
  const int data_was_loaded;
};

Entries *new_loaded_entries( size_t num_entries_per_bucket,
			     size_t total_num_entries,
			     void **data );

/* Unfortunately, templates require definitions in the same file
 * as their declarations
 */
template <typename T>
Entries_der<T>::Entries_der( size_t new_num_entries_per_bucket,
			     size_t new_total_num_entries,
			     T *loaded_data )
  : Entries( new_num_entries_per_bucket, new_total_num_entries ),
    data_was_loaded( loaded_data != NULL ? 1 : 0 )
{
  if( loaded_data != NULL ) {
    entries = loaded_data;
  } else {
    entries = ( T * ) calloc( total_num_entries, sizeof( T ) );
    /* If you hit this assert, you have run out of RAM!
     * Use a smaller game or coarser abstractions.
     */
    assert( entries != NULL );
  }
}

template <typename T>
Entries_der<T>::~Entries_der( )
{
  if( !data_was_loaded ) {
    free( entries );
  }
  entries = NULL;
}

template <typename T>
uint64_t Entries_der<T>::get_pos_values( const int bucket,
					 const int64_t soln_idx,
					 const int num_choices,
					 uint64_t *values ) const
{
  /* Get the local entries at this index */
  size_t base_index = get_entry_index( bucket, soln_idx );
  T local_entries[ num_choices ];
  memcpy( local_entries, &entries[ base_index ], num_choices * sizeof( T ) );

  /* Zero out negative values and store in the returned array */
  uint64_t sum_values = 0;
  for( int c = 0; c < num_choices; ++c ) {
    local_entries[ c ] *= ( local_entries[ c ] > 0 );
    values[ c ] = local_entries[ c ];
    sum_values += local_entries[ c ];
  }

  return sum_values;
}

template <typename T>
void Entries_der<T>::update_regret( const int bucket,
				    const int64_t soln_idx,
				    const int num_choices,
				    const int *values,
				    const int retval )
{
  /* Get a pointer to the local entries at this index */
  size_t base_index = get_entry_index( bucket, soln_idx );
  T *local_entries = &entries[ base_index ];

  for( int c = 0; c < num_choices; ++c ) {
    int diff = values[ c ] - retval;
    T new_regret = local_entries[ c ] + diff;
    /* Only update regret if no overflow occurs */
    if( ( ( diff < 0 ) && ( new_regret < local_entries[ c ] ) )
	|| ( ( diff > 0 ) && ( new_regret > local_entries[ c ] ) ) ) {
      local_entries[ c ] = new_regret;
    }
  }
}

template <typename T>
int Entries_der<T>::increment_entry( const int bucket, const int64_t soln_idx, const int choice )
{
  /* Get a pointer to the local entries at this index */
  size_t base_index = get_entry_index( bucket, soln_idx );
  T *local_entries = &entries[ base_index ];

  local_entries[ choice ] += 1;

  if( local_entries[ choice ] <= 0 ) {
    /* Overflow! */
    return 1;
  }

  return 0;
}

template <typename T>
int Entries_der<T>::write( FILE *file ) const
{
  if( data_was_loaded ) {
    fprintf( stderr, "tried to write data that was loaded at instantiation, "
	     "which is not allowed\n" );
    return 1;
  }
  
  /* First, write the type to file */
  pure_cfr_entry_type_t type = get_entry_type( );
  size_t num_written = fwrite( &type, sizeof( pure_cfr_entry_type_t ), 1, file );
  if( num_written != 1 ) {
    fprintf( stderr, "error while writing dump type [%d]\n", type );
    return 1;
  }

  /* Dump entries */
  num_written = fwrite( entries, sizeof( T ), total_num_entries, file );
  if( num_written != total_num_entries ) {
    fprintf( stderr, "error while writing; only wrote %jd of %jd entries\n",
	     ( intmax_t ) num_written, ( intmax_t ) total_num_entries );
    return 1;
  }  
  
  return 0;
}

template <typename T>
int Entries_der<T>::load( FILE *file )
{
  if( data_was_loaded ) {
    fprintf( stderr, "tried to load from file on top of loaded data at "
	     "instantiation, which is not allowed\n" );
    return 1;
  }
  
  /* First, load the type and double-check that it matches */
  pure_cfr_entry_type_t type;
  size_t num_read = fread( &type,
			   sizeof( pure_cfr_entry_type_t ),
			   1,
			   file );
  if( num_read != 1 ) {
    fprintf( stderr, "failed to read entry type\n" );
    return 1;
  }
  pure_cfr_entry_type_t this_type = get_entry_type( );
  if( type != this_type ) {
    fprintf( stderr, "type [%d] found, but expected type [%d]\n",
	     type, this_type );
    return 1;
  }

  /* Now load the entries */
  num_read = fread( entries, sizeof( T ), total_num_entries, file );
  if( num_read != total_num_entries ) {
    fprintf( stderr, "error while loading; only read %jd of %jd entries\n",
	     ( intmax_t ) num_read, ( intmax_t ) total_num_entries );
    return 1;
  }  
  
  return 0;
}

template <typename T>
pure_cfr_entry_type_t Entries_der<T>::get_entry_type( ) const
{
  if( typeid( T ) == typeid( uint8_t ) ) {
    return TYPE_UINT8_T;
  } else if( typeid( T ) == typeid( int ) ) {
    return TYPE_INT;
  } else if( typeid( T ) == typeid( uint32_t ) ) {
    return TYPE_UINT32_T;
  } else if( typeid( T ) == typeid( uint64_t ) ) {
    return TYPE_UINT64_T;
  } else {
    fprintf( stderr, "called get_entry_type for unrecognized template type!\n" );
    assert( 0 );
    return TYPE_NUM_TYPES;
  }
}

template <typename T>
void Entries_der<T>::get_values( const int bucket,
				 const int64_t soln_idx,
				 const int num_choices,
				 T *values ) const
{
  size_t base_index = get_entry_index( bucket, soln_idx );

  /* Copy the values over */
   memcpy( values, &entries[ base_index ], num_choices * sizeof( T ) );
}

template <typename T>
void Entries_der<T>::set_values( const int bucket,
				 const int64_t soln_idx,
				 const int num_choices,
				 const T *values )
{
  size_t base_index = get_entry_index( bucket, soln_idx );

  /* Copy the values over */
  memcpy( &entries[ base_index ], values, num_choices * sizeof( T ) );
}

#endif
