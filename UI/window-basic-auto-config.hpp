#pragma once

#include <QWizard>
#include <QWizardPage>
#include <util/threading.h>
#include <thread>

class Ui_AutoConfigStartPage;
class Ui_AutoConfigStreamPage;
class Ui_AutoConfigTestPage;
class Ui_AutoConfigFinishPage;

class AutoConfig : public QWizard {
	Q_OBJECT

	friend class AutoConfigStartPage;
	friend class AutoConfigStreamPage;
	friend class AutoConfigTestPage;
	friend class AutoConfigFinishPage;

	enum class Type {
		Invalid,
		Streaming,
		Recording,
		Both
	};

	Type type = Type::Invalid;
	int idealBitrate = 2500;
	int idealResolutionCX = 1280;
	int idealResolutionCY = 720;
	int idealFPS = 60;

public:
	AutoConfig(QWidget *parent);

	enum Page {
		StartPage,
		StreamPage,
		TestPage,
		FinishPage
	};
};

class AutoConfigStartPage : public QWizardPage {
	Q_OBJECT

	friend class AutoConfig;

	Ui_AutoConfigStartPage *ui;
	bool clickedAnItem = false;

public:
	AutoConfigStartPage(QWidget *parent = nullptr);
	~AutoConfigStartPage();

	virtual bool isComplete() const override;
	virtual int nextId() const override;

public slots:
	void on_prioritizeStreaming_clicked();
	void on_prioritizeRecording_clicked();
	void on_prioritizeBoth_clicked();
};

class AutoConfigStreamPage : public QWizardPage {
	Q_OBJECT

	friend class AutoConfig;

	Ui_AutoConfigStreamPage *ui;

public:
	AutoConfigStreamPage(QWidget *parent = nullptr);
	~AutoConfigStreamPage();

	virtual int nextId() const override;
};

class AutoConfigTestPage : public QWizardPage {
	Q_OBJECT

	friend class AutoConfig;

	Ui_AutoConfigTestPage *ui;
	std::thread testThread;
	os_event_t *stop = nullptr;

	enum class Stage {
		Starting,
		BandwidthTest,
		StreamEncoder,
		RecordingEncoder,
		Finished
	};

	Stage stage = Stage::Starting;
	bool resolutionTested = false;

	void StartBandwidthStage();
	void StartStreamEncoderStage();
	void StartRecordingEncoderStage();

	void TestResolution();

	void TestBandwidthThread();
	void TestStreamEncoderThread();
	void TestRecordingEncoderThread();

public:
	AutoConfigTestPage(QWidget *parent = nullptr);
	~AutoConfigTestPage();

	virtual void initializePage() override;
	virtual bool isComplete() const override;
	virtual int nextId() const override;

public slots:
	void NextStage();
	void UpdateMessage(QString message);
};

class AutoConfigFinishPage : public QWizardPage {
	Q_OBJECT

	friend class AutoConfig;

	Ui_AutoConfigFinishPage *ui;

public:
	AutoConfigFinishPage(QWidget *parent = nullptr);
	~AutoConfigFinishPage();

	virtual int nextId() const override;
};
