#ifndef __BOA_LOG_H
#define __BOA_LOG_H /* */

#include <iostream>
#include <iomanip>
#include <string>
#include <time.h>

using std::endl;
using std::ostream;
using std::setw;
using std::string;

namespace boa {

namespace log {
  extern void set(ostream &os);
  extern ostream& os();
}

}  // namespace boa

#define LOG log::os() << std::setiosflags(std::ios::left) << setw(30) << __FILE__ << ":" << setw(4) << __LINE__ << std::resetiosflags(std::ios::left) << "  "
#define PROF log::os() << "PROF\t" << setw(30) << 1000 * clock() / CLOCKS_PER_SEC << "\t" << __PRETTY_FUNCTION__ << "\t"

#endif /* __BOA_LOG_H */
