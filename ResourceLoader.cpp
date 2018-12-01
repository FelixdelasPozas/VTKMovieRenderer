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

// Project
#include <ResourceLoader.h>
#include "Utils.h"

// Qt
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QIcon>
#include <QMessageBox>
#include <QDebug>

// VTK
#include <vtkActor2D.h>
#include <vtkVolume.h>
#include <vtkPlane.h>
#include <vtkImageData.h>
#include <vtkOBJReader.h>
#include <vtkCleanPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
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
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkDepthSortPolyData.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPoints.h>
#include <vtkImageResize.h>
#include <vtkImageInterpolator.h>

#include <itkImage.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>

//--------------------------------------------------------------------
void ResourceLoaderThread::run()
{
  // direct path to resources.
  auto currentDir = QCoreApplication::applicationDirPath() + "/resources/";
  auto logosofia  = currentDir + "FRS_M_ISCIII_FCIEN2.tif";
  auto logocajal  = currentDir + "cajalbbp.png";
  auto logoaa     = currentDir + "aa.png";

  meshLoader();

  // ACTORS REPOSITION (centers are in 0,0,0 for an easier rotation).
  // Values have been previously precomputed for the scene to rotate
  // the volumes and meshes in 0,0,0.
  const double center[3]{90.8, 108.8, 90.8};
  double position[3]{-90.8, -108.8, -90.8};

  for(auto vol: m_volumes)
  {
    vol->SetOrigin(center);
    vol->SetPosition(position);
  }

  for(auto data: m_polyDatas)
  {
    double point[3];
    auto points = data->GetPoints();
    for(int i = 0; i < points->GetNumberOfPoints(); ++i)
    {
      points->GetPoint(i, point);
      for(auto j: {0,1,2}) point[j] -= center[j];
      points->SetPoint(i, point);
    }
    data->Modified();
  }

  for(auto actor: m_actors)
  {
    actor->SetOrigin(center);
    actor->SetPosition(position);
  }

  // LOGOS LOADING & ACTOR CREATION
  QStringList logopics{logosofia, logocajal, logoaa};
  int xPos = 0; // precomputed value, modify SetPosition() line to reposition the logos.
  for(auto logo: logopics)
  {
    QFileInfo logoImageFile{logo};
    if(!logoImageFile.exists())
    {
      error(QString("Can't find %1").arg(logoImageFile.absoluteFilePath()));
      return;
    }

    auto image = vtkSmartPointer<vtkImageData>::New();
    if(logoImageFile.suffix() == "tif")
    {
      auto reader = vtkSmartPointer<vtkTIFFReader>::New();
      reader->SetFileName(logo.toStdString().c_str());
      reader->Update();

      image->DeepCopy(reader->GetOutput());
    }

    if(logoImageFile.suffix() == "png")
    {
      auto reader = vtkSmartPointer<vtkPNGReader>::New();
      reader->SetFileName(logo.toStdString().c_str());
      reader->Update();

      image->DeepCopy(reader->GetOutput());
    }

    auto interpolator = vtkSmartPointer<vtkImageInterpolator>::New();
    interpolator->SetInterpolationModeToCubic();

    auto resizer = vtkSmartPointer<vtkImageResize>::New();
    resizer->SetInputData(image);
    resizer->SetInterpolator(interpolator);
    resizer->SetOutputDimensions(image->GetDimensions()[0]*3, image->GetDimensions()[1]*3, 1);
    resizer->Update();

    auto width = resizer->GetOutput()->GetExtent()[1];
    image->DeepCopy(resizer->GetOutput());

    auto imageMapper = vtkSmartPointer<vtkImageMapper>::New();
    imageMapper->SetInputData(image);
    imageMapper->SetColorWindow(255);
    imageMapper->SetColorLevel(127.5);

    auto imageActor = vtkSmartPointer<vtkActor2D>::New();
    imageActor->SetMapper(imageMapper);
    imageActor->SetPosition(xPos, 0);

    xPos += width;

    m_logos << imageActor;
  }
}

