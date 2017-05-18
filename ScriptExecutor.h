/*
 File: ScriptExecutor.h
 Created on: 15/05/2017
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

#ifndef SCRIPTEXECUTOR_H_
#define SCRIPTEXECUTOR_H_

#include <QThread>

#include <vtkSmartPointer.h>
#include <vtkActor.h>

#include <QList>
#include <QMutex>
#include <QWaitCondition>

class vtkRenderer;
class vtkActor;
class vtkVolume;
class vtkPlane;
class vtkImageData;
class vtkPolyData;

class ResourceLoaderThread;

class ScriptExecutor
: public QThread
{
    Q_OBJECT
  public:
    explicit ScriptExecutor(vtkSmartPointer<vtkRenderer> renderer, ResourceLoaderThread* loader, QObject *parent = nullptr);

    virtual ~ScriptExecutor()
    {}

    const QString getError() const
    { return m_error; }

    void abort()
    { m_abort = true; }

    void nextFrame()
    { m_waitCondition.wakeOne(); }

    void restart()
    { m_abort = false; }

  signals:
    void render();

  protected:
    virtual void run();

  private:
    /** \brief Renderes a given number of still frames.
     * \param[in] numFrames number of still frames to render.
     *
     */
    void waitFrames(const unsigned int numFrames);

    void getResources(ResourceLoaderThread *loader);

    void error(const QString &message)
    { m_error = message; }

    void waitForFrameToRender();

    QString m_error;
    bool m_abort;

    QMutex m_mutex;
    QWaitCondition m_waitCondition;

    // script commands
    void threesixtynoscope();
    void fadeOutVolume();
    void fadeInVolume();
    void reslice();

    // data for the script.
    vtkSmartPointer<vtkRenderer>  m_renderer;
    vtkSmartPointer<vtkVolume>    m_volume;
    vtkSmartPointer<vtkActor>     m_mesh;
    vtkSmartPointer<vtkPlane>     m_plane;
    vtkSmartPointer<vtkImageData> m_image;
    vtkSmartPointer<vtkPolyData>  m_polyData;
};

#endif // SCRIPTEXECUTOR_H_
