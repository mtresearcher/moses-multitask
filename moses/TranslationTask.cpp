#include "TranslationTask.h"
#include "moses/StaticData.h"
#include "moses/Sentence.h"
#include "moses/IOWrapper.h"
#include "moses/TranslationAnalysis.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/InputType.h"
#include "moses/OutputCollector.h"
#include "moses/Incremental.h"
#include "mbr.h"
#include "moses/FF/OnlineLearningFeature.h"

#include "moses/Syntax/S2T/Parsers/RecursiveCYKPlusParser/RecursiveCYKPlusParser.h"
#include "moses/Syntax/S2T/Parsers/Scope3Parser/Parser.h"

#include "util/exception.hh"

using namespace std;

namespace Moses
{

TranslationTask::TranslationTask(InputType* source, Moses::IOWrapper &ioWrapper, int pbOrChart)
: m_source(source)
, m_ioWrapper(ioWrapper)
, m_pbOrChart(pbOrChart)
{}

TranslationTask::~TranslationTask() {
  delete m_source;
}

void TranslationTask::Run()
{
	switch (m_pbOrChart)
	{
	case 1:
		RunPb();
		break;
	case 2:
		RunChart();
		break;
	default:
	    UTIL_THROW(util::Exception, "Unknown value: " << m_pbOrChart);
	}
}


void TranslationTask::RunPb()
{
  // shorthand for "global data"
  const StaticData &staticData = StaticData::Instance();

  // input sentence
  Sentence sentence;

  // report wall time spent on translation
  Timer translationTime;
  translationTime.start();

  // report thread number
#if defined(WITH_THREADS) && defined(BOOST_HAS_PTHREADS)
  TRACE_ERR("Translating line " << m_source->GetTranslationId() << "  in thread id " << pthread_self() << endl);
#endif


  // execute the translation
  // note: this executes the search, resulting in a search graph
  //       we still need to apply the decision rule (MAP, MBR, ...)
  Timer initTime;
  initTime.start();
  Manager manager(*m_source,staticData.GetSearchAlgorithm());
  VERBOSE(1, "Line " << m_source->GetTranslationId() << ": Initialize search took " << initTime << " seconds total" << endl);
  manager.Decode();
  const OnlineLearningFeature *ol = &OnlineLearningFeature::Instance();
  if(ol != NULL && OnlineLearningFeature::Instance().OnlineLearningActivated()){
	OnlineLearningFeature::InstanceNonConst().RunOnlineLearning(manager);
	OnlineLearningFeature::InstanceNonConst().RemoveJunk();
  }

  // we are done with search, let's look what we got
  Timer additionalReportingTime;
  additionalReportingTime.start();

  // output word graph
  manager.OutputWordGraph(m_ioWrapper.GetWordGraphCollector());

  // output search graph
  manager.OutputSearchGraph(m_ioWrapper.GetSearchGraphOutputCollector());

  manager.OutputSearchGraphSLF();

  // Output search graph in hypergraph format for Kenneth Heafield's lazy hypergraph decoder
  manager.OutputSearchGraphHypergraph();

  additionalReportingTime.stop();

  // apply decision rule and output best translation(s)
  if (m_ioWrapper.GetSingleBestOutputCollector()) {
    ostringstream out;
    ostringstream debug;
    FixPrecision(debug,PRECISION);

    // all derivations - send them to debug stream
    if (staticData.PrintAllDerivations()) {
      additionalReportingTime.start();
      manager.PrintAllDerivations(m_source->GetTranslationId(), debug);
      additionalReportingTime.stop();
    }

    Timer decisionRuleTime;
    decisionRuleTime.start();

    // MAP decoding: best hypothesis
    const Hypothesis* bestHypo = NULL;

    if (!staticData.UseMBR() && ol != NULL && !OnlineLearningFeature::Instance().OnlineLearningActivated()) {
      bestHypo = manager.GetBestHypothesis();
      if (bestHypo) {
        if (StaticData::Instance().GetOutputHypoScore()) {
          out << bestHypo->GetTotalScore() << ' ';
        }
        if (staticData.IsPathRecoveryEnabled()) {
          m_ioWrapper.OutputInput(out, bestHypo);
          out << "||| ";
        }

        const PARAM_VEC *params = staticData.GetParameter().GetParam("print-id");
        if (params && params->size() && Scan<bool>(params->at(0)) ) {
          out << m_source->GetTranslationId() << " ";
        }

	  if (staticData.GetReportSegmentation() == 2) {
	    manager.GetOutputLanguageModelOrder(out, bestHypo);
	  }
        m_ioWrapper.OutputBestSurface(
          out,
          bestHypo,
          staticData.GetOutputFactorOrder(),
          staticData.GetReportSegmentation(),
          staticData.GetReportAllFactors());
        if (staticData.PrintAlignmentInfo()) {
          out << "||| ";
          m_ioWrapper.OutputAlignment(out, bestHypo);
        }

        manager.OutputAlignment(m_ioWrapper.GetAlignmentInfoCollector());

        IFVERBOSE(1) {
          debug << "BEST TRANSLATION: " << *bestHypo << endl;
        }
      } else {
        VERBOSE(1, "NO BEST TRANSLATION" << endl);
      }

      out << endl;
    }

    // MBR decoding (n-best MBR, lattice MBR, consensus)
    else {
      // we first need the n-best translations
      size_t nBestSize = staticData.GetMBRSize();
      if (nBestSize <= 0) {
        cerr << "ERROR: negative size for number of MBR candidate translations not allowed (option mbr-size)" << endl;
        exit(1);
      }
      TrellisPathList nBestList;
      manager.CalcNBest(nBestSize, nBestList,true);
      VERBOSE(2,"size of n-best: " << nBestList.GetSize() << " (" << nBestSize << ")" << endl);
      IFVERBOSE(2) {
        PrintUserTime("calculated n-best list for (L)MBR decoding");
      }

      // lattice MBR
      if (staticData.UseLatticeMBR()) {
        if (m_ioWrapper.GetNBestOutputCollector()) {
          //lattice mbr nbest
          vector<LatticeMBRSolution> solutions;
          size_t n  = min(nBestSize, staticData.GetNBestSize());
          getLatticeMBRNBest(manager,nBestList,solutions,n);
          ostringstream out;
          m_ioWrapper.OutputLatticeMBRNBest(out, solutions,m_source->GetTranslationId());
          m_ioWrapper.GetNBestOutputCollector()->Write(m_source->GetTranslationId(), out.str());
        } else {
          //Lattice MBR decoding
          vector<Word> mbrBestHypo = doLatticeMBR(manager,nBestList);
          m_ioWrapper.OutputBestHypo(mbrBestHypo, m_source->GetTranslationId(), staticData.GetReportSegmentation(),
                         staticData.GetReportAllFactors(),out);
          IFVERBOSE(2) {
            PrintUserTime("finished Lattice MBR decoding");
          }
        }
      }

      // consensus decoding
      else if (staticData.UseConsensusDecoding()) {
        const TrellisPath &conBestHypo = doConsensusDecoding(manager,nBestList);
        m_ioWrapper.OutputBestHypo(conBestHypo, m_source->GetTranslationId(),
                       staticData.GetReportSegmentation(),
                       staticData.GetReportAllFactors(),out);
        m_ioWrapper.OutputAlignment(m_ioWrapper.GetAlignmentInfoCollector(), m_source->GetTranslationId(), conBestHypo);
        IFVERBOSE(2) {
          PrintUserTime("finished Consensus decoding");
        }
      }

      // n-best MBR decoding
      else {
        const TrellisPath &mbrBestHypo = doMBR(nBestList);
        m_ioWrapper.OutputBestHypo(mbrBestHypo, m_source->GetTranslationId(),
                       staticData.GetReportSegmentation(),
                       staticData.GetReportAllFactors(),out);
        m_ioWrapper.OutputAlignment(m_ioWrapper.GetAlignmentInfoCollector(), m_source->GetTranslationId(), mbrBestHypo);
        IFVERBOSE(2) {
          PrintUserTime("finished MBR decoding");
        }
      }
    }

    // report best translation to output collector
    if((ol != NULL && !OnlineLearningFeature::Instance().OnlineLearningActivated())
    		|| ol == NULL)
    	m_ioWrapper.GetSingleBestOutputCollector()->Write(m_source->GetTranslationId(),out.str(),debug.str());

    decisionRuleTime.stop();
    VERBOSE(1, "Line " << m_source->GetTranslationId() << ": Decision rule took " << decisionRuleTime << " seconds total" << endl);
  }

  additionalReportingTime.start();

  // output n-best list
  manager.OutputNBest(m_ioWrapper.GetNBestOutputCollector());

  //lattice samples
  manager.OutputLatticeSamples(m_ioWrapper.GetLatticeSamplesCollector());

  // detailed translation reporting
  manager.OutputDetailedTranslationReport(m_ioWrapper.GetDetailedTranslationCollector());

  //list of unknown words
  manager.OutputUnknowns(m_ioWrapper.GetUnknownsCollector());

  // report additional statistics
  manager.CalcDecoderStatistics();
  VERBOSE(1, "Line " << m_source->GetTranslationId() << ": Additional reporting took " << additionalReportingTime << " seconds total" << endl);
  VERBOSE(1, "Line " << m_source->GetTranslationId() << ": Translation took " << translationTime << " seconds total" << endl);
  IFVERBOSE(2) {
    PrintUserTime("Sentence Decoding Time:");
  }
}


void TranslationTask::RunChart()
{
	const StaticData &staticData = StaticData::Instance();
	const size_t translationId = m_source->GetTranslationId();

	VERBOSE(2,"\nTRANSLATING(" << translationId << "): " << *m_source);

    if (staticData.UseS2TDecoder()) {
      S2TParsingAlgorithm algorithm = staticData.GetS2TParsingAlgorithm();
      if (algorithm == RecursiveCYKPlus) {
        typedef Syntax::S2T::EagerParserCallback Callback;
        typedef Syntax::S2T::RecursiveCYKPlusParser<Callback> Parser;
        DecodeS2T<Parser>();
      } else if (algorithm == Scope3) {
        typedef Syntax::S2T::StandardParserCallback Callback;
        typedef Syntax::S2T::Scope3Parser<Callback> Parser;
        DecodeS2T<Parser>();
      } else {
        UTIL_THROW2("ERROR: unhandled S2T parsing algorithm");
      }
      return;
    }

	if (staticData.GetSearchAlgorithm() == ChartIncremental) {
	  Incremental::Manager manager(*m_source);
	  manager.Decode();
	  const std::vector<search::Applied> &nbest = manager.GetNBest();
	  if (!nbest.empty()) {
		m_ioWrapper.OutputBestHypo(nbest[0], translationId);

		manager.OutputDetailedTranslationReport(m_ioWrapper.GetDetailedTranslationCollector());
	    manager.OutputDetailedTreeFragmentsTranslationReport(m_ioWrapper.GetDetailTreeFragmentsOutputCollector());

	  } else {
		m_ioWrapper.OutputBestNone(translationId);
	  }

	  manager.OutputNBest(m_ioWrapper.GetNBestOutputCollector());

	  return;
	}

	ChartManager manager(*m_source);
	manager.Decode();

	UTIL_THROW_IF2(staticData.UseMBR(), "Cannot use MBR");

	// Output search graph in hypergraph format for Kenneth Heafield's lazy hypergraph decoder
	manager.OutputSearchGraphHypergraph();

	// 1-best
	const ChartHypothesis *bestHypo = manager.GetBestHypothesis();
	m_ioWrapper.OutputBestHypo(bestHypo, translationId);
	IFVERBOSE(2) {
	  PrintUserTime("Best Hypothesis Generation Time:");
	}

    manager.OutputAlignment(m_ioWrapper.GetAlignmentInfoCollector());
    manager.OutputDetailedTranslationReport(m_ioWrapper.GetDetailedTranslationCollector());
    manager.OutputDetailedTreeFragmentsTranslationReport(m_ioWrapper.GetDetailTreeFragmentsOutputCollector());
	manager.OutputUnknowns(m_ioWrapper.GetUnknownsCollector());

	// n-best
	manager.OutputNBest(m_ioWrapper.GetNBestOutputCollector());

	manager.OutputSearchGraph(m_ioWrapper.GetSearchGraphOutputCollector());

	IFVERBOSE(2) {
	  PrintUserTime("Sentence Decoding Time:");
	}
	manager.CalcDecoderStatistics();
}

}
