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

// Project
#include <ScriptExecutor.h>
#include <ResourceLoader.h>

// Qt
#include <QApplication>
#include <QDebug>

// VTK
#include <vtkSphereSource.h>
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkVolume.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkVolumeProperty.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkProperty.h>
#include <vtkMapper.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkImageReslice.h>
#include <vtkImageMapToColors.h>
#include <vtkTexture.h>
#include <vtkCutter.h>
#include <vtkDataSetMapper.h>
#include <vtkPNGWriter.h>
#include <vtkPointData.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>
#include <vtkImageProperty.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkImageProperty.h>
#include <vtkFloatArray.h>
#include <vtkScalarBarActor.h>
#include <vtkRenderWindow.h>
#include <vtkImageBlend.h>
#include <vtkCullerCollection.h>
#include <vtkCuller.h>
#include <vtkPlaneCollection.h>
#include <vtkPlane.h>
#include <vtkAbstractVolumeMapper.h>
#include <vtkClipPolyData.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkDepthSortPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkContourTriangulator.h>
#include <vtkImageCanvasSource2D.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

// C++
#include <cstring> // memcpy
#include <limits>  // min, max
#include <chrono>
#include <thread>

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

  fadeIn();

  if(m_abort) return;
  waitFrames(5);

  if(m_abort) return;
  reslice();

  if(m_abort) return;
  waitFrames(5);

  if(m_abort) return;
  fadeOut();

  if(m_abort) return;
  waitFrames(5);

  if(m_abort) return;
  threesixtynoscope();

  if(m_abort) return;
  waitFrames(10);

  // finished!
}

//--------------------------------------------------------------------
void ScriptExecutor::getResources(ResourceLoaderThread *loader)
{
  m_image     = loader->images().first();
  m_mciImage  = loader->images().last();
  m_brainMesh = loader->polyDatas().first();
  m_mciMesh   = loader->polyDatas().last();

  if(!m_renderer || !m_image || !m_mciImage || !m_brainMesh || !m_mciMesh)
  {
    error("Invalid data.");
    std::cout << "error in parameters in script executor" << std::endl << std::flush;
    return;
  }

  // Define a clipping plane
  m_plane = vtkSmartPointer<vtkPlane>::New();
  m_plane->SetNormal(0., -1., 0.);
  m_plane->SetOrigin(0, 108.8, 0);

  // Clip the source with the plane
  auto clipper = vtkSmartPointer<vtkClipPolyData>::New();
  clipper->DebugOn();
  clipper->GlobalWarningDisplayOn();
  clipper->ReleaseDataFlagOff();
  clipper->SetInputData(m_brainMesh);
  clipper->SetClipFunction(m_plane);

  //Create a mapper and actor
  auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->ReleaseDataFlagOff();
  mapper->SetInputConnection(clipper->GetOutputPort());
  mapper->SetScalarVisibility(false);

  m_brainActor = vtkSmartPointer<vtkActor>::New();
  m_brainActor->SetMapper(mapper);
  m_brainActor->GetProperty()->SetColor(0.3, 0.3, 0.3);
  m_brainActor->GetProperty()->SetInterpolationToPhong();
  m_brainActor->GetProperty()->SetOpacity(0.4);
  m_brainActor->GetProperty()->SetBackfaceCulling(true);
  m_brainActor->Modified();

  m_renderer->AddActor(m_brainActor);

  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(m_mciMesh);
  mapper->SetScalarVisibility(false);
  mapper->Update();

  m_mciActor = vtkSmartPointer<vtkActor>::New();
  m_mciActor->GetProperty()->SetColor(1.,0.,0.);
  m_mciActor->GetProperty()->SetBackfaceCulling(true);
  m_mciActor->GetProperty()->SetOpacity(0.6);
  m_mciActor->SetMapper(mapper);

  m_renderer->AddActor(m_mciActor);

  for(auto actor: loader->logos())
  {
    m_renderer->AddActor(actor);
  }

  auto windowSize = m_renderer->GetRenderWindow()->GetSize();

  auto textActor = vtkSmartPointer<vtkTextActor>::New();
  textActor->SetInput("Predictor of impending MCI");
  textActor->SetPosition(10, windowSize[1]-40);
// 4k doesn't scale 2D actors, needs those modifications.
//  textActor->SetPosition(100, 2160-140);
//  textActor->GetTextProperty()->SetFontSize(88);
  textActor->GetTextProperty()->SetFontFamilyToArial();
  textActor->GetTextProperty()->SetFontSize(28);
  textActor->GetTextProperty()->SetColor(1.0, 1.0, 1.0);

  m_renderer->AddActor2D(textActor);
}

