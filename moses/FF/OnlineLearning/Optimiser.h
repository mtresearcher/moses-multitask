/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/
#ifndef _MIRA_OPTIMISER_H_
#define _MIRA_OPTIMISER_H_

#include <vector>

#include "moses/ScoreComponentCollection.h"
#include "moses/StaticData.h"
#include "moses/FF/OnlineLearning/Hildreth.h"
#include <Eigen/Dense>
#include <unsupported/Eigen/MatrixFunctions>
using namespace Moses;
using namespace std;

namespace Optimizer {

class Optimisers {
public:
	Optimisers() {}

	Optimisers(
			float slack, bool scale_margin, bool scale_margin_precision,
			bool scale_update, bool scale_update_precision, bool normaliseMargin,
			float sigmoidParam, bool l1, bool l2, float LR) :
				m_slack(slack),
				m_scale_margin(scale_margin),
				m_scale_margin_precision(scale_margin_precision),
				m_scale_update(scale_update),
				m_scale_update_precision(scale_update_precision),
				m_precision(1),
				m_normaliseMargin(normaliseMargin),
				m_l1(l1),
				m_l2(l2),
				m_sigmoidParam(sigmoidParam),
				learningRate(LR){ }

    size_t updateMultiTaskWeights(
    			Moses::ScoreComponentCollection& weightUpdate,
    			const std::vector<std::vector<Moses::ScoreComponentCollection> >& featureValues,
    			const std::vector<std::vector<float> >& losses,
    			const std::vector<std::vector<float> >& bleuScores,
    			const std::vector<std::vector<float> >& modelScores,
    			const std::vector< Moses::ScoreComponentCollection>& oracleFeatureValues,
    			const std::vector<float> oracleBleuScores,
    			const std::vector<float> oracleModelScores,
    			const Eigen::MatrixXd& regularizer,
    			const uint8_t task,
    			const uint8_t task_id,
    			float learning_rate){
    		// vector of feature values differences for all created constraints
    		vector<ScoreComponentCollection> featureValueDiffs;
    		vector<float> lossMinusModelScoreDiffs;
    		vector<float> all_losses;
    		std::cerr<<"Regularizer is of size "<<regularizer.rows()<<"x"<<regularizer.cols();
    		// Make constraints for new hypothesis translations
    		float epsilon = 0.0001;
    		int violatedConstraintsBefore = 0;
    		float oldDistanceFromOptimum = 0;
    		// iterate over input sentences (1 (online) or more (batch))
    		for (size_t i = 0; i < featureValues.size(); ++i) {
    			//size_t sentenceId = sentenceIds[i];
    			// iterate over hypothesis translations for one input sentence
    			for (size_t j = 0; j < featureValues[i].size(); ++j) {
    				ScoreComponentCollection featureValueDiff = oracleFeatureValues[i];
    				featureValueDiff.MinusEquals(featureValues[i][j]);

    				if (featureValueDiff.GetL1Norm() == 0) { // over sparse & core features values
    					continue;
    				}

    				float loss = losses[i][j];

    				// check if constraint is violated
    				bool violated = false;
    				//		    float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
    				float modelScoreDiff = oracleModelScores[i] - modelScores[i][j];
    				float diff = 0;

    				if (loss > modelScoreDiff)
    					diff = loss - modelScoreDiff;

    				if (diff > epsilon)
    					violated = true;

    				if (m_normaliseMargin) {
    					modelScoreDiff = (2*m_sigmoidParam/(1 + exp(-modelScoreDiff))) - m_sigmoidParam;
    					loss = (2*m_sigmoidParam/(1 + exp(-loss))) - m_sigmoidParam;
    					diff = 0;
    					if (loss > modelScoreDiff) {
    						diff = loss - modelScoreDiff;
    					}
    				}

    				if (m_scale_margin) {
    					diff *= oracleBleuScores[i];
    				}

    				featureValueDiffs.push_back(featureValueDiff);
    				lossMinusModelScoreDiffs.push_back(diff);
    				all_losses.push_back(loss);
    				if (violated) {
    					++violatedConstraintsBefore;
    					oldDistanceFromOptimum += diff;
    				}
    			}
    		}

    		// run optimisation: compute alphas for all given constraints
    		vector<float> alphas;
    		ScoreComponentCollection summedUpdate;
    		if (violatedConstraintsBefore > 0) {
    //			cerr<<"Features values diff size : "<<featureValueDiffs.size() << " (of which violated: " << violatedConstraintsBefore << ")" << endl;
    			if (m_slack != 0) {
    				alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs, m_slack);
    //				cerr<<"Alphas : ";for (int i=0;i<alphas.size();i++) cerr<<alphas[i]<<" ";cerr<<"\n";
    			} else {
    				alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs);
    //				cerr<<"Alphas : ";for (int i=0;i<alphas.size();i++) cerr<<alphas[i]<<" ";cerr<<"\n";
    			}

