
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

  float GetTotalCount() const
  { return m_totalCount; }

  float GetCount(size_t index, const Factor *factor) const;
  float GetSize(size_t index) const
  { return m_vec[index].size(); }

protected:
  typedef std::map<const Factor*, float> Map; // map word -> prob
  typedef std::vector<Map> Vec; // left outside, left inside, right inside, right outside
  Vec m_vec;

  float m_totalCount;

  void AddToMap(size_t index, const Factor *factor, float count);

};

} // namespace Moses