//--------------------------------------------------------------------
void ScriptExecutor::waitForFrameToRender()
{
  QApplication::processEvents();

  if(!m_abort)
  {
    // really, this is needed to allow the vtk pipelines to finish before the main thread executes a
    // render in the main thread. Thinking on moving the execution thread to the same thread of the
    // application and remove this.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    emit render();

    m_mutex.lock();
    m_waitCondition.wait(&m_mutex);
    m_mutex.unlock();
  }
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
  for(double i = 0; i < 360.0; i += rad)
  {
    if(m_abort) return;

    m_brainActor->RotateWXYZ(rad, 0, 0, 1);
    m_mciActor->RotateWXYZ(rad, 0, 0, 1);

    waitForFrameToRender();
  }
}

//--------------------------------------------------------------------
void ScriptExecutor::reslice()
{
  const QString shotPath{"D:\\Descargas\\test\\"};

  // SCALAR BAR
  auto barLut = vtkSmartPointer<vtkLookupTable>::New();
  barLut->SetTableRange(4.2, 6.2);
  barLut->SetHueRange(0.0, 1.0);
  barLut->SetSaturationRange(1.0, 1.0);
  barLut->SetAlphaRange(1.0, 1.0);
  barLut->SetValueRange(1.0, 1.0);
  barLut->Build();

  auto scalarBar = vtkSmartPointer<vtkScalarBarActor>::New();
  scalarBar->SetLookupTable(barLut);
  scalarBar->SetTitle("t-values");
  scalarBar->SetTitleRatio(0.8);
  scalarBar->GetLabelTextProperty()->SetFontFamilyToArial();
  scalarBar->GetLabelTextProperty()->SetColor(1.0, 1.0, 1.0);
  scalarBar->SetNumberOfLabels(4);
// 4K doesn't scale 2D actors, needs those modifications.
//  scalarBar->GetLabelTextProperty()->SetFontSize(58);
//  scalarBar->SetMaximumHeightInPixels(1200);
//  scalarBar->SetMaximumWidthInPixels(140);
  scalarBar->SetMaximumHeightInPixels(400);
  scalarBar->SetMaximumWidthInPixels(50);
  scalarBar->SetAnnotationTextScaling(true);

  auto windowSize = m_renderer->GetRenderWindow()->GetSize();
  scalarBar->SetDisplayPosition(windowSize[0]-100, windowSize[1]/2 - 200);
//  scalarBar->SetDisplayPosition(3840-275, 480);
  scalarBar->Modified();

  m_renderer->AddActor(scalarBar);

  int extent[6];
  m_image->GetExtent(extent);

  const double coronal[16] = { 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1 };

  auto matrix = vtkSmartPointer<vtkMatrix4x4>::New();
  matrix->DeepCopy(coronal);

  // reslice pipeline to generate the brain texture.
  auto reslice = vtkSmartPointer<vtkImageReslice>::New();
  reslice->SetInputData(m_image);
  reslice->SetOutputDimensionality(2);
  reslice->SetNumberOfThreads(1);
  reslice->SetResliceAxes(matrix);
  reslice->SetInterpolationModeToCubic();
  reslice->SetOutputExtent(extent[0], extent[1], extent[4], extent[5], 0, 0);

  auto brainLookupTable = vtkSmartPointer<vtkLookupTable>::New();
  brainLookupTable->Allocate();
  brainLookupTable->SetTableRange(0,255);
  brainLookupTable->SetValueRange(0., 1.);
  brainLookupTable->SetHueRange(0.,0.);
  brainLookupTable->SetAlphaRange(1., 1.);
  brainLookupTable->SetNumberOfColors(256);
  brainLookupTable->SetRampToLinear();
  brainLookupTable->SetSaturationRange(0.,0.);
  brainLookupTable->SetNumberOfTableValues(256);
  brainLookupTable->Build();

  auto resliceMapper = vtkSmartPointer<vtkImageMapToColors>::New();
  resliceMapper->SetLookupTable(brainLookupTable);
  resliceMapper->SetNumberOfThreads(1);
  resliceMapper->SetOutputFormatToRGBA();
  resliceMapper->SetInputConnection(reslice->GetOutputPort());
  resliceMapper->SetUpdateExtentToWholeExtent();

  // other data: MCI
  auto resliceMCI = vtkSmartPointer<vtkImageReslice>::New();
  resliceMCI->SetInputData(m_mciImage);
  resliceMCI->SetOutputDimensionality(2);
  resliceMCI->SetNumberOfThreads(1);
  resliceMCI->SetResliceAxes(matrix);
  resliceMCI->SetInterpolationModeToCubic();
  resliceMCI->SetOutputExtent(extent[0], extent[1], extent[4], extent[5], 0, 0);

  auto MCILookupTable = vtkSmartPointer<vtkLookupTable>::New();
  MCILookupTable->Allocate();
  MCILookupTable->SetTableRange(150, 255);
  MCILookupTable->SetValueRange(1., 1.);
  MCILookupTable->SetHueRange(0.,1.);
  MCILookupTable->SetAlphaRange(1., 1.);
  MCILookupTable->SetSaturationRange(1.,1.);
  //MCILookupTable->SetNumberOfColors(256);
  MCILookupTable->SetRampToLinear();
  MCILookupTable->SetNumberOfTableValues(255-150);
  MCILookupTable->Build();

  auto rgba = MCILookupTable->GetTableValue(0);
  rgba[3] = 0.; // Make the first complete transparent.
  MCILookupTable->SetTableValue(0, 0,0,0);

  auto resliceMapperMCI = vtkSmartPointer<vtkImageMapToColors>::New();
  resliceMapperMCI->SetLookupTable(MCILookupTable);
  resliceMapperMCI->SetNumberOfThreads(1);
  resliceMapperMCI->SetOutputFormatToRGBA();
  resliceMapperMCI->SetInputConnection(resliceMCI->GetOutputPort());
  resliceMapperMCI->SetUpdateExtentToWholeExtent();

  auto blend = vtkSmartPointer<vtkImageBlend>::New();
  blend->AddInputData(resliceMapper->GetOutput());
  blend->AddInputData(resliceMapperMCI->GetOutput());
  blend->SetOpacity(0, 0.7);
  blend->SetOpacity(1, 0.3);
  blend->SetBlendModeToNormal();

  // texture of the slice actor.
  auto texture = vtkSmartPointer<vtkTexture>::New();
  texture->SetInputConnection(blend->GetOutputPort());
  texture->InterpolateOn();

  auto plane = vtkPlane::New();
  plane->SetOrigin(0, 108.8,0);
  plane->SetNormal(0.,-1.,0.);

  auto cutter = vtkSmartPointer<vtkCutter>::New();
  cutter->SetInputData(m_brainMesh);
  cutter->SetCutFunction(plane);
  cutter->Update();

  // triangulator fills the contour creating a polygon that can be textured.
  auto triangulator = vtkSmartPointer<vtkContourTriangulator>::New();
  triangulator->SetInputConnection(cutter->GetOutputPort());

  auto cutterMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  cutterMapper->SetInputData(triangulator->GetOutput());

  // final textured actor.
  auto surfaceActor = vtkSmartPointer<vtkActor>::New();
  surfaceActor->SetMapper(cutterMapper);
  surfaceActor->SetTexture(texture);

  m_renderer->AddActor(surfaceActor);

  auto step = 217.6/400.;
  const auto initialPoint = 108.8 - (20 * step);
  const auto finalPoint   = -108.8 + (20 * step);
  auto reslicePoint = initialPoint;

  auto length = 181.6; // the same for length in X and Z axis.

  while(reslicePoint > finalPoint)
  {
    matrix->SetElement(1, 3, reslicePoint + 108.8); // images have not been translated after loading.
    matrix->Modified();

    resliceMapper->Update();
    resliceMapperMCI->Update();
    blend->Update();

    texture->Update();

    plane->SetOrigin(0, reslicePoint, 0.);
    plane->Modified();

    triangulator->Update();

    auto data = triangulator->GetOutput();
    auto array = vtkSmartPointer<vtkFloatArray>::New();
    array->SetNumberOfComponents(3);
    array->SetNumberOfTuples(data->GetNumberOfPoints());
    array->SetName("TextureCoordinates");
    array->Allocate(data->GetNumberOfPoints());

    for(int i = 0; i < data->GetNumberOfPoints(); ++i)
    {
      double coords[3], tcoords[3];
      data->GetPoint(i, coords);
      tcoords[0] = (coords[0]+90.8)/length;
      tcoords[1] = (coords[2]+90.8)/length;
      tcoords[2] = 0;

      array->SetTuple(i, tcoords);
    }

    triangulator->GetOutput()->GetPointData()->SetTCoords(array);

    cutterMapper->SetInputData(triangulator->GetOutput());
    cutterMapper->Update();
    surfaceActor->Modified();

    m_plane->SetOrigin(0., reslicePoint, 0.);

    waitForFrameToRender();
    if(m_abort) return;

    reslicePoint -= step;
  }

  waitFrames(15);

  while(reslicePoint < initialPoint)
  {
    matrix->SetElement(1, 3, reslicePoint + 108.8); // images have not been translated after loading.
    matrix->Modified();

    resliceMapper->Update();
    resliceMapperMCI->Update();
    blend->Update();

    texture->Update();

    plane->SetOrigin(0, reslicePoint, 0.);
    plane->Modified();

    triangulator->Update();

    auto data = triangulator->GetOutput();
    auto array = vtkSmartPointer<vtkFloatArray>::New();
    array->SetNumberOfComponents(3);
    array->SetNumberOfTuples(data->GetNumberOfPoints());
    array->SetName("TextureCoordinates");
    array->Allocate(data->GetNumberOfPoints());

    for(int i = 0; i < data->GetNumberOfPoints(); ++i)
    {
      double coords[3], tcoords[3];
      data->GetPoint(i, coords);
      tcoords[0] = (coords[0]+90.8)/length;
      tcoords[1] = (coords[2]+90.8)/length;
      tcoords[2] = 0;

      array->SetTuple(i, tcoords);
    }

    triangulator->GetOutput()->GetPointData()->SetTCoords(array);

    cutterMapper->SetInputData(triangulator->GetOutput());
    cutterMapper->Update();
    surfaceActor->Modified();

    m_plane->SetOrigin(0., reslicePoint, 0.);

    waitForFrameToRender();
    if(m_abort) return;

    reslicePoint += step;
  }

  m_renderer->RemoveActor(scalarBar);
}

