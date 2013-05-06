/*! \file 

\version 3.1.5 doc

\brief Afficheur oxygene 3 sondes, moyenne robuste et HUD

* <ul>
* <li>Prevue pour HUD
* <li>Clignotement rapide en cas d'alarme, lent si tout est vert
* <li>Moyenne robuste des 3 sondes
* <li>Affichage de CELL WARNING si diff entre deux sonde > 0.2 b
* </ul>
* boutons 1 et 2 actif bas
* resistance de pull up sur les boutons 1 et 2
* rajouter filtre passe bas sur les ampli op (60 nf pour couper a 4 Hz)

* TODO 
* Controle pile (pas de mesure implantee)

* Systeme de recuperation des coupures d'alim :
*
* lors du passage en mode "plongee" 
* Les coeff de calibration des sondes sont stockes en EEPROM 
* (adresse 0, 4 et 8, 4 octets par coeff)
* Un flag (adresse 12, un octet) est mis a 0
* Lors d'un arret normal le flag est mis à 1
* Au demarrage le flag est teste. 
* Si le flag est a 1, 
* la sequence de demarrage normal est lancee : 
* aff version -> test LED -> test sondes -> calibration -> plongee
* Si le flag est a 0 le mode recuperation est lance
* les coeff de calibrations precedent sont recuperes en EEPROM et 
* on passe en mode plongee en affichant "RECUPERATION"
*<pre>
*
*                     ARRET 0 ------------>----------------->------------>
*                        |                                                |
*                        | BOUT_1 presse                                  |
*                        |                                                |
*                     REVEIL  -> affiche num de version -------->-------->|
*                        |                                                |
*                        | BOUT_1 presse                                  | TEST : SI Flag arret correct non a 1
*                        |                                                |
*                     CHECK_LED_1                                       Lecture en EEPROM des ancien coeffs
*                        |                                                |
*                        | BOUT_1 lache                                 Affichage de "RECUPERATION" sur LCD
*                        |                                                |
*                     CHECK_LED_2                                         |
*                        |                                                |
*                        | BOUT_1 presse                                  |  
*                        |                                                |
*                     CHECK_SONDES_1                                      |
*                        |                                                |
*                        | BOUT_1 lache                                   |
*                        |                                                |
*                     CHECK_SONDES_2                                      |
*                        |                                                |
*                        | BOUT_1 presse                                  |
*                        |                                                |
*                     CALIBRATE -> stockage coeff en EEPROM               |
*                        |         Flag arret correct 0                   |
*                        |                                                |
*                        | BOUT_1 lache                                   |
*                        |                                                |
*                     MESURE <----------<---------------<-----------------
*                        |                                                ^
*                        | BOUT_1 presse                                  |
*                        |                                                |
*       <--------- CONF_ARRET -------->                                   |
*      |                               |                                  ^
*      | BOUT_2 presse                 | tempo 5 secondes                 |
*      |                               |                                  |
*    ARRET Flag arret correct 1        ------------->----------->-------->
*</pre>
*/       

#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <EEPROM.h>
#include "EntSorties.h"

#include "lib_eeprom.h"
#include "led_boutons.c"
#include <LiquidCrystal.h>

#define VERSION "3.1.5"
/*! Objet afficheur LCD connection for 4 bits command 8->RS 7->CS 6->D4 5->D5 6->D6 7->D7*/  
LiquidCrystal lcd(8,7, 6, 5, 4, 3) ;
/*! Etat de la machine d etat, voir "ETAT DE LA MACHINE D'ETAT" dans les modules*/
char ETAT ;
/*! Tableau contenant les datas des trois canaux de mesure*/
struct mesure canal[]={
  {sonde_1, LED_R1, LED_V1, 0, 0, 0, 0, 0, 0} ,
  {sonde_2, LED_R2, LED_V2, 4, 0, 0, 0, 0, 5 } ,
  {sonde_3, LED_R3, LED_V3, 8, 0, 0, 0, 0, 10 } , 
};
/*! position en EEPROM du FLAG d'arret correct (apres le dernier coeff de conversion de la sonde 3) */
#define FLAG_CORRECT_STOP 12 

