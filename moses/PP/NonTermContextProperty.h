
#pragma once

#include "moses/PP/PhraseProperty.h"
#include "util/exception.hh"
#include <string>
#include <list>
#include <map>
#include <vector>

namespace Moses
{
class Factor;

class NonTermContextProperty : public PhraseProperty
{
public:

  NonTermContextProperty();

  virtual void ProcessValue(const std::string &value);


  virtual const std::string *GetValueString() const {
    UTIL_THROW2("NonTermContextProperty: value string not available in this phrase property");
    return NULL;
  };

protected:
  typedef std::map<const Factor*, float> Map; // map word -> prob
  typedef std::vector<Map> Vec; // left outside, left inside, right inside, right outside
  Vec m_vec;

  void AddToMap(size_t j, const Factor *factor, float count);

};

} // namespace Moses

