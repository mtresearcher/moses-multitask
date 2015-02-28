/*
 * OnlineLearner.h
 *
 */

/*
 * Online feature keeps track of recent source and target phrases in hope / fear hypothesis.
 */

#pragma once

#include "moses/Util.h"
#include "moses/TypeDef.h"
#include "moses/FF/FeatureFunction.h"
#include "moses/Hypothesis.h"
#include "moses/Factor.h"
#include "moses/TrellisPath.h"
#include "moses/TrellisPathList.h"
#include "moses/Manager.h"
#include "math.h"
#include "moses/StaticData.h"

#include "moses/FF/OnlineLearning/Optimiser.h"
#include "boost/unordered_map.hpp"
#include "boost/unordered_set.hpp"
#include "mert/TER/terAlignment.h"


#ifndef ONLINELEARNINGFEATURE_H_
#define ONLINELEARNINGFEATURE_H_

using namespace std;
using namespace Optimizer;

typedef boost::unordered_map<std::string, boost::unordered_map<std::string, float> > pp_feature;
typedef boost::unordered_map<std::string, boost::unordered_map<std::string, int> > pp_list;


typedef float learningrate;

namespace Moses {

class Optimiser;

class OnlineLearningFeature : public StatelessFeatureFunction{

	friend class PhraseDictionaryDynamicCacheBased;

protected:
	void InsertSparseFeature(std::string, std::string, int age=1, float margin=0.0);
	bool IfForceAlign() const {return m_forceAlign;}

public:
	OnlineLearningFeature(const std::string&);
	virtual ~OnlineLearningFeature();

	const bool OnlineLearningActivated() const {return m_learn;}
	void DeactivateOnlineLearning() {m_learn=false;}
	void ActivateOnlineLearning() {m_learn=true;}

	bool SetSourceSentence(std::string);
	const std::string GetSourceSentence() const {return m_source;}
	bool SetPostEditedSentence(std::string);
	bool SetAlignments(std::string);
	void RunOnlineLearning(Manager& manager);
	void RunOnlineMultiTaskLearning(Manager& manager, uint8_t task);
	void RemoveJunk();

	//	inline size_t GetNumScoreComponents() const { return 1; };

	inline std::string GetScoreProducerWeightShortName(unsigned) const { return "ol"; };

	virtual void EvaluateInIsolation(const Moses::Phrase&, const Moses::TargetPhrase&,
			Moses::ScoreComponentCollection&, Moses::ScoreComponentCollection&) const ;
	void EvaluateWithSourceContext(const InputType &input
			, const InputPath &inputPath
			, const TargetPhrase &targetPhrase
			, const StackVec *stackVec
			, ScoreComponentCollection &scoreBreakdown
			, ScoreComponentCollection *estimatedFutureScore = NULL) const  {}
	void EvaluateTranslationOptionListWithSourceContext(const Moses::InputType&,
			const Moses::TranslationOptionList&) const {}
	void EvaluateWhenApplied(const Hypothesis& hypo,
			ScoreComponentCollection* accumulator) const {}
	void EvaluateWhenApplied(const ChartHypothesis &hypo,
			ScoreComponentCollection* accumulator) const {assert(false);}

	void EvaluateWhenApplied(const Syntax::SHyperedge &,
			ScoreComponentCollection*) const { assert(false); }

	void SetParameter(const std::string& key, const std::string& value);

	static const OnlineLearningFeature& Instance() {
		return *s_instance;
	}
	static OnlineLearningFeature& InstanceNonConst() {
		return *s_instance;
	}

	bool IsUseable(const FactorMask &mask) const {
		return true;
	}

	void Load();
	void Load(const std::string filename);

	enum Algorithm {
		SparseFeatures = 0
		, FOnlyPerceptron = 1
		, FPercepWMira = 2
		, Mira = 3
	};

	enum UpdateStep {
		vonNeumann = 0
		, logDet = 1
	};

	enum Language {
		french = 0
		, spanish = 1
		, italian = 2
		, english = 3
	};

private:
	static OnlineLearningFeature *s_instance;
	Algorithm implementation;
	UpdateStep m_updateType;
	boost::unordered_set<std::string> m_vocab, m_stopwords;
	boost::unordered_map<std::string, std::string> curr_wordpair, postedit_wordpair;
	pp_feature m_feature;
	pp_list m_featureIdx;
	pp_list PP_ORACLE, PP_BEST, PP_NEW;
	learningrate flr, wlr;
	float m_decayValue, m_initScore;
	size_t m_nbestSize;
	std::string m_source, m_postedited, m_alignments;
	std::string m_sctype;
	std::vector< std::pair<std::string, std::string> > alignVec;
	Language m_language;
	bool m_normaliseScore, m_normaliseMargin, m_learn, m_cbtm, m_ngrams,
	m_triggerTargetWords, m_l1, m_l2, m_updateFeatures, m_forceAlign, m_terAlign;
	bool scale_margin, scale_margin_precision, scale_update, scale_update_precision;
	MiraOptimiser* optimiser;
	std::string m_filename;
	float w_init, w_initTargetWords,slack, m_sigmoidParam;
	std::string m_algorithm;

	inline std::string &trim(std::string &s);
	inline std::string &rtrim(std::string &s);
	inline std::string &ltrim(std::string &s);

	void Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const;

	void ShootUp(std::string, std::string, float);
	void ShootDown(std::string, std::string, float);

	void make_align_pairs();
	float calcMargin(Hypothesis* oracle, Hypothesis* bestHyp);
	void PrintHypo(const Hypothesis* hypo, ostream& HypothesisStringStream);
	bool has_only_spaces(const std::string& str);

	float GetBleu(std::string hypothesis, std::string reference);
	float GetTer(std::string hypothesis, std::string reference);

	void compareNGrams(boost::unordered_map<string, int>& hyp, boost::unordered_map<string, int>& ref,
			boost::unordered_map<int, float>& countNgrams, boost::unordered_map<int, float>& TotalNgrams);
	int getNGrams(std::string& str, boost::unordered_map<string, int>& ngrams);
	int split_marker_perl(string& str, string marker, vector<string> &array);

	int getNGrams(std::string& str, boost::unordered_map<string, int>& ngrams) const ;
	int split_marker_perl(string& str, string marker, vector<string> &array) const ;

	void updateFeatureValues();
	void chop(std::string &str);
	void Decay();

	void DumpFeatures(std::string);
	void ReadFeatures(std::string);
	void InsertTargetWords();
	void InsertNGrams();

	void updateIntMatrix();

	void GetPE2HypAlignments(const TERCPPNS_TERCpp::terAlignment&);
	void Update(std::string& , std::string& , std::string);
};
}
#endif /* ONLINELEARNINGFEATURE_H_ */
