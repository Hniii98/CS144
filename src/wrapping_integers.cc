#include "wrapping_integers.hh"
#include "debug.hh"
#include <vector>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + static_cast<uint32_t>(n);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  /*
   * raw_data_ =  zero_point + n mod 2^32 = zero_point + remainder
   * n = (zero_point - raw_data_) + k * 2^32 = offset + k * 2^32
   * Since we need to find the corresponding absolute sequnence number that is closest to checkponit,
   * so consider base of checkpoint as rough base. When the absolute sequence number is in the same base,
   * rough base is the real base. Otherwise check the nearby by shifting rough base.
   */
  uint64_t remainder = static_cast<uint64_t>(raw_value_ - zero_point.raw_value_);
  uint64_t rough_quotient = checkpoint & ~0xFFFFFFFFULL; // equal to checkpoint mod 2^32
  uint64_t candidate = rough_quotient + remainder;

  uint64_t candidate_diff = ( checkpoint > candidate ) ? checkpoint - candidate : candidate - checkpoint;
  uint64_t best = candidate;

  for ( int shift : { -1, 1 } ) {
    uint64_t alter = candidate + ( shift * ( 1ULL << 32 ) );
    uint64_t alter_diff = ( checkpoint > alter ) ? checkpoint - alter : alter - checkpoint;
    if ( alter_diff < candidate_diff ) {
      best = alter;
      candidate_diff = alter_diff;
    }
  }

  return best;
}
