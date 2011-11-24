#ifndef IV_USTRINGPIECE_H_
#define IV_USTRINGPIECE_H_
#include <iv/detail/cstdint.h>
#include <iv/stringpiece.h>
namespace iv {
namespace core {

typedef BasicStringPiece<uint16_t, std::char_traits<uint16_t> > UStringPiece;

inline UStringPiece AdoptPiece(const UStringPiece& piece) {
  return piece;
}

} }  // namespace iv::core
#endif  // IV_USTRINGPIECE_H_
