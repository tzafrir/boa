#include "log.h"

using std::ostream;

namespace {
}

namespace boa {
  namespace log {
    class nullstream : ostream {
      template <class T> const nullstream& operator<<(const T& m) const {return *this;}
    } devNull;

    ostream *os_ = &devNull;
    
    void set(ostream &os) {
      os_=&os;
    }
    ostream &os() {
      return *os_;
    }
  }
}
