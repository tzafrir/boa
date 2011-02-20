#include "Constraint.h"

namespace boa {


const Constraint::Expression Constraint::Expression::NegInfinity(std::numeric_limits<int>::min());
const Constraint::Expression Constraint::Expression::PosInfinity(std::numeric_limits<int>::max());

}
