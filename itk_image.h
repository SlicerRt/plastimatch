/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _itk_image_h_
#define _itk_image_h_

#include "plm_config.h"
#include <stdio.h>
#include "itkImage.h"
#if (PLM_ITK_ORIENTED_IMAGES)
#include "itkOrientedImage.h"
#endif
#include "itkImageIOBase.h"
#include "plm_image_type.h"
#include "plm_image_patient_position.h"

// const unsigned int Dimension = 3;

/* 4D images */
typedef itk::Image < unsigned char, 4 > UCharImage4DType;

/* 3D images */
typedef itk::Image < unsigned char, 3 > UCharImageType;
typedef itk::Image < short, 3 > ShortImageType;
typedef itk::Image < unsigned short, 3 > UShortImageType;
#if (CMAKE_SIZEOF_UINT == 4)
typedef itk::Image < int, 3 > Int32ImageType;
typedef itk::Image < unsigned int, 3 > UInt32ImageType;
#else
typedef itk::Image < long, 3 > Int32ImageType;
typedef itk::Image < unsigned long, 3 > UInt32ImageType;
#endif
typedef itk::Image < float, 3 > FloatImageType;
typedef itk::Image < double, 3 > DoubleImageType;

/* 2D images */
typedef itk::Image < unsigned char, 2 > UCharImage2DType;
typedef itk::Image < short, 2 > ShortImage2DType;
typedef itk::Image < unsigned short, 2 > UShortImage2DType;
#if (CMAKE_SIZEOF_UINT == 4)
typedef itk::Image < int, 2 > Int32Image2DType;
typedef itk::Image < unsigned int, 2 > UInt32Image2DType;
#else
typedef itk::Image < long, 2 > Int32Image2DType;
typedef itk::Image < unsigned long, 2 > UInt32Image2DType;
#endif
typedef itk::Image < float, 2 > FloatImage2DType;
typedef itk::Image < double, 2 > DoubleImage2DType;

/* Points & vectors */
typedef itk::Point < float, 3 > FloatPointType;
typedef itk::Point < double, 3 > DoublePointType;

typedef itk::Vector < float, 3 > FloatVectorType;
typedef itk::Vector < double, 3 > DoubleVectorType;

/* Vector field */
typedef itk::Image < FloatVectorType, 3 > DeformationFieldType;

/* Other types */
typedef itk::Size < 3 > SizeType;
typedef itk::Point < double, 3 >  OriginType;
typedef itk::Vector < double, 3 > SpacingType;
typedef itk::Matrix<double, 3, 3> DirectionType;
typedef itk::ImageRegion < 3 > ImageRegionType;

/* -----------------------------------------------------------------------
   Function prototypes
   ----------------------------------------------------------------------- */
plastimatch1_EXPORT UCharImageType::Pointer itk_image_load_uchar (const char* fname, Plm_image_type* original_type);
plastimatch1_EXPORT ShortImageType::Pointer itk_image_load_short (const char* fname, Plm_image_type* original_type);
plastimatch1_EXPORT UShortImageType::Pointer itk_image_load_ushort (const char* fname, Plm_image_type* original_type);
plastimatch1_EXPORT Int32ImageType::Pointer itk_image_load_int32 (const char* fname, Plm_image_type* original_type);
plastimatch1_EXPORT UInt32ImageType::Pointer itk_image_load_uint32 (const char* fname, Plm_image_type* original_type);
plastimatch1_EXPORT FloatImageType::Pointer itk_image_load_float (const char* fname, Plm_image_type* original_type);
plastimatch1_EXPORT DeformationFieldType::Pointer itk_image_load_float_field (const char* fname);

plastimatch1_EXPORT void itk__GetImageType (std::string fileName,
			itk::ImageIOBase::IOPixelType &pixelType,
			itk::ImageIOBase::IOComponentType &componentType);

template<class T> plastimatch1_EXPORT void get_image_header (int dim[3], float offset[3], float spacing[3], T image);

template<class T> void itk_image_save (T img_ptr, const char* fname);
template<class T> void itk_image_save_short_dicom (T image, const char* dir_name, Plm_image_patient_position);
template<class T> plastimatch1_EXPORT void itk_image_save_uchar (T img_ptr, char* fname);
template<class T> plastimatch1_EXPORT void itk_image_save_short (T img_ptr, char* fname);
template<class T> plastimatch1_EXPORT void itk_image_save_ushort (T img_ptr, char* fname);
template<class T> plastimatch1_EXPORT void itk_image_save_uint32 (T img_ptr, char* fname);
template<class T> plastimatch1_EXPORT void itk_image_save_float (T img_ptr, const char* fname);
#endif