//--------------------------------------------------------------------
void ScriptExecutor::fadeIn()
{
  int mciOpacity = m_mciActor->GetProperty()->GetOpacity() * 100;
  int brainOpacity = m_brainActor->GetProperty()->GetOpacity() * 100;

  for(int i = 40; brainOpacity <= 100 && mciOpacity >= 0 && i <= 100; i+=2)
  {
    brainOpacity += 2;
    mciOpacity -= 2;
    m_mciActor->GetProperty()->SetOpacity(mciOpacity/100.);
    m_brainActor->GetProperty()->SetOpacity(brainOpacity/100.);

    waitForFrameToRender();
  }

  m_mciActor->GetProperty()->SetOpacity(0);
  m_brainActor->GetProperty()->SetOpacity(1);

  waitForFrameToRender();
}

//--------------------------------------------------------------------
void ScriptExecutor::fadeOut()
{
  int mciOpacity = m_mciActor->GetProperty()->GetOpacity() * 100;
  int brainOpacity = m_brainActor->GetProperty()->GetOpacity() * 100;

  for(int i = 0; brainOpacity >= 40 && mciOpacity <= 60; i+=2)
  {
    brainOpacity -= 2;
    mciOpacity += 2;
    m_mciActor->GetProperty()->SetOpacity(mciOpacity/100.);
    m_brainActor->GetProperty()->SetOpacity(brainOpacity/100.);

    waitForFrameToRender();
  }

  m_mciActor->GetProperty()->SetOpacity(0.6);
  m_brainActor->GetProperty()->SetOpacity(0.4);

  waitForFrameToRender();
}
