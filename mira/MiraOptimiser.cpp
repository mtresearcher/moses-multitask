#include "Optimiser.h"
#include "Hildreth.h"

using namespace Moses;
using namespace std;

namespace Mira {

int MiraOptimiser::updateWeights(ScoreComponentCollection& currWeights,
    const vector<vector<ScoreComponentCollection> >& featureValues,
    const vector<vector<float> >& losses,
    const vector<std::vector<float> >& bleuScores, const vector<
        ScoreComponentCollection>& oracleFeatureValues,
    const vector<float> oracleBleuScores, const vector<size_t> sentenceIds,
    float learning_rate, float max_sentence_update, size_t rank,
    int updates_per_epoch,
    bool controlUpdates) {

	// add every oracle in batch to list of oracles (under certain conditions)
	for (size_t i = 0; i < oracleFeatureValues.size(); ++i) {
		float newWeightedScore = oracleFeatureValues[i].GetWeightedScore();
		size_t sentenceId = sentenceIds[i];

		// compare new oracle with existing oracles:
		// if same translation exists, just update the bleu score
		// if not, add the oracle
		bool updated = false;
		size_t indexOfWorst = 0;
		float worstWeightedScore = 0;
		for (size_t j = 0; j < m_oracles[sentenceId].size(); ++j) {
			float currentWeightedScore = m_oracles[sentenceId][j].GetWeightedScore();
			if (currentWeightedScore == newWeightedScore) {
				cerr << "updated.." << endl;
				m_bleu_of_oracles[sentenceId][j] = oracleBleuScores[j];
				updated = true;
				break;
			} else if (worstWeightedScore == 0 || currentWeightedScore
			    > worstWeightedScore) {
				worstWeightedScore = currentWeightedScore;
				indexOfWorst = j;
			}
		}

		if (!updated) {
			// add if number of maximum oracles not exceeded, otherwise override the worst
			if (m_max_number_oracles > m_oracles[sentenceId].size()) {
				m_oracles[sentenceId].push_back(oracleFeatureValues[i]);
				m_bleu_of_oracles[sentenceId].push_back(oracleBleuScores[i]);
			} else {
				m_oracles[sentenceId][indexOfWorst] = oracleFeatureValues[i];
				m_bleu_of_oracles[sentenceId][indexOfWorst] = oracleBleuScores[i];
			}
		}
	}

	// vector of feature values differences for all created constraints
	vector<ScoreComponentCollection> featureValueDiffs;
	int violatedConstraintsBefore = 0;
	vector<float> lossMinusModelScoreDiff;

	// find most violated constraint
	float maxViolationLossMarginDistance;
	ScoreComponentCollection maxViolationfeatureValueDiff;

	float epsilon = 0.0001;
	float oldDistanceFromOptimum = 0;
	// iterate over input sentences (1 (online) or more (batch))
	for (size_t i = 0; i < featureValues.size(); ++i) {
		size_t sentenceId = sentenceIds[i];
		if (m_oracles[sentenceId].size() > 1)
			cerr << "Rank " << rank << ", available oracles for source sentence " << sentenceId << ": "  << m_oracles[sentenceId].size() << endl;
		// iterate over hypothesis translations for one input sentence
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			// check if optimisation criterion is violated for one hypothesis and the oracle
			// h(e*) >= h(e_ij) + loss(e_ij)
			// h(e*) - h(e_ij) >= loss(e_ij)

			// iterate over all available oracles (1 if not accumulating, otherwise one per started epoch)
			for (size_t k = 0; k < m_oracles[sentenceId].size(); ++k) {
				//cerr << "Oracle " << k << ": " << m_oracles[sentenceId][k] << " (BLEU: " << m_bleu_of_oracles[sentenceId][k] << ", model score: " <<  m_oracles[sentenceId][k].GetWeightedScore() << ")" << endl;
				ScoreComponentCollection featureValueDiff = m_oracles[sentenceId][k];
				featureValueDiff.MinusEquals(featureValues[i][j]);
//				cerr << "\nFeature value diff: " << featureValueDiff << endl;
				float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
				float loss = losses[i][j] * m_marginScaleFactor;
				if (m_weightedLossFunction) {
					loss *= log10(bleuScores[i][j]);
				}

//				cerr << "Rank " << rank << ", model score diff: " << modelScoreDiff << ", loss: " << loss << endl;

				bool addConstraint = true;
				if (modelScoreDiff < loss) {
					float diff = loss - modelScoreDiff;
					// approximate comparison between floats
					if (diff > epsilon) {
						// constraint violated
						++violatedConstraintsBefore;
						oldDistanceFromOptimum += (loss - modelScoreDiff);
					}
				} else if (m_onlyViolatedConstraints) {
					// constraint not violated
					addConstraint = false;
				}

				if (addConstraint) {
					float lossMarginDistance = loss - modelScoreDiff;

					if (m_accumulateMostViolatedConstraints
					    && !m_pastAndCurrentConstraints) {
						if (lossMarginDistance > maxViolationLossMarginDistance) {
							maxViolationLossMarginDistance = lossMarginDistance;
							maxViolationfeatureValueDiff = featureValueDiff;
						}
					} else if (m_pastAndCurrentConstraints) {
						if (lossMarginDistance > maxViolationLossMarginDistance) {
							maxViolationLossMarginDistance = lossMarginDistance;
							maxViolationfeatureValueDiff = featureValueDiff;
						}

						featureValueDiffs.push_back(featureValueDiff);
						lossMinusModelScoreDiff.push_back(lossMarginDistance);
					} else {
						// Objective: 1/2 * ||w' - w||^2 + C * SUM_1_m[ max_1_n (l_ij - Delta_h_ij.w')]
						// To add a constraint for the optimiser for each sentence i and hypothesis j, we need:
						// 1. vector Delta_h_ij of the feature value differences (oracle - hypothesis)
						// 2. loss_ij - difference in model scores (Delta_h_ij.w') (oracle - hypothesis)
						featureValueDiffs.push_back(featureValueDiff);
//						cerr << "feature value diff (A): " << featureValueDiff << endl;
						lossMinusModelScoreDiff.push_back(lossMarginDistance);
//						cerr << "loss - model score diff (b): " << lossMarginDistance << endl << endl;
					}
				}
			}
		}
	}

