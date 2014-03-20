#ifndef RNG_SFMT_H
#define RNG_SFMT_H

#include <climits>
#include "sfmt/SFMT.h"

#define RAND(min, max) RNG_SFMT::getInstance()->rand(min, max)

/**
 * This class encapsulates a state of the art PRNG in a singleton class and can be used
 * to return uniformly distributed integer random numbers from a range [min, max]
 *
 * As the class is a singleton, only one instance exists. Access it by using the
 * getInstance() method, e.g. by writing
 * RNG_SFMT::getInstance()->rand(1,6);
 * which may be abbreviated by
 * RAND(1,6);
 *
 * You should never call this function with min > max, this throws a
 * std::invalid_argument exception. It is best practice to use rand() in a try block and
 * catch the exception if min, max are entered by the user or computed somehow.
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
 * There are more comments in the code about that.
 */

class RNG_SFMT {
private:
  // only one instance will be allowed, use getInstance
  RNG_SFMT();

  // SFMT's internal state
  sfmt_t sfmt;

public:
  unsigned int rand(unsigned int min = 0, unsigned int max = UINT_MAX-1);
  static RNG_SFMT* getInstance();
};

#endif
