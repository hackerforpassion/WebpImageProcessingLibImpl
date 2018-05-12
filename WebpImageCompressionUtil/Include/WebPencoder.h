#pragma once
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	WebpEncoder.h
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
# include "stdint.h"
# include <vector>

using namespace std;

enum IMG_PIXEL_FORMATS { PIXEL_FORMAT_RGBA, PIXEL_FORMAT_RGB };

struct ImageCompressionProperties
{
	unsigned int												n_bit_rate;

	IMG_PIXEL_FORMATS									pixel_format;

	bool												b_use_lossless_image_compression;

	float												f_quality_factor;

	float												f_image_scale_factor;

	bool												b_scale_image;

	bool												b_retain_alpha;

	int													n_alpha_quality;

	int													n_speed;

	bool												b_use_gamma_correction;

	bool												b_apply_histrogram_equalization;

	bool												b_use_sse2_instruction_set;

	bool												b_use_parallel_processing;

	bool												b_use_gpu_for_processing;



	ImageCompressionProperties()
	{
		n_bit_rate = 0;

		pixel_format = PIXEL_FORMAT_RGBA;

		b_use_lossless_image_compression = false;

		f_quality_factor = 75.0;

		f_image_scale_factor = 1.0;

		b_scale_image = false;

		b_retain_alpha = false;

		b_use_gamma_correction = false;

		b_apply_histrogram_equalization = false;

		b_use_sse2_instruction_set = true;

		b_use_parallel_processing = true;

		b_use_gpu_for_processing = true;
	}
};

class WebpEncoder
{

private:

	size_t		(*ptr_encode_image)										( _In_ const uint8_t* rgb, _In_ int width, _In_ int height, _In_ int stride, _In_ float quality_factor, _Inout_ uint8_t** output); // function ptr to hold the encoder function's address

public:

				WebpEncoder												( );

				~WebpEncoder											( );

public:

	bool		InitEncoder												( ImageCompressionProperties	&config );
				

public:

	bool		SetPixelFormat											( IMG_PIXEL_FORMATS pixel_format );

	bool		EncodeImage												( _In_ uint8_t* in_image, _In_ unsigned int width, _In_ unsigned int height, _In_ unsigned int	n_bytes_per_pixel, _In_ float f_quality_factor, _Inout_ std::vector<char> &out_img, _Inout_ size_t &output_size );

	bool		EncodeImage												( _In_ uint8_t* in_image, _In_ unsigned int width, _In_ unsigned int height, _In_ unsigned int	n_bytes_per_pixel, _Inout_ std::vector<char> &out_img, _Inout_ size_t &output_size );

	bool		EncodeImageAndWriteToFile								( _In_ uint8_t* in_image, _In_ unsigned int width, _In_ unsigned int height, _In_ unsigned int	n_bytes_per_pixel, _In_ const char* out_file );

	bool		EncodeImageFromTestFile									( _In_ const char *img_file, _Inout_ std::vector<char> &out_img, _Inout_ size_t &output_size );

protected:

	bool																b_scale;

	bool																b_crop;

	bool																b_blend_alpha; 

	bool																resize_w;		// used if b_scale is set

	bool																resize_h;		// used if b_scale is set

	int																	crop_x, crop_y, crop_w, crop_h; // used if b_crop is set

	uint32_t															background_color; // used if bleand alpha is set


private:

	IMG_PIXEL_FORMATS													n_pixel_format;


};