#include <QThread>

#include <obs.hpp>

#include "window-basic-auto-config.hpp"
#include "obs-app.hpp"

#include "Ui_AutoConfigTestPage.h"

#define wiz reinterpret_cast<AutoConfig*>(wizard())

/* ------------------------------------------------------------------------- */

class ActiveOutputSourceStorage {
	OBSSource source[6];

public:
	inline ActiveOutputSourceStorage()
	{
		for (size_t i = 0; i < 6; i++) {
			source[i] = obs_get_output_source(i);
			obs_source_release(source[i]);
			obs_set_output_source(i, nullptr);
		}
	}

	inline ~ActiveOutputSourceStorage()
	{
		for (size_t i = 0; i < 6; i++)
			obs_set_output_source(source[i]);
	}
};

/* ------------------------------------------------------------------------- */

AutoConfigTestPage::AutoConfigTestPage(QWidget *parent)
	: QWizardPage (parent),
	  ui          (new Ui_AutoConfigTestPage)
{
	ui->setupUi(this);
	setTitle(QTStr("Basic.AutoConfig.TestPage"));
	setSubTitle(QTStr("Basic.AutoConfig.TestPage.SubTitle"));

	os_event_init(&stop, OS_EVENT_TYPE_MANUAL);
}

AutoConfigTestPage::~AutoConfigTestPage()
{
	delete ui;

	if (testThread.joinable()) {
		os_event_signal(stop);
		testThread.join();
	}

	os_event_destroy(stop);
}

void AutoConfigTestPage::initializePage()
{
	stage = Stage::Starting;
	NextStage();
}

bool AutoConfigTestPage::isComplete() const
{
	return stage == Stage::Finished;
}

int AutoConfigTestPage::nextId() const
{
	return AutoConfig::FinishPage;
}

void AutoConfigTestPage::StartBandwidthStage()
{
	ui->progressLabel->setText(
			QTStr("Basic.AutoConfig.TestPage.Bandwidth"));

	testThread = std::thread([this] () {TestBandwidthThread();});
}

void AutoConfigTestPage::StartStreamEncoderStage()
{
	ui->progressLabel->setText(
			QTStr("Basic.AutoConfig.TestPage.StreamEncoder"));

	testThread = std::thread([this] () {TestStreamEncoderThread();});
}

void AutoConfigTestPage::StartRecordingEncoderStage()
{
	ui->progressLabel->setText(
			QTStr("Basic.AutoConfig.TestPage.RecordingEncoder"));

	testThread = std::thread([this] () {TestRecordingEncoderThread();});
}

void AutoConfigTestPage::TestBandwidthThread()
{
	ActiveOutputSourceStorage store;
	/*
	 * create encoders
	 * create output
	 * test for 10 seconds
	 */

	
}

void AutoConfigTestPage::TestStreamEncoderThread()
{
	ActiveOutputSourceStorage store;
	/*
	 * create encoders
	 * create dummy output
	 * test for 10 seconds
	 */
}

void AutoConfigTestPage::TestRecordingEncoderThread()
{
	ActiveOutputSourceStorage store;
}

void AutoConfigTestPage::NextStage()
{
	if (testThread.joinable())
		testThread.join();

	ui->subProgressLabel->setText(QString());

	/* make it skip to bandwidth stage if only set to config recording */
	if (stage == Stage::Starting &&
	    wiz->type == AutoConfig::Type::Recording)
		stage = Stage::StreamEncoder;

	if (stage == Stage::Starting) {
		ui->progressBar->setValue(0);
		stage = Stage::BandwidthTest;
		StartBandwidthStage();

	} else if (stage == Stage::BandwidthTest) {
		ui->progressBar->setValue(33);
		stage = Stage::StreamEncoder;
		StartStreamEncoderStage();

	} else if (stage == Stage::StreamEncoder) {
		ui->progressBar->setValue(66);
		stage = Stage::RecordingEncoder;
		StartRecordingEncoderStage();

	} else {
		ui->progressBar->setValue(100);
		stage = Stage::Finished;
		emit completeChanged();
	}
}

void AutoConfigTestPage::UpdateMessage(QString message)
{
	ui->subProgressLabel->setText(message);
}
