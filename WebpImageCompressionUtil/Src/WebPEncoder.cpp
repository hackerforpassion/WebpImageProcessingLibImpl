/********************************************************************************************************************************************************************************************
* FileName   : WebPEncoder.cpp
* Description: LibWebP encoder wrapper
* Date		 : 25/5/2017
* Author     : Ramesh Kumar K
*
********************************************************************************************************************************************************************************************/

# include "WebPEncoder.h"
#ifdef _USE_WEBP_
# include "webp/encode.h"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _UNIT_TEST_WEBP
#include "imageio/image_dec.h"
#include "imageio/imageio_util.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Macros and forward declarations
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef WEBP_DLL
#ifdef __cplusplus
extern "C" {
#endif

extern void* VP8GetCPUInfo;   // opaque forward declaration.

#ifdef __cplusplus
}    // extern "C"
#endif
#endif  // WEBP_DLL


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Static and globals variables
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _USE_WEBP_

static WebPConfig												m_webp_config;

#endif

#define TRACE(...)

/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
*
* Error messages
*
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#ifdef _USE_WEBP_

static const char* const kErrorMessages[VP8_ENC_ERROR_LAST] = {
  "OK",
  "OUT_OF_MEMORY: Out of memory allocating objects",
  "BITSTREAM_OUT_OF_MEMORY: Out of memory re-allocating byte buffer",
  "NULL_PARAMETER: NULL parameter passed to function",
  "INVALID_CONFIGURATION: configuration is invalid",
  "BAD_DIMENSION: Bad picture dimension. Maximum width and height "
  "allowed is 16383 pixels.",
  "PARTITION0_OVERFLOW: Partition #0 is too big to fit 512k.\n"
  "To reduce the size of this partition, try using less segments "
  "with the -segments option, and eventually reduce the number of "
  "header bits using -partition_limit. More details are available "
  "in the manual (`man cwebp`)",
  "PARTITION_OVERFLOW: Partition is too big to fit 16M",
  "BAD_WRITE: Picture writer returned an I/O error",
  "FILE_TOO_BIG: File would be too big to fit in 4G",
  "USER_ABORT: encoding abort requested by user"
};

#endif

#ifdef _UNIT_TEST_WEBP

static int ReadYUV(const uint8_t* const data, size_t data_size,
                   WebPPicture* const pic) {
  const int use_argb = pic->use_argb;
  const int uv_width = (pic->width + 1) / 2;
  const int uv_height = (pic->height + 1) / 2;
  const int y_plane_size = pic->width * pic->height;
  const int uv_plane_size = uv_width * uv_height;
  const size_t expected_data_size = y_plane_size + 2 * uv_plane_size;

  if (data_size != expected_data_size) {
    fprintf(stderr,
            "input data doesn't have the expected size (%d instead of %d)\n",
            (int)data_size, (int)expected_data_size);
    return 0;
  }

  pic->use_argb = 0;
  if (!WebPPictureAlloc(pic)) return 0;
  ImgIoUtilCopyPlane(data, pic->width, pic->y, pic->y_stride,
                     pic->width, pic->height);
  ImgIoUtilCopyPlane(data + y_plane_size, uv_width,
                     pic->u, pic->uv_stride, uv_width, uv_height);
  ImgIoUtilCopyPlane(data + y_plane_size + uv_plane_size, uv_width,
                     pic->v, pic->uv_stride, uv_width, uv_height);
  return use_argb ? WebPPictureYUVAToARGB(pic) : 1;
}

#ifdef HAVE_WINCODEC_H

static int ReadPicture(const char* const filename, WebPPicture* const pic,
                       int keep_alpha, Metadata* const metadata) {
  int ok = 0;
  const uint8_t* data = NULL;
  size_t data_size = 0;
  if (pic->width != 0 && pic->height != 0) {
    ok = ImgIoUtilReadFile(filename, &data, &data_size);
    ok = ok && ReadYUV(data, data_size, pic);
  } else {
    // If no size specified, try to decode it using WIC.
    ok = ReadPictureWithWIC(filename, pic, keep_alpha, metadata);
    if (!ok) {
      ok = ImgIoUtilReadFile(filename, &data, &data_size);
      ok = ok && ReadWebP(data, data_size, pic, keep_alpha, metadata);
    }
  }
  if (!ok) {
    LOG(CRITICAL) << _T("Error! Could not process file ") << filename;
  }
  free((void*)data);
  return ok;
}

