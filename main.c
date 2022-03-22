#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "error.h"
#include "jpeg_manip.h"
#include "TODO.h"


/// @brief Permet de modifier la valeur d'un coefficient DCT
///           La fonction modifie le coeff de +modif
/// @param img        pointeur vers la strucutre JPEGimg contenant le tableau de coeffs DCT
/// @param pos        pointeur vers la position du coeff à modifier
/// @param modif    valeur de la modification
void modifDCTcoeff(JPEGimg* img, DCTpos* pos, int modif)
{
    img->dctCoeffs[pos->comp][pos->lin][pos->col][pos->coeff] += modif;
}


/// @brief Compte le nombre de coefficients DCT de l'image
/// @param img pointeur vers la strucutre JPEGimg contenant le tableau de coeffs DCT
/// @return le nombre de coefficients DCT présents dans l'image
int nb_DCT_coeffs(JPEGimg* img)
{
    int nb_coeff_in_image=0, i=0;
    for (i = 0; i<img->cinfo->num_components; i++){
        int nb_block_in_composant= img->cinfo->comp_info[i].height_in_blocks*img->cinfo->comp_info[i].width_in_blocks;
        int nb_coef_in_block = nb_block_in_composant*64;
        nb_coeff_in_image+=nb_coef_in_block;
    }
    return nb_coeff_in_image;
}

/// @brief Lecture du fichier path, récupération de la taille dans size et retour du contenu
/// @param[in]    path chemin vers le fichier à lire
/// @param[out]    size pointeur vers la taille du fichier (doit être préalablement alloué)
/// @return        un pointeur sur les données lues
byte* read_file(char* path, int* size)
{
    FILE* fileptr;
    byte* buffer;

    fileptr = fopen(path, "rb");
    fseek(fileptr, 0, SEEK_END);
    *size = (int)ftell(fileptr);
    rewind(fileptr);

    buffer = (byte*)malloc(*size * sizeof(byte));
    fread(buffer, 1, *size, fileptr);

    return buffer;
}

/// @brief Ecrit le buffer buf dans le fichier path
/// @param[in] buf        tableau contenant les données à écrire
/// @param[in] buf_size taille du tableau buf
/// @param[in] path        chemin sur lequel écrire les données
/// @return EXIT_SUCCESS ou une valeur négative en cas d'erreur
int write_file(byte* buf, int buf_size, char* path)
{
    return EXIT_SUCCESS;
}

/// @brief Insertion d'un bit dans un coefficient DCT
///        L'insertion doit être du LSB replacing, rien d'autre !
/// @param[in,out] img    pointeur vers la strucutre JPEGimg contenant le tableau de coeffs DCT
/// @param[in] pos        pointeur vers la position du coeff à modifier
/// @param[in] bit        valeur du bit à insérer
/// @return 1 si le coeff DCT a été modifié, 0 sinon
int bit_insert(JPEGimg* img, DCTpos* pos, int bit)
{
    int coef = img->dctCoeffs[pos->comp][pos->lin][pos->col][pos->coeff];
    if ((coef & 1) != bit) {
        coef ^= 1;
        img->dctCoeffs[pos->comp][pos->lin][pos->col][pos->coeff] = coef;
        return 1;
    }
    return 0;
}

/// @brief Insertion d'un message dans une image JPEG
///        L'insertion doit être du LSB replacing séquentiel, rien d'autre !
///        Si le message est trop grand pour l'image, retourner la valeur ERR_TREAT
/// @param[in] msg        pointeur vers le message (tableau de unsigned char)
/// @param[in] size        taille du message (en octets)
/// @param[in,out] img    pointeur sur l'image cover
/// @return EXIT_SUCCESS ou une valeur négative en cas d'erreur
///         EXIT_FAILURE si la taille du message est trop grande
int basic_insert(byte* msg, int size, JPEGimg* img)
{
    DCTpos pos = { 0 };
    int countBits = 0;

    // insertion de la taille du message: 4 octets => 8*4 = 32 bits
    for (int i = 0; i < 32; i++) {
        getDCTpos(img, countBits, &pos);
        byte b = (size >> 31 - i) & 1;
        bit_insert(img, &pos, b);
        countBits += 1;
    }

    if (((size * 8) + 32) <= nb_DCT_coeffs(img)) {
        // insertion du message
        for (int j = 0; j < size; j++) {
            // parcourir bit par bit msg[j] => k
            if (countBits == nb_DCT_coeffs(img)) {
                return EXIT_FAILURE;
            }
            for (int k = 0; k < 8; k++)
            {
                getDCTpos(img, countBits, &pos);
                byte b = (msg[j] >> 7 - k) & 1;
                bit_insert(img, &pos, b);
                countBits += 1;
            }
        }
        return EXIT_SUCCESS;
    }
    return ERR_TREAT;
}

