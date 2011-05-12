#ifndef _IV_LV5_RUNTIME_MATH_H_
#define _IV_LV5_RUNTIME_MATH_H_
#include <cmath>
#include "round.h"
#include "platform_math.h"
#include "lv5/error_check.h"
#include "lv5/constructor_check.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/context.h"
#include "lv5/error.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

static const double kMathInfinity = std::numeric_limits<double>::infinity();

}  // namespace detail

inline JSVal MathAbs(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.abs", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return std::abs(x);
  }
  return JSNaN;
}

inline JSVal MathAcos(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.acos", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return std::acos(x);
  }
  return JSNaN;
}

inline JSVal MathAsin(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.asin", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return std::asin(x);
  }
  return JSNaN;
}

inline JSVal MathAtan(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.atan", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return std::atan(x);
  }
  return JSNaN;
}

inline JSVal MathAtan2(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.atan2", args, error);
  if (args.size() > 1) {
    const double y = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    const double x = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return std::atan2(y, x);
  }
  return JSNaN;
}

inline JSVal MathCeil(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.ceil", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return std::ceil(x);
  }
  return JSNaN;
}

inline JSVal MathCos(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.cos", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return std::cos(x);
  }
  return JSNaN;
}

inline JSVal MathExp(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.exp", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return std::exp(x);
  }
  return JSNaN;
}

inline JSVal MathFloor(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.floor", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return std::floor(x);
  }
  return JSNaN;
}

inline JSVal MathLog(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.log", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return std::log(x);
  }
  return JSNaN;
}

inline JSVal MathMax(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.max", args, error);
  double max = -detail::kMathInfinity;
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    const double x = it->ToNumber(args.ctx(), IV_LV5_ERROR(error));
    if (core::IsNaN(x)) {
      return x;
    } else if (x > max || (x == 0.0 && max == 0.0 && !core::Signbit(x))) {
      max = x;
    }
  }
  return max;
}

inline JSVal MathMin(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.min", args, error);
  double min = detail::kMathInfinity;
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    const double x = it->ToNumber(args.ctx(), IV_LV5_ERROR(error));
    if (core::IsNaN(x)) {
      return x;
    } else if (x < min || (x == 0.0 && min == 0.0 && core::Signbit(x))) {
      min = x;
    }
  }
  return min;
}

inline JSVal MathPow(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.pow", args, error);
  if (args.size() > 1) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    const double y = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    if (y == 0) {
      return 1.0;
    } else if (core::IsNaN(y) ||
               ((x == 1 || x == -1) && std::isinf(y))) {
      return JSNaN;
    } else {
      return std::pow(x, y);
    }
  }
  return JSNaN;
}

inline JSVal MathRandom(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.random", args, error);
  return args.ctx()->Random();
}

inline JSVal MathRound(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.round", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return core::Round(x);
  }
  return JSNaN;
}

inline JSVal MathSin(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.sin", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return std::sin(x);
  }
  return JSNaN;
}

inline JSVal MathSqrt(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.sqrt", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return std::sqrt(x);
  }
  return JSNaN;
}

inline JSVal MathTan(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("Math.tan", args, error);
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return std::tan(x);
  }
  return JSNaN;
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_MATH_H_
