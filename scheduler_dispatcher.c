#include "globals.h"
#include "lista.h"
void *scheduler_Dispatcher(void *hari_param){
    //Erabaki zein nukleori pasa prozesua. lehentasun handieneko prozesua hartzen du.
    struct PCB P;//hartuko den prozesua
    int txikiena;
    while(1){
        if(!listaHutsa(q)){
            //begiratu zer coretan dauden prozesu gutxien
            txikiena=0;
            for(int i=1;i<corekop;i++){
                if(coreak[i].zenbat<coreak[txikiena].zenbat){
                    txikiena=i;
                }
            }      
        	
            if(coreak[txikiena].zenbat<50){//50 baino prozesu gehiago ez edukitzeko
            	//hartu prozesu bat
            	pthread_mutex_lock(&beg_Sche);
            	P=listaHartu(q);//prozesu bat hartzen du.Lehentasunaren arabera.
            	pthread_mutex_unlock(&beg_Sche);
            	//bertan sartu prozesua;
            	pthread_mutex_lock(&kont_core);
            	listaSartu(&coreak[txikiena],P);
            	pthread_mutex_unlock(&kont_core);
            }
            //printf("kaixo");
            //listaInprimatu(&coreak[0]);
       }
    }
}