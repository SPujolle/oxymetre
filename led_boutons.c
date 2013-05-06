
#include <WConstants.h>
#include <Arduino.h>

#include "EntSorties.h"

char coherent_cell (float A, float B, float C ){
	char CW ;
	CW = ALL_GOOD ;
	if(ABS(A - B) > 0.2 || ABS(A - C)> 0.2 || ABS(B - C)> 0.2){	
		CW = PANIC ;
		if (ABS(A - B) < 0.2 || ABS(A - C)< 0.2 || ABS(B - C)< 0.2)  CW = WARNING ;
	}
	return CW ;
}

float moyenne_robuste(float A, float B, float C ){
    float Pa = 1; float Pb = 1;  float Pc = 1; // ponderations des mesures
    float Ra, Rb , Rc ; // residus par rapport à la moyenne
    float S = 20 ;
    float Mr ; // Moyenne robuste
    int i ;
    for(i=0 ; i < 5 ; i++) { 
      Mr = (Pa*A + Pb*B + Pc*C)/(Pa + Pb + Pc) ;
      Ra = (Mr-A)*(Mr-A) ; Rb = (Mr-B)*(Mr-B) ; Rc = (Mr-C)*(Mr-C) ;
      Pa = 1/(1+S*Ra);
      Pb = 1/(1+S*Rb) ;
      Pc = 1/(1+S*Rc) ;   
    }
    return Mr ;
}

 //================================== //
 int detect_appui(struct btn *BT){ 
     	if(micros() - BT->StartMicros > 10000) { // evaluation pulse toutes 10 ms
       		BT->StartMicros = micros() ;
       		// ++++++++++++++++ //
       		if (BT->Positif == digitalRead(BT->PIN)) {
         		BT->n++ ;
       		}else{ 
         		BT->n-- ;
       		}
       	if (BT->n > 100 ) BT->n = 100 ;
       	if (BT->n < 0 )   BT->n = 0 ;
       	if (BT->n > 70) {BT->ButtonState = 1 ;}
       	if (BT->n < 20) {BT->ButtonState = 0 ;}   
     	}
   	return BT->ButtonState ;    
 }
//===============================================================================//
//              Fonctions de gestion des LED du HUD                              //
//===============================================================================//
void led_flash(float Mr , char CELL_WARNING, struct mesure *canal){
	char i ;
	char P ;
	static unsigned long Led_Millis = 0 ;
	static char LED_PULSE = LOW ;
	static int PULSE = PULSE_LENT ;
	static char Num_Pulse = 0 ;
        if(millis() - Led_Millis > PULSE) { 
        	Led_Millis = millis() ;
		// calcul du nombre de pulse rouge ou orange 
		P = (char) 10*(Mr-0.35) ; P = P - 9 ; 
                P = (P < -(MAX_PULSE - 1) ? -(MAX_PULSE - 1) : P) ; 
		P = (P >   MAX_PULSE -1   ?   MAX_PULSE - 1  : P) ; 
                if (LED_PULSE == LOW)  {
                    	LED_PULSE = HIGH ;  PULSE = TIME_HIGH ;
			if (Num_Pulse != 0 ) { 
				if (P > 0 ){	
                        		if (Num_Pulse <= P) { 
                                        	FOR_3 { digitalWrite(canal[i].LED_R, HIGH) ; digitalWrite(canal[i].LED_V, HIGH) ; } 
                                        } 
					if (Num_Pulse == P+1 && CELL_WARNING == WARNING) { 
						PULSE = LONG_PULSE ;
                                        	FOR_3 {  digitalWrite(canal[i].LED_R, HIGH) ; } 
                                        } 
				} else {
					P = -P ;
					if (Num_Pulse <= P) { 
                                        	FOR_3 { digitalWrite(canal[i].LED_R, HIGH);  } 
                                        }
					if (Num_Pulse == P+1 && CELL_WARNING == WARNING) { 
						PULSE = LONG_PULSE ;
                                        	FOR_3 {  digitalWrite(canal[i].LED_R, HIGH) ;  digitalWrite(canal[i].LED_V, HIGH) ; } 
                                        } 
				}
                        } else {
                        	FOR_3 { digitalWrite(canal[i].LED_V, HIGH) ;}  
                        }
			Num_Pulse ++ ; Num_Pulse = (Num_Pulse > MAX_PULSE ? 0 : Num_Pulse) ;   
                } else {
                	LED_PULSE = LOW  ;  FOR_3  {ETEINT(i);}
			if (Num_Pulse == 0) { 
                                PULSE = PULSE_LENT ; 
			} else {
				if (Num_Pulse == 1) { PULSE = TIME_LOW_1 ;} else { ; PULSE = TIME_LOW_INT ; }
			} 
 
                }
        }
}

 //===============================================================================//
void rapid_flash(int COLOR, struct mesure *canal){
	char i ;
	static unsigned long Led_Millis = 0 ;
	static int PULSE = TIME_HIGH_RAPIDE ;
	static char LED_PULSE = LOW ; 
	static char LED_COL = ORANGE ; 
        if(millis() - Led_Millis > PULSE) { // clignotement led toutes les 0,2 sec
        	Led_Millis = millis() ;
               if (LED_PULSE == LOW)  {
                    	LED_PULSE = HIGH ;  PULSE = TIME_HIGH_RAPIDE ;  
                        if (COLOR == ROUGE ) { FOR_3 { digitalWrite(canal[i].LED_R, HIGH);  }   } 
                        if (COLOR == VERT  ) { FOR_3 { digitalWrite(canal[i].LED_V, HIGH);  }   } 
                        if (COLOR == ORANGE) { FOR_3 { digitalWrite(canal[i].LED_V, HIGH); digitalWrite(canal[i].LED_R, HIGH);   }   }  
        		if (COLOR == BI_COL) {
				LED_COL = (LED_COL == ORANGE ? VERT : ORANGE) ;
				if (LED_COL == VERT   ) { FOR_3 { digitalWrite(canal[i].LED_V, HIGH);   }   }  
				if (LED_COL == ORANGE ) { FOR_3 { digitalWrite(canal[i].LED_R, HIGH);} FOR_3 { digitalWrite(canal[i].LED_V, HIGH);   }   }  
			}
               } else {
                        LED_PULSE = LOW  ;  PULSE = TIME_LOW_RAPIDE ; 
                        FOR_3  {ETEINT(i);}
               }        
        }
}
 //===============================================================================//
void test_led(struct mesure *canal){
	char i , k ;
   	for (k = 0 ; k < 2 ; k++){
     		for (i = 0 ; i < 3 ; i++){
        	digitalWrite(canal[i].LED_R, HIGH) ;
        	delay(200); ETEINT(i); delay(200);
     		}
     		for (i = 0 ; i < 3 ; i++){
       			digitalWrite(canal[i].LED_V, HIGH) ;
       			delay(200); ETEINT(i); delay(200);
     		} 
   	}
 }
