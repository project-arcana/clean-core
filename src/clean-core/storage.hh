#pragma once

#include <clean-core/typedefs.hh>

namespace cc
{
/**
 * Usage:
 *
 *  template <class T>
 *  struct my_optional
 *  {
 *      void set_value(T t) {
 *          if (_has_value)
 *              _storage.value.~T();
 *          new (&_storage.value) T(t);
 *          _has_value = true;
 *      }
 *
 *  private:
 *      storage_for<T> _storage;
 *      bool _has_value = false;
 *  };
 */
template <class T>
union storage_for {
    // empty ctor/dtor in order to not initialize value
    storage_for() {}
    ~storage_for() {}
    T value;
};
}
