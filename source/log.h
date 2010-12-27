#ifndef __BOA_LOG_H
#define __BOA_LOG_H /* */

#include <string>
#include <ostream>

using std::string;
using std::ostream;
using std::endl;

namespace boa {
  namespace log {
    extern void set(ostream &os);
    extern ostream& os();
  }
}
#endif /* __BOA_LOG_H */
