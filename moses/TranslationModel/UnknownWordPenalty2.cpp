// vim:tabstop=2
#include "UnknownWordPenalty2.h"
#include "moses/TypeDef.h"
#include "moses/ChartRuleLookupManager.h"
#include "moses/DecodeGraph.h"
#include "moses/DecodeStep.h"

using namespace std;

namespace Moses
{
class ChartRuleLookupUnk : public ChartRuleLookupManager
{
public:
	ChartRuleLookupUnk(const ChartParser &parser,
						 const ChartCellCollectionBase &cellColl)
	:ChartRuleLookupManager(parser, cellColl)
	{}

  virtual void GetChartRuleCollection(
	const WordsRange &range,
	size_t lastPos,  // last position to consider if using lookahead
	ChartParserCallback &outColl)
  {}

};

UnknownWordPenalty2::UnknownWordPenalty2(const std::string &line)
:PhraseDictionary(line)
,m_maxPhraseLength(1)
{
  m_tuneable = false;

  ReadParameters();
}

void UnknownWordPenalty2::Load()
{
	SetFeaturesToApply();
}

void UnknownWordPenalty2::InitializeForInput(InputType const& source)
{
  ReduceCache();
}

void UnknownWordPenalty2::GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
{
  CacheColl &cache = GetCache();
  const DecodeGraph &decodeGraph = GetContainer().GetContainer();
  size_t backoff = decodeGraph.GetBackoff();

  InputPathList::const_iterator iter;
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &inputPath = **iter;
    const Phrase &sourcePhrase = inputPath.GetPhrase();

    if (sourcePhrase.GetSize() > m_maxPhraseLength) {
    	// over max length
    	continue;
    }

    if (backoff == 0 || (backoff && inputPath.GetTotalRuleSize() == 0)) {
		TargetPhrase *tp = CreateTargetPhrase(sourcePhrase);
		TargetPhraseCollection *tpColl = new TargetPhraseCollection();
		tpColl->Add(tp);

		// add target phrase to phrase-table cache
		size_t hash = hash_value(sourcePhrase);
		std::pair<const TargetPhraseCollection*, clock_t> value(tpColl, clock());
		cache[hash] = value;

		inputPath.SetTargetPhrases(*this, tpColl, NULL);
    }
  }
}

TargetPhrase *UnknownWordPenalty2::CreateTargetPhrase(const Phrase &sourcePhrase) const
{
  TargetPhrase *tp = new TargetPhrase(sourcePhrase);

  // score for this phrase table
  vector<float> scores(m_numScoreComponents, LOWEST_SCORE);
  tp->GetScoreBreakdown().PlusEquals(this, scores);

  // score of all other ff when this rule is being loaded
  tp->Evaluate(sourcePhrase, GetFeaturesToApply());

  return tp;
}

void
UnknownWordPenalty2::
SetParameter(const std::string& key, const std::string& value)
{
  if (key == "max-phrase-length") {
	  m_maxPhraseLength = Scan<size_t>(value);
  } else {
	  PhraseDictionary::SetParameter(key, value);
  }

}

std::vector<float> UnknownWordPenalty2::DefaultWeights() const
{
  std::vector<float> ret(m_numScoreComponents, 1.0f);
  return ret;
}


ChartRuleLookupManager* UnknownWordPenalty2::CreateRuleLookupManager(const ChartParser &parser,
    const ChartCellCollectionBase &cellCollection,
    std::size_t /*maxChartSpan*/)
{
  cerr << "BEFORE\n";
  return new ChartRuleLookupUnk(parser, cellCollection);
  cerr << "AFTER\n";
}


TO_STRING_BODY(UnknownWordPenalty2);

// friend
ostream& operator<<(ostream& out, const UnknownWordPenalty2& phraseDict)
{
  return out;
}

}
