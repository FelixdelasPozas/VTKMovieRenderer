/*
 File: MovieRenderer.cpp
 Created on: 11/05/2017
 Author: Felix de las Pozas Alvarez

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project
#include "MovieRenderer.h"

// Qt
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QObject>
#include <QEvent>
#include <QTimer>
#include <QSettings>
#include <QDebug>

// VTK
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>
#include <vtkWindowToImageFilter.h>
#include <vtkImageSincInterpolator.h>
#include <vtkImageResize.h>
#include <vtkPNGWriter.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>

// C++
#include <chrono>
#include <thread>

const QString VIDEO_4K_ENABLED         = "Video 4k enabled";
const QString VIDEO_HD_ENABLED         = "Video HD enabled";
const QString VIDEO_SD_ENABLED         = "Video SD enabled";
const QString POINT_SMOOTHING_ENABLE   = "Point smoothing enabled";
const QString LINE_SMOOTHING_ENABLE    = "Line smoothing enabled";
const QString POLYGON_SMOOTHING_ENABLE = "Polygon smoothing enabled";
const QString SHADOWS_ENABLED          = "Shadows enabled";
const QString MOTION_BLUR_ENABLED      = "Motion blur enabled";
const QString MOTION_BLUR_FRAMES       = "Motion blur frames";
const QString ANTIALIAS_ENABLED        = "Antialias enabled";
const QString ANTIALIAS_FRAMES         = "Antialias frames";
const QString AXES_SHOWN               = "Axes shown";
const QString OUTPUT_DIR               = "Output directory";
const QString FFMPEG_BINARY            = "FFMPEG binary";
const QString CAMERA_X_POS             = "Camera x position";
const QString CAMERA_Y_POS             = "Camera y position";
const QString CAMERA_Z_POS             = "Camera z position";
const QString CAMERA_FOCAL_X_POS       = "Camera focal point x position";
const QString CAMERA_FOCAL_Y_POS       = "Camera focal point y position";
const QString CAMERA_FOCAL_Z_POS       = "Camera focal point z position";
const QString CAMERA_ZOOM              = "Camera zoom";
const QString CAMERA_ROLL              = "Camera roll";

//--------------------------------------------------------------------
MovieRenderer::MovieRenderer()
: m_frameNum{0}
, m_loader{nullptr}
, m_executor{nullptr}
{
  setupUi(this);

  connectSignals();

  setupVTKView();

  restoreSettings();

  updateRendererSettings();

  m_render->setEnabled(false);

  showMaximized();

  QTimer::singleShot(100, this, SLOT(onReloadPressed()));
}

//--------------------------------------------------------------------
void MovieRenderer::onRenderPressed()
{
  if(m_render->text() == "Render")
  {
    if(!m_renderFull->isChecked() && !m_renderHalf->isChecked() && !m_render4K->isChecked())
    {
      QMessageBox msgbox;
      msgbox.setWindowIcon(QIcon(":/MovieRenderer/application.svg"));
      msgbox.setWindowTitle("Error starting Render");
      msgbox.setText(tr("At least one of these options must be enabled:\n - full hd\n - half hd\n - 4k"));
      msgbox.setIcon(QMessageBox::Icon::Critical);
      msgbox.exec();

      return;
    }

    if(m_ffmpegExe->text().isEmpty() || !QFileInfo{m_ffmpegExe->text()}.exists())
    {
      QMessageBox msgbox;
      msgbox.setWindowIcon(QIcon(":/MovieRenderer/application.svg"));
      msgbox.setWindowTitle("Error starting Render");
      msgbox.setText(tr("Invalid MMMPEG executable! Select a valid one"));
      msgbox.setIcon(QMessageBox::Icon::Critical);
      msgbox.exec();

      return;
    }

    if(!QDir{m_directory->text()}.exists())
    {
      QMessageBox msgbox;
      msgbox.setWindowIcon(QIcon(":/MovieRenderer/application.svg"));
      msgbox.setWindowTitle("Error starting Render");
      msgbox.setText(tr("The directory '%1' to store frames\ndoesn't exist! Select a valid directory.").arg(m_directory->text()));
      msgbox.setIcon(QMessageBox::Icon::Critical);
      msgbox.exec();

      return;
    }

    startRender();
  }
  else
  {
    stopRender();
  }
}

//--------------------------------------------------------------------
void MovieRenderer::stopRender()
{
  statusBar()->showMessage(tr("Rendering cancelled."));

  m_executor->abort();

  modifyUI(true);

  QApplication::processEvents();

  m_frameNum = 0;
}

//--------------------------------------------------------------------
void MovieRenderer::startRender()
{
  statusBar()->showMessage(tr("Start rendering frames."));

  m_frameNum = 0;

  modifyUI(false);

  updateRendererSettings();

  renderScript();
}

//--------------------------------------------------------------------
void MovieRenderer::updateRendererSettings()
{
  auto renderWindow = m_renderer->GetRenderWindow();

  //m_renderer->SetUseShadows(m_shadows->isChecked());

  renderWindow->SetPointSmoothing(m_pointSmoothing->isChecked());
  renderWindow->SetLineSmoothing(m_lineSmoothing->isChecked());
  renderWindow->SetPolygonSmoothing(m_polygonSmoothing->isChecked());

  if(m_motionBlur->isChecked())
  {
    renderWindow->SetSubFrames(m_motionBlurFrames->value());
  }
  else
  {
    renderWindow->SetSubFrames(0);
  }

  if(m_antiAlias->isChecked())
  {
    renderWindow->SetMultiSamples(m_aliasFrames->value());
  }
  else
  {
    renderWindow->SetMultiSamples(0);
  }
}

//--------------------------------------------------------------------
void MovieRenderer::connectSignals()
{
  connect(m_quit, SIGNAL(pressed()), this, SLOT(close()));
  connect(m_render, SIGNAL(pressed()), this, SLOT(onRenderPressed()));
  connect(m_motionBlur, SIGNAL(stateChanged(int)), this, SLOT(onMotionBlurChanged(int)));
  connect(m_antiAlias, SIGNAL(stateChanged(int)), this, SLOT(onAntiAliasChanged(int)));
  connect(m_dirButton, SIGNAL(pressed()), this, SLOT(onDirButtonPressed()));
  connect(m_ffmpegDir, SIGNAL(pressed()), this, SLOT(onFFMPEGDirButtonPressed()));
  connect(m_resetCamera, SIGNAL(pressed()), this, SLOT(onCameraResetPressed()));
  connect(m_reloadResources, SIGNAL(pressed()), this, SLOT(onReloadPressed()));
  connect(m_axes, SIGNAL(stateChanged(int)), this, SLOT(onAxesValueChanged(int)));
  connect(m_lineSmoothing, SIGNAL(stateChanged(int)), this, SLOT(updateRendererSettings()));
  connect(m_pointSmoothing, SIGNAL(stateChanged(int)), this, SLOT(updateRendererSettings()));
  connect(m_polygonSmoothing, SIGNAL(stateChanged(int)), this, SLOT(updateRendererSettings()));
  connect(m_saveCamera, SIGNAL(pressed()), this, SLOT(saveCameraPosition()));
}

//--------------------------------------------------------------------
void MovieRenderer::onResourcesLoaded()
{
  auto loader = qobject_cast<ResourceLoaderThread *>(sender());

  if(loader && !loader->isAborted())
  {
    if(!loader->getError().isEmpty())
    {
      auto title = QString("Error loading resources");
      errorDialog(title, loader->getError());
      return;
    }

    statusBar()->showMessage("Resources loaded");

    // prepare script to run
    m_executor = std::make_shared<ScriptExecutor>(m_renderer, loader, this);
    connect(m_executor.get(), SIGNAL(finished()), this, SLOT(onScriptFinished()));
    connect(m_executor.get(), SIGNAL(render()), this, SLOT(onRenderSignaled()));

    if(!m_executor->getError().isEmpty())
    {
      errorDialog(tr("Error loading resources."), m_executor->getError());
    }
  }
  else
  {
    if(!loader)
    {
      errorDialog(tr("Error loading resources."), tr("Invalid sender pointer."));
    }
    else
    {
      errorDialog(tr("Loading aborted"), tr("Loading process was aborted."));
    }
  }

  onCameraResetPressed();

  m_render->setEnabled(true);
  m_resetCamera->setEnabled(true);
  m_reloadResources->setEnabled(true);

  m_loader = nullptr;
}

//--------------------------------------------------------------------
void MovieRenderer::setupVTKView()
{
  m_view->show();
  m_renderer = vtkSmartPointer<vtkRenderer>::New();
  m_renderer->LightFollowCameraOn();
  m_renderer->SetUseFXAA(false);
  m_renderer->GetActiveCamera(); // creates default camera.

  auto interactorstyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  interactorstyle->AutoAdjustCameraClippingRangeOn();
  interactorstyle->KeyPressActivationOff();

  auto renderWindow = m_view->GetRenderWindow();
  renderWindow->AddRenderer(m_renderer);
  renderWindow->GetInteractor()->SetInteractorStyle(interactorstyle);

  // Color background
  QPalette pal = this->palette();
  pal.setColor(QPalette::Base, pal.color(QPalette::Window));
  this->setPalette(pal);

  auto axes = vtkSmartPointer<vtkAxesActor>::New();
  axes->DragableOff();
  axes->PickableOff();

  m_axesWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
  m_axesWidget->SetOrientationMarker(axes);
  m_axesWidget->SetInteractor(renderWindow->GetInteractor());
  m_axesWidget->SetViewport(0.0, 0.0, 0.3, 0.3);
  m_axesWidget->SetEnabled(true);
  m_axesWidget->InteractiveOff();
  m_axesWidget->SetEnabled(false);

  m_view->installEventFilter(this);
}

//--------------------------------------------------------------------
void MovieRenderer::onMotionBlurChanged(int value)
{
  m_motionBlurFrames->setEnabled(m_motionBlur->isChecked());

  updateRendererSettings();
}

//--------------------------------------------------------------------
void MovieRenderer::onAntiAliasChanged(int int1)
{
  m_aliasFrames->setEnabled(m_antiAlias->isChecked());

  updateRendererSettings();
}

//--------------------------------------------------------------------
void MovieRenderer::renderScript()
{
  if(m_executor->isRunning())
  {
    m_executor->abort();

    QApplication::processEvents();
  }

  m_executor->restart();
  m_executor->start();
}

//--------------------------------------------------------------------
void MovieRenderer::onDirButtonPressed()
{
  auto dir = QFileDialog::getExistingDirectory(centralWidget(), tr("Open Directory"), m_directory->text(), QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);

  m_directory->setText(QDir::toNativeSeparators(dir));
}

//--------------------------------------------------------------------
void MovieRenderer::modifyUI(bool value)
{
  m_videoGroup->setEnabled(value);
  m_rendererGroup->setEnabled(value);
  m_outputGroup->setEnabled(value);
  m_ffmpegGroup->setEnabled(value);
  m_resetCamera->setEnabled(value);
  m_reloadResources->setEnabled(value);

  if(value)
  {
    m_render->setText("Render");
  }
  else
  {
    m_render->setText("Stop");
  }
}

//--------------------------------------------------------------------
void MovieRenderer::onAxesValueChanged(int value)
{
  auto enabled = value == Qt::Checked;
  if(enabled != m_axesWidget->GetEnabled())
  {
    m_axesWidget->SetEnabled(value);
    m_renderer->GetRenderWindow()->Render();
    m_view->update();
  }
}

//--------------------------------------------------------------------
void MovieRenderer::errorDialog(const QString &title, const QString& message)
{
  QMessageBox msgbox;
  msgbox.setWindowIcon(QIcon(":/MovieRenderer/application.svg"));
  msgbox.setWindowTitle(title);
  msgbox.setText(message);
  msgbox.setIcon(QMessageBox::Icon::Critical);

  msgbox.exec();
}

//--------------------------------------------------------------------
void MovieRenderer::onRenderSignaled()
{
  if(m_executor->isFinished()) return;

  m_view->update();
  auto renderWindow = m_view->GetRenderWindow();

  auto outputDir = QDir::toNativeSeparators(m_directory->text() + "/");

  if(m_renderFull->isChecked() || m_renderHalf->isChecked())
  {
    // Screenshot
    auto windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
    windowToImageFilter->SetInput(renderWindow);
    windowToImageFilter->SetMagnification(1);
    windowToImageFilter->SetFixBoundary(true);
    windowToImageFilter->SetInputBufferTypeToRGBA();
    windowToImageFilter->Update();

    auto screenshot = windowToImageFilter->GetOutput();

    if(m_renderFull->isChecked()) // 1280x720
    {
      auto name = outputDir + QString("Frame_HD_%1.png").arg(QString::number(m_frameNum), 5, QChar('0'));

      auto writer = vtkSmartPointer<vtkPNGWriter>::New();
      writer->SetFileName(name.toStdString().c_str());
      writer->SetInputData(screenshot);
      writer->Write();
    }

    if(m_renderHalf->isChecked()) // 640x360
    {
      auto interpolator = vtkSmartPointer<vtkImageSincInterpolator>::New();
      interpolator->SetWindowFunctionToLanczos();
      interpolator->AntialiasingOn();

      auto resize = vtkSmartPointer<vtkImageResize>::New();
      resize->InterpolateOn();
      resize->SetInterpolator(interpolator);
      resize->SetInputData(screenshot);
      resize->SetOutputDimensions(1280/2, 720/2, 1);
      resize->Update();

      auto name = outputDir + QString("Frame_Half_%1.png").arg(QString::number(m_frameNum), 5, QChar('0'));

      auto writer = vtkSmartPointer<vtkPNGWriter>::New();
      writer->SetFileName(name.toStdString().c_str());
      writer->SetInputData(resize->GetOutput());
      writer->Write();
    }
  }

  if(m_render4K->isChecked()) // 3840x2160
  {
    // Screenshot
    auto windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
    windowToImageFilter->SetInput(renderWindow);
    windowToImageFilter->SetMagnification(3);
    windowToImageFilter->SetFixBoundary(true);
    windowToImageFilter->SetInputBufferTypeToRGBA();
    windowToImageFilter->Update();

    auto name = outputDir + QString("Frame_4K_%1.png").arg(QString::number(m_frameNum), 5, QChar('0'));

    auto writer = vtkSmartPointer<vtkPNGWriter>::New();
    writer->SetFileName(name.toStdString().c_str());
    writer->SetInputData(windowToImageFilter->GetOutput());
    writer->Write();
  }

  statusBar()->showMessage(tr("Wrote frame number %1").arg(QString::number(m_frameNum)));

  ++m_frameNum;

  QApplication::processEvents();

  m_executor->nextFrame();
}

//--------------------------------------------------------------------
void MovieRenderer::onFFMPEGDirButtonPressed()
{
  auto dir = QFileDialog::getOpenFileName(centralWidget(), tr("Open FFMPEG executable location"), QDir::homePath(), tr("Executables (*.exe)"), 0, QFileDialog::DontResolveSymlinks|QFileDialog::ReadOnly);

  m_ffmpegExe->setText(QDir::toNativeSeparators(dir));
}

//--------------------------------------------------------------------
void MovieRenderer::onReloadPressed()
{
  statusBar()->showMessage("Cleaning view and launching resources loader");

  m_renderer->RemoveAllViewProps();

  m_reloadResources->setEnabled(false);
  m_resetCamera->setEnabled(false);

  if(m_loader)
  {
    disconnect(m_loader.get(), SIGNAL(finished()), this, SLOT(onResourcesLoaded()));
    m_loader = nullptr;
  }

  m_loader = std::make_shared<ResourceLoaderThread>(this);

  connect(m_loader.get(), SIGNAL(finished()), this, SLOT(onResourcesLoaded()));

  m_loader->start();
}

//--------------------------------------------------------------------
void MovieRenderer::onCameraResetPressed()
{
  auto name = QApplication::applicationDirPath() + "/VTKMovieRenderer.ini";
  QSettings settings(name, QSettings::IniFormat);

  double cameraPos[3], focalPoint[3], zoom, roll;
  cameraPos[0] = settings.value(CAMERA_X_POS, 0).toDouble();
  cameraPos[1] = settings.value(CAMERA_Y_POS, 10).toDouble();
  cameraPos[2] = settings.value(CAMERA_Z_POS, 10).toDouble();
  focalPoint[0] = settings.value(CAMERA_FOCAL_X_POS, 0).toDouble();
  focalPoint[1] = settings.value(CAMERA_FOCAL_Y_POS, 0).toDouble();
  focalPoint[2] = settings.value(CAMERA_FOCAL_Z_POS, 0).toDouble();
  zoom = settings.value(CAMERA_ZOOM, 1).toDouble();
  roll = settings.value(CAMERA_ROLL, 0).toDouble();

  auto camera = m_renderer->GetActiveCamera();

  camera->SetFocalPoint(focalPoint[0], focalPoint[1], focalPoint[2]);
  camera->SetPosition(cameraPos[0], cameraPos[1], cameraPos[2]);
  camera->SetViewAngle(zoom);
  camera->SetRoll(roll);

  m_renderer->GetRenderWindow()->Render();

  m_view->update();
}

//--------------------------------------------------------------------
void MovieRenderer::onScriptFinished()
{
  m_executor->restart();
  makeMovie();

  modifyUI(true);
}

//--------------------------------------------------------------------
bool MovieRenderer::eventFilter(QObject *object, QEvent *e)
{
  if(m_render->text() == "Stop")
  {
    // If rendering frames just swallow the event and do nothing to avoid interruption any camera animation.
    e->accept();
    return true;
  }

  return QMainWindow::eventFilter(object, e);
}

//--------------------------------------------------------------------
void MovieRenderer::makeMovie()
{
  QStringList resolutions, frameStrings, outputNames;

  if(m_renderFull->isChecked())
  {
    resolutions << "1280x720";
    frameStrings << QString{"\"%1Frame_HD_%05d.png\""};
    outputNames << QString{"%1out_HD.mp4"};
  }

  if(m_renderHalf->isChecked())
  {
    resolutions << "640x360";
    frameStrings << QString{"\"%1Frame_Half_%05d.png\""};
    outputNames << QString{"%1out_HalfHD.mp4"};
  }

  if(m_render4K->isChecked())
  {
    resolutions << "3840x2160";
    frameStrings << QString{"\"%1Frame_4k_%05d.png\""};
    outputNames << QString{"%1out_4K.mp4"};
  }

  for(int i = 0; i < resolutions.size(); ++i)
  {
    auto resolution  = resolutions.at(i);
    auto frameString = frameStrings.at(i);
    auto outputName  = outputNames.at(i);

    statusBar()->showMessage(tr("Creating %1 movie").arg(resolution));

    QProcess ffmpegProcess{this};
    auto path = QDir::toNativeSeparators(m_directory->text() + "/");

    QStringList arguments;
    arguments << "-r" << "30";
    arguments << "-y";
    arguments << "-s" << resolution;
    arguments << "-i" << frameString.arg(path);
    arguments << "-vcodec" << "libx264";
    arguments << "-crf" << "1";
    arguments << "-pix_fmt" << "yuv420p";
    arguments << "-qp" << "0";
    arguments << "-f" << "mp4";
    arguments << outputName.arg(path);

    connect(&ffmpegProcess, SIGNAL(readyReadStandardError()),
            this,          SLOT(onDataAvailable()));
    connect(&ffmpegProcess, SIGNAL(readyReadStandardOutput()),
            this,          SLOT(onDataAvailable()));
    connect(&ffmpegProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
            this,          SLOT(onProcessFinished(int, QProcess::ExitStatus)));

    auto command = "\"" + QDir::toNativeSeparators(m_ffmpegExe->text()) + "\" " + arguments.join(" ");

    ffmpegProcess.start(command);
    ffmpegProcess.waitForStarted();

    ffmpegProcess.waitForFinished(-1);
    if(!ffmpegProcess.readAllStandardError().isEmpty())
    {
      qDebug() << ffmpegProcess.readAllStandardError();
      errorDialog(tr("Error creating the %1 HD video").arg(resolution), tr("Read the log for details."));
    }

    statusBar()->showMessage(tr("Created %1 video.").arg(resolution));
  }
}


//--------------------------------------------------------------------
void MovieRenderer::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  auto process = dynamic_cast<QProcess *>(sender());
  if(process)
  {
    statusBar()->showMessage(tr("Finished creating a video"));
  }
}

//--------------------------------------------------------------------
void MovieRenderer::onDataAvailable()
{
  auto object = dynamic_cast<QProcess *>(sender());
  if(!object) return;

  auto data = object->readAllStandardError();
  qDebug() << QString().fromLocal8Bit(data);
}

//--------------------------------------------------------------------
void MovieRenderer::saveSettings() const
{
  auto name = QApplication::applicationDirPath() + "/VTKMovieRenderer.ini";
  QSettings settings(name, QSettings::IniFormat);
  settings.setValue(VIDEO_4K_ENABLED, m_render4K->isChecked());
  settings.setValue(VIDEO_HD_ENABLED, m_renderFull->isChecked());
  settings.setValue(VIDEO_SD_ENABLED, m_renderHalf->isChecked());
  settings.setValue(POINT_SMOOTHING_ENABLE, m_pointSmoothing->isChecked());
  settings.setValue(LINE_SMOOTHING_ENABLE, m_lineSmoothing->isChecked());
  settings.setValue(POLYGON_SMOOTHING_ENABLE, m_polygonSmoothing->isChecked());
  settings.setValue(SHADOWS_ENABLED, m_shadows->isChecked());
  settings.setValue(MOTION_BLUR_ENABLED, m_motionBlur->isChecked());
  settings.setValue(MOTION_BLUR_FRAMES, m_motionBlurFrames->value());
  settings.setValue(ANTIALIAS_ENABLED, m_antiAlias->isChecked());
  settings.setValue(ANTIALIAS_FRAMES, m_aliasFrames->value());
  settings.setValue(AXES_SHOWN, m_axes->isChecked());
  settings.setValue(OUTPUT_DIR, m_directory->text());
  settings.setValue(FFMPEG_BINARY, m_ffmpegExe->text());

  settings.sync();
}

//--------------------------------------------------------------------
void MovieRenderer::restoreSettings()
{
  auto name = QApplication::applicationDirPath() + "/VTKMovieRenderer.ini";
  QSettings settings(name, QSettings::IniFormat);
  m_render4K->setChecked(settings.value(VIDEO_4K_ENABLED, false).toBool());
  m_renderFull->setChecked(settings.value(VIDEO_HD_ENABLED, true).toBool());
  m_renderHalf->setChecked(settings.value(VIDEO_SD_ENABLED, false).toBool());
  m_pointSmoothing->setChecked(settings.value(POINT_SMOOTHING_ENABLE, true).toBool());
  m_lineSmoothing->setChecked(settings.value(LINE_SMOOTHING_ENABLE, true).toBool());
  m_polygonSmoothing->setChecked(settings.value(POLYGON_SMOOTHING_ENABLE, true).toBool());
  m_shadows->setChecked(settings.value(SHADOWS_ENABLED, true).toBool());
  m_motionBlur->setChecked(settings.value(MOTION_BLUR_ENABLED, true).toBool());
  m_motionBlurFrames->setValue(settings.value(MOTION_BLUR_FRAMES, 1).toInt());
  m_antiAlias->setChecked(settings.value(ANTIALIAS_ENABLED, true).toBool());
  m_aliasFrames->setValue(settings.value(ANTIALIAS_FRAMES, 5).toInt());
  m_axes->setChecked(settings.value(AXES_SHOWN, false).toBool());
  m_directory->setText(settings.value(OUTPUT_DIR, QCoreApplication::applicationDirPath()).toString());
  m_ffmpegExe->setText(settings.value(FFMPEG_BINARY, QString()).toString());
}

//--------------------------------------------------------------------
void MovieRenderer::closeEvent(QCloseEvent* event)
{
  saveSettings();

  if(m_loader && m_loader->isRunning())
  {
    m_loader->abort();
    m_loader->thread()->wait(10000);
    m_loader = nullptr;
  }

  if(m_executor && m_executor->isRunning())
  {
    m_executor->abort();
    m_executor->thread()->wait(10000);
    m_executor = nullptr;
  }
}

//--------------------------------------------------------------------
void MovieRenderer::saveCameraPosition() const
{
  double cameraPos[3], focalPoint[3], zoom, roll;

  auto camera = m_renderer->GetActiveCamera();
  camera->GetPosition(cameraPos);
  camera->GetFocalPoint(focalPoint);
  zoom = camera->GetViewAngle();
  roll = camera->GetRoll();

  auto name = QApplication::applicationDirPath() + "/VTKMovieRenderer.ini";
  QSettings settings(name, QSettings::IniFormat);
  settings.setValue(CAMERA_X_POS, cameraPos[0]);
  settings.setValue(CAMERA_Y_POS, cameraPos[1]);
  settings.setValue(CAMERA_Z_POS, cameraPos[2]);
  settings.setValue(CAMERA_FOCAL_X_POS, focalPoint[0]);
  settings.setValue(CAMERA_FOCAL_Y_POS, focalPoint[1]);
  settings.setValue(CAMERA_FOCAL_Z_POS, focalPoint[2]);
  settings.setValue(CAMERA_ZOOM, zoom);
  settings.setValue(CAMERA_ROLL, roll);

  settings.sync();
}
