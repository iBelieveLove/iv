#ifndef _IV_LV5_NAME_ITERATOR_H_
#define _IV_LV5_NAME_ITERATOR_H_
#include <gc/gc_cpp.h>
#include "lv5/jsobject.h"
namespace iv {
namespace lv5 {

class NameIterator : public gc_cleanup {
 public:
  NameIterator(Context* ctx, JSObject* obj)
    : keys_(),
      iter_() {
    obj->GetPropertyNames(ctx, &keys_, JSObject::kExcludeNotEnumerable);
    iter_ = keys_.begin();
  }

  Symbol Get() const {
    return *iter_;
  }

  bool Has() const {
    return iter_ != keys_.end();
  }

  void Next() {
    ++iter_;
  }

  static NameIterator* New(Context* ctx, JSObject* obj) {
    return new NameIterator(ctx, obj);
  }

 private:
  std::vector<Symbol> keys_;
  std::vector<Symbol>::const_iterator iter_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_NAME_ITERATOR_H_