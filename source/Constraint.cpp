#include "Constraint.h"

namespace boa {


Constraint::Expression Constraint::Expression::NegInfinity(std::numeric_limits<int>::min());
Constraint::Expression Constraint::Expression::PosInfinity(std::numeric_limits<int>::max());

}
