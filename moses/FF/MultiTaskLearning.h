/*
 * MultiTaskLearning.h
 *
 *  Created on: Mar 1, 2015
 */
#pragma once 
#ifndef MULTITASKLEARNING_H_
#define MULTITASKLEARNING_H_


#include <map>
#include <vector>
#include "moses/Util.h"
#include "moses/ScoreComponentCollection.h"
#include "StatelessFeatureFunction.h"

#include <Eigen/Dense>
#include <unsupported/Eigen/MatrixFunctions>
#include <unsupported/Eigen/KroneckerProduct>

using namespace std;

namespace Moses {
// this is a stateless function because we need to add an additional bias feature

class MultiTaskLearning : public StatelessFeatureFunction{
    
public:

	enum UpdateInteractionMatrixType {
		noupdate = 0,
		vonNeumann = 1,
		logDet = 2,
		Burg = 3
	};

	// functions on task
	const uint8_t GetNumberOfTasks() const {return m_users;};
	void SetCurrentTask(int taskid){m_currtask=taskid;};
	const uint8_t GetCurrentTask() const {return m_currtask;};

	// function related to Interaction Matrix
	void SetInteractionMatrix(Eigen::MatrixXd& interactionMatrix){m_intMatrix = interactionMatrix;}
	const Eigen::MatrixXd& GetInteractionMatrix() const {return m_intMatrix;};
	void updateIntMatrix();

	// functions related to kdkdmatrix
	void SetKdKdMatrix(Eigen::MatrixXd& kdkdmatrix){m_kdkdmatrix=kdkdmatrix;}
	const Eigen::MatrixXd& GetKdKdMatrix() const {return m_kdkdmatrix;};

	// functions related to registered weight vectors
	ScoreComponentCollection GetWeightsVector(uint8_t) ;
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
	Eigen::MatrixXd GetWeightsMatrix() ;

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
	void SetParameter(const std::string& , const std::string& );

	bool IsUseable(const FactorMask &mask) const {
		return true;
	}
	static const MultiTaskLearning& Instance() {
		return *s_instance;
	}
	static MultiTaskLearning& InstanceNonConst() {
		return *s_instance;
	}

private:
	uint8_t m_users;
	map<uint8_t, ScoreComponentCollection> m_user2weightvec;
	int m_currtask;
	float m_learningrate;
	bool m_multitask;
	UpdateInteractionMatrixType m_implementation;
	Eigen::MatrixXd m_kdkdmatrix;
	Eigen::MatrixXd m_intMatrix;
	static MultiTaskLearning *s_instance;
	void chop(string &str);
	int split_marker_perl(std::string& str, std::string marker, std::vector<std::string> &array);
	void ReadMatrix(std::string filename);

};

} /* namespace Moses */

#endif /* MULTITASKLEARNING_H_ */
