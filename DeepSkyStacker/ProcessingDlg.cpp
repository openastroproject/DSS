#include "stdafx.h"
#include "DeepSkyStacker.h"
#include "ProcessingDlg.h"
#include "progressdlg.h"
#include "selectrect.h"
#include "FrameInfoSupport.h"
#include "ProcessingSettingsDlg.h"
#include "SavePicture.h"
#include "StarMaskDlg.h"
#include "StarMask.h"
#include "FITSUtil.h"
#include "TIFFUtil.h"

#define dssApp DeepSkyStacker::instance()

/* ------------------------------------------------------------------- */

namespace
{
	class ColorOrder
	{
	public:
		QRgb		m_crColor;		// Qt 32-bit QRgb format (0xAARRGGBB)
		int			m_lSize;

		ColorOrder() :
			m_crColor{ qRgb(0, 0, 0) },
			m_lSize{ 0 }
		{
		}

		ColorOrder(QRgb crColor, int lSize)
		{
			m_crColor = crColor;
			m_lSize = lSize;
		};
		virtual ~ColorOrder() {};
		ColorOrder(const ColorOrder& co) = default;

		ColorOrder& operator = (const ColorOrder& co) = default;

		bool operator < (const ColorOrder& co) const
		{
			return m_lSize < co.m_lSize;
		};
	};
};

namespace DSS
{
	ProcessingDlg::ProcessingDlg(QWidget *parent)
		: QWidget(parent),
		dirty_ { false },
		timer {this},
		redAdjustmentCurve_{ HistogramAdjustmentCurve::Linear },
		greenAdjustmentCurve_{ HistogramAdjustmentCurve::Linear },
		blueAdjustmentCurve_{ HistogramAdjustmentCurve::Linear },
		hacMenu{ this }
	{
		ZFUNCTRACE_RUNTIME();
		setupUi(this);
		tabWidget->setCurrentIndex(0);	// Position on the RGB/K tab

		Qt::ColorScheme colorScheme{ QGuiApplication::styleHints()->colorScheme() };
		if (Qt::ColorScheme::Dark == colorScheme)
			iconModifier = "-dark";

		setButtonIcons();

		//
		// Initialise the popup menu for the "Histogram Adjustment Type" on the RGB tab
		//
		linearAction = hacMenu.addAction(HistoAdjustTypeText(HistogramAdjustmentCurve::Linear));
		cubeRootAction = hacMenu.addAction(HistoAdjustTypeText(HistogramAdjustmentCurve::CubeRoot));
		squareRootAction = hacMenu.addAction(HistoAdjustTypeText(HistogramAdjustmentCurve::SquareRoot));
		logAction = hacMenu.addAction(HistoAdjustTypeText(HistogramAdjustmentCurve::Log));
		logLogAction = hacMenu.addAction(HistoAdjustTypeText(HistogramAdjustmentCurve::LogLog));
		logSquareRootAction = hacMenu.addAction(HistoAdjustTypeText(HistogramAdjustmentCurve::LogSquareRoot));
		asinHAction = hacMenu.addAction(HistoAdjustTypeText(HistogramAdjustmentCurve::ASinH));

		initialiseSliders();

		//
		// Allow selection of partial image, don't display "Drizzle" rectangles.
		//
		selectRect = new SelectRect(picture);
		selectRect->setShowDrizzle(false);
		selectRect->rectButtonPressed();

		connect(selectRect, &SelectRect::selectRectChanged, this, &ProcessingDlg::setSelectionRect);
		connectSignalsToSlots();

		timer.start(50);	// 50 ms timeout

		updateControls();


	}

	ProcessingDlg::~ProcessingDlg()
	{
		ZFUNCTRACE_RUNTIME();
	}

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::initialiseSliders()
	{

		//
		// Initialise the "sliders" on the RGB tab
		//
		redGradient->setColorAt(0.5, QColor(qRgb(128, 0, 0)));
		redGradient->setColorAt(0.999, Qt::red);
		redGradient->setColorAt(1.0, Qt::red);

		greenGradient->setColorAt(0.5, QColor(qRgb(0, 128, 0)));
		greenGradient->setColorAt(0.999, Qt::green);
		greenGradient->setColorAt(1.0, Qt::green);

		blueGradient->setColorAt(0.5, QColor(qRgb(0, 0, 128)));
		blueGradient->setColorAt(0.999, Qt::blue);
		blueGradient->setColorAt(1.0, Qt::blue);

		//
		// Set the initial values for the sliders on the Luminance tab and set the text to match
		//
		darkAngle->setMinimum(0);
		darkAngle->setMaximum(maxAngle);		// const value of 45
		darkAngle->setValue(darkAngleInitialValue);
		darkPower->setMinimum(0);
		darkPower->setMaximum(maxLuminance);	// const value of 1000
		darkPower->setValue(darkPowerInitialValue);	// const value of 800
		updateDarkText();

		midAngle->setMinimum(0);
		midAngle->setMaximum(maxAngle);			// const value of 45
		midAngle->setValue(midAngleInitialValue);	// const value of 20
		midTone->setMinimum(0);
		midTone->setMaximum(maxLuminance);		// const value of 1000
		midTone->setValue(midToneInitialValue);	// const value of 330
		updateMidText();

		highAngle->setMinimum(0);
		highAngle->setMaximum(maxAngle);		// const value of 45
		highAngle->setValue(highAngleInitialPostion);	// const value of 0
		highPower->setMinimum(0);
		highPower->setMaximum(maxLuminance);	// const value of 1000
		highPower->setValue(highPowerInitialValue);	// const value of 500
		updateHighText();

		//
		// Set the range and setting for the Saturation shift slider on the Saturation tab
		//
		saturation->setMinimum(minSaturation);	// const value of -50
		saturation->setMaximum(maxSaturation);	// const value of 50;
		saturation->setValue(initialSaturation);	// Set to a saturation shift of 20
		updateSaturationText();
	}

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::updateControls()
	{
		//
		// Has an image been loaded, if so enable the RGB, Luminance and Saturation tabs, and
		// also enable the buttons
		// 
		if (dssApp->deepStack().IsLoaded())
		{
			tabWidget->setEnabled(true);
			buttonWidget->setEnabled(true);
			//
			// If there are saved processing settings we can navigate, enable the undo and redo
			// buttons as appropriate
			//
			undoButton->setEnabled(processingSettingsList.IsBackwardAvailable());
			redoButton->setEnabled(processingSettingsList.IsForwardAvailable());
		}
		else
		{
			tabWidget->setEnabled(false);
			buttonWidget->setEnabled(false);
		};
	};