#else  // !HAVE_WINCODEC_H

static int ReadPicture(const TCHAR* const filename, WebPPicture* const pic,
                       int keep_alpha, Metadata* const metadata) {
  const uint8_t* data = NULL;
  size_t data_size = 0;
  int ok = 0;

  ok = ImgIoUtilReadFile(filename, &data, &data_size);
  if (!ok) goto End;

  if (pic->width == 0 || pic->height == 0) {
    WebPImageReader reader = WebPGuessImageReader(data, data_size);
    ok = reader(data, data_size, pic, keep_alpha, metadata);
  } else {
    // If image size is specified, infer it as YUV format.
    ok = ReadYUV(data, data_size, pic);
  }
 End:
  if (!ok) {
    LOG(CRITICAL) << _T("Error! Could not process file ") << filename;
  }
  free((void*)data);
  return ok;
}

// Dumps a picture as a PGM file using the IMC4 layout.
static int DumpPicture(const WebPPicture* const picture, const char* PGM_name) {
  int y;
  const int uv_width = (picture->width + 1) / 2;
  const int uv_height = (picture->height + 1) / 2;
  const int stride = (picture->width + 1) & ~1;
  const uint8_t* src_y = picture->y;
  const uint8_t* src_u = picture->u;
  const uint8_t* src_v = picture->v;
  const uint8_t* src_a = picture->a;
  const int alpha_height =
      WebPPictureHasTransparency(picture) ? picture->height : 0;
  const int height = picture->height + uv_height + alpha_height;
  FILE* const f = fopen(PGM_name, "wb");
  if (f == NULL) return 0;
  fprintf(f, "P5\n%d %d\n255\n", stride, height);
  for (y = 0; y < picture->height; ++y) {
    if (fwrite(src_y, picture->width, 1, f) != 1) return 0;
    if (picture->width & 1) fputc(0, f);  // pad
    src_y += picture->y_stride;
  }
  for (y = 0; y < uv_height; ++y) {
    if (fwrite(src_u, uv_width, 1, f) != 1) return 0;
    if (fwrite(src_v, uv_width, 1, f) != 1) return 0;
    src_u += picture->uv_stride;
    src_v += picture->uv_stride;
  }
  for (y = 0; y < alpha_height; ++y) {
    if (fwrite(src_a, picture->width, 1, f) != 1) return 0;
    if (picture->width & 1) fputc(0, f);  // pad
    src_a += picture->a_stride;
  }
  fclose(f);
  return 1;
}

static int MyWriter(const uint8_t* data, size_t data_size,
                    const WebPPicture* const pic) {
  FILE* const out = (FILE*)pic->custom_ptr;
  return data_size ? (fwrite(data, data_size, 1, out) == 1) : 1;
}


#endif
#endif

/*
* Constructor
*/

WebpEncoder::WebpEncoder()
{
	n_pixel_format						= PIXEL_FORMAT_RGBA;

	background_color					= 0xffffffu;

#ifdef _USE_WEBP_
	WebPConfigInit(&m_webp_config);
#endif

}

