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

class ScriptExecutor
: public QThread
{
    Q_OBJECT
  public:
    explicit ScriptExecutor(QObject *parent = nullptr);

    virtual ~ScriptExecutor()
    {}

    void setData(vtkRenderer *renderer, vtkActor *mesh, vtkVolume *volume, vtkPlane *plane, vtkImageData *image, vtkSmartPointer<vtkPolyData> polyData);

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
    void rotateScene(const double degrees);

    void error(const QString &message)
    { m_error = message; }

    void waitForFrameToRender();

    vtkRenderer *m_renderer;
    vtkVolume *m_volume;
    vtkActor *m_mesh;
    vtkPlane *m_plane;
    vtkImageData *m_image;
    vtkSmartPointer<vtkPolyData> m_polyData;

    QString m_error;
    bool m_abort;

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
};

#endif // SCRIPTEXECUTOR_H_
