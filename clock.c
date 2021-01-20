#include <pthread.h>
#include "globals.h"
void *clocka (void *hari_param){
    tik=0;
    struct Informazioa *param;
    param = (struct Informazioa *)hari_param;
    int maizta = param->maiztasuna;
    while(1){
    	if(tik!=maizta){//ez pasatzeko maiztasunetik
        	pthread_mutex_lock(&kont_tik);
        	tik++;
        	pthread_mutex_unlock(&kont_tik);
        }
    }
}