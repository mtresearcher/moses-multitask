
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
//////////////////////////////////////
  class ProbStore {
	  typedef std::map<const Factor*, float> Map; // map word -> prob
	  typedef std::vector<Map> Vec;
	  Vec m_vecInd; // left outside, left inside, right inside, right outside

	  typedef std::map< std::pair<const Factor*, const Factor*>, float> MapJoint; // map word -> prob
	  typedef std::vector<MapJoint> VecJoint;
	  VecJoint m_vecJoint; // inside, outside
	  float m_totalCount;

	  float GetCount(size_t contextInd,
			  const Factor *factor,
			  float smoothConstant) const;
	  float GetTotalCount(size_t contextInd, float smoothConstant) const;

  public:

	  ProbStore()
	  :m_vecInd(4)
	  ,m_vecJoint(2)
	  ,m_totalCount(0)
	  {}

	  float GetProb(size_t contextInd, // left outside, left inside, right inside, right outside
			  const Factor *factor,
			  float smoothConstant) const;

	  float GetSize(size_t index) const
	  { return m_vecInd[index].size(); }

	  void AddToMap(size_t index, const Factor *factor, float count);
	  void AddToMap(size_t index, const Factor *left, const Factor *right, float count);

  };
/////////////////////////////////////////////
// class NonTermContextProperty

public:

  NonTermContextProperty();
  ~NonTermContextProperty();

  virtual void ProcessValue(const std::string &value);

  virtual const std::string *GetValueString() const {
    UTIL_THROW2("NonTermContextProperty: value string not available in this phrase property");
    return NULL;
  };

  float GetProb(size_t ntInd,
		  size_t contextInd,
		  const Factor *factor,
		  float smoothConstant) const;

  // 0 = inside. 1 = outside
  float GetProb(size_t ntInd,
		  size_t contextInd,
		  const Factor *factor1,
		  const Factor *factor2,
		  float smoothConstant) const;

protected:
  // by nt index
  std::vector<ProbStore> m_probStores;

  void AddToMap(size_t ntIndex, size_t contextIndex, const Factor *factor, float count);
  void AddToMap(size_t ntIndex,
				size_t contextIndex,
				const Factor *left,
				const Factor *right,
				float count);

};

} // namespace Moses

