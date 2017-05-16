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

#include "MovieRenderer.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkWindowToImageFilter.h>
#include <vtkImageSincInterpolator.h>
#include <vtkImageResize.h>
#include <vtkPNGWriter.h>
#include <QVTKWidget.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPolyDataNormals.h>

#include <QMouseEvent>
#include <QObject>
#include <QEvent>

//--------------------------------------------------------------------
MovieRenderer::MovieRenderer()
: m_frameNum{0}
, m_loader{nullptr}
, m_executor{nullptr}
{
  setupUi(this);

  statusBar()->insertPermanentWidget(0, m_progressBar);
  statusBar()->showMessage("Initializing");

  m_directory->setText("D:\\Descargas\\Render");
  m_ffmpegExe->setText("D:\\Program Files\\ffmpeg-20150928-git-235381e-win64-static\\bin\\ffmpeg.exe");

  connectSignals();

  setupVTKView();
  updateRendererSettings();

  showMaximized();
}

//--------------------------------------------------------------------
MovieRenderer::~MovieRenderer()
{
}

//--------------------------------------------------------------------
void MovieRenderer::onRenderPressed()
{
  if(m_render->text() == "Render")
  {
    if(!m_renderFull->isChecked() && !m_renderHalf->isChecked())
    {
      QMessageBox msgbox;
      msgbox.setWindowIcon(QIcon(":/MovieRenderer/application.svg"));
      msgbox.setWindowTitle("Error starting Render");
      msgbox.setText(tr("At least one of these options must be enabled:\n - full hd\n - half hd"));
      msgbox.setIcon(QMessageBox::Icon::Critical);
      msgbox.exec();

      return;
    }

    if(m_ffmpegExe->text().isEmpty() || !QFileInfo{m_ffmpegExe->text()}.exists())
    {
      QMessageBox msgbox;
      msgbox.setWindowIcon(QIcon(":/MovieRenderer/application.svg"));
      msgbox.setWindowTitle("Error starting Render");
      msgbox.setText(tr("Invalid MMMPEG executable, select a valid one"));
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
  m_frameNum = 0;
  m_executor->abort();
  modifyUI(true);
}

//--------------------------------------------------------------------
void MovieRenderer::startRender()
{
  m_frameNum = 0;
  modifyUI(false);

  updateRendererSettings();

  renderScript();
}

//--------------------------------------------------------------------
void MovieRenderer::updateRendererSettings()
{
  auto renderWindow = m_renderer->GetRenderWindow();

  m_renderer->SetUseShadows(m_shadows->isChecked());
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
    renderWindow->SetAAFrames(m_aliasFrames->value());
  }
  else
  {
    renderWindow->SetAAFrames(0);
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
}

//--------------------------------------------------------------------
void MovieRenderer::showEvent(QShowEvent*)
{
  statusBar()->showMessage("Launching resources loader");

  onReloadPressed();
}

//--------------------------------------------------------------------
void MovieRenderer::onResourcesLoaded()
{
  auto loader = qobject_cast<ResourceLoaderThread *>(sender());

  if(loader)
  {
    if(!loader->getError().isEmpty())
    {
      auto title = QString("Error loading resources");
      errorDialog(title, loader->getError());
      return;
    }

    for(auto actor: loader->actors())
    {
      m_renderer->AddActor(actor);
    }

    m_renderer->AddVolume(loader->volume());

    for(auto actor: loader->logos())
    {
      m_renderer->AddActor(actor);
    }

    statusBar()->showMessage("Resources loaded");

    // prepare script to run
    m_executor = std::make_shared<ScriptExecutor>(this);
    m_executor->setData(m_renderer.Get(), loader->actors().first().Get(), loader->volume().Get(), loader->plane().Get(), loader->image().Get(), loader->polyData());

    connect(m_executor.get(), SIGNAL(finished()), this, SLOT(onScriptFinished()));
    connect(m_executor.get(), SIGNAL(render()), this, SLOT(onRenderSignaled()));
  }
  else
  {
    std::cout << "error loading resources" << std::endl << std::flush;
  }

  onCameraResetPressed();

  m_renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
  m_renderer->GetActiveCamera()->SetPosition(2, 3, -1);
  m_renderer->GetActiveCamera()->Roll(-90);
  m_renderer->GetActiveCamera()->Zoom(1.3);
  m_renderer->ResetCamera();

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
  m_renderer->BackingStoreOff();
  m_renderer->GetActiveCamera(); // creates default camera.

  auto interactorstyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  interactorstyle->AutoAdjustCameraClippingRangeOn();
  interactorstyle->KeyPressActivationOff();

  m_view->GetRenderWindow()->AddRenderer(m_renderer);
  m_view->GetRenderWindow()->GetInteractor()->SetInteractorStyle(interactorstyle);

  // Color background
  QPalette pal = this->palette();
  pal.setColor(QPalette::Base, pal.color(QPalette::Window));
  this->setPalette(pal);

  auto axes = vtkSmartPointer<vtkAxesActor>::New();
  axes->DragableOff();
  axes->PickableOff();

  m_axesWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
  m_axesWidget->SetOrientationMarker(axes);
  m_axesWidget->SetInteractor(m_renderer->GetRenderWindow()->GetInteractor());
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
}

//--------------------------------------------------------------------
void MovieRenderer::onAntiAliasChanged(int int1)
{
  m_aliasFrames->setEnabled(m_antiAlias->isChecked());
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
  m_renderer->GetRenderWindow()->Render();
  m_view->update();

  // Screenshot
  auto windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
  windowToImageFilter->SetInput(m_renderer->GetRenderWindow());
  windowToImageFilter->SetMagnification(1);
  windowToImageFilter->SetInputBufferTypeToRGBA();
  windowToImageFilter->Update();

  auto screenshot = windowToImageFilter->GetOutput();
  auto outputDir = QDir::toNativeSeparators(m_directory->text() + "/");

  if(m_renderFull->isChecked())
  {
    auto name = outputDir + QString("Frame_HD_%1.png").arg(QString::number(m_frameNum), 5, QChar('0'));

    auto writer = vtkSmartPointer<vtkPNGWriter>::New();
    writer->SetFileName(name.toStdString().c_str());
    writer->SetInputData(screenshot);
    writer->Write();
  }

  if(m_renderHalf->isChecked())
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

  ++m_frameNum;

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
  auto camera = m_renderer->GetActiveCamera();

  camera->SetFocalPoint(0, 0, 0);
  camera->SetPosition(0, 10, 10);

  m_renderer->ResetCamera();
  m_renderer->GetRenderWindow()->Render();

  m_view->update();
}

//--------------------------------------------------------------------
void MovieRenderer::onScriptFinished()
{
  if(m_executor->isFinished())
  {
    m_executor->restart();
    modifyUI(true);
  }
}

//--------------------------------------------------------------------
bool MovieRenderer::eventFilter(QObject *object, QEvent *e)
{

  if(m_axesWidget->GetEnabled() && m_view == dynamic_cast<QVTKWidget *>(object))
  {
    if(e && e->type() == QEvent::MouseButtonPress)
    {
      auto me = static_cast<QMouseEvent*>(e);
      if(me && me->button() == Qt::RightButton)
      {
        if(m_renderer)
        {
          double fp[3], pos[3];
          std::cout << "camera position\nposition: " << pos[0] << "," << pos[1] << "," << pos[2] << "\nfocal point: " << fp[0] << "," << fp[1] << "," << fp[2] << std::endl << std::flush;
        }
      }
    }

    return m_view->eventFilter(object, e);
  }

  return QMainWindow::eventFilter(object, e);
}
