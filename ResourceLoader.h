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

#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkImageData.h>
#include <vtkVolume.h>
#include <vtkPlane.h>
#include <vtkPolyData.h>

#include <QThread>

class ResourceLoaderThread
: public QThread
{
    Q_OBJECT
  public:
    explicit ResourceLoaderThread(QObject *parent = nullptr);

    virtual ~ResourceLoaderThread()
    {}

    QList<vtkSmartPointer<vtkActor>> actors() const
    { return m_actors; }

    vtkSmartPointer<vtkImageData> image() const
    { return m_image; };

    vtkSmartPointer<vtkVolume> volume() const
    { return m_volume; }

    QList<vtkSmartPointer<vtkActor2D>> logos() const
    { return m_logos; }

    vtkSmartPointer<vtkPlane> plane() const
    { return m_plane; }

    vtkSmartPointer<vtkPolyData> polyData() const
    { return m_polyData; }

    const QString getError() const
    { return m_error; }

  protected:
    virtual void run() override;

  signals:
    void finishedLoading();

  private:
    void error(const QString &message)
    { m_error = message; }

    QList<vtkSmartPointer<vtkActor>> m_actors;
    vtkSmartPointer<vtkImageData> m_image;
    vtkSmartPointer<vtkVolume> m_volume;
    QList<vtkSmartPointer<vtkActor2D>> m_logos;
    vtkSmartPointer<vtkPlane> m_plane;
    vtkSmartPointer<vtkPolyData> m_polyData;

    QString m_error;

};

#endif // RESOURCELOADER_H_
