/*
 File: ScriptExecutor.cpp
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

#include <ScriptExecutor.h>

#include <vtkPolyData.h>
#include <vtkActor.h>
#include <vtkVolume.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkVolumeProperty.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkClipPolyData.h>
#include <vtkProperty.h>
#include <vtkPolyDataNormals.h>
#include <vtkMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkPlane.h>
#include <vtkMatrix4x4.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkImageReslice.h>
#include <vtkImageMapToColors.h>
#include <vtkTexture.h>
#include <vtkCutter.h>
#include <vtkContourTriangulator.h>
#include <vtkDataSetMapper.h>
#include <vtkPNGWriter.h>
#include <vtkPointData.h>
#include <vtkSelectEnclosedPoints.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>

#include <cstring> // memcpy

//--------------------------------------------------------------------
ScriptExecutor::ScriptExecutor(QObject* parent)
: QThread{parent}
, m_renderer{nullptr}
, m_volume{nullptr}
, m_mesh{nullptr}
, m_plane{nullptr}
, m_image{nullptr}
, m_polyData{nullptr}
, m_abort{false}
{
}

//--------------------------------------------------------------------
void ScriptExecutor::run()
{
  if(!m_renderer || !m_mesh || !m_volume || !m_plane || !m_polyData) return;

  // 360-noscope
//  double rad = 1;
//  for(double i = 0; i <= 360; i += rad)
//  {
//    if(m_abort) return;
//
//    m_mesh->RotateWXYZ(rad, 0, 0, 1);
//    m_volume->RotateWXYZ(rad, 0, 0, 1);
//
//    waitForFrameToRender();
//  }
//
//  // wait a bit
//  for (int i = 0; i < 20; ++i)
//  {
//    if(m_abort) return;
//    waitForFrameToRender();
//  }
//
//  // fade out volume
//  auto opacity = m_volume->GetProperty()->GetGradientOpacity();
//  for (double i = 0; i <= 1; i += 0.025)
//  {
//    if(m_abort) return;
//    opacity->RemoveAllPoints();
//    for (int j = 0; j < 256; ++j)
//    {
//      auto point = j / 255.0 - i;
//      if (point < 0) point = 0;
//      opacity->AddPoint(i, point);
//    }
//
//    m_volume->Update();
//    waitForFrameToRender();
//  }
//
//  // wait a bit
//  for (int i = 0; i < 20; ++i)
//  {
//    if(m_abort) return;
//    waitForFrameToRender();
//  }

  // reslice out

  double meshBounds[6];
  m_polyData->GetBounds(meshBounds);
  auto increment = (meshBounds[3]-meshBounds[2])/200.0;
  auto betweenMesh = [meshBounds] (double point) { return (meshBounds[2] <= point && point <= meshBounds[3]); };

  int imageExtent[6];
  double imageSpacing[6];
  m_image->GetExtent(imageExtent);
  m_image->GetSpacing(imageSpacing);
  double imageBounds[6]{imageExtent[0]*imageSpacing[0], imageExtent[1]*imageSpacing[0],
                        imageExtent[2]*imageSpacing[1], imageExtent[3]*imageSpacing[1],
                        imageExtent[4]*imageSpacing[2], imageExtent[5]*imageSpacing[2]};
  auto betweenImage = [imageBounds] (double point) { return (imageBounds[2] <= point && point <= imageBounds[3]); };

  std::cout << "mesh bounds " << meshBounds[0] << "," << meshBounds[1] << "," << meshBounds[2] << "," << meshBounds[3] << "," << meshBounds[4] << "," << meshBounds[5] << std::endl;
  std::cout << "image extent " << imageExtent[0] << "," << imageExtent[1] << "," << imageExtent[2] << "," << imageExtent[3] << "," << imageExtent[4] << "," << imageExtent[5] << std::endl;
  std::cout << "image spacing " << imageSpacing[0] << "," << imageSpacing[1] << "," << imageSpacing[2] << std::endl;
  std::cout << "image bounds " << imageBounds[0] << "," << imageBounds[1] << "," << imageBounds[2] << "," << imageBounds[3] << "," << imageBounds[4] << "," << imageBounds[5] << std::endl << std::flush;

  double meshActorBounds[6];
  m_mesh->GetBounds(meshActorBounds);
  double volumeBounds[6];
  m_volume->GetBounds(volumeBounds);

  std::cout << "mesh actor bounds " << meshActorBounds[0] << "," << meshActorBounds[1] << "," << meshActorBounds[2] << "," << meshActorBounds[3] << "," << meshActorBounds[4] << "," << meshActorBounds[5] << std::endl;
  std::cout << "volume actor bounds" << volumeBounds[0] << "," << volumeBounds[1] << "," << volumeBounds[2] << "," << volumeBounds[3] << "," << volumeBounds[4] << "," << volumeBounds[5] << std::endl << std::flush;

  double meshPosition[3], volumePosition[3];
  m_mesh->GetPosition(meshPosition);
  m_volume->GetPosition(volumePosition);
  std::cout << "mesh actor position " << meshPosition[0] << "," << meshPosition[1] << "," << meshPosition[2] << std::endl;
  std::cout << "volume actor position " << volumePosition[0] << "," << volumePosition[1] << "," << volumePosition[2] << std::endl << std::flush;

  std::cout << "--------------------------\n";
  std::cout << "mesh length " << meshBounds[3]-meshBounds[2] << std::endl;
  std::cout << "mesh actor length " << meshActorBounds[3]-meshActorBounds[2] << std::endl;
  std::cout << "mesh pos difference " << meshBounds[2]-meshActorBounds[2] << std::endl;
  std::cout << "volume lenth " << imageBounds[3]-imageBounds[2] << std::endl;
  std::cout << "volume actor length " << volumeBounds[3]-volumeBounds[2] << std::endl;
  std::cout << "volume pos difference " << imageBounds[2] - volumeBounds[2] << std::endl;

  auto world2mesh = [meshBounds, meshActorBounds] (double point) { return point - meshActorBounds[2] + meshBounds[2]; };
  auto world2volu = [imageBounds, volumeBounds] (double point) { return point - volumeBounds[2]; }; // imagebounds[2] = 0;
  double step = (meshBounds[3]-meshBounds[2])/200.0;

  auto colorTable = vtkSmartPointer<vtkLookupTable>::New();
  colorTable->SetTableRange(0, 255);
  colorTable->SetValueRange(0.0, 1.0);
  colorTable->SetSaturationRange(0.0, 0.0);
  colorTable->SetHueRange(0.0, 0.0);
  colorTable->SetAlphaRange(1.0, 1.0);
  colorTable->SetNumberOfColors(256);
  colorTable->Build();
  colorTable->SetTableValue(0, 0,0,0,0);

  auto cutterPlane = vtkSmartPointer<vtkPlane>::New();
  cutterPlane->SetNormal(0, -1.0, 0);
  cutterPlane->SetOrigin(0.0, imageBounds[3]-0.1, 0.0);

  auto cutter = vtkSmartPointer<vtkCutter>::New();
  cutter->SetInputData(m_image);
  cutter->SetCutFunction(cutterPlane);

  auto cutterMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  cutterMapper->SetInputConnection(cutter->GetOutputPort());
//  cutterMapper->SetLookupTable(colorTable);
  cutterMapper->ScalarVisibilityOn();

  auto surfaceActor = vtkSmartPointer<vtkActor>::New();
  surfaceActor->SetMapper(cutterMapper);
  surfaceActor->SetPosition(m_volume->GetPosition());

  m_renderer->AddActor(surfaceActor);

  auto begin = std::max(meshBounds[3], imageBounds[3]);

  double worldPoint = meshActorBounds[3]; // std:min(meshActorBounds[3], volumeBounds[3]);

  std::cout << "starting with " << worldPoint << " - in mesh: " << world2mesh(worldPoint) << " - in image: " << world2volu(worldPoint) << std::endl << std::flush;
  std::cout << "valid in mesh " << (betweenMesh(world2mesh(worldPoint)) ? "true" : "false") << " - valid in image " << (betweenImage(world2volu(worldPoint)) ? "true" : "false") << std::endl << std::flush;

  while(betweenMesh(world2mesh(worldPoint)) && betweenImage(world2volu(worldPoint)))
  {
    if(m_abort) return;

    auto imagePoint = world2volu(worldPoint);
    cutterPlane->SetOrigin(0, imagePoint, 0);

    auto meshPoint = world2mesh(worldPoint);
    m_plane->SetOrigin(0, meshPoint, 0);

    std::cout << "world point " << worldPoint << std::endl << std::flush;
    worldPoint -= step;
    waitForFrameToRender();
  }

//  // wait a bit
//  for (int i = 0; i < 20; ++i)
//  {
//    if(m_abort) return;
//    waitForFrameToRender();
//  }
//
//  // reslice in
//  for (double y = bounds[2]-22; y <= bounds[3]-10; y += increment)
//  {
//    m_plane->SetOrigin(0, y, 0);
//
//    if(m_abort) return;
//    waitForFrameToRender();
//  }
//
//  // fade in volume
//  for(double i = 1; i >= 0; i -= 0.025)
//  {
//    if(m_abort) return;
//    opacity->RemoveAllPoints();
//    for(int j = 0; j < 256; ++j)
//    {
//      auto point = j/255.0 - i;
//      if(point < 0) point = 0;
//      opacity->AddPoint(i, point);
//    }
//
//    m_volume->Update();
//    waitForFrameToRender();
//  }

  // finished!
}

//--------------------------------------------------------------------
void ScriptExecutor::setData(vtkRenderer* renderer, vtkActor* mesh, vtkVolume* volume, vtkPlane *plane, vtkImageData *image, vtkSmartPointer<vtkPolyData> polyData)
{
  if(!renderer || !mesh || !volume || !plane || !image || !polyData)
  {
    error("Invalid data.");
    std::cout << "error in parameters in script executor" << std::endl << std::flush;
    return;
  }

  m_renderer = renderer;
  m_volume = volume;
  m_mesh = mesh;
  m_plane = plane;
  m_image = image;
  m_polyData = polyData;
}

//--------------------------------------------------------------------
void ScriptExecutor::waitForFrameToRender()
{
  emit render();

  m_mutex.lock();
  m_waitCondition.wait(&m_mutex);
  m_mutex.unlock();
}
