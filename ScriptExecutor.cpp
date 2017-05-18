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
#include <ResourceLoader.h>

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
#include <vtkImageProperty.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkImageProperty.h>

#include <cstring> // memcpy
#include <limits>  // min, max

//--------------------------------------------------------------------
ScriptExecutor::ScriptExecutor(vtkSmartPointer<vtkRenderer> renderer, ResourceLoaderThread *loader, QObject* parent)
: QThread   {parent}
, m_renderer{renderer}
, m_abort   {false}
{
  getResources(loader);
}

//--------------------------------------------------------------------
void ScriptExecutor::run()
{
  if(!m_error.isEmpty()) return;

//  waitFrames(25);
//
//  fadeOutVolume();
//
//  waitFrames(25);
//
//  reslice();
//
//  waitFrames(25);
//
//  fadeInVolume();
//
//  waitFrames(25);

  threesixtynoscope();

  // finished!
}

//--------------------------------------------------------------------
void ScriptExecutor::getResources(ResourceLoaderThread *loader)
{
  m_volume   = loader->volume();
  m_mesh     = loader->actors().first();
  m_plane    = loader->plane();
  m_image    = loader->image();
  m_polyData = loader->polyData();

  if(!m_renderer || !m_mesh || !m_volume || !m_plane || !m_image || !m_polyData)
  {
    error("Invalid data.");
    std::cout << "error in parameters in script executor" << std::endl << std::flush;
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
}

//--------------------------------------------------------------------
void ScriptExecutor::waitForFrameToRender()
{
  emit render();

  m_mutex.lock();
  m_waitCondition.wait(&m_mutex);
  m_mutex.unlock();
}

//--------------------------------------------------------------------
void ScriptExecutor::waitFrames(const unsigned int numFrames)
{
  for (unsigned int i = 0; i < numFrames; ++i)
  {
    if(m_abort) return;
    waitForFrameToRender();
  }
}

//--------------------------------------------------------------------
void ScriptExecutor::threesixtynoscope()
{
  double rad = 0.25;
  for(double i = 0; i <= 360; i += rad)
  {
    if(m_abort) return;

    m_mesh->RotateWXYZ(rad, 0, 0, 1);
    m_volume->RotateWXYZ(rad, 0, 0, 1);

    waitForFrameToRender();
  }
}

//--------------------------------------------------------------------
void ScriptExecutor::fadeOutVolume()
{
  auto opacity = m_volume->GetProperty()->GetGradientOpacity();
  for (double i = 0; i <= 1; i += 0.015)
  {
    if (m_abort) return;
    opacity->RemoveAllPoints();
    for (int j = 0; j < 256; ++j)
    {
      auto point = j / 255.0 - i;
      if (point < 0) point = 0;
      opacity->AddPoint(i, point);
    }

    m_volume->Update();
    waitForFrameToRender();
  }
}

//--------------------------------------------------------------------
void ScriptExecutor::fadeInVolume()
{
  auto opacity = m_volume->GetProperty()->GetGradientOpacity();
  for (double i = 1; i >= 0; i -= 0.015)
  {
    if (m_abort) return;
    opacity->RemoveAllPoints();
    for (int j = 0; j < 256; ++j)
    {
      auto point = j / 255.0 - i;
      if (point < 0) point = 0;
      opacity->AddPoint(i, point);
    }

    m_volume->Update();
    waitForFrameToRender();
  }
}

//--------------------------------------------------------------------
void ScriptExecutor::reslice()
{

  double meshBounds[6];
  m_polyData->GetBounds(meshBounds);
  auto betweenMesh = [meshBounds] (double point) { return (meshBounds[2] <= point && point <= meshBounds[3]);};

  int imageExtent[6];
  double imageSpacing[6];
  m_image->GetExtent(imageExtent);
  m_image->GetSpacing(imageSpacing);
  double imageBounds[6] { imageExtent[0] * imageSpacing[0], imageExtent[1] * imageSpacing[0], imageExtent[2] * imageSpacing[1], imageExtent[3] * imageSpacing[1], imageExtent[4] * imageSpacing[2], imageExtent[5] * imageSpacing[2] };
  auto betweenImage = [imageBounds] (double point) { return (imageBounds[2] <= point && point <= imageBounds[3]);};

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

  std::cout << "mesh length " << meshBounds[3] - meshBounds[2] << std::endl;
  std::cout << "mesh actor length " << meshActorBounds[3] - meshActorBounds[2] << std::endl;
  std::cout << "mesh pos difference " << meshBounds[2] - meshActorBounds[2] << std::endl;
  std::cout << "volume lenth " << imageBounds[3] - imageBounds[2] << std::endl;
  std::cout << "volume actor length " << volumeBounds[3] - volumeBounds[2] << std::endl;
  std::cout << "volume pos difference " << imageBounds[2] - volumeBounds[2] << std::endl;

  auto world2mesh = [meshBounds, meshActorBounds] (double point) { return point - meshActorBounds[2] + meshBounds[2];};
  auto world2volu = [imageBounds, volumeBounds] (double point)   { return point - volumeBounds[2];}; // imagebounds[2] = 0;
  double step = (meshBounds[3] - meshBounds[2]) / 400.0;

  //  auto colorTable = vtkSmartPointer<vtkLookupTable>::New();
  //  colorTable->SetTableRange(0, 255);
  //  colorTable->SetValueRange(0.0, 1.0);
  //  colorTable->SetSaturationRange(0.0, 0.0);
  //  colorTable->SetHueRange(0.0, 0.0);
  //  colorTable->SetAlphaRange(1.0, 1.0);
  //  colorTable->SetNumberOfColors(256);
  //  colorTable->Build();
  //  colorTable->SetTableValue(0, 0,0,0,0);
  //
  //  auto imageProperty = vtkSmartPointer<vtkImageProperty>::New();
  //  imageProperty->SetLookupTable(colorTable);
  //  imageProperty->SetUseLookupTableScalarRange(true);
  //  imageProperty->SetInterpolationTypeToCubic();

  auto sliceMapper = vtkSmartPointer<vtkImageSliceMapper>::New();
  sliceMapper->SetInputData(m_image);
  sliceMapper->SetOrientationToY();
  sliceMapper->SetSliceNumber(1089);

  auto slice = vtkSmartPointer<vtkImageSlice>::New();
  slice->SetMapper(sliceMapper);
  //  slice->SetProperty(imageProperty);
  slice->SetPosition(m_volume->GetPosition()[0] - 2, m_volume->GetPosition()[1], m_volume->GetPosition()[2]);

  m_renderer->AddActor(slice);

  //
  //  double coronal[16] = { 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1 };
  //
  //  auto matrix = vtkSmartPointer<vtkMatrix4x4>::New();
  //  matrix->DeepCopy(coronal);
  //
  //  auto reslice = vtkSmartPointer<vtkImageReslice>::New();
  //  reslice->SetInputData(m_image);
  //  reslice->SetOutputDimensionality(2);
  //  reslice->SetResliceAxes(matrix);
  //  reslice->SetInterpolationModeToCubic();
  //
  //  auto resliceMapper = vtkSmartPointer<vtkImageMapToColors>::New();
  //  resliceMapper->SetLookupTable(colorTable);
  //  resliceMapper->SetOutputFormatToRGBA();
  //  resliceMapper->SetInputConnection(reslice->GetOutputPort());
  //  resliceMapper->SetUpdateExtentToWholeExtent();
  //
  //  auto resliceActor = vtkSmartPointer<vtkImageActor>::New();
  //  resliceActor->SetInputData(resliceMapper->GetOutput());
  //  resliceActor->SetInterpolate(false);
  //  resliceActor->SetPosition(m_volume->GetPosition()[0], m_volume->GetPosition()[1]-100, m_volume->GetPosition()[2]);
  //
  //  m_renderer->AddActor(resliceActor);

  //  auto cutterPlane = vtkSmartPointer<vtkPlane>::New();
  //  cutterPlane->SetNormal(0, -1.0, 0);
  //  cutterPlane->SetOrigin(0.0, imageBounds[3]-0.1, 0.0);
  //
  //  auto cutter = vtkSmartPointer<vtkCutter>::New();
  //  cutter->SetInputData(m_image);
  //  cutter->SetCutFunction(cutterPlane);
  //  cutter->Update();
  //
  //  auto cutterMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  //  cutterMapper->SetInputConnection(cutter->GetOutputPort());
  //  cutterMapper->ScalarVisibilityOn();
  //
  //  auto surfaceActor = vtkSmartPointer<vtkActor>::New();
  //  surfaceActor->SetMapper(cutterMapper);
  //  surfaceActor->SetPosition(m_volume->GetPosition());
  //
  //  m_renderer->AddActor(surfaceActor);

  auto cutterPlane = vtkSmartPointer<vtkPlane>::New();
  cutterPlane->SetNormal(0, -1.0, 0);
  cutterPlane->SetOrigin(0.0, meshBounds[3], 0.0);

  auto cutter = vtkSmartPointer<vtkCutter>::New();
  cutter->SetInputData(m_polyData);
  cutter->SetCutFunction(cutterPlane);
  cutter->Update();

  auto cutterMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  cutterMapper->SetInputConnection(cutter->GetOutputPort());
  cutterMapper->ScalarVisibilityOn();

  auto surfaceActor = vtkSmartPointer<vtkActor>::New();
  surfaceActor->SetMapper(cutterMapper);
  surfaceActor->GetProperty()->SetColor(0, 0, 1);
  surfaceActor->GetProperty()->SetLineWidth(3);
  surfaceActor->GetProperty()->ShadingOn();
  surfaceActor->SetPosition(m_mesh->GetPosition()[0], m_mesh->GetPosition()[1] + 0.1, m_mesh->GetPosition()[2]);

  m_renderer->AddActor(surfaceActor);

  double worldPoint = meshActorBounds[3]; // std:min(meshActorBounds[3], volumeBounds[3]);

  std::cout << "starting with " << worldPoint << " - in mesh: " << world2mesh(worldPoint) << " - in image: " << world2volu(worldPoint) << std::endl;
  std::cout << "valid in mesh " << (betweenMesh(world2mesh(worldPoint)) ? "true" : "false") << " - valid in image " << (betweenImage(world2volu(worldPoint)) ? "true" : "false") << std::endl << std::flush;

  while (betweenMesh(world2mesh(worldPoint - step)) && betweenImage(world2volu(worldPoint - step)))
  {
    if (m_abort) return;

    //    int oExtent[6];
    //    resliceMapper->GetOutput()->GetExtent(oExtent);
    //    std::cout << "output extent " << oExtent[0] << "," << oExtent[1] << "," << oExtent[2] << "," << oExtent[3] << "," << oExtent[4] << "," << oExtent[5] << std::endl;
    auto imagePoint = world2volu(worldPoint);
    sliceMapper->SetSliceNumber(imagePoint / imageSpacing[1]);
    sliceMapper->Update();
    slice->Modified();

    //    matrix->SetElement(1, 3, imagePoint);
    //    matrix->Modified();
    //    reslice->Update();
    //    resliceMapper->UpdateWholeExtent();
    //    resliceActor->SetDisplayExtent(oExtent);
    //    resliceActor->Modified();

    auto meshPoint = world2mesh(worldPoint);
    cutterPlane->SetOrigin(0, meshPoint + 0.1, 0);

    //auto meshPoint = world2mesh(worldPoint);
    m_plane->SetOrigin(0, meshPoint, 0);

    //    std::cout << "world point " << worldPoint << std::endl << std::flush;
    worldPoint -= step;
    waitForFrameToRender();
  }

  worldPoint += 2 * step;

  while (betweenMesh(world2mesh(worldPoint)) && betweenImage(world2volu(worldPoint)))
  {
    if (m_abort) return;

    //    int oExtent[6];
    //    resliceMapper->GetOutput()->GetExtent(oExtent);
    //    std::cout << "output extent " << oExtent[0] << "," << oExtent[1] << "," << oExtent[2] << "," << oExtent[3] << "," << oExtent[4] << "," << oExtent[5] << std::endl;
    auto imagePoint = world2volu(worldPoint);
    sliceMapper->SetSliceNumber(imagePoint / imageSpacing[1]);
    sliceMapper->Update();
    slice->Modified();

    //    matrix->SetElement(1, 3, imagePoint);
    //    matrix->Modified();
    //    reslice->Update();
    //    resliceMapper->UpdateWholeExtent();
    //    resliceActor->SetDisplayExtent(oExtent);
    //    resliceActor->Modified();

    auto meshPoint = world2mesh(worldPoint);
    cutterPlane->SetOrigin(0, meshPoint + 0.1, 0);

    //auto meshPoint = world2mesh(worldPoint);
    m_plane->SetOrigin(0, meshPoint, 0);

    //    std::cout << "world point " << worldPoint << std::endl << std::flush;
    worldPoint += step;
    waitForFrameToRender();
  }

  m_renderer->RemoveActor(slice);
  m_renderer->RemoveActor(surfaceActor);
}
