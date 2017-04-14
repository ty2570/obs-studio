#include "window-basic-auto-config.hpp"
#include "obs-app.hpp"

#include "Ui_AutoConfigStartPage.h"
#include "Ui_AutoConfigStreamPage.h"
#include "Ui_AutoConfigFinishPage.h"

#define wiz reinterpret_cast<AutoConfig*>(wizard())

/* ------------------------------------------------------------------------- */

AutoConfigStartPage::AutoConfigStartPage(QWidget *parent)
	: QWizardPage (parent),
	  ui          (new Ui_AutoConfigStartPage)
{
	ui->setupUi(this);
	setTitle(QTStr("Basic.AutoConfig.StartPage"));
	setSubTitle(QTStr("Basic.AutoConfig.StartPage.SubTitle"));

	auto setComplete = [this] ()
	{
		if (!clickedAnItem) {
			clickedAnItem = true;
			emit completeChanged();
		}
	};

	connect(ui->prioritizeStreaming, &QRadioButton::clicked, setComplete);
	connect(ui->prioritizeRecording, &QRadioButton::clicked, setComplete);
	connect(ui->prioritizeBoth, &QRadioButton::clicked, setComplete);
}

AutoConfigStartPage::~AutoConfigStartPage()
{
	delete ui;
}

bool AutoConfigStartPage::isComplete() const
{
	return clickedAnItem;
}

int AutoConfigStartPage::nextId() const
{
	return ui->prioritizeRecording->isChecked()
		? AutoConfig::TestPage
		: AutoConfig::StreamPage;
}

void AutoConfigStartPage::on_prioritizeStreaming_clicked()
{
	wiz->type = AutoConfig::Type::Streaming;
}

void AutoConfigStartPage::on_prioritizeRecording_clicked()
{
	wiz->type = AutoConfig::Type::Recording;
}

void AutoConfigStartPage::on_prioritizeBoth_clicked()
{
	wiz->type = AutoConfig::Type::Both;
}

/* ------------------------------------------------------------------------- */

AutoConfigStreamPage::AutoConfigStreamPage(QWidget *parent)
	: QWizardPage (parent),
	  ui          (new Ui_AutoConfigStreamPage)
{
	ui->setupUi(this);
	setTitle(QTStr("Basic.AutoConfig.StreamPage"));
	setSubTitle(QTStr("Basic.AutoConfig.StreamPage.SubTitle"));
}

AutoConfigStreamPage::~AutoConfigStreamPage()
{
	delete ui;
}

int AutoConfigStreamPage::nextId() const
{
	return AutoConfig::TestPage;
}

/* ------------------------------------------------------------------------- */

AutoConfigFinishPage::AutoConfigFinishPage(QWidget *parent)
	: QWizardPage (parent),
	  ui          (new Ui_AutoConfigFinishPage)
{
	ui->setupUi(this);
	setTitle(QTStr("Basic.AutoConfig.FinishPage"));
	setSubTitle(QTStr("Basic.AutoConfig.FinishPage.SubTitle"));
}

AutoConfigFinishPage::~AutoConfigFinishPage()
{
	delete ui;
}

int AutoConfigFinishPage::nextId() const
{
	return -1;
}

/* ------------------------------------------------------------------------- */

AutoConfig::AutoConfig(QWidget *parent)
	: QWizard(parent)
{
#ifdef _WIN32
	setWizardStyle(QWizard::ModernStyle);
#endif
	setPage(StartPage, new AutoConfigStartPage());
	setPage(StreamPage, new AutoConfigStreamPage());
	setPage(TestPage, new AutoConfigTestPage());
	setPage(FinishPage, new AutoConfigFinishPage());
	setWindowTitle(QTStr("Basic.AutoConfig"));
}
