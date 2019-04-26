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

// Project
#include "ScriptExecutor.h"
#include "ResourceLoader.h"

// Qt
#include "ui_MovieRenderer.h"
#include <QMainWindow>
#include <QProcess>

// VTK
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkRenderer.h>

// C++
#include <memory>
#include <atomic>

class QObject;
class QEvent;

class vtkOrientationMarkerWidget;

/** \class MovieRenderer
 * \brief Main application dialog.
 *
 */
class MovieRenderer
: public QMainWindow
, private Ui::MovieRenderer
{
    Q_OBJECT
  public:
    /** \brief MovieRenderer class constructor.
     *
     */
    explicit MovieRenderer();

    /** \brief MovieRenderer class virtual destructor.
     *
     */
    virtual ~MovieRenderer()
    {}

  protected:
    virtual bool eventFilter(QObject *, QEvent *);
    virtual void closeEvent(QCloseEvent *event);

  private slots:
    /** \brief Starts/Stops the script execution.
     *
     */
    void onRenderPressed();

    /** \brief Modifies the UI, enables/disables the frames field for motion blur.
     * \param[in] value true to enable the frames field and false to disable it.
     *
     */
    void onMotionBlurChanged(int value);

    /** \brief Modifies the UI, enables/disables the frames field for anti-aliasing.
     * \param[in] value true to enable the frames field and false to disable it.
     *
     */
    void onAntiAliasChanged(int value);

    /** \brief Asks the user for an output directory for the frames and the video.
     *
     */
    void onDirButtonPressed();

    /** \brief Asks the user the location of the ffmpeg executable.
     *
     */
    void onFFMPEGDirButtonPressed();

    /** \brief Creates the script executor once the resources have been loaded.
     *
     */
    void onResourcesLoaded();

    /** \brief Saves current frame to disk.
     *
     */
    void onRenderSignaled();

    /** \brief Reloads the resources (to allow resource modification on disk on the fly).
     *
     */
    void onReloadPressed();

    /** \brief Resets the camera position to view all the resources in the renderer.
     *
     */
    void onCameraResetPressed();

    /** \brief Launches the ffmpeg process to create a video.
     *
     */
    void onScriptFinished();

    /** \brief Shows/hides the axis widget.
     * \param[in] value true to enable and false to disable the widget.
     *
     */
    void onAxesValueChanged(int value);

    /** \brief Dumps the output of the ffmpeg process to standard output (for debugging).
     *
     */
    void onDataAvailable();

    /** \brief Reports the result of the ffmpeg process.
     * \param[in] exitCode process exit code.
     * \param[in] exitStatus QProcess exit status code.
     *
     */
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    /** \brief Updates the vtk render window settings with the settings of the UI.
     *
     */
    void updateRendererSettings();

    /** \brief Saves the camera position to the ini file.
     *
     */
    void saveCameraPosition() const;

  private:
    /** \brief Runs the script executor and disables part of the UI.
     *
     */
    void startRender();

    /** \brief Stops the script executor and enables part of the UI.
     *
     */
    void stopRender();

    /** \brief Creates the movie using a separate QProcess.
     *
     */
    void makeMovie();

    /** \brief Helper method to disable parts of the UI when rendering.
     * \param[in] value true to enable UI and false otherwise.
     *
     */
    void modifyUI(bool);

    /** \brief Helper method to start the script thread.
     *
     */
    void renderScript();

    /** \brief Initializes the vtk render window.
     *
     */
    void setupVTKView();

    /** \brief Helper method to connect the UI signals.
     *
     */
    void connectSignals();

    /** \brief Helper method to show a dialog on error.
     * \param[in] title error dialog title.
     * \param[in] message error message.
     *
     */
    void errorDialog(const QString &title, const QString& message);

    /** \brief Saves the current settings to a ini file in the same directory as the executable.
     *
     */
    void saveSettings() const;

    /** \brief Restores the application settings from an ini file in the application path.
     *
     */
    void restoreSettings();

    // view, size is 1280x720
    vtkSmartPointer<vtkRenderer>                m_renderer;   /** vtk main renderer.         */
    vtkSmartPointer<vtkOrientationMarkerWidget> m_axesWidget; /** orientation marker widget. */
    std::atomic<unsigned long>                  m_frameNum;   /** current frame number.      */

    // threads
    std::shared_ptr<ResourceLoaderThread>       m_loader;     /** resource loader thread.    */
    std::shared_ptr<ScriptExecutor>             m_executor;   /** script executor thread.    */
};

#endif
