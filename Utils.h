/*
		File: Utils.h
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

#ifndef UTILS_H_
#define UTILS_H_

// VTK
#include <itkImageRegionConstIterator.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>

// Qt
#include <QString>

// C++
#include <exception>

class vtkPolyData;

/** \brief Helper method to blend two pictures of the same size producing 'steps' intermediate pictures. Returns
 *         true on success and false otherwise. The output properties are the same that the first input image.
 * \param[in] first First picture.
 * \param[in] second Second picture.
 * \param[in] steps Number of intermediate pictures, steps >= 1.
 * \param[in] filename Output filename, it will add %d after the name. It will be saved in the application directory.
 *
 */
static bool blendPictures(const vtkSmartPointer<vtkImageData> first,
                          const vtkSmartPointer<vtkImageData> second,
                          const int steps, const QString filename);

/** \brief Print the minimum and maximum values of the given image. The minimum skips 0.
 * \param[in] image itk::Image pointer.
 *
 */
template<typename T> void minmax(const typename T::Pointer image)
{
  auto min = std::numeric_limits<typename T::ValueType>::max();
  auto max = std::numeric_limits<typename T::ValueType>::min();

  if(image)
  {
    auto it = itk::ImageRegionConstIterator<T>(image, image->GetLargestPossibleRegion());
    it.GoToBegin();
    while(!it.IsAtEnd())
    {
      if(it.Value() != 0) min = std::min(min, it.Value());
      max = std::max(max, it.Value());

      ++it;
    }
  }

  std::cout << "image min: " << static_cast<int>(min) << std::endl;
  std::cout << "image max: " << static_cast<int>(max) << std::endl << std::flush;
}

/** \brief Returns a mesh generated from the given image with the given value.
 * \param[in] image vtkImageData smartpointer.
 * \param[in] value Numeric value for the marching cubes.
 *
 */
vtkSmartPointer<vtkPolyData> imageToMesh(const vtkSmartPointer<vtkImageData> image, const unsigned char value);

/** \brief Helper to save an image to disk. Returns false if file exists or no valid image is given, returns true otherwise.
 * \param[in] image VTK image smartpointer.
 * \param[in] filename Name of file on disk.
 *
 */
bool saveImageToDisk(const vtkSmartPointer<vtkImageData> &image, const QString &filename);

/** \brief Helper to save a mesh to disk. Returns false if file exists or no valid mesh is given, returns true otherwise.
 * \param[in] mesh VTK mesh smartpointer.
 * \param[in] filename Name of file on disk.
 *
 */
bool saveMeshToDisk(const vtkSmartPointer<vtkPolyData> &mesh, const QString &filename);

/** \brief Helper to save a 2D image to disk. Does nothing if the input image is 3D.
 * \param[in] image Image to save.
 * \param[in] filename Image filename on disk.
 *
 */
void savePNG(vtkImageData *image, const QString &filename);


#endif // UTILS_H_