	if (m_max_number_oracles == 1) {
		for (size_t k = 0; k < sentenceIds.size(); ++k) {
			size_t sentenceId = sentenceIds[k];
			m_oracles[sentenceId].clear();
		}
	}

	if (featureValueDiffs.size() != 30) {
		cerr << "Rank " << rank << ", number of constraints passed to optimiser: "
		    << featureValueDiffs.size() << endl;
	}

	// run optimisation: compute alphas for all given constraints
	vector<float> alphas;
	ScoreComponentCollection summedUpdate;
	if (m_accumulateMostViolatedConstraints && !m_pastAndCurrentConstraints) {
		m_featureValueDiffs.push_back(maxViolationfeatureValueDiff);
		m_lossMarginDistances.push_back(maxViolationLossMarginDistance);

		if (m_slack != 0) {
			alphas = Hildreth::optimise(m_featureValueDiffs, m_lossMarginDistances, m_slack);
		} else {
			alphas = Hildreth::optimise(m_featureValueDiffs, m_lossMarginDistances);
		}

		// Update the weight vector according to the alphas and the feature value differences
		// * w' = w' + delta * Dh_ij ---> w' = w' + delta * (h(e*) - h(e_ij))
		for (size_t k = 0; k < m_featureValueDiffs.size(); ++k) {
			float alpha = alphas[k];
			m_featureValueDiffs[k].MultiplyEquals(alpha);

			// sum up update
			summedUpdate.PlusEquals(m_featureValueDiffs[k]);
		}
	} else if (violatedConstraintsBefore > 0) {
		if (m_pastAndCurrentConstraints) {
			// add all (most violated) past constraints to the list of current constraints
			for (size_t i = 0; i < m_featureValueDiffs.size(); ++i) {
				featureValueDiffs.push_back(m_featureValueDiffs[i]);
				lossMinusModelScoreDiff.push_back(m_lossMarginDistances[i]);
			}

			// add new most violated constraint to list
			m_featureValueDiffs.push_back(maxViolationfeatureValueDiff);
			m_lossMarginDistances.push_back(maxViolationLossMarginDistance);
		}

		if (m_slack != 0) {
			alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiff, m_slack);
		} else {
			alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiff);
		}