	/* ------------------------------------------------------------------- */

	void ProcessingDlg::connectSignalsToSlots()
	{
		connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
			this, &ProcessingDlg::onColorSchemeChanged);

		connect(applyButton, &QPushButton::pressed, this, &ProcessingDlg::onApply);
		connect(undoButton, &QPushButton::pressed, this, &ProcessingDlg::onUndo);
		connect(settingsButton, &QPushButton::pressed, this, &ProcessingDlg::onSettings);
		connect(redoButton, &QPushButton::pressed, this, &ProcessingDlg::onRedo);
		connect(resetButton, &QPushButton::pressed, this, &ProcessingDlg::onReset);

		//
		// The source for the slots below are in RGBTab.cpp
		//
		connect(redGradient, &QLinearGradientCtrl::pegMove, this, &ProcessingDlg::redChanging);
		connect(redGradient, &QLinearGradientCtrl::pegMoved, this, &ProcessingDlg::redChanged);

		connect(greenGradient, &QLinearGradientCtrl::pegMove, this, &ProcessingDlg::greenChanging);
		connect(greenGradient, &QLinearGradientCtrl::pegMoved, this, &ProcessingDlg::greenChanged);

		connect(blueGradient, &QLinearGradientCtrl::pegMove, this, &ProcessingDlg::blueChanging);
		connect(blueGradient, &QLinearGradientCtrl::pegMoved, this, &ProcessingDlg::blueChanged);

		connect(redHAC, &QPushButton::pressed, this, &ProcessingDlg::redButtonPressed);
		connect(greenHAC, &QPushButton::pressed, this, &ProcessingDlg::greenButtonPressed);
		connect(blueHAC, &QPushButton::pressed, this, &ProcessingDlg::blueButtonPressed);

		//
		// If the luminance tab sliders are moved, update the text to match and process the
		// change
		//
		connect(darkAngle, &QSlider::valueChanged, this, &ProcessingDlg::darkAngleChanged);
		connect(darkPower, &QSlider::valueChanged, this, &ProcessingDlg::darkPowerChanged);
		connect(midAngle, &QSlider::valueChanged, this, &ProcessingDlg::midAngleChanged);
		connect(midTone, &QSlider::valueChanged, this, &ProcessingDlg::midToneChanged);
		connect(highAngle, &QSlider::valueChanged, this, &ProcessingDlg::highAngleChanged);
		connect(highPower, &QSlider::valueChanged, this, &ProcessingDlg::highPowerChanged);

		//
		// if the saturation slider is moved, update the text to match and process the
		// change
		//
		connect(saturation, &QSlider::valueChanged, this, &ProcessingDlg::saturationChanged);