//===============================================================================//
void sleepNow(void){
	lcd.clear(); lcd.noDisplay();
  	attachInterrupt(0, pinInterrupt, LOW);     // Set pin 2 as interrupt and attach handler:
    	set_sleep_mode(SLEEP_MODE_PWR_DOWN);     // Choose our preferred sleep mode:
    	cli() ;
    	sleep_enable();   // Set sleep enable (SE) bit:
    	sei() ; 
    	sleep_cpu() ;
    	// Upon waking up, sketch continues from this point.
    	sleep_disable();
}
//===============================================================================//
void pinInterrupt(void){
    	detachInterrupt(0);
    	ETAT = REVEIL ;
    	lcd.display();
        CLEAR_PRINT("Version");
    	LINE_1_PRINT(VERSION);
}
//===============================================================================//
void setup(void){
	char i ;
  	lcd.begin(16, 2);
        FOR_3 {
    	  pinMode(canal[i].PIN_sonde,INPUT); 	 // ANALOGIQUE Entree sonde O2
          pinMode(canal[i].LED_R,OUTPUT);    // Diode d'alerte d'hypoxie
          pinMode(canal[i].LED_V,OUTPUT);    // Diode d'alerte d'hypoxie
        }
    	pinMode(bouton_2,INPUT); digitalWrite(bouton_2, HIGH) ; // pull up resistor Entree bouton n° 2
    	pinMode(bouton_1,INPUT); digitalWrite(bouton_1, HIGH) ; // pull up resistor  Bouton n° 1 (Reveil)

    	ETAT = ARRET ; // init : etat d'arret
}
//===============================================================================//
void loop(void){

	static float Mr ; // PO2 Estimee par moyenne robuste
	static char CELL_WARNING ;
	char i ;
	
	static struct btn BOUT1 = {bouton_1, LOW,0,0,0} ;
	static struct btn BOUT2 = {bouton_2, LOW,0,0,0} ;

	static unsigned long  StartMillis = 0 , startE_5 ;

 	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
	int B_1 = detect_appui(&BOUT1);
	int B_2 = detect_appui(&BOUT2);
    	// lcd.setCursor(13,0); lcd.print(ETAT) ;lcd.print(B_1) ;lcd.print(B_2) ;
        //==================================================================================// 
	//     					 GESTION DU HUD                             //
        //==================================================================================//   
        if (ETAT == MESURE || ETAT == CONF_ARRET || ETAT == RECUP) {
		if (CELL_WARNING == PANIC){
			rapid_flash(BI_COL, canal);
		}else{
                	if (Mr >= 0.4 && Mr <= 1.6){
                        	led_flash(Mr,CELL_WARNING, canal);
                	} else {
                        	if (Mr < 0.4) { 
					rapid_flash(ROUGE, canal);
                        	}else{ 
					rapid_flash(ORANGE, canal);
				}
                	}
		}
        }
        //==================================================================================// 
	//     			MACHINE D'ETAT			                            //
        //==================================================================================//   
  	if(millis() - StartMillis > 500) { // changement d'etat toutes les demi secondes
        	StartMillis = millis() ;
		//
                FOR_3 {
                  canal[i].CanValue = analogRead(canal[i].PIN_sonde); 
                } 
               	//==================================================================================//
                if ((ETAT == REVEIL || ETAT == ARRET) && EEPROM.read(FLAG_CORRECT_STOP) != 1){ 
			ETAT = RECUP ;
                  	FOR_3  { canal[i].K = EepromReadFloat(canal[i].EEmem) ; }
                      	// recup des ancien coeff de conversion
                }
                if (ETAT == ARRET) sleepNow();
               	//==================================================================================//
                if (ETAT == REVEIL && B_1==1) { 
			ETAT = CHECK_LED_1 ;
              	  	CLEAR_PRINT("Controle LED"); test_led(canal);
                 }  
               	//==================================================================================//
                if (ETAT == CHECK_LED_1 && B_1==0) { 
			ETAT = CHECK_LED_2 ;
		}
               	//==================================================================================//            
        	if (ETAT == CHECK_LED_2 && B_1==1) { 
			ETAT = CHECK_SONDES_1 ;
			CLEAR_PRINT("Controle sonde"); delay(1000) ;
			CLEAR_PRINT("Ancienne cal (b)"); LINE_1_PRINT("tension (mV)"); delay(1000) ; 
                    	FOR_3 {    // recup des anciens coeff pour affichage indicatif de PO2
                        	canal[i].KK = EepromReadFloat(canal[i].EEmem) ;
				delay(100); 
                        	canal[i].K = 0.155 ;
                    	}
          	}
               	//==================================================================================//
        	if (ETAT == CHECK_SONDES_1 && B_1==0) {
			ETAT = CHECK_SONDES_2 ;
		}
               	//==================================================================================//        	
        	if (ETAT == CHECK_SONDES_2 && B_1==1) { 
			ETAT = CALIBRATE ;
          		CLEAR_PRINT("Calib 0,97 b");     
                        FOR_3 {
                          	canal[i].K = 0.97/canal[i].CanValue ; 
                          	delay(100); 
                          	eepromWriteFloat(canal[i].EEmem, canal[i].K);
                        }
        	}
               	//==================================================================================//
        	if (ETAT == CALIBRATE && B_1==0) { 
			ETAT = MESURE ;
                        if (EEPROM.read(12) != 0) EEPROM.write(12, 0);  // flag indicating system is mesuring  
        	}
               	//==================================================================================//
       	        if ((ETAT == MESURE || ETAT == RECUP) && B_1==1) { 
			ETAT = CONF_ARRET ;
       		        startE_5 = millis();
        	}
               	//==================================================================================//
        	if (ETAT == CONF_ARRET && (millis() - startE_5 > 5000) ){ 
			ETAT = MESURE ;
        	}
               	//==================================================================================//
        	if (ETAT == CONF_ARRET && B_2==1) { 
			ETAT = ARRET ; 
                        if (EEPROM.read(12) != 1) EEPROM.write(FLAG_CORRECT_STOP, 1);  // flag d'arret correct    
                        FOR_3  { ETEINT(i);}
        	}
        	//==================================================================================// 
		//   LECTURE DES SONDES, CONVERSION  ET AFFICHAGE AU LCD		            //
        	//==================================================================================//   
                FOR_3 canal[i].PO2 = canal[i].CanValue * canal[i].K; 
               	//==================================================================================//
        	if (ETAT == CHECK_SONDES_1 || ETAT == CHECK_SONDES_2) {         
                  	lcd.clear();
                  	FOR_3 {
            	    		lcd.setCursor(canal[i].Curs_Pos, 0);  lcd.print(canal[i].CanValue*canal[i].KK,2); 
            	    		lcd.setCursor(canal[i].Curs_Pos, 1);  lcd.print(canal[i].PO2,1); 
                  	}
        	}
               	//==================================================================================//        
        	if (ETAT == MESURE || ETAT == CONF_ARRET || ETAT == RECUP) {
                  	Mr = moyenne_robuste(canal[0].PO2, canal[1].PO2,canal[2].PO2  );
			CELL_WARNING = coherent_cell(canal[0].PO2, canal[1].PO2,canal[2].PO2 );	
		
			if (ETAT == RECUP) 		{CLEAR_PRINT("RECUP"); }
		  	if (ETAT == MESURE) 		{CLEAR_PRINT("M ROBUSTE=");  }
			if (CELL_WARNING == PANIC ) 	{CLEAR_PRINT("CellPanic"); }
			if (CELL_WARNING == WARNING ) 	{CLEAR_PRINT("CellWarning"); }
			if (ETAT == CONF_ARRET) 	{CLEAR_PRINT("STOP -> B2");  } 

                  	lcd.setCursor(12, 0); lcd.print(Mr,2);
                  	FOR_3 { lcd.setCursor(canal[i].Curs_Pos, 1);  lcd.print(canal[i].PO2,2);  }     
       	        }
	} // Fin boucle tempo de lecture toute les demi secondes
} // Fin MAIN

