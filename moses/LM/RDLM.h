#include <string>
#include <map>
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/FFState.h"
#include "moses/FF/InternalTree.h"

#include <boost/thread/tss.hpp>
#include <boost/array.hpp>

// relational dependency language model, described in:
// Sennrich, Rico (2015). Modelling and Optimizing on Syntactic N-Grams for Statistical Machine Translation. Transactions of the Association for Computational Linguistics.
// see 'scripts/training/rdlm' for training scripts

namespace nplm {
  class neuralTM;
}

namespace Moses
{

class RDLMState : public TreeState
{
  float m_approx_head; //score that was approximated due to lack of context
  float m_approx_label;
  size_t m_hash;
public:
  RDLMState(TreePointer tree, float approx_head, float approx_label, size_t hash)
    : TreeState(tree)
    , m_approx_head(approx_head)
    , m_approx_label(approx_label)
    , m_hash(hash)
  {}

  float GetApproximateScoreHead() const {
      return m_approx_head;
  }

  float GetApproximateScoreLabel() const {
      return m_approx_label;
  }

  size_t GetHash() const {
      return m_hash;
  }

  int Compare(const FFState& other) const {
      if (m_hash == static_cast<const RDLMState*>(&other)->GetHash()) return 0;
      else if (m_hash > static_cast<const RDLMState*>(&other)->GetHash()) return 1;
      else return -1;
  }
};

class RDLM : public StatefulFeatureFunction
{
  typedef std::map<InternalTree*,TreePointer> TreePointerMap;

  nplm::neuralTM* lm_head_base_instance_;
  mutable boost::thread_specific_ptr<nplm::neuralTM> lm_head_backend_;

  nplm::neuralTM* lm_label_base_instance_;
  mutable boost::thread_specific_ptr<nplm::neuralTM> lm_label_backend_;

  std::string dummy_head;
  std::string m_glueSymbol;
  std::string m_startSymbol;
  std::string m_endSymbol;
  std::string m_endTag;
  std::string m_path_head_lm;
  std::string m_path_label_lm;
  bool m_isPTKVZ;
  bool m_isPretermBackoff;
  size_t m_context_left;
  size_t m_context_right;
  size_t m_context_up;
  bool m_premultiply;
  bool m_rerank;
  bool m_normalizeHeadLM;
  bool m_normalizeLabelLM;
  bool m_sharedVocab;
  std::string m_debugPath; // score all trees in the provided file, then exit
  int m_binarized;
  int m_cacheSize;

  size_t offset_up_head;
  size_t offset_up_label;

  size_t size_head;
  size_t size_label;
  std::vector<int> static_label_null;
  std::vector<int> static_head_null;
  int static_dummy_head;
  int static_start_head;
  int static_start_label;
  int static_stop_head;
  int static_stop_label;
  int static_head_head;
  int static_head_label;
  int static_root_head;
  int static_root_label;

  int static_head_label_output;
  int static_stop_label_output;
  int static_start_label_output;

public:
  RDLM(const std::string &line)
    : StatefulFeatureFunction(2, line)
    , dummy_head("<dummy_head>")
    , m_glueSymbol("Q")
    , m_startSymbol("SSTART")
    , m_endSymbol("SEND")
    , m_endTag("</s>")
    , m_isPTKVZ(false)
    , m_isPretermBackoff(true)
    , m_context_left(3)
    , m_context_right(0)
    , m_context_up(2)
    , m_premultiply(true)
    , m_rerank(false)
    , m_normalizeHeadLM(false)
    , m_normalizeLabelLM(false)
    , m_sharedVocab(false)
    , m_binarized(0)
    , m_cacheSize(1000000)
    {
      ReadParameters();
    }

  ~RDLM();

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new RDLMState(TreePointer(), 0, 0, 0);
  }

  void Score(InternalTree* root, const TreePointerMap & back_pointers, boost::array<float,4> &score, std::vector<int> &ancestor_heads, std::vector<int> &ancestor_labels, size_t &boundary_hash, int num_virtual = 0, int rescoring_levels = 0) const;
  InternalTree* GetHead(InternalTree* root, const TreePointerMap & back_pointers, std::pair<int,int> & IDs, InternalTree * head_ptr=NULL) const;
  void GetChildHeadsAndLabels(InternalTree *root, const TreePointerMap & back_pointers, int reached_end, const nplm::neuralTM *lm_head, const nplm::neuralTM *lm_labels, std::vector<int> & heads, std::vector<int> & labels, std::vector<int> & heads_output, std::vector<int> & labels_output) const;
  void GetIDs(const std::string & head, const std::string & preterminal, std::pair<int,int> & IDs) const;
  void ScoreFile(std::string &path); //for debugging
  void PrintInfo(std::vector<int> &ngram, nplm::neuralTM* lm) const; //for debugging

  TreePointerMap AssociateLeafNTs(InternalTree* root, const std::vector<TreePointer> &previous) const;

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void SetParameter(const std::string& key, const std::string& value);
  void EvaluateInIsolation(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const {};
  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const {};
  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {};
  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const {UTIL_THROW(util::Exception, "Not implemented");};
  FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  void Load();

  // Iterator-class that yields all children of a node; if child is virtual node of binarized tree, its children are yielded instead.
  class UnbinarizedChildren
  {
  private:
      std::vector<TreePointer>::const_iterator iter;
      std::vector<TreePointer>::const_iterator _begin;
      std::vector<TreePointer>::const_iterator _end;
      InternalTree* current;
      const TreePointerMap & back_pointers;
      bool binarized;
      std::vector<std::pair<InternalTree*,std::vector<TreePointer>::const_iterator> > stack;

  public:
      UnbinarizedChildren(InternalTree* root, const TreePointerMap & pointers, bool binary):
        current(root),
        back_pointers(pointers),
        binarized(binary)
        {
          stack.reserve(10);
          _end = current->GetChildren().end();
          iter = current->GetChildren().begin();
          // expand virtual node
          while (binarized && !(*iter)->GetLabel().empty() && (*iter)->GetLabel()[0] == '^') {
            stack.push_back(std::make_pair(current, iter));
            // also go through trees or previous hypotheses to rescore nodes for which more context has become available
            if ((*iter)->IsLeafNT()) {
              current = back_pointers.find(iter->get())->second.get();
            }
            else {
              current = iter->get();
            }
            iter = current->GetChildren().begin();
          }
          _begin = iter;
        }

      std::vector<TreePointer>::const_iterator begin() const { return _begin; }
      std::vector<TreePointer>::const_iterator end() const { return _end; }

      std::vector<TreePointer>::const_iterator operator++() {
        iter++;
        if (iter == current->GetChildren().end()) {
          while (!stack.empty()) {
            std::pair<InternalTree*,std::vector<TreePointer>::const_iterator> & active = stack.back();
            current = active.first;
            iter = ++active.second;
            stack.pop_back();
            if (iter != current->GetChildren().end()) {
              break;
            }
          }
          if (iter == _end) {
            return iter;
          }
        }
        // expand virtual node
        while (binarized && !(*iter)->GetLabel().empty() && (*iter)->GetLabel()[0] == '^') {
          stack.push_back(std::make_pair(current, iter));
          // also go through trees or previous hypotheses to rescore nodes for which more context has become available
          if ((*iter)->IsLeafNT()) {
            current = back_pointers.find(iter->get())->second.get();
          }
          else {
            current = iter->get();
          }
          iter = current->GetChildren().begin();
        }
        return iter;
      }
  };

};

}