		//
		// When the timer fires, drive the timer handler
		//
		connect(&timer, &QTimer::timeout, this, &ProcessingDlg::onTimer);

	}

	/* ------------------------------------------------------------------- */

	bool ProcessingDlg::saveOnClose()
	{
		return askToSave();
	}

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::copyToClipboard()
	{
		ZFUNCTRACE_RUNTIME();
		if (dssApp->deepStack().IsLoaded())
		{
			QClipboard* clipboard = QGuiApplication::clipboard();

			timer.stop();
			QPixmap pix{ picture->grab() };
			clipboard->setPixmap(pix);
			timer.start();
		}
	}

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::createStarMask()
	{
		ZFUNCTRACE_RUNTIME();
		if (dssApp->deepStack().IsLoaded())
		{
			timer.stop();
			StarMaskDlg dlg{ this, currentFile };

			if (QDialog::Accepted == dlg.exec())
			{
				ProgressDlg progress{ DeepSkyStacker::instance() };
				StarMaskEngine starMaskEngine;

				progress.SetJointProgress(true);
				std::shared_ptr<CMemoryBitmap> pBitmap = dssApp->deepStack().GetStackedBitmap().GetBitmap(&progress);
				if (std::shared_ptr<CMemoryBitmap> pStarMask = starMaskEngine.createStarMask(pBitmap.get(), &progress))
				{
					// Save the star mask to a file
					fs::path file{ dlg.outputFile() };
					bool isFits{ dlg.outputIsFits() };

					const QString description{ tr("Star Mask created by DeepSkyStacker", "IDS_STARMASKDESCRIPTION") };

					const QString strText(tr("Saving the Star Mask in %1", "IDS_SAVINGSTARMASK").arg(file.generic_u16string()));
					progress.Start2(strText, 0);
					if (isFits)
						WriteFITS(file, pStarMask.get(), &progress, description);
					else
						WriteTIFF(file, pStarMask.get(), &progress, description);
				}
				progress.End2();

			}

			timer.start();
		}
		return;
	}

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::loadStackedImage(const fs::path& file)
	{
		ZFUNCTRACE_RUNTIME();

		//
		// Load the output file created at the end of the stacking process.
		//
		ProgressDlg dlg{ DeepSkyStacker::instance() };
		bool ok { false };

		timer.stop();

		QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		dssApp->deepStack().reset();
		dssApp->deepStack().SetProgress(&dlg);
		ok = dssApp->deepStack().LoadStackedInfo(file);
		dssApp->deepStack().SetProgress(nullptr);
		QGuiApplication::restoreOverrideCursor();

		if (ok)
		{
			currentFile = file;

			modifyRGBKGradientControls();
			updateInformation();
			QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

			dssApp->deepStack().GetStackedBitmap().GetBezierAdjust(processingSettings.bezierAdjust_);
			dssApp->deepStack().GetStackedBitmap().GetHistogramAdjust(processingSettings.histoAdjust_);
			updateControlsFromSettings();

			resetSliders();		// Calls showHistogram

			int height = dssApp->deepStack().GetHeight();
			rectToProcess.Init(dssApp->deepStack().GetWidth(), height, height / 3);

			processingSettingsList.clear();
			picture->clear();
			processAndShow(true);
			QGuiApplication::restoreOverrideCursor();
			setDirty(false);
		};

		timer.start();
	}

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::loadImage()
	{
		ZFUNCTRACE_RUNTIME();
		qDebug() << "Load image";

		if (askToSave())
		{

			QFileDialog			fileDialog(this);
			QSettings			settings;
			QString				directory;
			QString				extension;
			QString				strTitle;
			fs::path file;

			DSS::ProgressDlg dlg{ DeepSkyStacker::instance() };

			timer.stop();

			directory = settings.value("Folders/SaveDSIFolder").toString();
			extension = settings.value("Folders/SavePictureExtension").toString();
			if (extension.isEmpty()) extension = "tif";
			fileDialog.setDefaultSuffix(extension);
			fileDialog.setFileMode(QFileDialog::ExistingFile);	// There can be only one

			fileDialog.setNameFilter(tr("TIFF and FITS Files (*.tif *.tiff *.fits *.fit *.fts)", "IDS_FILTER_DSIIMAGETIFF"));
			fileDialog.selectFile(QString());		// No file(s) selected
			if (!directory.isEmpty())
				fileDialog.setDirectory(directory);

			if (QDialog::Accepted == fileDialog.exec())
			{
				QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
				QStringList files = fileDialog.selectedFiles();

				//
				// Now get the file as a standard fs::path object
				//
				if (!files.empty())		// Never, ever attempt to add zero rows!!!
				{
					file = files.at(0).toStdU16String();

					if (file.has_parent_path())
						directory = QString::fromStdU16String(file.parent_path().generic_u16string());
					else
						directory = QString::fromStdU16String(file.root_path().generic_u16string());

					extension = QString::fromStdU16String(file.extension().generic_u16string());
				}

				settings.setValue("Folders/SaveDSIFolder", directory);
				settings.setValue("Folders/SavePictureExtension", extension);

				//
				// Finally load the file of interest
				//
				currentFile = file;				// Remember the current file
				dssApp->deepStack().reset();
				dssApp->deepStack().SetProgress(&dlg);
				bool result = dssApp->deepStack().LoadStackedInfo(file);
				if (!result)
				{
					QString message{ QString("Failure processing file %1").arg(file.generic_u16string()) };
					QMessageBox::warning(this, "DeepSkyStacker",
							message);
				}

				dssApp->deepStack().SetProgress(nullptr);

				modifyRGBKGradientControls();
				updateInformation();

				dssApp->deepStack().GetStackedBitmap().GetBezierAdjust(processingSettings.bezierAdjust_);
				dssApp->deepStack().GetStackedBitmap().GetHistogramAdjust(processingSettings.histoAdjust_);

				updateControlsFromSettings();

				showHistogram(false);
				resetSliders();
				int height = dssApp->deepStack().GetHeight();
				rectToProcess.Init(dssApp->deepStack().GetWidth(), height, height / 3);

				processingSettingsList.clear();
				picture->clear();
				processAndShow(true);

				setDirty(false);

				timer.start();

				QGuiApplication::restoreOverrideCursor();

			}
		}
	}

	/* ------------------------------------------------------------------- */

	bool ProcessingDlg::saveImage()
	{
		ZFUNCTRACE_RUNTIME();
		qDebug() << "Save image to file";
		bool result = false;


		if (dssApp->deepStack().IsLoaded())
		{
			QSettings settings;

			auto directory{ settings.value("Folders/SavePictureFolder").toString() };
			auto extension{ settings.value("Folders/SavePictureExtension").toString().toLower() };
			auto apply{ settings.value("Folders/SaveApplySetting", false).toBool() };
			TIFFCOMPRESSION compression{ static_cast<TIFFCOMPRESSION>(
				settings.value("Folders/SaveCompression", (uint)TC_NONE).toUInt()) };
			auto filterIndex{ settings.value("Folders/SavePictureIndex", 0).toUInt() };
			if (filterIndex > 5) filterIndex = 0;

			QStringList fileFilters{
				tr("TIFF Image 16 bit/ch (*.tif)", "IDS_FILTER_OUTPUT"),
				tr("TIFF Image 32 bit/ch - integer (*.tif)", "IDS_FILTER_OUTPUT"),
				tr("TIFF Image 32 bit/ch - rational (*.tif)", "IDS_FILTER_OUTPUT"),
				tr("FITS Image 16 bit/ch (*.fts)", "IDS_FILTER_OUTPUT"),
				tr("FITS Image 32 bit/ch - integer (*.fts)", "IDS_FILTER_OUTPUT"),
				tr("FITS Image 32 bit/ch - rational (*.fts)", "IDS_FILTER_OUTPUT")
			};

			if (filterIndex > 2) extension = ".fts";
			else extension = ".tif";

			//
			// SavePicture is a sub-class of QFileDialog, we'll set the QFileDialog vars first
			//
			SavePicture dlg{ this, tr("Save Image"), directory };
			dlg.setDefaultSuffix(extension);
			dlg.setNameFilters(fileFilters);
			auto filter{ fileFilters.at(filterIndex) };
			dlg.selectNameFilter(filter);
			//
			// selectNameFilter doesn't fire the QFileDialog::filterSelected signal
			// so need to drive the slot ourself
			//
			dlg.onFilter(filter);

			//
			// Now set our sub-class variables
			//
			dlg.setCompression(TIFFCOMPRESSION(compression));
			dlg.setApply(apply);
			if (!selectionRect.isEmpty()) dlg.setUseRect(true);

			//
			// display the dialogue
			//
			if (QDialog::Accepted == dlg.exec())
			{
				fs::path file = dlg.selectedFiles().at(0).toStdU16String();

				apply = dlg.apply();
				compression = dlg.compression();
				bool useRect = dlg.useRect();
				DSSRect rect;			// Empty rectangle
				DSS::ProgressDlg progress{ DeepSkyStacker::instance() };


				//
				// If only wish to save the selected rectangle, copy it
				// to our working rectangle
				//
				if (useRect) rect = selectionRect;

				//
				// Save the image in the format the user has selected
				//
				auto index{ fileFilters.indexOf(dlg.selectedNameFilter()) };
				StackedBitmap& stackedBitmap{ dssApp->deepStack().GetStackedBitmap() };

				QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

				switch (index)
				{
				case 0:		// TIFF 16 bit
					stackedBitmap.SaveTIFF16Bitmap(file, rect, &progress, apply, compression);
					break;
				case 1:		// TIFF 32 bit integer
					stackedBitmap.SaveTIFF32Bitmap(file, rect, &progress, apply, false, compression);
					break;
				case 2:		// TIFF 32 bit rational
					stackedBitmap.SaveTIFF32Bitmap(file, rect, &progress, apply, true, compression);
					break;
				case 3:		// FITS 16 bit
					stackedBitmap.SaveFITS16Bitmap(file, rect, &progress, apply);
					break;
				case 4:		// FITS 32 bit integer
					stackedBitmap.SaveFITS32Bitmap(file, rect, &progress, apply, false);
					break;
				case 5:		// FITS 32 bit rational
					stackedBitmap.SaveFITS32Bitmap(file, rect, &progress, apply, true);
					break;
				}

				if (file.has_parent_path())
					directory = QString::fromStdU16String(file.parent_path().generic_u16string());
				else
					directory = QString::fromStdU16String(file.root_path().generic_u16string());

				extension = QString::fromStdU16String(file.extension().generic_u16string());

				settings.setValue("Folders/SavePictureFolder", directory);
				settings.setValue("Folders/SavePictureExtension", extension);
				settings.setValue("Folders/SavePictureIndex", index);
				settings.setValue("Folders/SaveApplySetting", apply);
				settings.setValue("Folders/SaveCompression", (uint)compression);

				QGuiApplication::restoreOverrideCursor();
				currentFile = file;
				updateInformation();
				setDirty(false);
				result = true;

			}
		}
		else
		{
			QMessageBox::information(this, "DeepSkyStacker",
				tr("There is no picture to save.", "IDS_MSG_NOPICTURETOSAVE"));
		}

		return result;
	}

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::updateInformation()
	{
		ZFUNCTRACE_RUNTIME();
		int		isoSpeed;
		int		gain;
		int		totalTime;
		int		nrFrames;
		QString text{ tr("No information available", "IDS_NOINFO") };

		if (!currentFile.empty())
		{
			StackedBitmap& bmp{ dssApp->deepStack().GetStackedBitmap() };
			isoSpeed = bmp.GetISOSpeed();
			gain = bmp.GetGain();
			totalTime = bmp.GetTotalTime();
			nrFrames = bmp.GetNrStackedFrames();
			QString isoText, gainText, timeText, framesText;

			if (isoSpeed) isoText = QString("%1 ISO - ").arg(isoSpeed);
			if (gain >= 0) gainText = QString("%1 Gain - ").arg(gain);
			if (totalTime) timeText = tr("Exposure: %1 ").arg(exposureToString(totalTime));
			if (nrFrames) framesText = tr("%n frames", "IDS_NRFRAMES", nrFrames);

			text = QString("%1\n%2%3%4(%5)").arg(currentFile.generic_u8string().c_str())
				.arg(isoText)
				.arg(gainText)
				.arg(timeText)
				.arg(framesText);
		}

		information->setText(text);
	}

	void ProcessingDlg::updateControlsFromSettings()
	{
		ZFUNCTRACE_RUNTIME();

		//
		// Position the controls to match the current settings
		//
		darkAngle->setValue(processingSettings.bezierAdjust_.m_fDarknessAngle);
		darkPower->setValue(processingSettings.bezierAdjust_.m_fDarknessPower * 10.0);
		updateDarkText();


		midAngle->setValue(processingSettings.bezierAdjust_.m_fMidtoneAngle);
		midTone->setValue(processingSettings.bezierAdjust_.m_fMidtone * 10.0);
		updateMidText();

		highAngle->setValue(processingSettings.bezierAdjust_.m_fHighlightAngle);
		highPower->setValue(processingSettings.bezierAdjust_.m_fHighlightPower * 10.0);
		updateHighText();

		saturation->setValue(processingSettings.bezierAdjust_.m_fSaturationShift);
		updateSaturationText();

		double	fMinRed, fMaxRed, fShiftRed;

		double	fMinGreen, fMaxGreen, fShiftGreen;

		double	fMinBlue, fMaxBlue,	fShiftBlue;

		fMinRed = processingSettings.histoAdjust_.GetRedAdjust().GetMin();
		fMaxRed = processingSettings.histoAdjust_.GetRedAdjust().GetMax();
		fShiftRed = processingSettings.histoAdjust_.GetRedAdjust().GetShift();

		fMinGreen = processingSettings.histoAdjust_.GetGreenAdjust().GetMin();
		fMaxGreen = processingSettings.histoAdjust_.GetGreenAdjust().GetMax();
		fShiftGreen = processingSettings.histoAdjust_.GetGreenAdjust().GetShift();

		fMinBlue = processingSettings.histoAdjust_.GetBlueAdjust().GetMin();
		fMaxBlue = processingSettings.histoAdjust_.GetBlueAdjust().GetMax();
		fShiftBlue = processingSettings.histoAdjust_.GetBlueAdjust().GetShift();

		double	fAbsMin, fAbsMax;
		double	fOffset;
		double	fRange;

		fAbsMin = min(fMinRed, min(fMinGreen, fMinBlue));
		fAbsMax = max(fMaxRed, min(fMaxGreen, fMaxBlue));

		fRange = fAbsMax - fAbsMin;
		if (fRange * 1.10 <= 65535.0)
			fRange *= 1.10;

		fOffset = (fAbsMin + fAbsMax - fRange) / 2.0;
		if (fOffset < 0)
			fOffset = 0.0;

		gradientOffset_ = fOffset;
		gradientRange_ = fRange;

		redGradient->setPeg(1, (float)((fMinRed - gradientOffset_) / gradientRange_));
		redGradient->setPeg(2, (float)(fShiftRed / 2.0 + 0.5));
		redGradient->setPeg(3, (float)((fMaxRed - gradientOffset_) / gradientRange_));
		setRedAdjustmentCurve(processingSettings.histoAdjust_.GetRedAdjust().GetAdjustMethod());

		redGradient->update();

		greenGradient->setPeg(1, (float)((fMinGreen - gradientOffset_) / gradientRange_));
		greenGradient->setPeg(2, (float)(fShiftGreen / 2.0 + 0.5));
		greenGradient->setPeg(3, (float)((fMaxGreen - gradientOffset_) / gradientRange_));
		setGreenAdjustmentCurve(processingSettings.histoAdjust_.GetGreenAdjust().GetAdjustMethod());

		greenGradient->update();

		blueGradient->setPeg(1, (float)((fMinBlue - gradientOffset_) / gradientRange_));
		blueGradient->setPeg(2, (float)(fShiftBlue / 2.0 + 0.5));
		blueGradient->setPeg(3, (float)((fMaxBlue - gradientOffset_) / gradientRange_));
		setBlueAdjustmentCurve(processingSettings.histoAdjust_.GetBlueAdjust().GetAdjustMethod());
		blueGradient->update();

	};

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::showHistogram(bool useLogarithm)
	{
		// Adjust Histogram
		RGBHistogram			Histo;
		RGBHistogramAdjust		HistoAdjust;

		Histo.SetSize(65535.0, histogram->width());

		const QGradientStops& redStops = redGradient->gradient().stops();
		const QGradientStops& greenStops = greenGradient->gradient().stops();
		const QGradientStops& blueStops = blueGradient->gradient().stops();


		double
			fMinRed = gradientOffset_ + redStops[1].first * gradientRange_,
			fShiftRed = (redStops[2].first - 0.5) * 2.0,
			fMaxRed = gradientOffset_ + redStops[3].first * gradientRange_;


		double
			fMinGreen = gradientOffset_ + greenStops[1].first * gradientRange_,
			fShiftGreen = (greenStops[2].first - 0.5) * 2.0,
			fMaxGreen = gradientOffset_ + greenStops[3].first * gradientRange_;


		double
			fMinBlue = gradientOffset_ + blueStops[1].first * gradientRange_,
			fShiftBlue = (blueStops[2].first - 0.5) * 2.0,
			fMaxBlue = gradientOffset_ + blueStops[3].first * gradientRange_;

		HistoAdjust.GetRedAdjust().SetAdjustMethod(redAdjustmentCurve());
		HistoAdjust.GetRedAdjust().SetNewValues(fMinRed, fMaxRed, fShiftRed);
		HistoAdjust.GetGreenAdjust().SetAdjustMethod(greenAdjustmentCurve());
		HistoAdjust.GetGreenAdjust().SetNewValues(fMinGreen, fMaxGreen, fShiftGreen);
		HistoAdjust.GetBlueAdjust().SetAdjustMethod(blueAdjustmentCurve());
		HistoAdjust.GetBlueAdjust().SetNewValues(fMinBlue, fMaxBlue, fShiftBlue);

		dssApp->deepStack().AdjustOriginalHistogram(Histo, HistoAdjust);

		drawHistogram(Histo, useLogarithm);
	}

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::drawHistoBar(QPainter& painter, int lNrReds, int lNrGreens, int lNrBlues, int X, int lHeight)
	{
		std::vector<ColorOrder>	vColors;
		int						lLastHeight = 0;

		vColors.emplace_back(qRgb(255, 0, 0), lNrReds);
		vColors.emplace_back(qRgb(0, 255, 0), lNrGreens);
		vColors.emplace_back(qRgb(0, 0, 255), lNrBlues);

		std::sort(vColors.begin(), vColors.end());

		for (int i = 0; i < vColors.size(); i++)
		{
			if (vColors[i].m_lSize > lLastHeight)
			{
				// Create a color from the remaining values
				double	fRed, fGreen, fBlue;
				int		lNrColors = 1;

				fRed = qRed(vColors[i].m_crColor);		// Get the red component of the colour
				fGreen = qGreen(vColors[i].m_crColor);	// Get the green component of the colour
				fBlue = qBlue(vColors[i].m_crColor);	// Get the blue component of the colour

				for (int j = i + 1; j < vColors.size(); j++)
				{
					fRed += qRed(vColors[j].m_crColor);		// Get the red component of the colour
					fGreen += qGreen(vColors[j].m_crColor);	// Get the green component of the colour
					fBlue += qBlue(vColors[j].m_crColor);	// Get the blue component of the colour
					lNrColors++;
				};

				QPen colorPen(QColor(fRed / lNrColors, fGreen / lNrColors, fBlue / lNrColors));
				painter.setPen(colorPen);

				painter.drawLine(X, lHeight - lLastHeight, X, lHeight - vColors[i].m_lSize);

				lLastHeight = vColors[i].m_lSize;
			};
		};
	}

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::drawGaussianCurves(QPainter& painter, RGBHistogram& histo, int lWidth, int lHeight)
	{
		int				lNrValues;
		double				fAverage[3] = { 0, 0, 0 };
		double				fStdDev[3] = { 0, 0, 0 };
		double				fSum[3] = { 0, 0, 0 };
		double				fSquareSum[3] = { 0, 0, 0 };
		double				fTotalPixels[3] = { 0, 0, 0 };
		int				i;

		lNrValues = histo.GetRedHistogram().GetNrValues();

		if (lNrValues)
		{
			for (i = 0; i < lNrValues; i++)
			{

				int			lNrReds;
				int			lNrGreens;
				int			lNrBlues;

				histo.GetValues(i, lNrReds, lNrGreens, lNrBlues);

				fSum[0] += lNrReds * i;
				fSum[1] += lNrGreens * i;
				fSum[2] += lNrBlues * i;
				fTotalPixels[0] += lNrReds;
				fTotalPixels[1] += lNrGreens;
				fTotalPixels[2] += lNrBlues;
			};

			fAverage[0] = fSum[0] / fTotalPixels[0];
			fAverage[1] = fSum[1] / fTotalPixels[1];
			fAverage[2] = fSum[2] / fTotalPixels[2];

			for (i = 0; i < lNrValues; i++)
			{
				int			lNrReds;
				int			lNrGreens;
				int			lNrBlues;

				histo.GetValues(i, lNrReds, lNrGreens, lNrBlues);

				fSquareSum[0] += pow(i - fAverage[0], 2) * lNrReds;
				fSquareSum[1] += pow(i - fAverage[1], 2) * lNrGreens;
				fSquareSum[2] += pow(i - fAverage[2], 2) * lNrBlues;
			};

			fStdDev[0] = sqrt(fSquareSum[0] / fTotalPixels[0]);
			fStdDev[1] = sqrt(fSquareSum[1] / fTotalPixels[1]);
			fStdDev[2] = sqrt(fSquareSum[2] / fTotalPixels[2]);

			//
			// Create the list of points with the initial size set correctly.
			//
			QList<QPointF>	redPoints{ lNrValues };
			QList<QPointF>	greenPoints{ lNrValues };
			QList<QPointF>	bluePoints{ lNrValues };

			bool				bShow = true;

			for (i = 0; i < lNrValues; i++)
			{
				double		fX,
					fY;
				fX = i;

				fY = exp(-(fX - fAverage[0]) * (fX - fAverage[0]) / (fStdDev[0] * fStdDev[0] * 2)) * lWidth / lNrValues;
				fY = lHeight - fY * lHeight;
				redPoints.emplace_back(fX, fY);

				bShow = bShow && (fX < 1000 && fY < 1000);

				fY = exp(-(fX - fAverage[1]) * (fX - fAverage[1]) / (fStdDev[1] * fStdDev[1] * 2)) * lWidth / lNrValues;
				fY = lHeight - fY * lHeight;
				greenPoints.emplace_back(fX, fY);

				bShow = bShow && (fX < 1000 && fY < 1000);

				fY = exp(-(fX - fAverage[2]) * (fX - fAverage[2]) / (fStdDev[2] * fStdDev[2] * 2)) * lWidth / lNrValues;
				fY = lHeight - fY * lHeight;
				bluePoints.emplace_back(fX, fY);

				bShow = bShow && (fX < 1000 && fY < 1000);
			};

			QPen redPen{ qRgba(255, 0, 0, 128) };
			QPen greenPen{ qRgba(0, 255, 0, 128) };
			QPen bluePen{ qRgba(0, 0, 255, 128) };

			redPen.setStyle(Qt::DashLine);
			greenPen.setStyle(Qt::DashLine);
			bluePen.setStyle(Qt::DashLine);

			if (bShow)
			{
				QPen oldPen = painter.pen();
				painter.setPen(redPen); painter.drawLines(redPoints);
				painter.setPen(greenPen); painter.drawLines(greenPoints);
				painter.setPen(bluePen); painter.drawLines(bluePoints);
				painter.setPen(oldPen);
			};
		};
	};

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::drawBezierCurve(QPainter& painter, int width, int height)
	{
		BezierAdjust		bezierAdjust;
		QPointF				point;

		bezierAdjust.m_fMidtone = midTone->value() / 10.0;
		bezierAdjust.m_fMidtoneAngle = midAngle->value();
		bezierAdjust.m_fDarknessAngle = darkAngle->value();
		bezierAdjust.m_fHighlightAngle = highAngle->value();
		bezierAdjust.m_fHighlightPower = highPower->value() / 10.0;
		bezierAdjust.m_fDarknessPower = darkPower->value() / 10.0;

		bezierAdjust.clear();

		//
		// Create a pen from the current window text color (which depends on 
		// the dark/light colour theme setting).
		//
		QPen pen(palette().color(QPalette::WindowText));

		//
		// Create the points array for the curve
		//
		QList<QPointF>	points{ 100 };

		pen.setStyle(Qt::DashLine);

		for (double i = 0; i <= 1.0; i += 0.01)
		{
			double	j;

			j = bezierAdjust.GetValue(i);
			points.emplace_back(i * width, height - j * height);
		};
		QPen oldPen{ painter.pen() };
		painter.setPen(pen);
		painter.drawLines(points);
		painter.setPen(oldPen);
	};


	void ProcessingDlg::drawHistogram(RGBHistogram& Histogram, bool useLogarithm)
	{
		QPixmap pix(histogram->size());
		QPainter painter;
		QBrush brush(palette().button());  // QPalette::Window was too dark - use QPalette::Button instead
		const QRect histogramRect{ histogram->rect() };
		const int width{ histogramRect.width() };
		const int height{ histogramRect.height() };

		painter.begin(&pix);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);

		painter.fillRect(histogramRect, brush);

		double	maxLogarithm = 0.0;

		int				lNrValues;
		int				lMaxValue = 0;

		lMaxValue = max(lMaxValue, Histogram.GetRedHistogram().GetMaximumNrValues());
		lMaxValue = max(lMaxValue, Histogram.GetGreenHistogram().GetMaximumNrValues());
		lMaxValue = max(lMaxValue, Histogram.GetBlueHistogram().GetMaximumNrValues());

		lNrValues = Histogram.GetRedHistogram().GetNrValues();

		if (lNrValues)
		{
			if (useLogarithm)
			{
				if (lMaxValue)
					maxLogarithm = exp(log((double)lMaxValue) / height);
				else
					useLogarithm = false;
			};

			for (int i = 0; i < lNrValues; i++)
			{
				int			lNrReds;
				int			lNrGreens;
				int			lNrBlues;

				Histogram.GetValues(i, lNrReds, lNrGreens, lNrBlues);

				if (useLogarithm)
				{
					if (lNrReds)
						lNrReds = log((double)lNrReds) / log(maxLogarithm);
					if (lNrGreens)
						lNrGreens = log((double)lNrGreens) / log(maxLogarithm);
					if (lNrBlues)
						lNrBlues = log((double)lNrBlues) / log(maxLogarithm);
				}
				else
				{
					lNrReds = (double)lNrReds / (double)lMaxValue * height;
					lNrGreens = (double)lNrGreens / (double)lMaxValue * height;
					lNrBlues = (double)lNrBlues / (double)lMaxValue * height;
				};

				drawHistoBar(painter, lNrReds, lNrGreens, lNrBlues, i, height);
			};


		}
		drawGaussianCurves(painter, Histogram, width, height);
		drawBezierCurve(painter, width, height);

		painter.end();
		histogram->setPixmap(pix);
	}

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::resetSliders()
	{
		RGBHistogram& Histogram = dssApp->deepStack().GetOriginalHistogram();

		processingSettings.bezierAdjust_.reset();

		float				RedMarks[3];
		float				GreenMarks[3];
		float				BlueMarks[3];

		gradientOffset_ = 0.0;
		gradientRange_ = 65535.0;

		RedMarks[0] = Histogram.GetRedHistogram().GetMin();
		GreenMarks[0] = Histogram.GetGreenHistogram().GetMin();
		BlueMarks[0] = Histogram.GetBlueHistogram().GetMin();

		RedMarks[2] = Histogram.GetRedHistogram().GetMax();
		GreenMarks[2] = Histogram.GetGreenHistogram().GetMax();
		BlueMarks[2] = Histogram.GetBlueHistogram().GetMax();


		redGradient->setPeg(1, (float)((RedMarks[0] - gradientOffset_) / gradientRange_));
		redGradient->setPeg(2, (float)0.5);
		redGradient->setPeg(3, (float)((RedMarks[2] - gradientOffset_) / gradientRange_));
		redGradient->update();
		setRedAdjustmentCurve(processingSettings.histoAdjust_.GetRedAdjust().GetAdjustMethod());

		greenGradient->setPeg(1, (float)((GreenMarks[0] - gradientOffset_) / gradientRange_));
		greenGradient->setPeg(2, (float)0.5);
		greenGradient->setPeg(3, (float)((GreenMarks[2] - gradientOffset_) / gradientRange_));
		greenGradient->update();
		setGreenAdjustmentCurve(processingSettings.histoAdjust_.GetGreenAdjust().GetAdjustMethod());

		blueGradient->setPeg(1, (float)((BlueMarks[0] - gradientOffset_) / gradientRange_));
		blueGradient->setPeg(2, (float)0.5);
		blueGradient->setPeg(3, (float)((BlueMarks[2] - gradientOffset_) / gradientRange_));
		blueGradient->update();
		setBlueAdjustmentCurve(processingSettings.histoAdjust_.GetBlueAdjust().GetAdjustMethod());

		//
		// Position the controls to match the current settings
		//
		darkAngle->setValue(processingSettings.bezierAdjust_.m_fDarknessAngle);
		darkPower->setValue(processingSettings.bezierAdjust_.m_fDarknessPower * 10.0);
		updateDarkText();


		midAngle->setValue(processingSettings.bezierAdjust_.m_fMidtoneAngle);
		midTone->setValue(processingSettings.bezierAdjust_.m_fMidtone * 10.0);
		updateMidText();

		highAngle->setValue(processingSettings.bezierAdjust_.m_fHighlightAngle);
		highPower->setValue(processingSettings.bezierAdjust_.m_fHighlightPower * 10.0);
		updateHighText();

		saturation->setValue(processingSettings.bezierAdjust_.m_fSaturationShift);
		updateSaturationText();

		showHistogram(false);
	};

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::UpdateHistogramAdjust()
	{

		const QGradientStops& redStops{ redGradient->gradient().stops() };
		const QGradientStops& greenStops{ greenGradient->gradient().stops() };
		const QGradientStops& blueStops{ blueGradient->gradient().stops() };

		double
			fMinRed = gradientOffset_ + redStops[1].first * gradientRange_,
			fShiftRed = (redStops[2].first - 0.5) * 2.0,
			fMaxRed = gradientOffset_ + redStops[3].first * gradientRange_;

		double
			fMinGreen = gradientOffset_ + greenStops[1].first * gradientRange_,
			fShiftGreen = (greenStops[2].first - 0.5) * 2.0,
			fMaxGreen = gradientOffset_ + greenStops[3].first * gradientRange_;

		double
			fMinBlue = gradientOffset_ + blueStops[1].first * gradientRange_,
			fShiftBlue = (blueStops[2].first - 0.5) * 2.0,
			fMaxBlue = gradientOffset_ + blueStops[3].first * gradientRange_;

		processingSettings.histoAdjust_.GetRedAdjust().SetNewValues(fMinRed, fMaxRed, fShiftRed);
		processingSettings.histoAdjust_.GetGreenAdjust().SetNewValues(fMinGreen, fMaxGreen, fShiftGreen);
		processingSettings.histoAdjust_.GetBlueAdjust().SetNewValues(fMinBlue, fMaxBlue, fShiftBlue);

		processingSettings.histoAdjust_.GetRedAdjust().SetAdjustMethod(redAdjustmentCurve());
		processingSettings.histoAdjust_.GetGreenAdjust().SetAdjustMethod(greenAdjustmentCurve());
		processingSettings.histoAdjust_.GetBlueAdjust().SetAdjustMethod(blueAdjustmentCurve());


		// Update gradient adjust values
		double				fAbsMin,
			fAbsMax;
		double				fOffset;
		double				fRange;

		fAbsMin = min(fMinRed, min(fMinGreen, fMinBlue));
		fAbsMax = max(fMaxRed, min(fMaxGreen, fMaxBlue));

		fRange = fAbsMax - fAbsMin;
		if (fRange * 1.10 <= 65535.0)
			fRange *= 1.10;

		fOffset = (fAbsMin + fAbsMax - fRange) / 2.0;
		if (fOffset < 0)
			fOffset = 0.0;

		gradientOffset_ = fOffset;
		gradientRange_ = fRange;

		redGradient->setPeg(1, (float)((fMinRed - gradientOffset_) / gradientRange_));
		redGradient->setPeg(2, (float)(fShiftRed / 2.0 + 0.5));
		redGradient->setPeg(3, (float)((fMaxRed - gradientOffset_) / gradientRange_));
		redGradient->update();

		greenGradient->setPeg(1, (float)((fMinGreen - gradientOffset_) / gradientRange_));
		greenGradient->setPeg(2, (float)(fShiftGreen / 2.0 + 0.5));
		greenGradient->setPeg(3, (float)((fMaxGreen - gradientOffset_) / gradientRange_));
		greenGradient->update();

		blueGradient->setPeg(1, (float)((fMinBlue - gradientOffset_) / gradientRange_));
		blueGradient->setPeg(2, (float)(fShiftBlue / 2.0 + 0.5));
		blueGradient->setPeg(3, (float)((fMaxBlue - gradientOffset_) / gradientRange_));
		blueGradient->update();
	};

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::processAndShow(bool bSaveUndo)
	{
		UpdateHistogramAdjust();

		processingSettings.bezierAdjust_.m_fMidtone = midTone->value() / 10.0;
		processingSettings.bezierAdjust_.m_fMidtoneAngle = midAngle->value();
		processingSettings.bezierAdjust_.m_fDarknessAngle = darkAngle->value();
		processingSettings.bezierAdjust_.m_fHighlightAngle = highAngle->value();
		processingSettings.bezierAdjust_.m_fHighlightPower = highPower->value() / 10.0;
		processingSettings.bezierAdjust_.m_fDarknessPower = darkPower->value() / 10.0;
		processingSettings.bezierAdjust_.m_fSaturationShift = saturation->value();
		processingSettings.bezierAdjust_.clear();

		if (bSaveUndo)
			processingSettingsList.AddParams(processingSettings);

		updateControls();

		//
		// selectionRect is set whenever signal SelectRect::selectRectChanged is emitted
		// It will be the null rectangle when no selection has been made by the user
		// 
		rectToProcess.SetProcessRect(selectionRect);

		rectToProcess.Reset();
	};

	/* ------------------------------------------------------------------- */

	bool ProcessingDlg::askToSave()
	{
		ZFUNCTRACE_RUNTIME();
		//
		// The existing image is being closed and has been changed, ask the user if they want to save it
		//
		if (dirty_)
		{
			QString message{ tr("Do you want to save the modifications?", "IDS_MSG_SAVEMODIFICATIONS") };
			auto result = QMessageBox::question(this, "DeepSkyStacker",
				message, (QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel), QMessageBox::No);
			switch (result)
			{
			case QMessageBox::Cancel:
				return false;
				break;
			case QMessageBox::No:
				return true;
				break;
			default:
				return saveImage();
			}
		}
		else return true;
	};


	//
	// Slots
	//

	void ProcessingDlg::onApply()
	{
		processAndShow(true);
	}

	void ProcessingDlg::onUndo()
	{
		processingSettingsList.MoveBackward();
		processingSettingsList.GetCurrentSettings(processingSettings);
		updateControlsFromSettings();
		processAndShow(false);
		showHistogram(false);
		//updateControls();
	}

	void ProcessingDlg::onRedo()
	{
		processingSettingsList.MoveForward();
		processingSettingsList.GetCurrentSettings(processingSettings);
		updateControlsFromSettings();
		processAndShow(false);
		showHistogram(false);
		//updateControls();
	};

	void ProcessingDlg::onReset()
	{
		setDirty();
		resetSliders();
	}


	void ProcessingDlg::onSettings()
	{
		//
		// Copy the current ProcessingSettings to the dialog
		// 
		ProcessingSettingsDlg dlg(this, processingSettings);

		timer.stop();
		
		//
		// If the dialog was exited normally and the user changed to
		// a different settings ...
		//
		if (QDialog::Accepted == dlg.exec() && dlg.settingsChanged())
		{
			processingSettings = dlg.settings();
			updateControlsFromSettings();
			processAndShow(false);
			showHistogram(false);
			updateControls();
			setDirty();
		}
		timer.start();
	};

	void ProcessingDlg::updateBezierCurve()
	{
		setDirty();
		showHistogram();
	};

	void ProcessingDlg::darkAngleChanged()
	{
		updateDarkText();
		emit updateBezierCurve();
	}

	void ProcessingDlg::darkPowerChanged()
	{
		updateDarkText();
		emit updateBezierCurve();
	}

	void ProcessingDlg::midAngleChanged()
	{
		updateMidText();
		emit updateBezierCurve();
	}

	void ProcessingDlg::midToneChanged()
	{
		updateMidText();
		emit updateBezierCurve();
	}

	void ProcessingDlg::highAngleChanged()
	{
		updateHighText();
		emit updateBezierCurve();
	}

	void ProcessingDlg::highPowerChanged()
	{
		updateHighText();
		emit updateBezierCurve();
	}

	void ProcessingDlg::saturationChanged()
	{
		setDirty();
		updateSaturationText();
	}


	void ProcessingDlg::setSelectionRect(const QRectF& rect)
	{
		selectionRect = DSSRect(rect.x(), rect.y(), rect.right(), rect.bottom());
	}

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::onColorSchemeChanged(Qt::ColorScheme colorScheme)
	{
		iconModifier.clear();
		//
		// Dark colour scheme?
		//
		if (Qt::ColorScheme::Dark == colorScheme)
			iconModifier = "-dark";

		setButtonIcons();		// in RGBTab.cpp
		update();
	}

	/* ------------------------------------------------------------------- */

	void ProcessingDlg::onTimer()
	{
		DSSRect			cell;

		if (rectToProcess.GetNextUnProcessedRect(cell))
		{
			dssApp->deepStack().PartialProcess(cell, processingSettings.bezierAdjust_, processingSettings.histoAdjust_);

			picture->setPixmap(QPixmap::fromImage(dssApp->deepStack().getImage()));

			// showHistogram(false);
			//resetSliders();		// Will call showHistogram()

			const int nProgress = static_cast<int>(rectToProcess.GetPercentageComplete());
			progressBar->setValue(min(max(0, nProgress), 100));
		};
	}



} // namespace DSS
