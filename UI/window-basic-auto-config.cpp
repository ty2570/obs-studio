#include "window-basic-auto-config.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "obs-app.hpp"

#include <QScreen>

#include <obs.hpp>

#include "Ui_AutoConfigStartPage.h"
#include "Ui_AutoConfigVideoPage.h"
#include "Ui_AutoConfigStreamPage.h"

#define wiz reinterpret_cast<AutoConfig*>(wizard())

/* ------------------------------------------------------------------------- */

#define SERVICE_PATH "service.json"

static OBSData OpenServiceSettings()
{
	const char *type;

	char serviceJsonPath[512];
	int ret = GetProfilePath(serviceJsonPath, sizeof(serviceJsonPath),
			SERVICE_PATH);
	if (ret <= 0)
		return OBSData();

	OBSData data = obs_data_create_from_json_file_safe(serviceJsonPath,
			"bak");
	obs_data_release(data);

	obs_data_set_default_string(data, "type", "rtmp_common");
	type = obs_data_get_string(data, "type");

	if (type && strcmp(type, "rtmp_common") != 0)
		return OBSData();

	OBSData settings = obs_data_get_obj(data, "settings");
	obs_data_release(settings);

	return settings;
}

static void GetServiceInfo(std::string &service, std::string &key)
{
	OBSData settings = OpenServiceSettings();

	service = obs_data_get_string(settings, "service");
	key = obs_data_get_string(settings, "key");
}

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
	return AutoConfig::VideoPage;
}

void AutoConfigStartPage::on_prioritizeStreaming_clicked()
{
	wiz->type = AutoConfig::Type::Streaming;
}

void AutoConfigStartPage::on_prioritizeRecording_clicked()
{
	wiz->type = AutoConfig::Type::Recording;
}

/* ------------------------------------------------------------------------- */

#define RES_TEXT(x)             "Basic.AutoConfig.VideoPage." x
#define RES_USE_CURRENT         RES_TEXT("BaseResolution.UseCurrent")
#define RES_USE_DISPLAY         RES_TEXT("BaseResolution.Display")
#define FPS_USE_CURRENT         RES_TEXT("FPS.UseCurrent")
#define FPS_PREFER_HIGH_FPS     RES_TEXT("FPS.PreferHighFPS")
#define FPS_PREFER_HIGH_RES     RES_TEXT("FPS.PreferHighRes")

AutoConfigVideoPage::AutoConfigVideoPage(QWidget *parent)
	: QWizardPage (parent),
	  ui          (new Ui_AutoConfigVideoPage)
{
	ui->setupUi(this);

	setTitle(QTStr("Basic.AutoConfig.VideoPage"));
	setSubTitle(QTStr("Basic.AutoConfig.VideoPage.SubTitle"));

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	long double fpsVal =
		(long double)ovi.fps_num / (long double)ovi.fps_den;

	QString fpsStr = (ovi.fps_den > 1)
		? QString::number(fpsVal, 'f', 2)
		: QString::number(fpsVal, 'g', 2);

	ui->fps->addItem(QTStr(FPS_PREFER_HIGH_FPS),
			(int)FPSType::PreferHighFPS);
	ui->fps->addItem(QTStr(FPS_PREFER_HIGH_RES),
			(int)FPSType::PreferHighRes);
	ui->fps->addItem(QTStr(FPS_USE_CURRENT).arg(fpsStr),
			(int)FPSType::UseCurrent);
	ui->fps->addItem(QStringLiteral("30"),    (int)FPSType::fps30);
	ui->fps->addItem(QStringLiteral("60"),    (int)FPSType::fps60);
	ui->fps->setCurrentIndex(0);

	QString cxStr = QString::number(ovi.base_width);
	QString cyStr = QString::number(ovi.base_height);

	int encRes = int(ovi.base_width << 16) | int(ovi.base_height);
	ui->canvasRes->addItem(QTStr(RES_USE_CURRENT).arg(cxStr, cyStr),
			(int)encRes);

	QList<QScreen*> screens = QGuiApplication::screens();
	for (int i = 0; i < screens.size(); i++) {
		QScreen *screen = screens[i];
		QSize as = screen->size();

		encRes = int(as.width() << 16) | int(as.height());

		QString str = QTStr(RES_USE_DISPLAY)
			.arg(QString::number(i + 1),
			     QString::number(as.width()),
			     QString::number(as.height()));

		ui->canvasRes->addItem(str, encRes);
	}

	auto addRes = [&] (int cx, int cy)
	{
		encRes = (cx << 16) | cy;
		QString str = QString::asprintf("%dx%d", cx, cy);
		ui->canvasRes->addItem(str, encRes);
	};

	addRes(1920, 1080);
	addRes(1280, 720);

	ui->canvasRes->setCurrentIndex(0);
}