/*
* Destructor
*/
WebpEncoder::~WebpEncoder()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// function definations
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
* Sets the pixel format to be used to handle the images
*/
bool WebpEncoder::SetPixelFormat( IMG_PIXEL_FORMATS pixel_format )
{
	n_pixel_format = pixel_format;

	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Inits the encoder with the image quality information
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool WebpEncoder::InitEncoder( ImageCompressionProperties	&config )
{

#ifdef _USE_WEBP_

	switch (n_pixel_format)
	{
	case PIXEL_FORMAT_RGBA:
		{
			ptr_encode_image = WebPEncodeRGBA;
		}

		break;

	case PIXEL_FORMAT_RGB:
		{
			ptr_encode_image = WebPEncodeRGB;
		}

		break;

	default:

		{
			ptr_encode_image = WebPEncodeRGBA;
		}
	}

	m_webp_config.quality			= config.f_quality_factor;
	m_webp_config.alpha_quality		= config.n_alpha_quality;
	m_webp_config.lossless			= config.b_use_lossless_image_compression;
	m_webp_config.method			= config.n_speed;

	m_webp_config.alpha_compression = false;
	m_webp_config.alpha_filtering	= false;
	m_webp_config.sns_strength		= 0;
	m_webp_config.use_sharp_yuv		= false;
	m_webp_config.autofilter		= false;
	m_webp_config.filter_type		= 0;
	m_webp_config.filter_sharpness	= false;
	m_webp_config.filter_strength	= 0;

	m_webp_config.near_lossless		= false;
	m_webp_config.thread_level		= 1;

	WebPPreset preset = WEBP_PRESET_TEXT;
	
	if (!WebPConfigPreset(&m_webp_config, preset, m_webp_config.quality)) 
	{
		return false;
	}
	
	if (!WebPValidateConfig(&m_webp_config)) 
	{
		return false;
	}

#endif

	return true;
}

#ifdef _UNIT_TEST_WEBP

bool WebpEncoder::EncodeImageFromTestFile( _In_ const char *in_file, _Inout_ std::vector<char> &out_img, _Inout_ size_t &output_size )
{
		bool result						= true;

		bool keep_metadata				= false;

		Metadata metadata; 
		MetadataInit(&metadata);

		int									return_value = false;

		WebPPicture							picture;
		WebPMemoryWriter					memory_writer;

		WebPMemoryWriterInit(&memory_writer);
		
		if (!WebPPictureInit(&picture))
		{
			TRACE(_T("Error! Version mismatch!"));
			return false;
		}
		
		// Read the input. We need to decide if we prefer ARGB or YUVA
		// samples, depending on the expected compression mode (this saves
		// some conversion steps).

		if (!ReadPicture(in_file, &picture, true, false == keep_metadata ? NULL : &metadata)) 
		{
			TRACE(_T("Error! Cannot read input picture file "));
			goto Error;
		}
		  
		picture.use_argb				= false;

		picture.writer					= WebPMemoryWrite;
		picture.custom_ptr				= (void*)&memory_writer;
	
#ifdef WEBP_PREPROCESS_IMAGE

		if (b_blend_alpha) 
		{
			WebPBlendAlpha(&picture, background_color);
		}
		
		if (b_crop)
		{
			// We use self-cropping using a view.
			if (!WebPPictureView(&picture, crop_x, crop_y, crop_w, crop_h, &picture)) 
			{
				TRACE(_T("Error! Cannot crop picture"));
				goto Error;
			}
		}
		
		if (b_scale)
		{
			if ((resize_w | resize_h) > 0) 
			{
				if (!WebPPictureRescale(&picture, resize_w, resize_h)) 
				{
					TRACE(_T("Error! Cannot resize picture"));
					goto Error;
				}
			}
		}

#endif

		// Compress.
		if (!WebPEncode(&m_webp_config, &picture)) 
		{
			TRACE(_T("Error! Cannot encode picture as WebP Error code: %d (%s)"), picture.error_code, kErrorMessages[picture.error_code]);
			goto Error;
		}

		output_size					= memory_writer.size;

		out_img.assign(memory_writer.mem, memory_writer.mem  + output_size);

		return_value = true;

Error:
		WebPMemoryWriterClear(&memory_writer);
		MetadataFree(&metadata);
		WebPPictureFree(&picture);

		return return_value;
}


bool WebpEncoder::EncodeImage( _In_ uint8_t* in_image, _In_ unsigned int width, _In_ unsigned int height, _In_ unsigned int	n_bytes_per_pixel, _In_ float f_quality_factor, _Inout_ std::vector<char> &out_img, _Inout_ size_t &output_size )
{
	bool result = true;

	if (ptr_encode_image)
	{
		uint8_t *lpByte = NULL;

		output_size = ptr_encode_image(in_image, width, height, width * n_bytes_per_pixel, f_quality_factor, &lpByte);


		if (lpByte)
		{
			out_img.assign(lpByte, lpByte + output_size);

			WebPFree(lpByte);
		}
		else
		{
			result = false;
		}
	}
	else
	{
		result = false;
	}

	return result;
}

bool WebpEncoder::EncodeImageAndWriteToFile(
																_In_					uint8_t*													in_image, 
																_In_					unsigned int														width, 
																_In_					unsigned int														height, 
																_In_					unsigned int														n_bytes_per_pixel,
																_In_					const char*													out_file
							  )
{
		int									return_value = false;


#ifdef _USE_WEBP_

		WebPPicture							picture;
		WebPMemoryWriter					memory_writer;

		WebPMemoryWriterInit(&memory_writer);
		
		if (!WebPPictureInit(&picture))
		{
			TRACE(_T("Error! Version mismatch!"));
			return false;
		}
		
		// Read the input. We need to decide if we prefer ARGB or YUVA
		// samples, depending on the expected compression mode (this saves
		// some conversion steps).
		  
		picture.use_argb				= false;
		picture.width					= width;
		picture.height					= height;

		FILE *out = fopen(out_file, "wb");

		if (!out)
		{
			return false;
		}
		
		picture.writer = MyWriter;
		picture.custom_ptr = (void*)out;

		WebPPictureImportRGBX(&picture, in_image, width * n_bytes_per_pixel);   // convert from RGB to internal YUV

		//WebPPictureSharpARGBToYUVA(&picture);
	
#ifdef WEBP_PREPROCESS_IMAGE

		if (b_blend_alpha) 
		{
			WebPBlendAlpha(&picture, background_color);
		}
		
		if (b_crop)
		{
			// We use self-cropping using a view.
			if (!WebPPictureView(&picture, crop_x, crop_y, crop_w, crop_h, &picture)) 
			{
				TRACE(_T("Error! Cannot crop picture"));
				goto Error;
			}
		}
		
		if (b_scale)
		{
			if ((resize_w | resize_h) > 0) 
			{
				if (!WebPPictureRescale(&picture, resize_w, resize_h)) 
				{
					TRACE(_T("Error! Cannot resize picture"));
					goto Error;
				}
			}
		}

#endif

		// Compress.
		if (!WebPEncode(&m_webp_config, &picture)) 
		{
			TRACE(_T("Error! Cannot encode picture as WebP Error code: %d (%s)"), picture.error_code, kErrorMessages[picture.error_code]);
			goto Error;
		}

		return_value = true;

Error:
		WebPMemoryWriterClear(&memory_writer);
		WebPPictureFree(&picture);

		fclose(out);

#endif

		return return_value;
}

#endif

bool WebpEncoder::EncodeImage(
																_In_					uint8_t*													in_image, 
																_In_					unsigned int														width, 
																_In_					unsigned int														height, 
																_In_					unsigned int														n_bytes_per_pixel, 	
																_Inout_					std::vector<char>											&imgData, 
																_Inout_					size_t														&output_size 
							  )
{
		int									return_value = false;

#ifdef _USE_WEBP_

		WebPPicture							picture;
		WebPMemoryWriter					memory_writer;

		WebPMemoryWriterInit(&memory_writer);
		
		if (!WebPPictureInit(&picture))
		{
			TRACE(_T("Error! Version mismatch!"));
			return false;
		}
		
		// Read the input. We need to decide if we prefer ARGB or YUVA
		// samples, depending on the expected compression mode (this saves
		// some conversion steps).
		  
		picture.use_argb				= false;
		picture.width					= width;
		picture.height					= height;

		picture.writer					= WebPMemoryWrite;
		picture.custom_ptr				= (void*)&memory_writer;

		WebPPictureImportRGBX(&picture, in_image, width * n_bytes_per_pixel);   // convert from RGB to internal YUV

		//WebPPictureSharpARGBToYUVA(&picture);
	
#ifdef WEBP_PREPROCESS_IMAGE

		if (b_blend_alpha) 
		{
			WebPBlendAlpha(&picture, background_color);
		}
		
		if (b_crop)
		{
			// We use self-cropping using a view.
			if (!WebPPictureView(&picture, crop_x, crop_y, crop_w, crop_h, &picture)) 
			{
				TRACE(_T("Error! Cannot crop picture"));
				goto Error;
			}
		}
		
		if (b_scale)
		{
			if ((resize_w | resize_h) > 0) 
			{
				if (!WebPPictureRescale(&picture, resize_w, resize_h)) 
				{
					TRACE(_T("Error! Cannot resize picture"));
					goto Error;
				}
			}
		}

#endif
		
#ifdef _PRINT_IMG_CONVERSION_TIME
		unsigned long long conversion_start_time = GetCurrentTimeMillis();
#endif

		// Compress.
		if (!WebPEncode(&m_webp_config, &picture)) 
		{
			TRACE(_T("Error! Cannot encode picture as WebP Error code: %d (%s)"), picture.error_code, kErrorMessages[picture.error_code]);
			goto Error;
		}

		output_size					= memory_writer.size;

		imgData.assign(memory_writer.mem, memory_writer.mem  + output_size);

		return_value = true;

Error:
		WebPMemoryWriterClear(&memory_writer);
		WebPPictureFree(&picture);

#endif

		return return_value;
}