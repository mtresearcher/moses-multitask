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
{

}

void NonTermContextProperty::ProcessValue(const std::string &value)
{
  vector<string> toks;
  Tokenize(toks, value);

  FactorCollection &fc = FactorCollection::Instance();

  for (size_t i = 0; i < toks.size(); i += 5) {
	  float count = Scan<float>(toks[i + 4]);

	  for (size_t j = 0; j < 4; ++j) {
		  const Factor *factor = fc.AddFactor(toks[i + j], false);
		  AddToMap(j, factor, count);
	  }
  }
}

void NonTermContextProperty::AddToMap(size_t j, const Factor *factor, float count)
{
	Map &map = m_vec[j];

	Map::iterator iter = map.find(factor);
	if (iter == map.end()) {
		map[factor] = count;
	}
	else {
		float &currCount = iter->second;
		currCount += count;
	}
}

} // namespace Moses