/// @brief Extraction d'un message d'une image JPEG
///        L'extraction doit permettre de récupérer le message précédement inséré avec
///        la fonction basic_insert
/// @param[in] img        pointeur vers l'image JPEG
/// @param[out] size    pointeur sur la taille du message extrait
/// @return un pointeur sur les données extraites
///         NULL si la taille du message est trop grande
byte* basic_extract(JPEGimg* img, int* size)
{
    int i = 0;
    int coeff = 0;
    int result = 0;
    int countBits = 0;
    char* msg;
    char c = ' ';
    DCTpos pos = { 0 };

    // récupération de la taille du message
    for (; i < 32; i++) {
        getDCTpos(img, countBits, &pos);
        getDCTcoeffValue(img, &pos, &coeff);
        byte b = coeff & 1; // LSB
        countBits += 1;
        int clearBit = ~(1 << 31 - i);
        int mask = result & clearBit;
        result = mask | (b << 31 - i);
    }

    *size = result;
    msg = malloc(sizeof(char) * *size);
    msg[*size] = '\0';
    if (*size * 8 + 32 < nb_DCT_coeffs(img)) {
        // lecture du message
        for (int j = 0; j < *size; j++) {
            // parcourir bit par bit msg[i] => k
            for (int k = 0; k < 8; k++)
            {
                getDCTpos(img, countBits, &pos);
                getDCTcoeffValue(img, &pos, &coeff);
                byte b = coeff & 1; // LSB
                countBits += 1;
                int clearBit = ~(1 << 7 - k);
                int mask = c & clearBit;
                c = mask | (b << 7 - k);
            }
            msg[j] = c;
        }
        return msg;
    } else return NULL;
}

/// @brief Insertion d'un message dans une image JPEG avec ré-insertion si zéro
///        Si le message est trop grand pour l'image, retourner la valeur ERR_TREAT
/// @param[in] msg        pointeur vers le message (tableau de unsigned char)
/// @param[in] size        taille du message (en octets)
/// @param[in,out] img    pointeur sur l'image cover
/// @return EXIT_SUCCESS ou une valeur négative en cas d'erreur
///         EXIT_FAILURE si la taille du message est trop grande
int advanced_insert(byte* msg, int size, JPEGimg* img)
{
    DCTpos pos = { 0 };
    int countBits = 0;
    int bitBefore = 0;
    int coeff = 0;

    // insertion de la taille du message: 4 octets => 8*4 = 32 bits
    for (int i = 0; i < 32; i++) {
        getDCTpos(img, countBits, &pos);
        getDCTcoeffValue(img, &pos, &coeff);
        if (coeff != 0) {
            getDCTpos(img, countBits, &pos);
            byte b = (size >> 31 - i) & 1;
            bit_insert(img, &pos, b);
            getDCTcoeffValue(img, &pos, &coeff);
            while (coeff == 0) {
                countBits += 1;
                getDCTpos(img, countBits, &pos);
                bit_insert(img, &pos, b);
                getDCTcoeffValue(img, &pos, &coeff);
            }
            countBits += 1;
        }
        else {
            i--;
            countBits += 1;
        }
    }

    if (((size * 8) + 32) <= nb_DCT_coeffs(img)) {
        // insertion du message
        for (int j = 0; j < size; j++) {
            // parcourir bit par bit msg[j] => k
            if (countBits == nb_DCT_coeffs(img)) {
                return EXIT_FAILURE;
            }
            for (int k = 0; k < 8; k++)
            {
                getDCTpos(img, countBits, &pos);
                getDCTcoeffValue(img, &pos, &coeff);
                if (coeff != 0) {
                    getDCTpos(img, countBits, &pos);
                    byte b = (msg[j] >> 7 - k) & 1;
                    bit_insert(img, &pos, b);
                    getDCTcoeffValue(img, &pos, &coeff);
                    while (coeff == 0) {
                        countBits += 1;
                        getDCTpos(img, countBits, &pos);
                        bit_insert(img, &pos, b);
                        getDCTcoeffValue(img, &pos, &coeff);
                    }
                    countBits += 1;
                }
                else {
                    k--;
                    countBits += 1;
                }
            }
        }
        return EXIT_SUCCESS;
    }
    return ERR_TREAT;
}

