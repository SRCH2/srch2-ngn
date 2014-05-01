#ifndef __TRANSPORT_CONNECTIONS_INLINE_H__
#define __TRANSPORT_CONNECTIONS_INLINE_H__

#include "RouteMap.h"

//FROM HEADER-- typedef std::vector<std::pair<ConnectionId, bool> > Pool

using namespace srch2::httpwrapper;

inline
Connections::Connections(Pool& pool) : pool(pool), place(pool.begin()) {}

inline
Connections::Connections(const Connections& toCpy) : 
  pool(toCpy.pool), place(toCpy.place) {}

inline
Connections& Connections::operator++() {
  for(int i=0; i < 2; ++i) {
    while(++place != pool.end() && place->second);
    if(place != pool.end()) return *this;
    place = pool.begin();
  }
  return *this;
}

inline
Connections Connections::operator++(int) {
  Connections rtn(*this);
  ++*this;
  return rtn;
}

inline
bool Connections::operator==(const Connections& rhs) {
  return &pool == &rhs.pool && place == rhs.place;
}

inline
bool Connections::operator!=(const Connections& rhs) {
  return &pool == &rhs.pool && place == rhs.place;
}

inline
std::pair<ConnectionId, bool>& Connections::operator*() {
  return *place;
}

inline
std::pair<ConnectionId, bool>* Connections::operator->() {
  return place.operator->();
}
#endif /*__TRANSPORT_CONNECTIONS_INLINE_H__ */