//--------------------------------------------------------------------
void ResourceLoaderThread::volumeLoaderUCHAR()
{
  // resources filenames.
  auto currentDir = QCoreApplication::applicationDirPath() + "/resources/";
  auto data1      = currentDir + "new_avg-2.mhd";
  auto data2      = currentDir + "ConvMCI-2.mhd";

  // VOLUME LOADING & ACTOR CREATION
  QFileInfo imageFile{data1};
  if(!imageFile.exists())
  {
    error(QString("Can't find %1").arg(imageFile.absoluteFilePath()));
    return;
  }

  auto imageReader = vtkSmartPointer<vtkMetaImageReader>::New();
  imageReader->SetFileName(QDir::toNativeSeparators(imageFile.absoluteFilePath()).toStdString().c_str());
  imageReader->Update();

  auto image = vtkSmartPointer<vtkImageData>::New();
  image->DeepCopy(imageReader->GetOutput());
  image->SetSpacing(0.4, 0.4, 0.4);

  m_images << image;

  auto volumeMapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
  volumeMapper->SetBlendModeToMaximumIntensity();
  volumeMapper->SetInputData(image);

  auto volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
  volumeProperty->ShadeOff();
  volumeProperty->SetSpecular(0.1);
  volumeProperty->SetInterpolationTypeToLinear();

  auto compositeOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
  for(int i = 0; i < 256; ++i)
  {
    compositeOpacity->AddPoint(i, static_cast<double>(i)/255.);
  }
  volumeProperty->SetScalarOpacity(compositeOpacity); // composite first.

  auto color = vtkSmartPointer<vtkColorTransferFunction>::New();
  for(auto i = 0; i < 256; ++i)
  {
    auto value = static_cast<double>(i)*0.5 + (255.*0.1);
    color->AddRGBPoint(i,  value/255., value/255., value/255.);
  }
  volumeProperty->SetColor(color);

  auto volume = vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  m_volumes << volume;

  // MCI
  imageFile = QFileInfo{data2};
  if(!imageFile.exists())
  {
    error(QString("Can't find %1").arg(imageFile.absoluteFilePath()));
    return;
  }

  imageReader = vtkSmartPointer<vtkMetaImageReader>::New();
  imageReader->SetFileName(QDir::toNativeSeparators(imageFile.absoluteFilePath()).toStdString().c_str());
  imageReader->Update();

  image = vtkSmartPointer<vtkImageData>::New();
  image->DeepCopy(imageReader->GetOutput());
  image->SetSpacing(0.4, 0.4, 0.4);

  m_images << image;

  volumeMapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
  volumeMapper->SetBlendModeToComposite();
  volumeMapper->SetInputData(image);

  volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
  volumeProperty->ShadeOff();
  volumeProperty->SetSpecular(0.2);
  volumeProperty->SetInterpolationTypeToLinear();

  compositeOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
  compositeOpacity->AddPoint(0, 0);
  for(int i = 1; i < 256; ++i)
  {
    compositeOpacity->AddPoint(i, 1.);
  }
  volumeProperty->SetScalarOpacity(compositeOpacity); // composite first.

  color = vtkSmartPointer<vtkColorTransferFunction>::New();
  for(auto i = 0; i < 256; ++i)
  {
    auto qColor = QColor::fromHsv(i, 255, 255, 255);
    color->AddRGBPoint(i,  qColor.redF(), qColor.greenF(), qColor.blueF());
  }
  volumeProperty->SetColor(color);

  volume = vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  m_volumes << volume;
}

//--------------------------------------------------------------------
void ResourceLoaderThread::volumeLoaderUSHORT()
{
  auto currentDir = QCoreApplication::applicationDirPath() + "/resources/";
  auto data       = currentDir + "fusion.mhd";

  auto imageFile = QFileInfo{data};
  if(!imageFile.exists())
  {
    error(QString("Can't find %1").arg(imageFile.absoluteFilePath()));
    return;
  }

  auto imageReader = vtkSmartPointer<vtkMetaImageReader>::New();
  imageReader->SetFileName(QDir::toNativeSeparators(imageFile.absoluteFilePath()).toStdString().c_str());
  imageReader->Update();

  auto image = vtkSmartPointer<vtkImageData>::New();
  image->DeepCopy(imageReader->GetOutput());
  image->SetSpacing(0.4, 0.4, 0.4);

  m_images << image;

  auto volumeMapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
  volumeMapper->SetBlendModeToComposite();
  volumeMapper->SetInputData(image);

  auto volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
  volumeProperty->ShadeOff();
  volumeProperty->SetSpecular(0.3);
  volumeProperty->SetInterpolationTypeToNearest();

  auto compositeOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
  for(int i = 0; i < 512; ++i)
  {
    if(i < 256)
    {
      compositeOpacity->AddPoint(i, static_cast<double>(i)/255.);
    }
    else
    {
      compositeOpacity->AddPoint(i, static_cast<double>(i-255)/255.);
    }
  }
  volumeProperty->SetScalarOpacity(compositeOpacity); // composite first.

  auto color = vtkSmartPointer<vtkColorTransferFunction>::New();
  for(int i = 0; i < 512; ++i)
  {
    if(i < 256)
    {
      auto value = static_cast<double>(i)*0.5 + (255.*0.1);
      color->AddRGBPoint(i,  value/255., value/255., value/255.);
    }
    else
    {
      auto qColor = QColor::fromHsv(i-256, 255, 255, 255);
      color->AddRGBPoint(i,  qColor.redF(), qColor.greenF(), qColor.blueF());
    }
  }
  volumeProperty->SetColor(color);

  auto volume = vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  m_volumes << volume;
}

