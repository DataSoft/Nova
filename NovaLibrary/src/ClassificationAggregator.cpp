#include "ClassificationEngineFactory.h"
#include "ClassificationAggregator.h"
#include "Config.h"
#include "Logger.h"
#include "Lock.h"

#include <stdlib.h>


using namespace std;



namespace Nova
{

ClassificationAggregator::ClassificationAggregator()
{
	pthread_mutex_init(&lock, NULL);
	LoadConfiguration("");
}

ClassificationAggregator::~ClassificationAggregator()
{
	Lock(&this->lock);
	for (uint i = 0; i < m_engines.size(); i++) {
		delete m_engines[i];
	}

	m_engines.clear();
	m_modes.clear();
	m_engineWeights.clear();
}

void ClassificationAggregator::Reload()
{
	Lock(&this->lock);
	for (uint i = 0; i < m_engines.size(); i++) {
		delete m_engines[i];
	}

	m_engines.clear();
	m_modes.clear();
	m_engineWeights.clear();

	LoadConfiguration("");
}

void ClassificationAggregator::LoadConfiguration(std::string filePath)
{
	vector<string> engines = Config::Inst()->GetClassificationEngines();
	vector<string> configs = Config::Inst()->GetClassifierConfigs();
	m_modes = Config::Inst()->GetClassifierModes();

	if (engines.size() == 0) {
		LOG(CRITICAL, "No classifications engines present! Unable to start.", "");
		exit(EXIT_FAILURE);
	}

	if (engines.size() != configs.size())
	{
		LOG(CRITICAL, "Invalid classification configuration!", "");
		exit(EXIT_FAILURE);
	}

	if (configs.size() != m_modes.size())
	{
		LOG(CRITICAL, "Invalid classification configuration!", "");
		exit(EXIT_FAILURE);
	}

	if (engines.size() != Config::Inst()->GetClassifierWeights().size())
	{
		LOG(CRITICAL, "Invalid classification configuration!", "");
		exit(EXIT_FAILURE);
	}

	// Compute the engine weighting
	double sum = 0;
	for (uint i = 0; i < Config::Inst()->GetClassifierWeights().size(); i++)
	{
		sum += Config::Inst()->GetClassifierWeights()[i];
	}

	for (uint i = 0; i < Config::Inst()->GetClassifierWeights().size(); i++)
	{
		m_engineWeights.push_back(Config::Inst()->GetClassifierWeights()[i]/sum);
	}


	for (uint i = 0; i < engines.size(); i++)
	{
		//cout << "Loading engine " << i << " " << engines[i] << endl;
		ClassificationEngine *engine = MakeEngine(engines[i]);

		if (engine == NULL)
		{
			LOG(CRITICAL, "Unable to create classification engine of type " + engines[i], "");
			exit(EXIT_FAILURE);
		}

		engine->LoadConfiguration(Config::Inst()->GetPathHome() + "/" + configs[i]);

		m_engines.push_back(engine);
	}
}

double ClassificationAggregator::Classify(Suspect *s)
{
	Lock(&this->lock);

	// Clear the classification notes. The child engines will append to this.
	s->m_classificationNotes = "";

	double classification = 0;

	double overrideClassification = -1;
	for (uint i = 0; i < m_engines.size(); i++)
	{
		double engineVote = m_engines.at(i)->Classify(s);

		classification += engineVote * m_engineWeights.at(i);

		if (m_modes[i] == CLASSIFIER_WEIGHTED)
		{
			//
		}
		else if (m_modes[i] == CLASSIFIER_HOSTILE_OVERRIDE)
		{
			if (engineVote > Config::Inst()->GetClassificationThreshold())
			{
				if (overrideClassification == -1)
					overrideClassification = engineVote;
			}
		}
		else if (m_modes[i] == CLASSIFIER_BENIGN_OVERRIDE)
		{
			if (engineVote < Config::Inst()->GetClassificationThreshold())
			{
				if (overrideClassification == -1)
					overrideClassification = engineVote;
			}
		}
	}

	// If there's a hostile or benign override CE triggered, we choose the engine vote of the first one that triggered it
	if (overrideClassification != -1)
		classification = overrideClassification;

    s->SetClassification(classification);


    if (classification > Config::Inst()->GetClassificationThreshold())
    {
    	s->SetIsHostile(true);
    }
    else
    {
    	s->SetIsHostile(false);
    }

	return classification;
}

} /* namespace Nova */