    			// Update the weight vector according to the alphas and the feature value differences
    			// * w' = w' + SUM alpha_i * (h_i(oracle) - h_i(hypothesis))
    			for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
    				float alpha = alphas[k];
    				ScoreComponentCollection update(featureValueDiffs[k]);
    				update.MultiplyEquals(alpha);
    				// sum updates
    				summedUpdate.PlusEquals(update);

    				// Update the weight vector according to the alphas and the feature value differences
    				// * w' = w' + SUM alpha_i * (h_i(oracle) - h_i(hypothesis))
    				for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
    					float alpha = alphas[k];
    					if(alpha != 0){
    						ScoreComponentCollection update(featureValueDiffs[k]);
    						// build the feature matrix \phi_t
    						Eigen::MatrixXd featureMatrix(update.Size()*task,1);
    						const Moses::FVector x = update.GetScoresVector();
    						for(size_t i=0; i<task; i++){
    							if(i == task_id){
    								for(int j=0;j<x.size(); j++){
    									featureMatrix (i*update.Size()+j,0) = x[j];
    								}
    							}
    							else{
    								for(int j=0;j<update.Size(); j++){
    									featureMatrix (i*update.Size()+j,0) = 0;
    								}
    							}
    						}
    						// take dot prod. of kdkd matrix and feature matrix
    						Eigen::MatrixXd C = regularizer * featureMatrix;
    						ScoreComponentCollection temp(update);
    						// slice the compound weight vector
    						for(size_t i=0;i<update.Size();i++){
    							float x = C(task_id*update.Size()+i, 0);
    							temp.Assign(i, x);
    						}
//    						cerr<<" <\Delta h> : ";
//    						temp.PrintCoreFeatures();
//    						cerr<<"\n";
    						// here we also multiply with the co-regularization vector
    						temp.MultiplyEquals(alpha);
    						// sum updates
    						summedUpdate.PlusEquals(temp);
    					}
    				}
    			}
    		}
    		else {
    			return 1;
    		}

    		// apply learning rate
    		if (learning_rate != 1) {
    			summedUpdate.MultiplyEquals(learning_rate);
    		}

    		// scale update by BLEU of oracle (for batch size 1 only)
    		if (oracleBleuScores.size() == 1) {
    			if (m_scale_update) {
    				summedUpdate.MultiplyEquals(oracleBleuScores[0]);
    			}
    		}
    		//    		cerr<<"Summed Update : "<<summedUpdate<<endl;
    		weightUpdate.PlusEquals(summedUpdate);
    		//    		cerr<<"Weight Update : "<<weightUpdate<<endl;

    		if(m_l1)
    			weightUpdate.SparseL1Regularize(0.01);
    		if(m_l2)
    			weightUpdate.SparseL2Regularize(0.01);

    		return 0;

    	}

	size_t updateWeights(
			Moses::ScoreComponentCollection& weightUpdate,
			const std::vector<std::vector<Moses::ScoreComponentCollection> >& featureValues,
			const std::vector<std::vector<float> >& losses,
			const std::vector<std::vector<float> >& bleuScores,
			const std::vector<std::vector<float> >& modelScores,
			const std::vector< Moses::ScoreComponentCollection>& oracleFeatureValues,
			const std::vector<float> oracleBleuScores,
			const std::vector<float> oracleModelScores,
			float learning_rate){

		// vector of feature values differences for all created constraints
		vector<ScoreComponentCollection> featureValueDiffs;
		vector<float> lossMinusModelScoreDiffs;
		vector<float> all_losses;

		// Make constraints for new hypothesis translations
		float epsilon = 0.0001;
		int violatedConstraintsBefore = 0;
		float oldDistanceFromOptimum = 0;
		// iterate over input sentences (1 (online) or more (batch))
		for (size_t i = 0; i < featureValues.size(); ++i) {
			//size_t sentenceId = sentenceIds[i];
			// iterate over hypothesis translations for one input sentence
			for (size_t j = 0; j < featureValues[i].size(); ++j) {
				ScoreComponentCollection featureValueDiff = oracleFeatureValues[i];
				featureValueDiff.MinusEquals(featureValues[i][j]);

				if (featureValueDiff.GetL1Norm() == 0) { // over sparse & core features values
					continue;
				}

				float loss = losses[i][j];

				// check if constraint is violated
				bool violated = false;
				//		    float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
				float modelScoreDiff = oracleModelScores[i] - modelScores[i][j];
				float diff = 0;

				if (loss > modelScoreDiff)
					diff = loss - modelScoreDiff;

				if (diff > epsilon)
					violated = true;

				if (m_normaliseMargin) {
					modelScoreDiff = (2*m_sigmoidParam/(1 + exp(-modelScoreDiff))) - m_sigmoidParam;
					loss = (2*m_sigmoidParam/(1 + exp(-loss))) - m_sigmoidParam;
					diff = 0;
					if (loss > modelScoreDiff) {
						diff = loss - modelScoreDiff;
					}
				}

				if (m_scale_margin) {
					diff *= oracleBleuScores[i];
				}

				featureValueDiffs.push_back(featureValueDiff);
				lossMinusModelScoreDiffs.push_back(diff);
				all_losses.push_back(loss);
				if (violated) {
					++violatedConstraintsBefore;
					oldDistanceFromOptimum += diff;
				}
			}
		}

		// run optimisation: compute alphas for all given constraints
		vector<float> alphas;
		ScoreComponentCollection summedUpdate;
		if (violatedConstraintsBefore > 0) {
//			cerr<<"Features values diff size : "<<featureValueDiffs.size() << " (of which violated: " << violatedConstraintsBefore << ")" << endl;
			if (m_slack != 0) {
				alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs, m_slack);
//				cerr<<"Alphas : ";for (int i=0;i<alphas.size();i++) cerr<<alphas[i]<<" ";cerr<<"\n";
			} else {
				alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs);
//				cerr<<"Alphas : ";for (int i=0;i<alphas.size();i++) cerr<<alphas[i]<<" ";cerr<<"\n";
			}

			// Update the weight vector according to the alphas and the feature value differences
			// * w' = w' + SUM alpha_i * (h_i(oracle) - h_i(hypothesis))
			for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
				float alpha = alphas[k];
				ScoreComponentCollection update(featureValueDiffs[k]);
				update.MultiplyEquals(alpha);
				// sum updates
				summedUpdate.PlusEquals(update);
			}
		}
		else {
			return 1;
		}

		// apply learning rate
		if (learning_rate != 1) {
			summedUpdate.MultiplyEquals(learning_rate);
		}

		// scale update by BLEU of oracle (for batch size 1 only)
		if (oracleBleuScores.size() == 1) {
			if (m_scale_update) {
				summedUpdate.MultiplyEquals(oracleBleuScores[0]);
			}
		}
		//    		cerr<<"Summed Update : "<<summedUpdate<<endl;
		weightUpdate.PlusEquals(summedUpdate);
		//    		cerr<<"Weight Update : "<<weightUpdate<<endl;

		if(m_l1)
			weightUpdate.SparseL1Regularize(0.01);
		if(m_l2)
			weightUpdate.SparseL2Regularize(0.01);

		return 0;

	}

	void setSlack(float slack) {
		m_slack = slack;
	}

	void setPrecision(float precision) {
		m_precision = precision;
	}

	size_t updateWeightsAdaGrad(
			Moses::ScoreComponentCollection& weightUpdate,
			std::vector<Moses::ScoreComponentCollection>& featureValues,
			Moses::ScoreComponentCollection& oracleFeatureValues,
			std::vector<float>& bleuScores,
			float oracleBleu){
		// capturing the fixed weights : 1,0
		vector<int> indexes_ones, indexes_zeroes;
		for (size_t j = 0; j < weightUpdate.GetScoresVector().coreSize(); ++j)
			if(weightUpdate.GetScoresVector()[j]==1)
				indexes_ones.push_back(j);
			else if(weightUpdate.GetScoresVector()[j]==0)
				indexes_zeroes.push_back(j);
		
		ScoreComponentCollection featureValueDiff(oracleFeatureValues);
		// scale with oracleBleu-bestBleu
		featureValueDiff.MultiplyEquals(oracleBleu-bleuScores[0]);

		// don't touch the fixed weights
		for (size_t j = 0; j < indexes_ones.size(); ++j) featureValueDiff.Assign(indexes_ones[j], 0);
		for (size_t j = 0; j < indexes_zeroes.size(); ++j) featureValueDiff.Assign(indexes_zeroes[j], 0);
		cerr<<"Feature Value Diff : ";
		featureValueDiff.PrintCoreFeatures();
		cerr<<endl;

		Moses::FVector scoreVec = featureValueDiff.GetScoresVector();
		Moses::ScoreComponentCollection summedUpdate;
		for (size_t idx=0; idx<scoreVec.coreSize(); ++idx){ // core features
			double curr_gradient = scoreVec[idx];
			if(sumGradient_core.find(idx)==sumGradient_core.end())
				sumGradient_core[idx] = curr_gradient * curr_gradient;
			else
				sumGradient_core[idx] += curr_gradient * curr_gradient;

			double adj_gradient = curr_gradient / sqrt(1 + sumGradient_core[idx]);
			summedUpdate.Assign(idx, adj_gradient);
		}
		for(Moses::FVector::iterator it = scoreVec.begin(); it!=scoreVec.end(); it++)
		{ // sparse features
			std::string featureName = it->first.name();
			double curr_gradient = scoreVec[it->first];
			if(sumGradient_sparse.find(featureName)==sumGradient_sparse.end())
				sumGradient_sparse[featureName] = curr_gradient * curr_gradient;
			else
				sumGradient_sparse[featureName] += curr_gradient * curr_gradient;
			double adj_gradient = curr_gradient / sqrt(1 + sumGradient_sparse[featureName]) ;
			summedUpdate.Assign(featureName, adj_gradient);
		}

		if(learningRate!=1) summedUpdate.MultiplyEquals(learningRate);

		weightUpdate.MinusEquals(summedUpdate);

		if(m_l1)
			weightUpdate.SparseL1Regularize(0.01);
		if(m_l2)
			weightUpdate.SparseL2Regularize(0.01);

		weightUpdate.CapMax(1);
		weightUpdate.CapMin(-1);

		return 0;
	}

	size_t updateWeightsPerceptron(
				Moses::ScoreComponentCollection& weightUpdate,
				std::vector<Moses::ScoreComponentCollection>& featureValues,
				Moses::ScoreComponentCollection& oracleFeatureValues,
				std::vector<float>& bleuScores,
				float oracleBleu) {
		ScoreComponentCollection featureValueDiff(featureValues[0]); // X
		// scale with oracleBleu-bestBleu
		featureValueDiff.MultiplyEquals(oracleBleu-bleuScores[0]); // \Delta Y \times X

		vector<int> indexes_ones, indexes_zeroes;
		// capturing the fixed weights : 1,0
		for (size_t j = 0; j < weightUpdate.Size(); ++j)
			if(weightUpdate.GetScoresVector()[j]==1)
				indexes_ones.push_back(j);
			else if(weightUpdate.GetScoresVector()[j]==0)
				indexes_zeroes.push_back(j);

		// don't touch the fixed weights
		for (size_t j = 0; j < indexes_ones.size(); ++j) featureValueDiff.Assign(indexes_ones[j], 0);
		for (size_t j = 0; j < indexes_zeroes.size(); ++j) featureValueDiff.Assign(indexes_zeroes[j], 0);

		if(learningRate!=0) // times learning rate , scaling with best Bleu , scaling with hits/NBestSize
			featureValueDiff.MultiplyEquals(learningRate);  // \alpha \times \Delta Y \times X

		weightUpdate.PlusEquals(featureValueDiff);

		weightUpdate.CapMax(1);
		weightUpdate.CapMin(-1);
		return 0;
	}


private:
	// regularise Hildreth updates
	float m_slack;

	// scale margin with BLEU score or precision
	bool m_scale_margin, m_scale_margin_precision;

	// use l1/l2 regularizer
	bool m_l1, m_l2;

	// scale update with oracle BLEU score or precision
	bool m_scale_update, m_scale_update_precision;

	float m_precision;

	// squash margin between 0 and 1 (or depending on m_sigmoidParam)
	bool m_normaliseMargin;

	// y=sigmoidParam is the axis that this sigmoid approaches
	float m_sigmoidParam ;

	// adagrad parameters
	boost::unordered_map<std::string, double> sumGradient_sparse;
	boost::unordered_map<size_t, double> sumGradient_core; // keep it as a map , one can delete the elements on the fly (regularization)
	float learningRate;
};

// separate classes for different learning algorithms
}

#endif
