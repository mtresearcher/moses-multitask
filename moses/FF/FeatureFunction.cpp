#include <stdexcept>

#include "util/exception.hh"

#include "FeatureFunction.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/TranslationOption.h"
#include "moses/Util.h"
#include "moses/FF/DistortionScoreProducer.h"

#include <boost/foreach.hpp>

using namespace std;

namespace Moses
{

multiset<string> FeatureFunction::description_counts;

std::vector<FeatureFunction*> FeatureFunction::s_staticColl;

FeatureFunction &FeatureFunction::FindFeatureFunction(const std::string& name)
{
  for (size_t i = 0; i < s_staticColl.size(); ++i) {
    FeatureFunction &ff = *s_staticColl[i];
    if (ff.GetScoreProducerDescription() == name) {
      return ff;
    }
  }

  throw "Unknown feature " + name;
}

void FeatureFunction::Destroy()
{
  RemoveAllInColl(s_staticColl);
}

// The original declaration as 
// void FeatureFunction::CallChangeSource(InputType *&input)
// had me a bit perplexed. Would you really want to allow 
// any feature function to replace the InputType behind the 
// back of the others? And change what the vector is pointing to?

void FeatureFunction::CallChangeSource(InputType * const&input)
{
  for (size_t i = 0; i < s_staticColl.size(); ++i) {
    const FeatureFunction &ff = *s_staticColl[i];
    ff.ChangeSource(input);
  }
}

void FeatureFunction::SetupAll(TranslationTask const& ttask)
{
  BOOST_FOREACH(FeatureFunction* ff, s_staticColl)
    ff->Setup(ttask);
}

FeatureFunction::
FeatureFunction(const std::string& line)
  : m_tuneable(true)
  , m_requireSortingAfterSourceContext(false)
  , m_verbosity(std::numeric_limits<std::size_t>::max())
  , m_numScoreComponents(1)
{
  m_numTuneableComponents = m_numScoreComponents;
  Initialize(line);
}

FeatureFunction::
FeatureFunction(size_t numScoreComponents,
                const std::string& line)
  : m_tuneable(true)
  , m_requireSortingAfterSourceContext(false)
  , m_verbosity(std::numeric_limits<std::size_t>::max())
  , m_numScoreComponents(numScoreComponents)
{
  m_numTuneableComponents = m_numScoreComponents;
  Initialize(line);
}

void
FeatureFunction::
Initialize(const std::string &line)
{
  ParseLine(line);

  ScoreComponentCollection::RegisterScoreProducer(this);
  s_staticColl.push_back(this);
}

FeatureFunction::~FeatureFunction() {}

void FeatureFunction::ParseLine(const std::string &line)
{
  vector<string> toks = Tokenize(line);
  UTIL_THROW_IF2(toks.empty(), "Empty line");

  string nameStub = toks[0];

  set<string> keys;

  for (size_t i = 1; i < toks.size(); ++i) {
    vector<string> args = TokenizeFirstOnly(toks[i], "=");
    UTIL_THROW_IF2(args.size() != 2,
                   "Incorrect format for feature function arg: " << toks[i]);

    pair<set<string>::iterator,bool> ret = keys.insert(args[0]);
    UTIL_THROW_IF2(!ret.second, "Duplicate key in line " << line);

    if (args[0] == "num-features") {
      m_numScoreComponents = Scan<size_t>(args[1]);
      m_numTuneableComponents = m_numScoreComponents;
    } else if (args[0] == "name") {
      m_description = args[1];
    } else {
      m_args.push_back(args);
    }
  }

  // name
  if (m_description == "") {
    size_t index = description_counts.count(nameStub);

    ostringstream dstream;
    dstream << nameStub;
    dstream << index;

    description_counts.insert(nameStub);
    m_description = dstream.str();
  }

}

void FeatureFunction::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "tuneable") {
    m_tuneable = Scan<bool>(value);
  } else if (key == "tuneable-components") {
    UTIL_THROW_IF2(!m_tuneable, GetScoreProducerDescription()
                   << ": tuneable-components cannot be set if tuneable=false");
    SetTuneableComponents(value);
  } else if (key == "require-sorting-after-source-context") {
    m_requireSortingAfterSourceContext = Scan<bool>(value);
  } else if (key == "verbosity") {
    m_verbosity = Scan<size_t>(value);
  } else if (key == "filterable") { //ignore
  } else {
    UTIL_THROW2(GetScoreProducerDescription() << ": Unknown argument " << key << "=" << value);
  }
}

void FeatureFunction::ReadParameters()
{
  while (!m_args.empty()) {
    const vector<string> &args = m_args[0];
    SetParameter(args[0], args[1]);

    m_args.erase(m_args.begin());
  }
}

std::vector<float> FeatureFunction::DefaultWeights() const
{
  UTIL_THROW2(GetScoreProducerDescription() << ": No default weights");
}

void FeatureFunction::SetTuneableComponents(const std::string& value)
{
  std::vector<std::string> toks = Tokenize(value,",");
  UTIL_THROW_IF2(toks.empty(), GetScoreProducerDescription()
                 << ": Empty tuneable-components");
  UTIL_THROW_IF2(toks.size()!=m_numScoreComponents, GetScoreProducerDescription()
                 << ": tuneable-components value has to be a comma-separated list of "
                 << m_numScoreComponents << " boolean values");

  m_tuneableComponents.resize(m_numScoreComponents);
  m_numTuneableComponents = m_numScoreComponents;

  for (size_t i = 0; i < toks.size(); ++i) {
    m_tuneableComponents[i] = Scan<bool>(toks[i]);
    if (!m_tuneableComponents[i]) {
      --m_numTuneableComponents;
    }
  }
}

}