AutoConfigVideoPage::~AutoConfigVideoPage()
{
	delete ui;
}

int AutoConfigVideoPage::nextId() const
{
	return wiz->type == AutoConfig::Type::Recording
		? AutoConfig::TestPage
		: AutoConfig::StreamPage;
}

bool AutoConfigVideoPage::validatePage()
{
	int encRes = ui->canvasRes->currentData().toInt();
	wiz->baseResolutionCX = encRes >> 16;
	wiz->baseResolutionCY = encRes & 0xFFFF;

	FPSType fpsType = (FPSType)ui->fps->currentData().toInt();

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	switch (fpsType) {
	case FPSType::PreferHighFPS:
		wiz->specificFPSNum = 0;
		wiz->specificFPSDen = 0;
		wiz->preferHighFPS = true;
		break;
	case FPSType::PreferHighRes:
		wiz->specificFPSNum = 0;
		wiz->specificFPSDen = 0;
		wiz->preferHighFPS = false;
		break;
	case FPSType::UseCurrent:
		wiz->specificFPSNum = ovi.fps_num;
		wiz->specificFPSDen = ovi.fps_den;
		wiz->preferHighFPS = false;
		break;
	case FPSType::fps30:
		wiz->specificFPSNum = 30;
		wiz->specificFPSDen = 1;
		wiz->preferHighFPS = false;
		break;
	case FPSType::fps60:
		wiz->specificFPSNum = 60;
		wiz->specificFPSDen = 1;
		wiz->preferHighFPS = false;
		break;
	}

	return true;
}

/* ------------------------------------------------------------------------- */

AutoConfigStreamPage::AutoConfigStreamPage(QWidget *parent)
	: QWizardPage (parent),
	  ui          (new Ui_AutoConfigStreamPage)
{
	ui->setupUi(this);
	ui->bitrateLabel->setVisible(false);
	ui->bitrate->setVisible(false);

	setTitle(QTStr("Basic.AutoConfig.StreamPage"));
	setSubTitle(QTStr("Basic.AutoConfig.StreamPage.SubTitle"));

	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_property_t *services = obs_properties_get(props, "service");

	size_t services_count = obs_property_list_item_count(services);
	for (size_t i = 0; i < services_count; i++) {
		const char *name = obs_property_list_item_string(services, i);
		ui->service->addItem(name);
	}

	obs_properties_destroy(props);

	connect(ui->service, SIGNAL(currentIndexChanged(int)),
			this, SLOT(ServiceChanged()));
	connect(ui->doBandwidthTest, SIGNAL(toggled(bool)),
			this, SLOT(ServiceChanged()));

	connect(ui->key, SIGNAL(textChanged(const QString &)),
			this, SLOT(UpdateCompleted()));
	connect(ui->regionUS, SIGNAL(toggled(bool)),
			this, SLOT(UpdateCompleted()));
	connect(ui->regionEU, SIGNAL(toggled(bool)),
			this, SLOT(UpdateCompleted()));
	connect(ui->regionAsia, SIGNAL(toggled(bool)),
			this, SLOT(UpdateCompleted()));
	connect(ui->regionOther, SIGNAL(toggled(bool)),
			this, SLOT(UpdateCompleted()));
}

