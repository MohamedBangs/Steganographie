#ifndef JPEG_STEGANO_H_
#define JPEG_STEGANO_H_

/**
 * \file jpeg_manip.h
 * \brief Librairie de gestion des coefficients DCT d'une image JPEG.
 * \author N.Bodin
 *
 * Utilisation de la libjpeg pour accéder aux coefficients DCT d'une image JPEG.
 * 
 * \defgroup JPEG_manip
 * \brief Manipulation de coefficients DCT d'une image JPEG
 * \{
 */

//#include <jpeglib.h>
#include "jpeg-8/cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */
#include "jpeg-8/jversion.h"	/* for version message */

/**
 * \defgroup constantes
 * \brief Constantes liées aux valeurs des coefficients DCT
 * \{
 */
/// @brief valeur maximale d'un coefficient DCT.
#define MAX_DCT_VALUE (255*32)
/// @brief valeur minimale d'un coefficient DCT.
#define MIN_DCT_VALUE (-255*32)
/// @brief nombre de valeurs possibles pour un coefficient DCT
#define NB_DCT_VALUES (MAX_DCT_VALUE - MIN_DCT_VALUE + 1)
/// \}


/// @brief Type interne à la libjpeg
typedef struct jpeg_decompress_struct sjdec;

/**
 * \defgroup structures
 * \brief Structures de jpeg_manip
 * \{
 */

/// @brief Position d'un coefficient DCT en 4D
typedef struct DCTpos_s 
{
	/// numéro de composante (entre 0 et 2 si couleur, 0 si niveau de gris)
	int comp;	
	/// numéro de ligne du bloc DCT (entre 0 et hauteur de l'image / 8) 
	int lin;	
	/// numéro de colonne du bloc DCT (entre 0 et largeur de l'image / 8) 
	int col;	
	/// index du coefficient dans le bloc (entre 0 et 63)
	int coeff;	
}DCTpos;


/// @brief Structure principale d'une image JPEG
typedef struct JPEGimg_s
{
	/// tableau à 4 dimensions donnant la valeur des coefficients DCT
	JBLOCKARRAY * dctCoeffs;
	/// informations relatives à l'image (largeur, hauteur, nombre de composantes...)
	sjdec * cinfo;	
	/// pointeur interne à la libjpeg (ne pas le modifier)
	jvirt_barray_ptr * virtCoeffs;			
} JPEGimg;

/// \}

/**
 * \defgroup fonctions
 * \brief Fonctions de jpeg_manip
 * \{
 */

/// @brief	Allocation d'une structure JPEGimg
/// @return une strucutre JPEG img correctement allouée, NULL en cas d'erreur d'allocation
JPEGimg * init_jpeg_img ( void );


/// @brief		Libère la structure passée en paramètres et ses champs
/// @param[in]	img	pointeur vers la structure à libérer
/// @return		EXIT_SUCCESS si tout ok, ERR_ARG si pas d'argument
int free_jpeg_img ( JPEGimg *img );


/// @brief		Récupère les coefficients DCT de l'image donnée en paramètres
/// @param[in]	path chemin de l'image JPEG à lire
/// @return		un pointeur sur une structure JPEGimg correctement allouée et initialisée, NULL en cas d'erreur
JPEGimg * jpeg_read (char *path);


/// @brief Ecrit l'image img dans le fichier outfile
/// @param[in] outfile	chemin de l'image JPEG à écrire
/// @param[in] img		structure contenant les informations de l'image à écrire
/// @return	EXIT_SUCCESS si tout ok, une valeur négative en cas d'erreur
int jpeg_write_from_coeffs (char *outfile, JPEGimg *img);


/// @brief	En fonction de la valeur pos, retourne une position unique dans l'image JPEG en terme 
///			de quadruplet (comp, lin, col, coeff).
///			Une position (int) est associée à une unique position (DCTpos) et inversement
/// @param[in] img			pointeur vers la structure contenant l'image JPEG
/// @param[in] pos			position du coefficient à trouver (entre 0 et nombre de coeffs DCT)
/// @param[out] position	pointeur sur la structure position à compléter (doit être alloué au préalable)
/// @return					EXIT_SUCCESS si tout ok, une valeur négative sinon
int getDCTpos (JPEGimg*img, int pos, DCTpos * const position);


/// @brief Retourne la valeur d'un coefficient DCT selon une position donnée
/// @param[in] img			pointeur vers la structure contenant l'image JPEG
/// @param[in] pos			pointeur sur la strucutre DCTpos contenant les informations de position
/// @param[out] coeffValue	pointeur sur la valeur retournée. doit être déjà alloué 
/// @return	EXIT_SUCCESS si tout ok, une valeur négative en cas d'erreur.
int getDCTcoeffValue(JPEGimg* img, DCTpos* pos, int* coeffValue);

/// \}

/// \}

#endif /*JPEG_STEGANO_H_*/
