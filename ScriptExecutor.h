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

/** \class ScriptExecutor
 * \brief Modifies the scene and creates actors if needed for the frame. Signals for a frame creation once finished modifying the scene.
 * This file needs to be modified as the script is pure C++.
 *
 */
class ScriptExecutor
: public QThread
{
    Q_OBJECT
  public:
    /** \brief ScriptExecutor class constructor.
     * \param[in] renderer scene vtk renderer.
     * \param[in] loader resource loader thread.
     * \param[in] parent raw pointer of the QObject owner of this one.
     *
     */
    explicit ScriptExecutor(vtkSmartPointer<vtkRenderer> renderer, ResourceLoaderThread* loader, QObject *parent = nullptr);

    /** \brief ScriptExecutor class virtual destructor.
     *
     */
    virtual ~ScriptExecutor()
    {}

    /** \brief Returns the error string.
     *
     */
    const QString getError() const
    { return m_error; }

    /** \brief Signals the need to abort the script.
     *
     */
    void abort()
    { m_abort = true; }

    /** \brief Wakes up the executor to create the next frame.
     *
     */
    void nextFrame()
    { m_waitCondition.wakeOne(); }

    /** \brief Resets the initial data.
     *
     */
    void restart()
    { m_abort = false; }

  signals:
    void render();

  protected:
    virtual void run();

  private:
    /** \brief Helper method to render a given number of still frames.
     * \param[in] numFrames number of still frames to render.
     *
     */
    void waitFrames(const unsigned int numFrames);

    /** \brief Helper method to get the resources from the resource loader thread.
     *
     */
    void getResources(ResourceLoaderThread *loader);

    /** \brief Modifies the error string.
     * \param[in] message error message.
     *
     */
    void error(const QString &message)
    { m_error = message; }

    /** \brief Signals the need to save the current frame and stops the execution until 'nextFrame()' method is called by the main application.
     *
     */
    void waitForFrameToRender();

    QString        m_error;         /** error mesasge or empty if everything is good.   */
    bool           m_abort;         /** true to abort the current render.               */
    QMutex         m_mutex;         /** mutex for the wait condition.                   */
    QWaitCondition m_waitCondition; /** wait condition for waiting for the main thread. */

    // script commands
    void threesixtynoscope(); // never got to make one in Counter Strike...
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
