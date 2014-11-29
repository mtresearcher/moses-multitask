#include <vector>
#include "BilingualLM.h"
#include "moses/ScoreComponentCollection.h"

using namespace std;

namespace Moses {

int BilingualLMState::Compare(const FFState& other) const
{
  const BilingualLMState &otherState = static_cast<const BilingualLMState&>(other);

  if (m_hash == otherState.m_hash)
    return 0;
  return (m_hash < otherState.m_hash) ? -1 : +1;
}

////////////////////////////////////////////////////////////////
BilingualLM::BilingualLM(const std::string &line)
    : StatefulFeatureFunction(1, line),
      word_factortype(0) {
  FactorCollection& factorFactory = FactorCollection::Instance(); //Factor Factory to use for BOS_ and EOS_
  BOS_factor = factorFactory.AddFactor(BOS_);
  BOS_word.SetFactor(0, BOS_factor);
  EOS_factor = factorFactory.AddFactor(EOS_);
  EOS_word.SetFactor(0, EOS_factor);
  
}

void BilingualLM::Load(){
  ReadParameters();
  loadModel();
}

//Populates words with amount words from the targetPhrase from the previous hypothesis where
//words[0] is the last word of the previous hypothesis, words[1] is the second last etc...
void BilingualLM::requestPrevTargetNgrams(
    const Hypothesis &cur_hypo, int amount, std::vector<int> &words) const {
  const Hypothesis * prev_hyp = cur_hypo.GetPrevHypo();
  int found = 0;

  while (prev_hyp && found != amount) {
    const TargetPhrase& currTargetPhrase = prev_hyp->GetCurrTargetPhrase();
    for (int i = currTargetPhrase.GetSize() - 1; i> -1; i--){
      if (found != amount){
        const Word& word = currTargetPhrase.GetWord(i);
        words[found] = getNeuralLMId(word, false);
        found++;
      } else {
        return; //We have gotten everything needed
      }
    }

    prev_hyp = prev_hyp->GetPrevHypo();
  }

  int neuralLM_wordID = getNeuralLMId(BOS_word, false);
  for (int i = found; i < amount; i++){
    words[i] = neuralLM_wordID;
  }
}

//Populates the words vector with target_ngrams sized that also contains the current word we are looking at. 
//(in effect target_ngrams + 1)
void BilingualLM::getTargetWords(
    const Hypothesis &cur_hypo,
    const TargetPhrase &targetPhrase,
    int current_word_index,
    std::vector<int> &words) const {
  //Check if we need to look at previous target phrases
  int additional_needed = current_word_index - target_ngrams;
  if (additional_needed < 0) {
    additional_needed = -additional_needed;
    std::vector<int> prev_words(additional_needed);
    requestPrevTargetNgrams(cur_hypo, additional_needed, prev_words);
    for (int i = additional_needed - 1; i >= 0; i--) {
      words.push_back(prev_words[i]);
    }
  }

  if (words.size() > 0) {
    //We have added some words from previous phrases
    //Just add until we reach current_word_index
    for (int i = 0; i <= current_word_index; i++) {
      const Word& word = targetPhrase.GetWord(i);
      words.push_back(getNeuralLMId(word, false));
    }
  } else {
    //We haven't added any words, proceed as before
    for (int i = current_word_index - target_ngrams; i <= current_word_index; i++){
      const Word& word = targetPhrase.GetWord(i);
      words.push_back(getNeuralLMId(word, false));
    }
  }
}

//Returns source words in the way NeuralLM expects them.

size_t BilingualLM::selectMiddleAlignment(
    const set<size_t>& alignment_links) const {

  set<size_t>::iterator it = alignment_links.begin();
  for (int i = 0; i < (alignment_links.size() - 1) / 2; ++i) {
    ++it;
  }

  return *it;
}

void BilingualLM::getSourceWords(
    const TargetPhrase &targetPhrase,
    int targetWordIdx,
    const Sentence &source_sent,
    const WordsRange &sourceWordRange,
    std::vector<int> &words) const {
  //Get source context

  //Get alignment for the word we require
  const AlignmentInfo& alignments = targetPhrase.GetAlignTerm();

  // We are getting word alignment for targetPhrase.GetWord(i + target_ngrams -1) according to the paper.
  // Find the closest target word with alignment links.
  std::set<size_t> last_word_al;
  for (int j = 0; j < targetPhrase.GetSize(); j++) {
    // Find the nearest aligned word with preference for right.
    if ((targetWordIdx + j) < targetPhrase.GetSize()){
      last_word_al = alignments.GetAlignmentsForTarget(targetWordIdx + j);
      if (!last_word_al.empty()) {
        break;
      }
    }

    // We couldn't find word on the right, try to the left.
    if ((targetWordIdx - j) >= 0) {
      last_word_al = alignments.GetAlignmentsForTarget(targetWordIdx - j);
      if (!last_word_al.empty()) {
        break;
      }
    }
  }

  //Assume we have gotten some alignment here. If we couldn't get an alignment from the above routine it means
  //that none of the words in the target phrase aligned to any word in the source phrase

  // Now we get the source words. First select middle alignment.
  //It should never be the case the the word_al size would be zero, but several times this has happened because
  //of a corrupt phrase table. It is best to have this check here, as it makes debugging the problem a lot easier.
  UTIL_THROW_IF2(last_word_al.size() == 0,
  "A target phrase with no alignments detected! " << targetPhrase << "Check if there is something wrong with your phrase table.");
  size_t source_center_index = selectMiddleAlignment(last_word_al);
  // We have found the alignment. Now determine how much to shift by to get the actual source word index.
  size_t phrase_start_pos = sourceWordRange.GetStartPos();
  // Account for how far the current word is from the start of the phrase.
  size_t source_word_mid_idx = phrase_start_pos + source_center_index;

  appendSourceWordsToVector(source_sent, words, source_word_mid_idx);
}

size_t BilingualLM::getState(const Hypothesis& cur_hypo) const {
  const TargetPhrase &targetPhrase = cur_hypo.GetCurrTargetPhrase();
  size_t hashCode = 0;

  // Check if we need to look at previous target phrases
  int additional_needed = targetPhrase.GetSize() - target_ngrams;
  if (additional_needed < 0) {
    additional_needed = -additional_needed;
    std::vector<int> prev_words(additional_needed);
    requestPrevTargetNgrams(cur_hypo, additional_needed, prev_words);
    for (int i = additional_needed - 1; i >= 0; i--) {
      boost::hash_combine(hashCode, prev_words[i]);
    }

    // Get the rest of the phrases needed
    for (int i = 0; i < targetPhrase.GetSize(); i++) {
      const Word& word = targetPhrase.GetWord(i);
      int neuralLM_wordID = getNeuralLMId(word, false);
      boost::hash_combine(hashCode, neuralLM_wordID);
    }
  } else {
    // We just need the last target_ngrams from the current target phrase.
    for (int i = targetPhrase.GetSize() - target_ngrams; i < targetPhrase.GetSize(); i++) {
      const Word& word = targetPhrase.GetWord(i);
      int neuralLM_wordID = getNeuralLMId(word, false);

      boost::hash_combine(hashCode, neuralLM_wordID);
    }
  }

  return hashCode;
}

void BilingualLM::EvaluateInIsolation(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const {}

void BilingualLM::EvaluateWithSourceContext(const InputType &input
                                  , const InputPath &inputPath
                                  , const TargetPhrase &targetPhrase
                                  , const StackVec *stackVec
                                  , ScoreComponentCollection &scoreBreakdown
                                  , ScoreComponentCollection *estimatedFutureScore) const
{

}


FFState* BilingualLM::EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const {
  Manager& manager = cur_hypo.GetManager();
  const Sentence& source_sent = static_cast<const Sentence&>(manager.GetSource());

  // Init vectors.
  std::vector<int> source_words;
  source_words.reserve(source_ngrams);
  std::vector<int> target_words;
  target_words.reserve(target_ngrams);

  float value = 0;
  const TargetPhrase& currTargetPhrase = cur_hypo.GetCurrTargetPhrase();
  const WordsRange& sourceWordRange = cur_hypo.GetCurrSourceWordsRange(); //Source words range to calculate offsets

  // For each word in the current target phrase get its LM score.
  for (int i = 0; i < currTargetPhrase.GetSize(); i++){
    getSourceWords(
        currTargetPhrase, i, source_sent, sourceWordRange, source_words);
    getTargetWords(cur_hypo, currTargetPhrase, i, target_words);
    value += Score(source_words, target_words);

    // Clear the vectors.
    source_words.clear();
    target_words.clear();
  }

  size_t new_state = getState(cur_hypo); 
  accumulator->PlusEquals(this, value);

  return new BilingualLMState(new_state);
}

void BilingualLM::getAllTargetIdsChart(const ChartHypothesis& cur_hypo, size_t featureID, std::vector<int>& wordIds) const {
  const TargetPhrase targetPhrase = cur_hypo.GetCurrTargetPhrase();
  int next_nonterminal_index = 0;

  for (int i = 0; i < targetPhrase.GetSize(); i++){
    if (targetPhrase.GetWord(i).IsNonTerminal()){ //Nonterminal get from prev state
      const ChartHypothesis * prev_hypo = cur_hypo.GetPrevHypo(next_nonterminal_index);
      const BilingualLMState * prev_state = static_cast<const BilingualLMState *>(prev_hypo->GetFFState(featureID));
      const std::vector<int> prevWordIDs = prev_state->GetWordIdsVector();
      for (std::vector<int>::const_iterator it = prevWordIDs.begin(); it!= prevWordIDs.end(); it++){
        wordIds.push_back(*it);
      }
      next_nonterminal_index++;
    } else {
      wordIds.push_back(getNeuralLMId(targetPhrase.GetWord(i), false));
    }
  }
} 

void BilingualLM::getAllAlignments(const ChartHypothesis& cur_hypo, size_t featureID, std::vector<int>& word_alignemnts) const {
  const TargetPhrase targetPhrase = cur_hypo.GetCurrTargetPhrase();
  int next_nonterminal_index = 0;
  int nonterm_length = 0; //Account for the size of nonterminals when calculating the alignment.
  int source_phrase_start_pos = cur_hypo.GetCurrSourceRange().GetStartPos();
  int source_word_mid_idx; //The word alignment

  //Get source sent
  const ChartManager& manager = cur_hypo.GetManager();
  const Sentence& source_sent = static_cast<const Sentence&>(manager.GetSource());
  const AlignmentInfo& alignments = targetPhrase.GetAlignTerm();

  for (int i = 0; i < targetPhrase.GetSize(); i++){
    //Sometimes we have to traverse more than one target words because of
    //unaligned words. This is O(n^2) in worst case, but usually closer to O(n)
    if (targetPhrase.GetWord(i).IsNonTerminal()){
      //If we have a non terminal we can get the alignments from the previous state
      const ChartHypothesis * prev_hypo = cur_hypo.GetPrevHypo(next_nonterminal_index);
      const BilingualLMState * prev_state = static_cast<const BilingualLMState *>(prev_hypo->GetFFState(featureID));
      const std::vector<int> prevWordAls = prev_state->GetWordAlignmentVector();
      nonterm_length += prevWordAls.size();
      for (std::vector<int>::const_iterator it = prevWordAls.begin(); it!= prevWordAls.end(); it++){
        word_alignemnts.push_back(*it);
      }
      next_nonterminal_index++;
    } else {
      std::set<size_t> word_al; //Keep word alignments
      bool resolvedIndexis = false; //If we are aligning to an existing nonterm we don't need to calculate offsets
      for (int j = 0; j < targetPhrase.GetSize(); j++){
        //Try to get alignment from the current word and if it is unaligned,
        //try from the first word to the right and then to the left
        if ((i+j) < targetPhrase.GetSize()) {
          if (targetPhrase.GetWord(i + j).IsNonTerminal()) {
            const ChartHypothesis * prev_hypo = cur_hypo.GetPrevHypo(next_nonterminal_index);
            const BilingualLMState * prev_state = static_cast<const BilingualLMState *>(prev_hypo->GetFFState(featureID));
            const std::vector<int>& word_alignments = prev_state->GetWordAlignmentVector();
            source_word_mid_idx = word_alignments.front(); // The first word on the right of our word
            resolvedIndexis = true;
            break;
          }
          word_al = alignments.GetAlignmentsForTarget(i + j);
          if (!word_al.empty()) {
            break;
          }
        }

        if ((i - j) >= 0) {
          if (targetPhrase.GetWord(i - j).IsNonTerminal()) {
            const ChartHypothesis * prev_hypo = cur_hypo.GetPrevHypo(next_nonterminal_index - 1); //We need to look at the nonterm on the left.
            const BilingualLMState * prev_state = static_cast<const BilingualLMState *>(prev_hypo->GetFFState(featureID));
            const std::vector<int>& word_alignments = prev_state->GetWordAlignmentVector();
            source_word_mid_idx = word_alignments.back(); // The first word on the left of our word
            resolvedIndexis = true;
            break;
          }

          word_al = alignments.GetAlignmentsForTarget(i - j);
          if (!word_al.empty()) {
            break;
          }
        }
      }

      if (!resolvedIndexis){
        //It should never be the case the the word_al size would be zero, but several times this has happened because
        //of a corrupt phrase table. It is best to have this check here, as it makes debugging the problem a lot easier.
        UTIL_THROW_IF2(word_al.size() == 0,
        "A target phrase with no alignments detected! " << targetPhrase << "Check if there is something wrong with your phrase table.");
        size_t source_center_index = selectMiddleAlignment(word_al);
        // We have found the alignment. Now determine how much to shift by to get the actual source word index.
        source_word_mid_idx = source_phrase_start_pos + (int)source_center_index + nonterm_length;
      }
      word_alignemnts.push_back(source_word_mid_idx);
    }
  }

}

size_t BilingualLM::getStateChart(std::vector<int>& neuralLMids) const {
  size_t hashCode = 0;
  for (int i = neuralLMids.size() - target_ngrams; i < neuralLMids.size(); i++){
    int neuralLM_wordID;
    if (i < 0) {
      neuralLM_wordID = getNeuralLMId(BOS_word, false);
    } else {
      neuralLM_wordID = neuralLMids[i];
    }
    boost::hash_combine(hashCode, neuralLM_wordID);
  }
  return hashCode;
}

void BilingualLM::getTargetWordsChart(
    std::vector<int>& neuralLMids,
    int current_word_index,
    std::vector<int>& words,
    bool sentence_begin) const {

  for (int i = current_word_index - target_ngrams; i <= current_word_index; i++) {
    if (i < 0) {
      if (sentence_begin) {
        words.push_back(getNeuralLMId(BOS_word, false));
      } else {
        words.push_back(getNeuralLMId(getNullWord(), false));
      }
    } else {
      words.push_back(neuralLMids[i]);
    }
  }
}

void BilingualLM::appendSourceWordsToVector(const Sentence &source_sent, std::vector<int> &words, int source_word_mid_idx) const {
  //Define begin and end indexes of the lookup. Cases for even and odd ngrams
  //This can result in indexes which span larger than the length of the source phrase.
  //In this case we just
  int begin_idx;
  int end_idx;

  if (source_ngrams % 2 == 0) {
    begin_idx = source_word_mid_idx - source_ngrams / 2 + 1;
    end_idx = source_word_mid_idx + source_ngrams / 2;
  } else {
    begin_idx = source_word_mid_idx - (source_ngrams - 1) / 2;
    end_idx = source_word_mid_idx + (source_ngrams - 1) / 2;
  }

  //Add words to vector
  for (int j = begin_idx; j <= end_idx; j++) {
    int neuralLM_wordID;
    if (j < 0) {
      neuralLM_wordID = getNeuralLMId(BOS_word, true);
    } else if (j >= source_sent.GetSize()) {
      neuralLM_wordID = getNeuralLMId(EOS_word, true);
    } else {
      const Word& word = source_sent.GetWord(j);
      neuralLM_wordID = getNeuralLMId(word, true);
    }
    words.push_back(neuralLM_wordID);
  }
}

FFState* BilingualLM::EvaluateWhenApplied(
    const ChartHypothesis& cur_hypo,
    int featureID, /* - used to index the state in the previous hypotheses */
    ScoreComponentCollection* accumulator) const {
  //Init vectors
  std::vector<int> source_words;
  source_words.reserve(source_ngrams);
  std::vector<int> target_words;
  target_words.reserve(target_ngrams);

  float value = 0; //NeuralLM score
  const TargetPhrase& currTargetPhrase = cur_hypo.GetCurrTargetPhrase();

  std::vector<int> neuralLMids; //Equivalent more or less to whole_phrase. Contains all word ids but not as expensive
  std::vector<int> alignments;
  //Estimate size and reserve vectors to avoid reallocation
  int future_size = currTargetPhrase.GetNumTerminals();
  for (int i =0; i<currTargetPhrase.GetNumNonTerminals(); i++){
    const ChartHypothesis * prev_hypo = cur_hypo.GetPrevHypo(i); //We need to look at the nonterm on the left.
    const BilingualLMState * prev_state = static_cast<const BilingualLMState *>(prev_hypo->GetFFState(featureID));
    const std::vector<int>& wordIds = prev_state->GetWordIdsVector();
    future_size += wordIds.size();
  }
  neuralLMids.reserve(future_size);
  neuralLMids.reserve(future_size);

  getAllTargetIdsChart(cur_hypo, featureID, neuralLMids);
  getAllAlignments(cur_hypo, featureID, alignments);

  bool sentence_begin = false; //Check if this hypothesis' target words are located in the beginning of the sentence
  if (neuralLMids[0] == getNeuralLMId(BOS_word, true)){
    sentence_begin = true;
  }
  
  //Get source sentence
  const ChartManager& manager = cur_hypo.GetManager();
  const Sentence& source_sent = static_cast<const Sentence&>(manager.GetSource());

  for (int i = 0; i < neuralLMids.size(); i++) { //This loop should be bigger as non terminals expand

    //We already have resolved the nonterminals, we are left with a simple loop.
    appendSourceWordsToVector(source_sent, source_words, alignments[i]);
    getTargetWordsChart(neuralLMids, i, target_words, sentence_begin);

    value += Score(source_words, target_words); // Get the score

    //Clear the vectors before the next iteration
    source_words.clear();
    target_words.clear();

  }
  size_t new_state = getStateChart(neuralLMids);

  accumulator->Assign(this, value);

  return new BilingualLMState(new_state, alignments, neuralLMids);
}

void BilingualLM::SetParameter(const std::string& key, const std::string& value) {
  if (key == "filepath") {
    m_filePath = value;
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

} // namespace Moses

