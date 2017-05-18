/*
 File: MovieRenderer.h
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

#ifndef VTK_MOVIE_RENDERER_H_
#define VTK_MOVIE_RENDERER_H_

#include "ScriptExecutor.h"
#include "ResourceLoader.h"

#include <QMainWindow>
#include "ui_MovieRenderer.h"
#include <QProcess>

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkRenderer.h>

#include <atomic>
#include <memory>

class QObject;
class QEvent;

class vtkOrientationMarkerWidget;

class MovieRenderer
: public QMainWindow
, private Ui::MovieRenderer
{
    Q_OBJECT
  public:
    explicit MovieRenderer();
    virtual ~MovieRenderer()
    {}

  protected:
    virtual bool eventFilter(QObject *, QEvent *);

  private slots:
    void onRenderPressed();
    void onMotionBlurChanged(int);
    void onAntiAliasChanged(int);
    void onDirButtonPressed();
    void onFFMPEGDirButtonPressed();
    void onResourcesLoaded();
    void onRenderSignaled();
    void onReloadPressed();
    void onCameraResetPressed();
    void onScriptFinished();
    void onAxesValueChanged(int);

    void onDataAvailable();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

  private:
    void updateRendererSettings();
    void startRender();
    void stopRender();
    void makeMovie();

    void modifyUI(bool);
    void renderScript();
    void setupVTKView();
    void connectSignals();
    void errorDialog(const QString &title, const QString& message);

    // view, size is 1280x720
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkOrientationMarkerWidget> m_axesWidget; /** orientation marker widget. */
    std::atomic<unsigned long> m_frameNum;

    // threads
    std::shared_ptr<ResourceLoaderThread> m_loader;
    std::shared_ptr<ScriptExecutor> m_executor;
};

#endif