AutoConfigStreamPage::~AutoConfigStreamPage()
{
	delete ui;
}

bool AutoConfigStreamPage::isComplete() const
{
	return ready;
}

int AutoConfigStreamPage::nextId() const
{
	return AutoConfig::TestPage;
}

bool AutoConfigStreamPage::validatePage()
{
	OBSData service_settings = obs_data_create();
	obs_data_release(service_settings);
	obs_data_set_string(service_settings, "service",
			QT_TO_UTF8(ui->service->currentText()));

	OBSService service = obs_service_create("rtmp_common", "temp_service",
			service_settings, nullptr);
	obs_service_release(service);

	int bitrate = 6000;
	if (!ui->doBandwidthTest->isChecked()) {
		bitrate = ui->bitrate->value();
		wiz->idealBitrate = bitrate;
	}

	OBSData settings = obs_data_create();
	obs_data_release(settings);
	obs_data_set_int(settings, "bitrate", bitrate);
	obs_service_apply_encoder_settings(service, settings, nullptr);

	wiz->bandwidthTest = ui->doBandwidthTest->isChecked();
	wiz->startingBitrate = (int)obs_data_get_int(settings, "bitrate");
	wiz->idealBitrate = wiz->startingBitrate;
	wiz->regionUS = ui->regionUS->isChecked();
	wiz->regionEU = ui->regionEU->isChecked();
	wiz->regionAsia = ui->regionAsia->isChecked();
	wiz->regionOther = ui->regionOther->isChecked();
	wiz->service = QT_TO_UTF8(ui->service->currentText());
	if (ui->preferHardware)
		wiz->preferHardware = ui->preferHardware->isChecked();
	wiz->key = QT_TO_UTF8(ui->key->text());

	if (wiz->service == "Twitch") {
		wiz->serviceType = AutoConfig::Service::Twitch;
		wiz->key += "?bandwidthtest";
	} else if (wiz->service == "hitbox.tv") {
		wiz->serviceType = AutoConfig::Service::Hitbox;
	} else if (wiz->service == "beam.pro") {
		wiz->serviceType = AutoConfig::Service::Beam;
	} else {
		wiz->serviceType = AutoConfig::Service::Other;
	}

	return true;
}

void AutoConfigStreamPage::on_show_clicked()
{
	if (ui->key->echoMode() == QLineEdit::Password) {
		ui->key->setEchoMode(QLineEdit::Normal);
		ui->show->setText(QTStr("Hide"));
	} else {
		ui->key->setEchoMode(QLineEdit::Password);
		ui->show->setText(QTStr("Show"));
	}
}

void AutoConfigStreamPage::ServiceChanged()
{
	std::string service = QT_TO_UTF8(ui->service->currentText());
	bool regionBased = service == "Twitch" ||
	                   service == "hitbox.tv" ||
	                   service == "beam.pro";
	bool testBandwidth = ui->doBandwidthTest->isChecked();

	wiz->testRegions = regionBased && testBandwidth;

	ui->region->setVisible(regionBased && testBandwidth);
	ui->bitrateLabel->setHidden(testBandwidth);
	ui->bitrate->setHidden(testBandwidth);
	UpdateCompleted();	
}

void AutoConfigStreamPage::UpdateCompleted()
{
	if (ui->key->text().isEmpty())
		ready = false;
	else
		ready = !wiz->testRegions ||
			ui->regionUS->isChecked() ||
			ui->regionEU->isChecked() ||
			ui->regionAsia->isChecked() ||
			ui->regionOther->isChecked();
	emit completeChanged();
}

/* ------------------------------------------------------------------------- */

