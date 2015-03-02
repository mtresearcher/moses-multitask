/*
 * MultiTaskLearning.h
 *
 *  Created on: Mar 1, 2015
 *      Author: prashant
 */

#ifndef MULTITASKLEARNING_H_
#define MULTITASKLEARNING_H_

#include "ScoreComponentCollection.h"
#include "Util.h"
#include <map>
#include <vector>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/lu.hpp>

using namespace std;

namespace Moses {
// this is a stateless function because we need to add an additional bias feature
class MultiTaskLearning : public StatelessFeatureFunction {

	enum UpdateInteractionMatrixType {
		noupdate = 0,
		vonNeumann = 1,
		logDet = 2,
		Burg = 3
	};

	uint8_t m_users;
	map<uint8_t, ScoreComponentCollection> m_user2weightvec;
	int m_currtask;
	float m_learningrate;
	bool m_multitask;
	UpdateInteractionMatrixType m_implementation;
	boost::numeric::ublas::matrix<double> m_kdkdmatrix;
	boost::numeric::ublas::matrix<double> m_intMatrix;
	static MultiTaskLearning *s_instance;

public:
	// functions on task
	const uint8_t GetNumberOfTasks() const {return m_users;};
	void SetCurrentTask(int taskid){m_currtask=taskid;};
	const uint8_t GetCurrentTask() const {return m_currtask;};

	// function related to Interaction Matrix
	void SetInteractionMatrix(boost::numeric::ublas::matrix<double>& interactionMatrix);
	const boost::numeric::ublas::matrix<double>& GetInteractionMatrix() const {return m_intMatrix;};

	// functions related to kdkdmatrix
	void SetKdKdMatrix(boost::numeric::ublas::matrix<double>&);
	const boost::numeric::ublas::matrix<double>& GetKdKdMatrix() const {return m_kdkdmatrix;};

	// functions related to registered weight vectors
	ScoreComponentCollection GetWeightsVector(int) const ;
	void SetWeightsVector(uint8_t, ScoreComponentCollection);
	void l1(float lambda); // l1 regularizer
	void l2(float lambda); // l2 regularizer
	void l1X(float lambda); // l1 infinity regularizer

	// functions related to constructor/desctructor
	MultiTaskLearning(const std::string &line);
	inline std::string GetScoreProducerWeightShortName(unsigned) const { return "mtl"; };
	virtual ~MultiTaskLearning();

	const bool IfMultiTask() const {return m_multitask;};
	const float GetLearningRateIntMatrix() const {return m_learningrate;};
	boost::numeric::ublas::matrix<double> GetWeightsMatrix() ;

	virtual void EvaluateInIsolation(const Moses::Phrase&, const Moses::TargetPhrase&,
			Moses::ScoreComponentCollection&, Moses::ScoreComponentCollection&) const;
	void EvaluateWithSourceContext(const InputType &input
			, const InputPath &inputPath
			, const TargetPhrase &targetPhrase
			, const StackVec *stackVec
			, ScoreComponentCollection &scoreBreakdown
			, ScoreComponentCollection *estimatedFutureScore = NULL) const {}
	void EvaluateTranslationOptionListWithSourceContext(const Moses::InputType&,
			const Moses::TranslationOptionList&) const {}
	void EvaluateWhenApplied(const Hypothesis& hypo,
			ScoreComponentCollection* accumulator) const {}
	void EvaluateWhenApplied(const ChartHypothesis &hypo,
			ScoreComponentCollection* accumulator) const {assert(false);}
	void EvaluateWhenApplied(const Syntax::SHyperedge &,
			ScoreComponentCollection*) const { assert(false); }

	void SetParameter(const std::string& , const std::string& );

	bool IsUseable(const FactorMask &mask) const {
		return true;
	}
	static const MultiTaskLearning& Instance() const {
		return *s_instance;
	}
	static MultiTaskLearning& InstanceNonConst() const {
		return *s_instance;
	}

};

} /* namespace Moses */

#endif /* MULTITASKLEARNING_H_ */
