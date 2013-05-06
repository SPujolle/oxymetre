/*! \file */
#include <WConstants.h>
#include <Arduino.h>

#ifndef ENTSORTIES
#define ENTSORTIES
// Numeros des entrees sorties

#ifndef A0
#define A0 14
#endif
#ifndef A1
#define A1 15
#endif
#ifndef A2
#define A2 16
#endif
#ifndef A3
#define A3 17
#endif
#ifndef A4
#define A4 18
#endif
#ifndef A5
#define A5 19
#endif
#ifndef A6
#define A6 20
#endif
#ifndef A7
#define A7 21
#endif

// INPUT OUTPUT
/** @defgroup group2 INPUT/OUTPUT PIN NUMBERS
 *  @{
 */
#define sonde_1 A6  // 
#define sonde_2 A5  // 
#define sonde_3 A7  // 
/*! Input for start button --> plug on D2 to be used as wake up interrupt*/
#define bouton_1 2
#define bouton_2 A4

#define LED_R1 9 
#define LED_V1 10
#define LED_R3 11
#define LED_V3 12
#define LED_R2 13 
#define LED_V2 A0
/** @} */ 

// ETAT DE LA MACHINE D ETAT :
/** @defgroup group1 ETAT DE LA MACHINE D'ETAT :
 *  La machine peut être dans l'un des 9 états suivants
 *  @{
 */
#define ARRET 		0
#define REVEIL 		1
#define CHECK_LED_1 	2
#define CHECK_LED_2 	3
#define CHECK_SONDES_1 	4
#define CHECK_SONDES_2 	5
#define CALIBRATE 	6
#define MESURE 		7
#define CONF_ARRET 	8
#define RECUP 		9
/** @} */ 

// Duree des pulses du HUD
/** @defgroup group3 HUD PULSE LENGHT
 *  @{
 */
#define PULSE_LENT 1000
#define TIME_HIGH 100
#define TIME_LOW_INT  450 
#define TIME_LOW_1  800 
#define MAX_PULSE 6
#define TIME_LOW_RAPIDE 120
#define TIME_HIGH_RAPIDE 80
#define LONG_PULSE 600
/*! @} */
/** @defgroup group4 HUD PULSE COLOR
 *  @{
 */
#define ROUGE 0
#define ORANGE 2
#define VERT 1
#define BI_COL 3
/*! @} */
// Etats de coherence cellule
/** @defgroup group5 MESURE COHERENCE LEVEL 
* Those states command the HUD display 
 *  @{
 */
/*! Mesures are coherent : there is no differences greater then 0,2 b between any two mesures */
#define ALL_GOOD 0
/*! Cell warning : there is one and only one differences greater then 0,2 b between any two mesures */
#define WARNING 1
/*! Totaly incoherent : there no two mesures in a 0,2 b interval  */
#define PANIC 2
/*! @} */

#define FOR_3 for(i = 0 ; i < 3 ; i++)
#define CLEAR_PRINT(A) lcd.clear(); lcd.print((A))
#define LINE_1_PRINT(A) lcd.setCursor(0, 1); lcd.print((A))
#define ABS(A) ((A) >= 0 ? (A) : -(A))
#define ETEINT(i) digitalWrite(canal[i].LED_V, LOW) ;  digitalWrite(canal[i].LED_R, LOW)

 struct btn {
   int PIN ; 			/*! Num de patte (arduino) ou est branche le bouton */
   int Positif ; 		/*! Etat actif du bouton LOW ou HIGH */
   int n ; 			/*! Compteur de filtrage passe bas du bouton */
   int ButtonState ;		/*! etat du bouton, renvoye au prog princ par la fontcion de filtrage du bouton */
   unsigned long StartMicros ;	/*! stockage du temps pour tempo d evaluation de l etat du bouton */
};

struct mesure {
  int PIN_sonde ; 		// Pin number CAN input
  int LED_R ;    	// PIN for RED LED associated 
  int LED_V ;     	// PIN for GREEN LED associated
  int EEmem ;     		// Memory address for K storage for recuperation wake up
  float K ; 			// coeff de conversion CAN -> PO2
  float KK ; 			// coeff de conv de l ancien calibrage
  int CanValue ; 		// mesure en sortie du CNA
  float PO2 ; 			// Pression part d O2
  int  Curs_Pos ; 		// Cursor position on LCD
} ;

int detect_appui(struct btn *BT) ;
char coherent_cell (float A, float B, float C );
float moyenne_robuste(float A, float B, float C ) ;
void led_flash(float Mr , char CELL_WARNING, struct mesure *canal) ;
void rapid_flash(int COLOR, struct mesure *canal) ;
void test_led(struct mesure *canal) ;

#endif