AutoConfig::AutoConfig(QWidget *parent)
	: QWizard(parent)
{
	OBSBasic *main = reinterpret_cast<OBSBasic*>(parent);

	GetServiceInfo(service, key);
#ifdef _WIN32
	setWizardStyle(QWizard::ModernStyle);
#endif
	AutoConfigStreamPage *streamPage = new AutoConfigStreamPage();

	setPage(StartPage, new AutoConfigStartPage());
	setPage(VideoPage, new AutoConfigVideoPage());
	setPage(StreamPage, streamPage);
	setPage(TestPage, new AutoConfigTestPage());
	setWindowTitle(QTStr("Basic.AutoConfig"));

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	baseResolutionCX = ovi.base_width;
	baseResolutionCY = ovi.base_height;

	if (!service.empty()) {
		QComboBox *serviceList = streamPage->ui->service;

		int count = serviceList->count();
		bool found = false;

		for (int i = 0; i < count; i++) {
			QString name = serviceList->itemText(i);

			if (name == service.c_str()) {
				serviceList->setCurrentIndex(i);
				found = true;
				break;
			}
		}

		if (!found) {
			serviceList->insertItem(0, service.c_str());
			serviceList->setCurrentIndex(0);
		}
	}

	if (!key.empty())
		streamPage->ui->key->setText(key.c_str());

	int bitrate = config_get_int(main->Config(), "SimpleOutput", "VBitrate");
	streamPage->ui->bitrate->setValue(bitrate);
	streamPage->ServiceChanged();

	TestHardwareEncoding();
	if (!hardwareEncodingAvailable) {
		delete streamPage->ui->preferHardware;
		streamPage->ui->preferHardware = nullptr;
	}
}

void AutoConfig::TestHardwareEncoding()
{
	size_t idx = 0;
	const char *id;
	while (obs_enum_encoder_types(idx++, &id)) {
		if (strcmp(id, "ffmpeg_nvenc") == 0)
			hardwareEncodingAvailable = nvencAvailable = true;
		else if (strcmp(id, "obs_qsv11") == 0)
			hardwareEncodingAvailable = qsvAvailable = true;
		else if (strcmp(id, "amd_amf_h264") == 0)
			hardwareEncodingAvailable = vceAvailable = true;
			
	}
}

bool AutoConfig::CanTestServer(const char *server)
{
	if (!testRegions || (regionUS && regionEU && regionAsia && regionOther))
		return true;

	if (serviceType == Service::Twitch) {
		if (astrcmp_n(server, "US West:", 8) == 0 ||
		    astrcmp_n(server, "US East:", 8) == 0 ||
		    astrcmp_n(server, "US Central:", 11) == 0 ||
		    astrcmp_n(server, "NA:", 3) == 0) {
			return regionUS;
		} else if (astrcmp_n(server, "EU:", 3) == 0) {
			return regionEU;
		} else if (astrcmp_n(server, "Asia:", 5) == 0) {
			return regionAsia;
		} else if (regionOther) {
			return true;
		}
	} else if (serviceType == Service::Hitbox) {
		if (strcmp(server, "Default") == 0) {
			return true;
		} else if (astrcmp_n(server, "US-West:", 8) == 0 ||
		           astrcmp_n(server, "US-East:", 8) == 0) {
			return regionUS;
		} else if (astrcmp_n(server, "EU-", 3) == 0) {
			return regionEU;
		} else if (astrcmp_n(server, "South Korea:", 12) == 0 ||
		           astrcmp_n(server, "Asia:", 5) == 0 ||
		           astrcmp_n(server, "China:", 6) == 0) {
			return regionAsia;
		} else if (regionOther) {
			return true;
		}
	} else if (serviceType == Service::Beam) {
		if (astrcmp_n(server, "US:", 3) == 0 ||
		    astrcmp_n(server, "Canada:", 7) == 0) {
			return regionUS;
		} else if (astrcmp_n(server, "EU:", 3) == 0) {
			return regionEU;
		} else if (astrcmp_n(server, "South Korea:", 12) == 0 ||
		           astrcmp_n(server, "Asia:", 5) == 0) {
			return regionAsia;
		} else if (regionOther) {
			return true;
		}
	} else {
		return true;
	}

	return false;
}
