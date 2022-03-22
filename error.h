#ifndef _ERROR_H_
#define _ERROR_H_

/** 
 * \file error.h
 * \brief Librairie de gestion d'ereurs.
 * \author N.Bodin
 *
 * Gestion simple des erreurs standards dans un programme C.
 * 
 * \defgroup Error
 * \brief Fonction d'affichage d'erreurs
 * \{
 */

/// \defgroup Types
/// \brief Types d'erreur disponibles
/// \{
#define ERR_MEM   -1
#define ERR_FOPEN -2
#define ERR_FREAD -3
#define ERR_ARG   -4
#define ERR_TREAT -5
/// \}

/// @brief Affiche un message d'erreur relatif au contexte de la fonction d'appel
/// @param function nom de la fonction en erreur
/// @param var nom de la variable en erreur 
/// @param ERR code d'erreur (voir error.h)
void print_err (char *function, char *var, int ERR);

/// \}

#endif /* _ERROR_H_ */
