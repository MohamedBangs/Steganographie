#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "error.h"
#include "jpeg_manip.h"


JPEGimg *init_jpeg_img ( void )
{
	JPEGimg * img = NULL;
	if ( (img = (JPEGimg*) calloc (1, sizeof(JPEGimg)) ) == NULL)
	{
		print_err ("init_jpeg_img", "img", ERR_MEM);
		return NULL;
	}

	if ((img->cinfo = (sjdec*)calloc(1, sizeof(sjdec))) == NULL)
	{
		print_err("init_jpeg_img", "img->cinfo", ERR_MEM);
		free(img);
		return NULL;
	}
	
	return img;
}


int free_jpeg_img ( JPEGimg *img )
{
	// Checkargs
	if (!img)
		return ERR_ARG;
	
	// Free memory
	if (img->dctCoeffs)
	{
		free (img->dctCoeffs);
		img->dctCoeffs = NULL;
	}
	
	jpeg_destroy_decompress(img->cinfo);

	if (img->cinfo)
	{
		free(img->cinfo);
		img->cinfo = NULL;
	}
	free(img);
	
	return EXIT_SUCCESS;
}


JPEGimg *jpeg_read (char *path)
{
	int comp;
	struct jpeg_error_mgr jerr; // Handle errors
	FILE *infile = NULL;
	JPEGimg *img = NULL;
	
	// Check args
	if (!path)
	{
		print_err ("jpeg_read()", "path", ERR_ARG);
		return NULL;
	}
	
	// Open path
	if ((infile = fopen(path, "rb") ) == NULL)
	{
		print_err ("jpeg_read()", path, ERR_FOPEN);
		return NULL;
	}
	
	// Memory allocation for img
	if ((img = init_jpeg_img()) == NULL)
	{
		fclose (infile);
		return NULL;
	}
	
	// Initialize the JPEG decompression object with default error handling.
	img->cinfo->err = jpeg_std_error (&jerr);
	jpeg_create_decompress (img->cinfo);
  
	// Specify data source for decompression
	jpeg_stdio_src (img->cinfo, infile);
  
	// Read header
	(void) jpeg_read_header (img->cinfo, TRUE);
  
	/* Get DCT coefficients
	 * dct_coeffs is a virtual array of the components Y, Cb, Cr
	 * access to the physical array with the function
	 * (cinfo->mem -> access_virt_barray)*/
	img->virtCoeffs = jpeg_read_coefficients (img->cinfo);
	
	// Structure allocation
	img->dctCoeffs = (JBLOCKARRAY*) malloc (sizeof(JBLOCKARRAY) * img->cinfo->num_components );
	if (img->dctCoeffs == NULL)
	{
		print_err ("jpeg_read()", "img->dctCoeffs", ERR_MEM);
		fclose (infile);
		return NULL;
	}
  
	// Loop on the components of the virtual array to get DCT coefficients
	for(comp = 0; comp < img->cinfo->num_components; comp++)
	{
  		img->dctCoeffs[comp] = (img->cinfo->mem -> access_virt_barray)((j_common_ptr) &(img->cinfo),
		img->virtCoeffs[comp], 0, 1, TRUE);
	}
  
	// free and close
	fclose (infile);
                        
	return img;
}


int jpeg_write_from_coeffs (char *outfile, JPEGimg *img)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *output = NULL;

	// Open file
	if ((output = fopen (outfile, "wb")) == NULL)
	{
		print_err( "jpeg_write_from_coeffs()", outfile, ERR_FOPEN);
		return ERR_FOPEN;
	}
	
	// Initialize the JPEG compression object with default error handling. 
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	
	// telling where to put jpeg data
	jpeg_stdio_dest(&cinfo, output);

	// Applying parameters from source jpeg 
	jpeg_copy_critical_parameters(img->cinfo, &cinfo);

	// copying DCT 
	jpeg_write_coefficients(&cinfo, img->virtCoeffs);

	// clean-up
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose (output);
	
	/*Done!*/
	return EXIT_SUCCESS;
}


int getDCTpos (JPEGimg *img, int pos, DCTpos * const position)
{
	int nbDCTblocks = 0; // Number of DCT coefficients into a component
	sjdec* cinfo;

	// Check arguments
	if (!img || !position)
	{
		print_err("getDCTpos()", "img, cinfo or position", ERR_ARG);
		return ERR_ARG;
	}

	cinfo = img->cinfo;
	// reset position
	memset(position, 0, sizeof(DCTpos));

	// Find position in DCT block position
	position->coeff = pos & 63; // equivalent to pos % 64
	pos >>= 6;                  // equivalent to pos / 64
	
	// Find component
	while (position->comp < cinfo->num_components)
	{
		nbDCTblocks = cinfo->comp_info[position->comp].height_in_blocks * cinfo->comp_info[position->comp].width_in_blocks;
		if (pos < nbDCTblocks)
			break;
		position->comp++;
		pos -= nbDCTblocks;
	}
	
	// Find line and column
	position->col = pos % cinfo->comp_info[position->comp].width_in_blocks;
	position->lin = pos / cinfo->comp_info[position->comp].width_in_blocks;
	
	//printf ("Pos = %3d %3d %3d %3d\n", position->comp, position->lin, position->col, position->coeff);
		
	return EXIT_SUCCESS; 
}



int getDCTcoeffValue(JPEGimg *img, DCTpos *pos, int *coeffValue)
{
	JBLOCKARRAY* dct_coeffs;
	sjdec* cinfo;
	// Check arguments
	if (!img || !pos || !coeffValue)
	{
		print_err("getDCTpos()", "img, cinfo or position", ERR_ARG);
		return EXIT_FAILURE;
	}
	dct_coeffs = img->dctCoeffs;
	cinfo = img->cinfo;

	if (pos->comp < 0 || pos->comp >= cinfo->num_components
		|| pos->lin < 0 || pos->lin >= cinfo->comp_info[pos->comp].height_in_blocks
		|| pos->col < 0 || pos->col >= cinfo->comp_info[pos->comp].width_in_blocks
		|| pos->coeff < 0 || pos->coeff >= 64 )
		return ERR_TREAT;
	
	*coeffValue = dct_coeffs[pos->comp][pos->lin][pos->col][pos->coeff];
	return EXIT_SUCCESS;
}