//		for (size_t i=0; i < alphas.size(); ++i) {
//			cerr << "alpha: " << alphas[i] << endl;
//		}

		// Update the weight vector according to the alphas and the feature value differences
		// * w' = w' + SUM alpha_i * (h_i(oracle) - h_i(hypothesis))
		for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
			float alpha = alphas[k];
			featureValueDiffs[k].MultiplyEquals(alpha);

			// sum up update
			summedUpdate.PlusEquals(featureValueDiffs[k]);
		}
	} else {
		cerr << "Rank " << rank << ", no constraint violated for this batch" << endl;
		return 1;
	}

	// sanity check: still violated constraints after optimisation?
	ScoreComponentCollection newWeights(currWeights);
	newWeights.PlusEquals(summedUpdate);
	if (updates_per_epoch > 0) {
		newWeights.PlusEquals(m_accumulatedUpdates);
	}
	int violatedConstraintsAfter = 0;
	float newDistanceFromOptimum = 0;
	for (size_t i = 0; i < featureValues.size(); ++i) {
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			ScoreComponentCollection featureValueDiff = oracleFeatureValues[i];
			featureValueDiff.MinusEquals(featureValues[i][j]);
			float modelScoreDiff = featureValueDiff.InnerProduct(newWeights);
			float loss = losses[i][j] * m_marginScaleFactor;
//				cerr << "Rank " << rank  << ", new model score diff: " << modelScoreDiff << ", loss: " << loss << endl;
			if (modelScoreDiff < loss) {
				float diff = loss - modelScoreDiff;
				// approximate comparison between floats!
				if (diff > epsilon) {
					++violatedConstraintsAfter;
					newDistanceFromOptimum += (loss - modelScoreDiff);
				}
			}
		}
	}
	cerr << "Rank " << rank << ", violated constraint before: " << violatedConstraintsBefore << ", after: " << violatedConstraintsAfter  << endl;
	if (violatedConstraintsAfter > 0) {
		float distanceChange = oldDistanceFromOptimum - newDistanceFromOptimum;
		cerr << "Rank " << rank << ", there are still violated constraints, the distance change is: " << distanceChange << endl;
		if (controlUpdates && (violatedConstraintsBefore - violatedConstraintsAfter) < 0 && distanceChange < 0) {
			return -1;
		}
	}

	// apply learning rate (fixed or flexible)
	if (learning_rate != 1) {
		cerr << "Rank " << rank << ", update before applying learning rate: " << summedUpdate << endl;
		summedUpdate.MultiplyEquals(learning_rate);
		cerr << "Rank " << rank << ", update after applying learning rate: " << summedUpdate << endl;
	}

	// apply threshold scaling
	if (max_sentence_update != -1) {
		cerr << "Rank " << rank << ", update before scaling to max-sentence-update: " << summedUpdate << endl;
		summedUpdate.ThresholdScaling(max_sentence_update);
		cerr << "Rank " << rank << ", update after scaling to max-sentence-update: " << summedUpdate << endl;
	}

	// Apply update to weight vector or store it for later
	if (updates_per_epoch > 0) {
		m_accumulatedUpdates.PlusEquals(summedUpdate);
		cerr << "Rank " << rank << ", new accumulated updates:" << m_accumulatedUpdates << endl;
	} else {
		// apply update to weight vector
		cerr << "Rank " << rank << ", weights before update: " << currWeights << endl;
		currWeights.PlusEquals(summedUpdate);
		cerr << "Rank " << rank << ", weights after update: " << currWeights << endl;
	}

	return 0;
}

}
