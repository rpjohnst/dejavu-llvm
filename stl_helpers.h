#ifndef STL_HELPERS_H
#define STL_HELPERS_H

#include <functional>

template <typename T>
static inline void deleter(T *p) { delete p; }

#endif
