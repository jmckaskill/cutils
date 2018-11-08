/*
 * The Mersenne Twister pseudo-random number generator (PRNG)
 *
 * This is an implementation of fast PRNG called MT19937,
 * meaning it has a period of 2^19937-1, which is a Mersenne
 * prime.
 *
 * This PRNG is fast and suitable for non-cryptographic code.
 * For instance, it would be perfect for Monte Carlo simulations,
 * etc.
 *
 * For all the details on this algorithm, see the original paper:
 * http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/ARTICLES/mt.pdf
 *
 * Written by Christian Stigen Larsen
 * http://csl.name
 *
 * Distributed under the modified BSD license.
 *
 * 2015-02-17
 */

#include "cutils/mersenne-twister.h"

/*
 * We have an array of 624 32-bit values, and there are
 * 31 unused bits, so we have a seed value of
 * 624*32-31 = 19937 bits.
 */
#define SIZE 624U
#define PERIOD 397U
#define DIFF (SIZE-PERIOD)

#define M32(x) (0x80000000 & x) // 32nd Most Significant Bit
#define L31(x) (0x7FFFFFFF & x) // 31 Least Significant Bits
#define ODD(x) (x & 1) // Check if number is odd

#define UNROLL(expr) \
  y = M32(m->MT[i]) | L31(m->MT[i+1]); \
  m->MT[i] = m->MT[expr] ^ (y>>1) ^ MATRIX[ODD(y)]; \
  ++i;

static void generate_numbers(struct mt_rand *m)
{
  /*
   * Originally, we had one loop with i going from [0, SIZE) and
   * two modulus operations:
   *
   * for ( register unsigned i=0; i<SIZE; ++i ) {
   *   register uint32_t y = M32(MT[i]) | L31(MT[(i+1) % SIZE]);
   *   MT[i] = MT[(i + PERIOD) % SIZE] ^ (y>>1);
   *   if ( ODD(y) ) MT[i] ^= 0x9908b0df;
   * }
   *
   * For performance reasons, we've unrolled the loop three times, thus
   * mitigating the need for any modulus operations.
   *
   * Anyway, it seems this trick is old hat:
   * http://www.quadibloc.com/crypto/co4814.htm
   *
   */

  static const uint32_t MATRIX[2] = {0, 0x9908b0df};
  register uint32_t y, i=0;

  // i = [0 ... 225]
  while ( i<(DIFF-1) ) {
    /*
     * We're doing 226 = 113*2, an even number of steps, so we can
     * safely unroll one more step here for speed:
     */
    UNROLL(i+PERIOD);
    UNROLL(i+PERIOD);
  }

  // i = 226
  UNROLL((i+PERIOD) % SIZE);

  // i = [227 ... 622]
  while ( i<(SIZE-1) ) {
    /*
     * 623-227 = 396 = 2*2*3*3*11, so we can unroll this loop in any number
     * that evenly divides 396 (2, 4, 6, etc). Here we'll unroll 11 times.
     */
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
    UNROLL(i-DIFF);
  }

  // i = 623
  y = M32(m->MT[SIZE-1]) | L31(m->MT[0]); \
  m->MT[SIZE-1] = m->MT[PERIOD-1] ^ (y>>1) ^ MATRIX[ODD(y)]; \
}

void mt_seed(struct mt_rand *m, uint32_t value)
{
  /*
   * The equation below is a linear congruential generator (LCG),
   * one of the oldest known pseudo-random number generator
   * algorithms, in the form X_(n+1) = = (a*X_n + c) (mod m).
   *
   * We've implicitly got m=32 (mask + word size of 32 bits), so
   * there is no need to explicitly use modulus.
   *
   * What is interesting is the multiplier a.  The one we have
   * below is 0x6c07865 --- 1812433253 in decimal, and is called
   * the Borosh-Niederreiter multiplier for modulus 2^32.
   *
   * It is mentioned in passing in Knuth's THE ART OF COMPUTER
   * PROGRAMMING, Volume 2, page 106, Table 1, line 13.  LCGs are
   * treated in the same book, pp. 10-26
   *
   * You can read the original paper by Borosh and Niederreiter
   * as well.  It's called OPTIMAL MULTIPLIERS FOR PSEUDO-RANDOM
   * NUMBER GENERATION BY THE LINEAR CONGRUENTIAL METHOD (1983) at
   * http://www.springerlink.com/content/n7765ku70w8857l7/
   *
   * You can read about LCGs at:
   * http://en.wikipedia.org/wiki/Linear_congruential_generator
   *
   * From that page, it says:
   * "A common Mersenne twister implementation, interestingly
   * enough, uses an LCG to generate seed data.",
   *
   * Since we're using 32-bits data types for our MT array, we can skip the
   * masking with 0xFFFFFFFF below.
   */

  m->MT[0] = value;
  m->index = 0;

  for ( register unsigned i=1; i<SIZE; ++i )
    m->MT[i] = 0x6c078965*(m->MT[i-1] ^ m->MT[i-1]>>30) + i;
}

uint32_t mt_rand_u32(struct mt_rand *m)
{
  if ( !m->index )
    generate_numbers(m);

  register uint32_t y = m->MT[m->index];

  // Tempering
  y ^= y>>11;
  y ^= y<< 7 & 0x9d2c5680;
  y ^= y<<15 & 0xefc60000;
  y ^= y>>18;

  if ( ++m->index == SIZE )
    m->index = 0;

  return y;
}
