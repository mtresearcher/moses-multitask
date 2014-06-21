#include "moses/PP/NonTermContextTargetProperty.h"
#include <string>
#include <assert.h>
#include "moses/Util.h"
#include "moses/FactorCollection.h"

using namespace std;

namespace Moses
{
NonTermContextTargetProperty::NonTermContextTargetProperty()
{
}

NonTermContextTargetProperty::~NonTermContextTargetProperty()
{
	//RemoveAllInColl(m_probStores);
}

void NonTermContextTargetProperty::ProcessValue(const std::string &value)
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

void NonTermContextTargetProperty::AddToMap(size_t ntIndex, size_t index, const Factor *factor, float count)
{
  if (ntIndex <= m_probStores.size()) {
	  m_probStores.resize(ntIndex + 1);
  }

  ProbStore &probStore = m_probStores[ntIndex];
  probStore.AddToMap(index, factor, count);
}

float NonTermContextTargetProperty::GetProb(size_t ntInd,
			size_t contextInd,
			const Factor *factor,
			float smoothConstant) const
{
	UTIL_THROW_IF2(ntInd >= m_probStores.size(), "Invalid nt index=" << ntInd);
	const ProbStore &probStore = m_probStores[ntInd];
	float ret = probStore.GetProb(contextInd, factor, smoothConstant);
	return ret;
}

//////////////////////////////////////////

void NonTermContextTargetProperty::ProbStore::AddToMap(size_t index, const Factor *factor, float count)
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


float NonTermContextTargetProperty::ProbStore::GetProb(size_t contextInd,
			const Factor *factor,
			float smoothConstant) const
{
  float count = GetCount(contextInd, factor, smoothConstant);
  float total = GetTotalCount(contextInd, smoothConstant);
  float ret = count / total;
  return ret;
}

float NonTermContextTargetProperty::ProbStore::GetCount(size_t contextInd,
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

float NonTermContextTargetProperty::ProbStore::GetTotalCount(size_t contextInd, float smoothConstant) const
{
	const Map &map = m_vec[contextInd];
	return m_totalCount + smoothConstant * map.size();
}


} // namespace Moses