//--------------------------------------------------------------------
void ResourceLoaderThread::meshLoader()
{
  // resources filenames.
  auto currentDir = QCoreApplication::applicationDirPath() + "/resources/";
  auto data1      = currentDir + "new_avg-2.mhd";
  auto data2      = currentDir + "filtered.mhd";
  auto mesh1      = currentDir + "meshBrain.vtp";
  auto mesh2      = currentDir + "meshMCI.vtp";

  // VOLUME LOADING & ACTOR CREATION
  QFileInfo imageFile{data1};
  if(!imageFile.exists())
  {
    error(QString("Can't find %1").arg(imageFile.absoluteFilePath()));
    return;
  }

  auto imageReader = vtkSmartPointer<vtkMetaImageReader>::New();
  imageReader->SetFileName(QDir::toNativeSeparators(imageFile.absoluteFilePath()).toStdString().c_str());
  imageReader->Update();

  auto image = vtkSmartPointer<vtkImageData>::New();
  image->DeepCopy(imageReader->GetOutput());
  image->SetSpacing(0.4, 0.4, 0.4);

  m_images << image;

  QFileInfo meshFile{mesh1};
  if(!meshFile.exists())
  {
    error(QString("Can't find %1").arg(meshFile.absoluteFilePath()));
    return;
  }

  auto meshReader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  meshReader->SetFileName(meshFile.absoluteFilePath().toStdString().c_str());
  meshReader->Update();

  auto polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata->DeepCopy(meshReader->GetOutput());

  m_polyDatas << polydata;

  // MCI
  imageFile = QFileInfo{data2};
  if(!imageFile.exists())
  {
    error(QString("Can't find %1").arg(imageFile.absoluteFilePath()));
    return;
  }

  imageReader = vtkSmartPointer<vtkMetaImageReader>::New();
  imageReader->SetFileName(QDir::toNativeSeparators(imageFile.absoluteFilePath()).toStdString().c_str());
  imageReader->Update();

  image = vtkSmartPointer<vtkImageData>::New();
  image->DeepCopy(imageReader->GetOutput());
  image->SetSpacing(0.4, 0.4, 0.4);

  m_images << image;

  meshFile = QFileInfo{mesh2};
  if(!meshFile.exists())
  {
    error(QString("Can't find %1").arg(meshFile.absoluteFilePath()));
    return;
  }

  meshReader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  meshReader->SetFileName(meshFile.absoluteFilePath().toStdString().c_str());
  meshReader->Update();

  polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata->DeepCopy(meshReader->GetOutput());

  m_polyDatas << polydata;
}

//--------------------------------------------------------------------
void ResourceLoaderThread::imagePreprocessing()
{
  auto currentDir = QCoreApplication::applicationDirPath() + "/resources/";
  auto data1      = currentDir + "Conversion_to_MCI.nii";

  using FloatType = itk::Image<float, 3>;
  using ImageType = itk::Image<unsigned char, 3>;

  auto reader = itk::ImageFileReader<FloatType>::New();
  reader->SetFileName(data1.toStdString().c_str());
  reader->Update();

  auto image = reader->GetOutput();
  auto it = itk::ImageRegionIteratorWithIndex<FloatType>(image, image->GetLargestPossibleRegion());
  it.GoToBegin();

  auto uImage = ImageType::New();
  uImage->SetNumberOfComponentsPerPixel(1);
  uImage->SetRegions(image->GetLargestPossibleRegion());
  uImage->SetSpacing(image->GetSpacing());
  uImage->SetOrigin(image->GetOrigin());
  uImage->Allocate(true);
  auto uit = itk::ImageRegionIteratorWithIndex<ImageType>(uImage, uImage->GetLargestPossibleRegion());
  uit.GoToBegin();

  while(!it.IsAtEnd())
  {
    auto value = it.Value();
    value = (value < 4.7) ? 0. : (value - 4.6999);
    // [I] min 0 max 6.17444 other 3.30694
    // [I] min 0 max 1.47454 other 0.00100485
    unsigned char uValue = (value * 255)/1.47454;
    uit.Set(uValue);

    ++it;
    ++uit;
  }

  auto writer = itk::ImageFileWriter<ImageType>::New();
  writer->SetFileName(QString{currentDir + "filtered.mhd"}.toStdString().c_str());
  writer->SetInput(uImage);
  writer->Write();
}
