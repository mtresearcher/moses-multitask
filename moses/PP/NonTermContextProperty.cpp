#include "moses/PP/NonTermContextProperty.h"
#include <string>
#include <assert.h>
#include "moses/Util.h"
#include "moses/FactorCollection.h"

using namespace std;

namespace Moses
{
NonTermContextProperty::NonTermContextProperty()
:m_vec(4)
,m_totalCount(0)
{

}

void NonTermContextProperty::ProcessValue(const std::string &value)
{
  vector<string> toks;
  Tokenize(toks, value);

  FactorCollection &fc = FactorCollection::Instance();

  for (size_t i = 0; i < toks.size(); i += 5) {
	  float count = Scan<float>(toks[i + 4]);
	  m_totalCount += count;

	  for (size_t j = 0; j < 4; ++j) {
		  const Factor *factor = fc.AddFactor(toks[i + j], false);
		  AddToMap(j, factor, count);
	  }
  }
}

void NonTermContextProperty::AddToMap(size_t index, const Factor *factor, float count)
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

float NonTermContextProperty::GetCount(size_t index, const Factor *factor) const
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

