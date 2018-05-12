/********************************************************************************************************************************************************************************************
* FileName   : WebpDecoder.cpp
* Description: LibWebP encoder wrapper
* Date		 : 25/5/2017
* Author     : Ramesh Kumar K
*
********************************************************************************************************************************************************************************************/

# include "WebpDecoder.h"

#ifdef _USE_WEBP_
# include "webp/decode.h"
#endif

WebpDecoder::WebpDecoder()
{
	n_pixel_format						= PIXEL_FORMAT_RGBA;
	ptr_decode_image					= NULL;
}

WebpDecoder::~WebpDecoder()
{

}

void WebpDecoder::InitDecoder( )
{

#ifdef _USE_WEBP_

	switch (n_pixel_format)
	{
	case PIXEL_FORMAT_RGBA:
		{
			ptr_decode_image = WebPDecodeRGBA;
		}

		break;

	case PIXEL_FORMAT_RGB:
		{
			ptr_decode_image = WebPDecodeRGB;
		}

		break;

	default:

		{
			ptr_decode_image = WebPDecodeRGBA;
		}
	}

#endif

}

bool WebpDecoder::SetOutPutPixelFormat( IMG_PIXEL_FORMATS pixel_format )
{
	n_pixel_format	= pixel_format;

	return true;
}

bool WebpDecoder::DecodeImage( _In_ const uint8_t* in_image, _In_ size_t data_size, _In_ int &width, _In_ int &height, _Inout_ uint8_t** out_image )
{
	bool result = true;

	try
	{
		*out_image = ptr_decode_image(in_image, data_size, &width, &height);
	}
	catch(...) 
	{
		result = false;
	}

	return result;
}