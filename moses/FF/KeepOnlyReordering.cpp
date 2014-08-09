#include <vector>
#include "KeepOnlyReordering.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/ChartTranslationOptionList.h"
#include "moses/ChartTranslationOptions.h"
#include "moses/PP/CountsPhraseProperty.h"
#include "moses/TranslationModel/PhraseDictionary.h"

using namespace std;

namespace Moses
{
KeepOnlyReordering::KeepOnlyReordering(const std::string &line)
:StatelessFeatureFunction(0, line)
,m_pt(NULL)
{
  m_tuneable = false;
  ReadParameters();
}

std::vector<float> KeepOnlyReordering::DefaultWeights() const
{
  UTIL_THROW_IF2(m_numScoreComponents != 0, "No scores here");
  vector<float> ret(0);
  return ret;
}

void KeepOnlyReordering::EvaluateInIsolation(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{
  targetPhrase.SetRuleSource(source);
}

void KeepOnlyReordering::EvaluateWithSourceContext(const InputType &input
                                   , const InputPath &inputPath
                                   , const TargetPhrase &targetPhrase
                                   , const StackVec *stackVec
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection *estimatedFutureScore) const
{
}

void KeepOnlyReordering::EvaluateWhenApplied(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const
{}

void KeepOnlyReordering::EvaluateWhenApplied(const ChartHypothesis &hypo,
                                        ScoreComponentCollection* accumulator) const
{}

void KeepOnlyReordering::EvaluateWithAllTransOpts(ChartTranslationOptionList &transOptList) const
{
	if (transOptList.GetSize() == 0) {
		return;
	}

	// only prune for a particular pt
	if (m_pt) {
  	  ChartTranslationOptions &transOpts = transOptList.Get(0);
		const ChartTranslationOption &transOpt = transOpts.Get(0);
		const TargetPhrase &tp = transOpt.GetPhrase();
		const PhraseDictionary *tpPt = tp.GetContainer();
		if (tpPt != m_pt) {
			return;
		}
	}

	// transopts to be deleted
	typedef set<ChartTranslationOptions*> Coll;
	Coll transOptsToDelete;

	//cerr << "ChartTranslationOptionList:" << endl;
	for (size_t i = 0; i < transOptList.GetSize(); ++i) {
		ChartTranslationOptions &transOpts = transOptList.Get(i);
		//cerr << "ChartTranslationOptions " << i << "=" << transOpts.GetSize() << endl;
		UTIL_THROW_IF2(transOpts.GetSize() == 0, "transOpts can't be empty");

		// got thru each rule
		bool hasReordering = false;
		for (size_t j = 0; j < transOpts.GetSize(); ++j) {
			const ChartTranslationOption &transOpt = transOpts.Get(j);
			const TargetPhrase &tp = transOpt.GetPhrase();
			const Phrase *sp = tp.GetRuleSource();
			assert(sp);

			hasReordering = ContainsReordering(*sp, tp);
			if (hasReordering) {
				break;
			}
		}

		if (!hasReordering) {
			transOptsToDelete.insert(&transOpts);
		}
	}

	// delete
	Coll::iterator iter;
	for (iter = transOptsToDelete.begin(); iter != transOptsToDelete.end(); ++iter) {
		ChartTranslationOptions *transOpts = *iter;
		transOpts->Clear();
	}
}

bool KeepOnlyReordering::ContainsReordering(const Phrase &sp, const TargetPhrase &tp) const
{
	const AlignmentInfo &ntAligns = tp.GetAlignNonTerm();
	const AlignmentInfo &termAligns = tp.GetAlignNonTerm();

	AlignmentInfo::const_iterator iterNT;
	for (iterNT = ntAligns.begin(); iterNT != ntAligns.end(); ++iterNT ) {
		const std::pair<size_t,size_t> &ntAlign = *iterNT;
		if (ntAligns.Cross(ntAlign) || termAligns.Cross(ntAlign)) {
			return true;
		}
	}
	return false;
}

void KeepOnlyReordering::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "phrase-table") {
	  FeatureFunction &ff = FindFeatureFunction(value);
	  m_pt = dynamic_cast<const PhraseDictionary*>(&ff);
	  UTIL_THROW_IF2(m_pt == NULL, "Not a phrase-table: " << value);
  }
  else {
	  StatelessFeatureFunction::SetParameter(key, value);
  }
}

}

