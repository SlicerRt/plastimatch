/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkExtractImageFilter.h"
#include "itkImage.h"
#include "itk_image.h"
#include "slice_extract.h"

template<class T>
typename itk::Image<typename T::ObjectType::PixelType,2>::Pointer
slice_extract 
(
    T in_img, 
    int slice_no
)
{
    typedef typename T::ObjectType InImgType;
    typedef typename T::ObjectType::PixelType PixelType;
    typedef typename itk::Image<PixelType, 2> OutImgType;
    typedef typename itk::ExtractImageFilter<InImgType, OutImgType> FilterType;

    typename FilterType::Pointer extraction = FilterType::New();
	
    /* ??? */
    try {
	in_img->Update(); 
	//std::cout << "Ho letto!" << std::endl;
    }
    catch ( itk::ExceptionObject &err) {
	std::cout << "ExceptionObject caught a !" << std::endl; 
	std::cout << err << std::endl; 
	//return -1;
    }
	
    typename InImgType::RegionType inputRegion 
	= in_img->GetLargestPossibleRegion();
    typename InImgType::SizeType size = inputRegion.GetSize();
    size[2] = 0;
	
    typename InImgType::IndexType start = inputRegion.GetIndex(); 
    start[2]=slice_no;

    typename InImgType::RegionType desiredRegion; 
    desiredRegion.SetSize(size);
    desiredRegion.SetIndex(start);

    extraction->SetExtractionRegion(desiredRegion);
    extraction->SetInput(in_img);

    typename OutImgType::Pointer out_img = OutImgType::New();
	
    try
    {
	extraction->Update();
	out_img = extraction->GetOutput();
    }
    catch (itk::ExceptionObject &err)
    {
	std::cout << "ExceptionObject caught a !" << std::endl; 
	std::cout << err << std::endl; 
    }
    return out_img;
}

template plastimatch1_EXPORT UCharImage2DType::Pointer slice_extract (UCharImageType::Pointer, int);
template plastimatch1_EXPORT FloatImage2DType::Pointer slice_extract (FloatImageType::Pointer, int);
template plastimatch1_EXPORT UInt32Image2DType::Pointer slice_extract (UInt32ImageType::Pointer, int);

#if defined (commentout)
template plastimatch1_EXPORT UCharVecImage2DType::Pointer slice_extract<UCharVecType> (UCharVecImageType::Pointer reader, int index);
#endif
