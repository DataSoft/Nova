#include "gtest/gtest.h"

#include "ThresholdTriggerClassification.h"
#include "Config.h"
#include "math.h"

using namespace std;
using namespace Nova;

class ConfigTest : public ::testing::Test {
protected:
	virtual void SetUp() {
		// Let Config call it's own constructor and get an instance
		Config::Inst();
	}

};

// Make sure the instance is real
TEST_F(ConfigTest, test_instanceNotNull)
{
	EXPECT_NE((Config*)0, Config::Inst());
}

TEST_F(ConfigTest, test_ReaderWriter)
{
	Config::Inst();
	EXPECT_TRUE(Config::Inst()->WriteSetting("INTERFACE", "foobar"));
	EXPECT_EQ(Config::Inst()->ReadSetting("INTERFACE"), "foobar");

	EXPECT_TRUE(Config::Inst()->WriteSetting("INTERFACE", "eth0"));
	EXPECT_EQ(Config::Inst()->ReadSetting("INTERFACE"), "eth0");
}

TEST_F(ConfigTest, test_classificationSettings)
{
	EXPECT_TRUE(Config::Inst()->WriteSetting("CLASSIFICATION_ENGINES", "KNN;THRESHOLD_TRIGGER"));
	EXPECT_TRUE(Config::Inst()->WriteSetting("CLASSIFICATION_CONFIGS", "foo.config;bar.config"));
	EXPECT_TRUE(Config::Inst()->WriteSetting("CLASSIFICATION_MODES", "WEIGHTED;HOSTILE_OVERRIDE"));

	Config::Inst()->LoadConfig();

	EXPECT_EQ(2, Config::Inst()->GetClassificationEngines().size());
	EXPECT_EQ("KNN", Config::Inst()->GetClassificationEngines().at(0));
	EXPECT_EQ("THRESHOLD_TRIGGER", Config::Inst()->GetClassificationEngines().at(1));

	EXPECT_EQ(2, Config::Inst()->GetClassifierConfigs().size());
	EXPECT_EQ("foo.config", Config::Inst()->GetClassifierConfigs().at(0));
	EXPECT_EQ("bar.config", Config::Inst()->GetClassifierConfigs().at(1));

	EXPECT_EQ(2, Config::Inst()->GetClassifierModes().size());
	EXPECT_EQ(CLASSIFIER_WEIGHTED, Config::Inst()->GetClassifierModes().at(0));
	EXPECT_EQ(CLASSIFIER_HOSTILE_OVERRIDE, Config::Inst()->GetClassifierModes().at(1));
}

TEST_F(ConfigTest, test_classificationLoading)
{
	ThresholdTriggerClassification engine;
	engine.LoadConfiguration(Config::Inst()->GetPathHome() + "/config/CE_1.config");

	// Random spot check on our current default values. Be sure to update this if config changes.
	ASSERT_EQ(DIM, engine.m_hostileThresholds.size());
	EXPECT_FALSE(engine.m_hostileThresholds[0].m_hasMaxValueTrigger);
	EXPECT_FALSE(engine.m_hostileThresholds[0].m_hasMinValueTrigger);

	EXPECT_TRUE(engine.m_hostileThresholds[DISTINCT_TCP_PORTS].m_hasMaxValueTrigger);
	EXPECT_FALSE(engine.m_hostileThresholds[DISTINCT_TCP_PORTS].m_hasMinValueTrigger);
	EXPECT_EQ(100, engine.m_hostileThresholds[DISTINCT_TCP_PORTS].m_maxValueTrigger);

}


// Tests that changing the enabled features sets all needed config options
/*
TEST_F(ConfigTest, test_setEnabledFeatures) {
	string enabledFeatureString = "11001001111010";
	Config::Inst()->SetEnabledFeatures(enabledFeatureString);

	// Check the enabled feature mask string
	EXPECT_EQ(enabledFeatureString, Config::Inst()->GetEnabledFeatures());

	// Check the count of enabled features
	EXPECT_EQ((uint)8, Config::Inst()->GetEnabledFeatureCount());

	// Check the squrt of the enabled features
	EXPECT_EQ(sqrt(8), Config::Inst()->GetSqurtEnabledFeatures());

	// Check if it correctly set all the enabled/disabled feature values
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(0));
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(1));
	EXPECT_FALSE(Config::Inst()->IsFeatureEnabled(2));
	EXPECT_FALSE(Config::Inst()->IsFeatureEnabled(3));
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(4));
	EXPECT_FALSE(Config::Inst()->IsFeatureEnabled(5));
	EXPECT_FALSE(Config::Inst()->IsFeatureEnabled(6));
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(7));
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(8));
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(9));
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(10));
	EXPECT_FALSE(Config::Inst()->IsFeatureEnabled(11));
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(12));
	EXPECT_FALSE(Config::Inst()->IsFeatureEnabled(13));
}
*/

