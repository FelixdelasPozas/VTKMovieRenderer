/*
		File: Utils.cpp
    Created on: 15/11/2018
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
#include "Utils.h"

// VTK
#include <vtkImageBlend.h>
#include <vtkImageData.h>
#include <vtkImageWriter.h>
#include <vtkMarchingCubes.h>
#include <vtkPolyDataNormals.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkCleanPolyData.h>
#include <vtkMetaImageWriter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkPNGWriter.h>

// Qt
#include <QApplication>
#include <QDir>
#include <QDebug>

// C++
#include <cstring>
#include <iostream>

//--------------------------------------------------------------------
bool blendPictures(const vtkSmartPointer<vtkImageData> first, const vtkSmartPointer<vtkImageData> second, const int steps, const QString filename)
{
  if(!first || !second || steps < 1 || filename.isEmpty()) return false;

  double step = 1./steps;
  auto name = QApplication::applicationDirPath() + QDir::separator() + QString{filename + "-%1.png"};

  auto blend = vtkSmartPointer<vtkImageBlend>::New();
  blend->AddInputData(1, first);
  blend->AddInputData(1, second);

  for(int i = 1; i <= steps; ++i)
  {
    blend->SetOpacity(0, step * i);
    blend->SetOpacity(1, 1 - (step * i));
    blend->Update();

    auto writer = vtkSmartPointer<vtkImageWriter>::New();
    writer->SetFileName(name.arg(i).toStdString().c_str());
    writer->Update();
  }

  return true;
}

//--------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> imageToMesh(const vtkSmartPointer<vtkImageData> image, const unsigned char value)
{
  if(!image) return nullptr;

  // Marching cubes, mesh smoothing.
  auto surface = vtkSmartPointer<vtkMarchingCubes>::New();
  surface->SetInputData(image);
  surface->SetComputeNormals(true);
  surface->SetValue(0, value);
  surface->Update();

  // Remove any duplicate points.
  auto cleanFilter = vtkSmartPointer<vtkCleanPolyData>::New();
  cleanFilter->SetInputData(surface->GetOutput());
  cleanFilter->Update();

  auto smoothFilter = vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
  smoothFilter->SetInputConnection(cleanFilter->GetOutputPort());
  smoothFilter->SetNumberOfIterations(250);
  smoothFilter->SetRelaxationFactor(0.1);
  smoothFilter->SetFeatureAngle(95);
  smoothFilter->SetEdgeAngle(95);
  smoothFilter->FeatureEdgeSmoothingOn();
  smoothFilter->BoundarySmoothingOn();
  smoothFilter->Update();

  auto normalsGenerator = vtkSmartPointer<vtkPolyDataNormals>::New();
  normalsGenerator->DebugOn();
  normalsGenerator->GlobalWarningDisplayOn();
  normalsGenerator->SetInputData(smoothFilter->GetOutput());
  normalsGenerator->ReleaseDataFlagOff();
  normalsGenerator->SetSplitting(1);
  normalsGenerator->SetConsistency(0);
  normalsGenerator->ComputeCellNormalsOn();
  normalsGenerator->ComputePointNormalsOn();
  normalsGenerator->SetFlipNormals(0);
  normalsGenerator->SetNonManifoldTraversal(1);
  normalsGenerator->Update();

  auto data = vtkSmartPointer<vtkPolyData>::New();
  data->DeepCopy(normalsGenerator->GetOutput());

  return data;
}

//--------------------------------------------------------------------
bool saveImageToDisk(const vtkSmartPointer<vtkImageData>& image, const QString& filename)
{
  if(!image)
  {
    qDebug() << "saveImageToDisk - no image. Cant save" << filename;
    return false;
  }

  std::ifstream file(filename.toStdString().c_str());
  file.open(filename.toStdString(), std::ios_base::openmode::_S_in);
  if(file.is_open())
  {
    qDebug() << "saveImageToDisk - already exists. Cant save" << filename;
    file.close();
    return false;
  }

  auto writer = vtkSmartPointer<vtkMetaImageWriter>::New();
  writer->SetInputData(image);
  writer->SetFileName(filename.toStdString().c_str());
  writer->Write();

  return true;

}

//--------------------------------------------------------------------
bool saveMeshToDisk(const vtkSmartPointer<vtkPolyData>& mesh, const QString& filename)
{
  if(!mesh)
  {
    qDebug() << "saveMeshToDisk - no mesh. Cant save" << filename;
    return false;
  }

  std::ifstream file(filename.toStdString().c_str());
  file.open(filename.toStdString(), std::ios_base::openmode::_S_in);
  if(file.is_open())
  {
    qDebug() << "saveMeshToDisk - already exists. Cant save" << filename;
    file.close();
    return false;
  }

  auto writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  writer->SetInputData(mesh);
  writer->SetFileName(filename.toStdString().c_str());
  writer->Write();

  return true;
}

//--------------------------------------------------------------------
void savePNG(vtkImageData *image, const QString &filename)
{
  if(!image)
  {
    qDebug() << "savePNG - no image! Can't save" << filename;
    return;
  }

  std::ifstream file(filename.toStdString().c_str());
  file.open(filename.toStdString(), std::ios_base::openmode::_S_in);
  if(file.is_open())
  {
    qDebug() << "savePNG - already exists! Can't save" << filename;
    file.close();
    return;
  }

  auto writer = vtkSmartPointer<vtkPNGWriter>::New();
  writer->SetInputData(image);
  writer->SetFileDimensionality(2);
  writer->SetFileName(filename.toStdString().c_str());
  writer->Write();
}
