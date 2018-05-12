#pragma once
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	WebpDecoder.h
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
# include "stdint.h"
# include "WebpEncoder.h"

class WebpDecoder
{

private:

	uint8_t* (*ptr_decode_image)										( _In_ const uint8_t* data, _In_ size_t data_size, _In_ int* width, _In_ int* height); // function ptr to hold the decoder function's address

public:

	WebpDecoder															( );

	
	~WebpDecoder														( );

public:

	void		InitDecoder												( );

	bool		SetOutPutPixelFormat									( IMG_PIXEL_FORMATS pixel_format );

public:

	bool		DecodeImage												( _In_ const uint8_t* in_image, _In_ size_t image_size, _In_ int &width, _In_ int &height, _Inout_ uint8_t** out_image );


private:

	IMG_PIXEL_FORMATS													n_pixel_format;


};