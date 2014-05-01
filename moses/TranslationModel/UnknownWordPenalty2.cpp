// vim:tabstop=2
#include "UnknownWordPenalty2.h"
#include "moses/TypeDef.h"

using namespace std;

namespace Moses
{
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

  InputPathList::const_iterator iter;
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &inputPath = **iter;
    const Phrase &sourcePhrase = inputPath.GetPhrase();

    if (inputPath.GetTotalRuleSize() == 0) {
		if (sourcePhrase.GetSize() <= m_maxPhraseLength) {
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
  return NULL;
}


TO_STRING_BODY(UnknownWordPenalty2);

// friend
ostream& operator<<(ostream& out, const UnknownWordPenalty2& phraseDict)
{
  return out;
}

}
