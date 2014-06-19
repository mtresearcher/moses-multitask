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

	  for (size_t j = 0; j < 4; ++j) {
		  const Factor *factor = fc.AddFactor(toks[i + j + 1], false);
		  AddToMap(ntInd, j, factor, count);
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
}

float NonTermContextProperty::ProbStore::GetCount(size_t index, const Factor *factor) const
{
	const Map &map = m_vec[index];

	Map::const_iterator iter = map.find(factor);
	if (iter == map.end()) {
		return 0;
	}
	else {
		float count = iter->second;
		return count;
	}
}


} // namespace Moses

