#ifndef __BOA_LOG_H
#define __BOA_LOG_H /* */

#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <time.h>

using std::endl;
using std::ostream;
using std::map;
using std::setw;
using std::string;

namespace boa {

namespace log {
  extern void set(ostream &os);
  extern ostream& os();
  static map<string, long> profTime;
}

}  // namespace boa

#define LOG log::os() << std::setiosflags(std::ios::left) << setw(30) << __FILE__ << ":" << setw(4) << __LINE__ << std::resetiosflags(std::ios::left) << "  "

/**
 * Usage:
 *
 *   SET_PROF("any string");
 *   [...]
 *   PROF("any string") << "Optional comment" << endl;
 *
 * You can then run using `prof` instead of `boa` to see the results.
 */
#define SET_PROF(key) log::profTime[key] = clock()
#define PROF(key) log::os() << "PROF\t" << setw(30) << (1000 * (clock() - log::profTime[key])) / CLOCKS_PER_SEC << "\t" << __PRETTY_FUNCTION__ << "\t"

#endif /* __BOA_LOG_H */
