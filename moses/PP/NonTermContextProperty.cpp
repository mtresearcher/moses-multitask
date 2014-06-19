#include "moses/PP/NonTermContextProperty.h"
#include <string>
#include <assert.h>
#include "moses/Util.h"
#include "moses/FactorCollection.h"

using namespace std;

namespace Moses
{
NonTermContextProperty::NonTermContextProperty()
{
}

NonTermContextProperty::~NonTermContextProperty()
{
	//RemoveAllInColl(m_probStores);
}

void NonTermContextProperty::ProcessValue(const std::string &value)
{
  vector<string> toks;
  Tokenize(toks, value);

  FactorCollection &fc = FactorCollection::Instance();

  for (size_t i = 0; i < toks.size(); i += 6) {
	  size_t ntInd = Scan<size_t>(toks[i]);
	  float count = Scan<float>(toks[i + 5]);

	  for (size_t contextInd = 0; contextInd < 4; ++contextInd) {
		  const Factor *factor = fc.AddFactor(toks[i + contextInd + 1], false);
		  AddToMap(ntInd, contextInd, factor, count);
	  }
  }
}

void NonTermContextProperty::AddToMap(size_t ntIndex, size_t index, const Factor *factor, float count)
{
  if (ntIndex <= m_probStores.size()) {
	  m_probStores.resize(ntIndex + 1);
  }

  ProbStore &probStore = m_probStores[ntIndex];
  probStore.AddToMap(index, factor, count);
}

float NonTermContextProperty::GetProb(size_t ntInd,
			size_t contextInd,
			const Factor *factor,
			float smoothConstant)
{
	UTIL_THROW_IF2(ntInd >= m_probStores.size(), "Invalid nt index=" << ntInd);
	ProbStore &probStore = m_probStores[ntInd];
	float ret = probStore.GetProb(contextInd, factor, smoothConstant);
	return ret;
}

//////////////////////////////////////////

void NonTermContextProperty::ProbStore::AddToMap(size_t index, const Factor *factor, float count)
{
	Map &map = m_vec[index];

	Map::iterator iter = map.find(factor);
	if (iter == map.end()) {
		map[factor] = count;
	}
	else {
		float &currCount = iter->second;
		currCount += count;
	}

	m_totalCount += count;
}


float NonTermContextProperty::ProbStore::GetProb(size_t contextInd,
			const Factor *factor,
			float smoothConstant) const
{
  float count = GetCount(contextInd, factor, smoothConstant);
  float total = GetTotalCount(contextInd, smoothConstant);
  float ret = count / total;
  return ret;
}

float NonTermContextProperty::ProbStore::GetCount(size_t contextInd,
			const Factor *factor,
			float smoothConstant) const
{
	const Map &map = m_vec[contextInd];

	float count = smoothConstant;
	Map::const_iterator iter = map.find(factor);
	if (iter == map.end()) {
		// nothing
	}
	else {
		count += iter->second;
	}

	return count;
}

float NonTermContextProperty::ProbStore::GetTotalCount(size_t contextInd, float smoothConstant) const
{
	const Map &map = m_vec[contextInd];
	return m_totalCount + smoothConstant * map.size();
}


} // namespace Moses

