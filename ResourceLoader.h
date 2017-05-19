/*
 File: ResourceLoader.h
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

#ifndef RESOURCELOADER_H_
#define RESOURCELOADER_H_

// VTK
#include <vtkSmartPointer.h>

// Qt
#include <QThread>
#include <QString>
#include <QList>

class vtkActor;
class vtkActor2D;
class vtkImageData;
class vtkVolume;
class vtkPlane;
class vtkPolyData;

/** \brief Loads the resources from disk and creates the actors. This class must be modified depending on
 * the scene being rendered. Just loads resources, it's meant to separate loading from executing. And return
 * only the objects meant not to be created mid-execution (permanent vtk pipelines).
 *
 */
class ResourceLoaderThread
: public QThread
{
    Q_OBJECT
  public:
    /** \brief ResourceLoaderThread class constructor.
     * \param[in] parent raw pointer of the QObject owner of this one.
     *
     */
    explicit ResourceLoaderThread(QObject *parent = nullptr)
    {}

    /** \brief ResourceLoaderThread class virtual destructor.
     *
     */
    virtual ~ResourceLoaderThread()
    {}

    /** \brief Returns the list of initial actors in the scene.
     *
     */
    QList<vtkSmartPointer<vtkActor>> actors() const
    { return m_actors; }

    /** \brief Returns the list of raw vtkImageData.
     *
     */
    QList<vtkSmartPointer<vtkImageData>> images() const
    { return m_images; };

    /** \brief Returns the list of vtk volumes.
     *
     */
    QList<vtkSmartPointer<vtkVolume>> volumes() const
    { return m_volumes; }

    /** \brief Returns the list of logos (2D actors).
     *
     */
    QList<vtkSmartPointer<vtkActor2D>> logos() const
    { return m_logos; }

    /** \brief Returns the slicing plane.
     *
     */
    vtkSmartPointer<vtkPlane> plane() const
    { return m_plane; }

    /** \brief Returns the list of raw polydatas.
     *
     */
    QList<vtkSmartPointer<vtkPolyData>> polyDatas() const
    { return m_polyDatas; }

    /** \brief Returns the error string.
     *
     */
    const QString getError() const
    { return m_error; }

  protected:
    virtual void run() override;

  signals:
    void finishedLoading();

  private:
    /** \brief Modifies the error string.
     * \param[in] message error message.
     *
     */
    void error(const QString &message)
    { m_error = message; }

    QList<vtkSmartPointer<vtkActor>>     m_actors;    /** list of vtk actors.                   */
    QList<vtkSmartPointer<vtkImageData>> m_images;    /** list of vtkImageData.                 */
    QList<vtkSmartPointer<vtkVolume>>    m_volumes;   /** list of vtkVolume.                    */
    QList<vtkSmartPointer<vtkActor2D>>   m_logos;     /** list of 2D actors.                    */
    QList<vtkSmartPointer<vtkPolyData>>  m_polyDatas; /** list of vtkPolyData.                  */
    vtkSmartPointer<vtkPlane>            m_plane;     /** vtkPlane object for slicing.          */

    QString                              m_error;     /** error message or empty if successful. */

};

#endif // RESOURCELOADER_H_
