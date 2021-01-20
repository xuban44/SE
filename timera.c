#include "globals.h"
void *timera (void *hari_param){
	mug=0;
    struct Informazioa *param;
    param = (struct Informazioa *)hari_param;
    int maizta = param->maiztasuna;
    while(1){
        if(tik==maizta){
            pthread_mutex_lock(&kont_tik);
            tik=0;
            pthread_mutex_unlock(&kont_tik);
            if(mug==0){
                mug=1;
            }
            else{
                mug=0;
            }
        }
    }
}