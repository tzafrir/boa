#ifndef __BOA_LOG_H
#define __BOA_LOG_H /* */

#include <string>
#include <iostream>
#include <iomanip>

using std::string;
using std::ostream;
using std::endl;
using std::setw;

namespace boa {

namespace log {
  extern void set(ostream &os);
  extern ostream& os();
}

}  // namespace boa

#define LOG log::os() << std::setiosflags(std::ios::left) << setw(30) << __FILE__ << ":" << setw(4) << __LINE__ << std::resetiosflags(std::ios::left) << "  "

#endif /* __BOA_LOG_H */
