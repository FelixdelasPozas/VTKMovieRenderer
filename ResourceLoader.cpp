/*
 File: ResourceLoader.cpp
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


#include <ResourceLoader.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QIcon>
#include <QMessageBox>

#include <vtkOBJReader.h>
#include <vtkAppendPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkTIFFReader.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolumeProperty.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>
#include <vtkPNGReader.h>
#include <vtkImageMapper.h>
#include <vtkMetaImageWriter.h>
#include <vtkMetaImageReader.h>
#include <vtkClipPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkLookupTable.h>
#include <vtkMatrix4x4.h>

//--------------------------------------------------------------------
ResourceLoaderThread::ResourceLoaderThread(QObject* parent)
: QThread{parent}
{
}

//--------------------------------------------------------------------
void ResourceLoaderThread::run()
{
  auto currentDir = QCoreApplication::applicationDirPath() + "/resources/";
  auto rh_mesh_filename = currentDir + "rhmesh.obj";
  auto lh_mesh_filename = currentDir + "lhmesh.obj";
  auto image_filename   = currentDir + "new_avg100_8b corrected.mhd";
  auto logocajal        = currentDir + "logocajalbbp.png";
  auto logocien         = currentDir + "logocien.png";
  auto logoreina        = currentDir + "logofundacionsofia.png";
  auto logovallecas     = currentDir + "logovallecas.png";

  // MESH LOADING & ACTOR CREATION
  QFileInfo rhMeshFile{rh_mesh_filename};
  if(!rhMeshFile.exists())
  {
    error(QString("Can't find %1").arg(rhMeshFile.absoluteFilePath()));
    return;
  }

  auto objReader1 = vtkSmartPointer<vtkOBJReader>::New();
  objReader1->SetFileName(QDir::toNativeSeparators(rhMeshFile.absoluteFilePath()).toStdString().c_str());
  objReader1->DebugOn();
  objReader1->GlobalWarningDisplayOn();
  objReader1->Update();

  QFileInfo lhMeshFile{lh_mesh_filename};
  if(!lhMeshFile.exists())
  {
    error(QString("Can't find %1").arg(lhMeshFile.absoluteFilePath()));
    return;
  }

  auto objReader2 = vtkSmartPointer<vtkOBJReader>::New();
  objReader2->SetFileName(QDir::toNativeSeparators(lhMeshFile.absoluteFilePath()).toStdString().c_str());
  objReader2->Update();

  // join polydata
  auto appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();
  appendFilter->AddInputData(objReader1->GetOutput());
  appendFilter->AddInputData(objReader2->GetOutput());
  appendFilter->Update();

  // Remove any duplicate points.
  auto cleanFilter = vtkSmartPointer<vtkCleanPolyData>::New();
  cleanFilter->SetInputConnection(appendFilter->GetOutputPort());
  cleanFilter->Update();

  auto smoothFilter = vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
  smoothFilter->SetInputConnection(cleanFilter->GetOutputPort());
  smoothFilter->SetNumberOfIterations(150);
  smoothFilter->SetRelaxationFactor(0.03);
  smoothFilter->FeatureEdgeSmoothingOff();
  smoothFilter->BoundarySmoothingOn();
  smoothFilter->Update();

  auto normalsGenerator = vtkSmartPointer<vtkPolyDataNormals>::New();
  normalsGenerator->DebugOn();
  normalsGenerator->GlobalWarningDisplayOn();
  normalsGenerator->SetInputData(smoothFilter->GetOutput());
  normalsGenerator->ReleaseDataFlagOn();
  normalsGenerator->SetSplitting(1);
  normalsGenerator->SetConsistency(0);
  normalsGenerator->ComputeCellNormalsOn();
  normalsGenerator->ComputePointNormalsOn();
  normalsGenerator->SetFlipNormals(0);
  normalsGenerator->SetNonManifoldTraversal(1);
  normalsGenerator->Update();

  m_polyData = vtkSmartPointer<vtkPolyData>::New();
  m_polyData->DeepCopy(normalsGenerator->GetOutput());
  m_polyData->Modified();

  double bounds[6];
  normalsGenerator->GetOutput()->GetBounds(bounds);

  // Define a clipping plane
  m_plane = vtkSmartPointer<vtkPlane>::New();
  m_plane->SetNormal(0, -1.0, 0);
  m_plane->SetOrigin(0.0, bounds[3], 0.0);

  // Clip the source with the plane
  auto clipper = vtkSmartPointer<vtkClipPolyData>::New();
  clipper->DebugOn();
  clipper->GlobalWarningDisplayOn();
  clipper->ReleaseDataFlagOff();
  clipper->SetInputConnection(normalsGenerator->GetOutputPort());
  clipper->SetClipFunction(m_plane);

  //Create a mapper and actor
  auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->ReleaseDataFlagOff();
  mapper->SetInputConnection(clipper->GetOutputPort());

  auto actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(0.3, 0.3, 0.3);
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 0.2);
  actor->GetProperty()->SetInterpolationToPhong();
  actor->GetProperty()->SetLighting(true);
  actor->GetProperty()->SetSpecular(0.6);

  m_actors << actor;

  // VOLUME LOADING & ACTOR CREATION
  QFileInfo imageFile{image_filename};
  if(!imageFile.exists())
  {
    error(QString("Can't find %1").arg(imageFile.absoluteFilePath()));
    return;
  }

  auto imageReader = vtkSmartPointer<vtkMetaImageReader>::New();
  imageReader->SetFileName(QDir::toNativeSeparators(imageFile.absoluteFilePath()).toStdString().c_str());
  imageReader->Update();

  m_image = vtkSmartPointer<vtkImageData>::New();
  m_image->DeepCopy(imageReader->GetOutput());
  m_image->SetSpacing(0.2, 0.2, 2.0);

  auto volumeMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
  volumeMapper->SetBlendModeToMaximumIntensity();
  volumeMapper->SetInputData(m_image);

  auto volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
  volumeProperty->ShadeOff();
  volumeProperty->SetSpecular(0.3);
  volumeProperty->SetInterpolationTypeToLinear();

  auto compositeOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
  for(int i = 0; i < 256; ++i)
  {
    compositeOpacity->AddPoint(i, i/255.0);
  }

  volumeProperty->SetScalarOpacity(compositeOpacity); // composite first.

  auto color = vtkSmartPointer<vtkColorTransferFunction>::New();
  for(auto i = 0; i < 256; ++i)
  {
    color->AddRGBPoint(i,  i/255.0, i/255.0, i/255.0);
  }

  volumeProperty->SetColor(color);

  m_volume = vtkSmartPointer<vtkVolume>::New();
  m_volume->SetMapper(volumeMapper);
  m_volume->SetProperty(volumeProperty);

  // ACTORS REPOSITION (centers are in 0,0,0 for an easier rotation).
  double center[3], position[3];
  actor->GetBounds(bounds);
  center[0] = (bounds[0]+bounds[1])/2.0;
  center[1] = (bounds[2]+bounds[3])/2.0;
  center[2] = (bounds[4]+bounds[5])/2.0;
  actor->SetOrigin(center[0],center[1],center[2]);
  actor->GetPosition(position);
  actor->SetPosition(position[0]-center[0], position[1]-center[1], position[2]-center[2]);

  m_volume->GetBounds(bounds);
  center[0] = (bounds[0]+bounds[1])/2.0;
  center[1] = (bounds[2]+bounds[3])/2.0;
  center[2] = (bounds[4]+bounds[5])/2.0;
  m_volume->SetOrigin(center[0],center[1],center[2]);
  m_volume->GetPosition(position);
  m_volume->SetPosition(position[0]-center[0], position[1]-center[1], position[2]-center[2]);

  // LOGOS LOADING & ACTOR CREATION
  QStringList logos{logocajal, logoreina, logocien, logovallecas};
  int xPos = 301; // precomputed to obtain logos centered in Y coord.
  for(auto logo: logos)
  {
    QFileInfo logoImageFile{logo};
    if(!logoImageFile.exists())
    {
      error(QString("Can't find %1").arg(logoImageFile.absoluteFilePath()));
      return;
    }

    auto reader = vtkSmartPointer<vtkPNGReader>::New();
    reader->SetFileName(logo.toStdString().c_str());
    reader->Update();

    auto width = reader->GetOutput()->GetExtent()[1];

    auto imageMapper = vtkSmartPointer<vtkImageMapper>::New();
    imageMapper->SetInputData(reader->GetOutput());
    imageMapper->SetColorWindow(255);
    imageMapper->SetColorLevel(127.5);

    auto imageActor = vtkSmartPointer<vtkActor2D>::New();
    imageActor->SetMapper(imageMapper);
    imageActor->SetPosition(xPos, 0);

    xPos += width;

    m_logos << imageActor;
  }
}