/// @brief Extraction d'un message d'une image JPEG
///        L'extraction doit permettre de récupérer le message précédement inséré avec
///        la fonction advanced_insert
/// @param[in] img        pointeur vers l'image JPEG
/// @param[out] size    pointeur sur la taille du message extrait
/// @return un pointeur sur les données extraites
///         null si la taille du message est trop grande
byte* advanced_extract(JPEGimg* img, int* size)
{
    int i = 0;
    int coeff = 0;
    int result = 0;
    int countBits = 0;
    char* msg;
    char c = ' ';
    DCTpos pos = { 0 };

    // récupération de la taille du message
    for (; i < 32; i++) {
        getDCTpos(img, countBits, &pos);
        getDCTcoeffValue(img, &pos, &coeff);
        if (coeff == 0) {
            i--;
        }
        else {
            byte b = coeff & 1; // LSB
            int clearBit = ~(1 << 31 - i);
            int mask = result & clearBit;
            result = mask | (b << 31 - i);
        }
        countBits += 1;
    }

    *size = result;
    msg = malloc(sizeof(char) * *size);
    //msg[*size] = '\0';
    if (*size * 8 + countBits < nb_DCT_coeffs(img)) {
        // lecture du message
        for (int j = 0; j < *size; j++) {
            // parcourir bit par bit msg[i] => k
            for (int k = 0; k < 8; k++)
            {
                getDCTpos(img, countBits, &pos);
                getDCTcoeffValue(img, &pos, &coeff);
                if (coeff == 0) {
                    k--;
                }
                else {
                    byte b = coeff & 1; // LSB
                    int clearBit = ~(1 << 7 - k);
                    int mask = c & clearBit;
                    c = mask | (b << 7 - k);
                }
                countBits += 1;
            }
            msg[j] = c;
        }
        return msg;
    }
    else return NULL;
}

/// @}

/// @brief Point d'entrée du programme
/// @param[in] argc nombre d'arguments de la ligne de commande
/// @param[in] argv arguments de la ligne de commande
/// @return EXIT_SUCCESS ou EXIT_FAILURE
int main(int argc, char** argv)
{
	int return_value;
	JPEGimg* img = NULL;
	DCTpos pos = { 0 };

	// Vérification du nombre d'arguments
	if (argc < 3)
	{
		printf("%s: Reads a jpeg image and write it in a new file\n", argv[0]);
		printf("Not enough arguments for %s\n", argv[0]);
		printf("Usage: %s <cover.jpg> <copy.jpg>\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Lecture de l'image
	img = jpeg_read(argv[1]);
	if (!img)
		return EXIT_FAILURE;

	// Informations basiques
	printf("Number of components: %d\n", img->cinfo->num_components);
	for (int i = 0; i < img->cinfo->num_components; i++)
	{
		printf("Component %d:\n   %d lines, %d columns\n\n", i, img->cinfo->comp_info[i].height_in_blocks, img->cinfo->comp_info[i].width_in_blocks);
	}
   /*
    call function here to hidde your message in image
    ex: advanced_insert(...)
    */

	// Ecriture dans un nouveau fichier
	return_value = jpeg_write_from_coeffs(argv[2], img);
	if (return_value == EXIT_SUCCESS)
		printf("\nImage written in %s\n", argv[2]);
	free_jpeg_img(img);

	return EXIT_SUCCESS;
}
