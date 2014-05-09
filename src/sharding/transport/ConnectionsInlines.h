#ifndef __TRANSPORT_CONNECTIONS_INLINE_H__
#define __TRANSPORT_CONNECTIONS_INLINE_H__

#include "RouteMap.h"

//FROM HEADER-- typedef std::vector<std::pair<ConnectionId, bool> > Pool

using namespace srch2::httpwrapper;

inline
Connections::Connections(RoutePool& pool) : routePool(pool), place(pool.begin()) {}

inline
Connections::Connections(const Connections& toCpy) : 
  routePool(toCpy.routePool), place(toCpy.place) {}

inline
Connections& Connections::operator++() {
  for(int i=0; i < 2; ++i) {
    while(++place != routePool.end() && place->second);
    if(place != routePool.end()) return *this;
    place = routePool.begin();
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
  return &routePool == &rhs.routePool && place == rhs.place;
}

inline
bool Connections::operator!=(const Connections& rhs) {
  return &routePool == &rhs.routePool && place == rhs.place;
}

inline
std::pair<ConnectionInfo, bool>& Connections::operator*() {
  return *place;
}

inline
std::pair<ConnectionInfo, bool>* Connections::operator->() {
  return place.operator->();
}
#endif /*__TRANSPORT_CONNECTIONS_INLINE_H__ */
