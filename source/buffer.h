#ifndef __BOA_BUFFER_H
#define __BOA_BUFFER_H /* */

#include <string>

using std::string;

namespace boa {
	class Buffer {
	 public:
	  virtual string getUniqueName();
	};
}

#endif // __BOA_BUFFER_H

