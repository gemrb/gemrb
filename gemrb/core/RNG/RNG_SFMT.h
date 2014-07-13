#ifndef RNG_SFMT_H
#define RNG_SFMT_H

#include <climits>
#include "sfmt/SFMT.h"

#define RAND(min, max) RNG_SFMT::getInstance()->rand(min, max)

/**
 * This class encapsulates a state of the art PRNG in a singleton class and can be used
 * to return uniformly distributed integer random numbers from a range [min, max].
 * Though technically possible, min must be >= 0 and max should always be > 0.
 * If max < 0 and min == 0 it is assumed that rand() % -max is wanted and the result will
 * be -rand(0, -max).
 * This is the only exception to the rule that !(min > max) and is used as a workaround
 * for some parts of the code where negative modulo could occur.
 *
 * As the class is a singleton, only one instance exists. Access it by using the
 * getInstance() method, e.g. by writing
 * RNG_SFMT::getInstance()->rand(1,6);
 * which may be abbreviated by
 * RAND(1,6);
 *
 * Technical details:
 * The RNG uses the SIMD-oriented Fast Mersenne Twister code v1.4.1 from
 * http://www.math.sci.hiroshima-u.ac.jp/~%20m-mat/MT/SFMT/index.html
 * The SFMT RNG creates unsigned int 64bit pseudo random numbers.
 *
 * These are mapped to values from the interval [min, max] without bias by using Knuth's
 * "Algorithm S (Selection sampling technique)" from "The Art of Computer Programming 3rd
 * Edition Volume 2 / Seminumerical Algorithms".
 *
 * In a threaded environment you probably must use mutexes to make this class thread-safe.
 * There are comments in the cdf-method's code about that.
 */

class RNG_SFMT {
private:
  // only one instance will be allowed, use getInstance
  RNG_SFMT();

  // The discrete cumulative distribution function for the RNG
  unsigned int cdf(unsigned int min, unsigned int max);

  // SFMT's internal state
  sfmt_t sfmt;

public:
  /* The RNG function to use via
   * RNG_SFMT::getInstance()->rand(min, max);
   * or
   * RAND(min, max);
   */
  unsigned int rand(int min = 0, int max = INT_MAX-1);
  static RNG_SFMT* getInstance();
};

#endif
