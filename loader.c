#include "globals.h"
#include "lista.h"
#include <stdlib.h>
void *loader(void *hari_param){
    //batzuetan prozesu bat sortzen du ditu
    int zenbakia;
    struct PCB P;//prozesua sortzeko
    int i=0;//zenbatgarren prozesua den jakiteko
    int non;//memoria birtualean non sartu jakiteko
    char randomletter;//datua ordez char bat sartuko da ranbom eginez
    Helbidea hel; //helbidea
    Datua dat; //datua
    mm memoManag; //memory management
    int offset; //luzera jakiteko
    int aurkitua;//aldagai laguntzaile bat
    while(1){
        if(q->zenbat<1000){//aldioro 1000 prozesu baino gehiago ez eduki. Memoria kontrolatzeko eta ez betetzeko.
          zenbakia = rand() % 100000;// %0,001 eko probabilitadea jarri dut
          if(zenbakia==0){
              P.pid=i;
                P.x_denb=rand() % 400 + 1;//exekuzoa denbora bat eman auzaz.
                P.ego='I';
                P.lehentasuna=rand() % 201;//lehentasuna jarri 0tik 200 era.

                //memory management-eko datuak sortu
                //nik hauek random-ekin sortzen ditut eta ez ditut irakurtzen
                //datuak sortu memoria birtualean eta helbidea gorde.
                offset=5;//denak 5-eko luzera edukiko dute
                //zer posiziotan jarri jakiteko
                aurkitua=0;
                while(aurkitua==0){//betea dagoen bitartean itxaron
                for(int j=0;j<memoria/offset;j++){
                    if(bitMapaMemoria[j]==0){
                      non=j;
                      aurkitua=1;
                    }
                  }
                }
                hel.h=non;//helbidea sortu
                for(int i=0; i<offset;i++){
                  randomletter = 'A' + (random() % 26);
                  dat.d=randomletter;//datua sortu
                  memoriaNagusia[non] = dat;//memorian sartu datuak
                }
                memoManag.data=hel;//helbidea gorde
                memoManag.offsetD=offset;//helbidea gorde

                //codeak sortu memoria birtualean eta helbidea gorde
                aurkitua=0;
                while(aurkitua==0){//betea dagoen bitartean itxaron
                for(int j=0;j<memoria/offset;j++){
                    if(bitMapaMemoria[j]==0){
                      non=j;
                      aurkitua=1;
                    }
                  }
                }
                hel.h=non;//helbidea sortu
                for(int i=0; i<offset;i++){
                  randomletter = 'A' + (random() % 26);
                  dat.d=randomletter;//codea sortu
                  memoriaNagusia[non] = dat;//memorian sartu codea
                }
                memoManag.code=hel;//helbidea gorde
                memoManag.offsetC=offset;//helbidea gorde

                //orri taula aldatu
                hel.h = memoManag.data.h*5;
                pageTable[memoManag.data.h]=hel;
                hel.h = memoManag.code.h*5;
                pageTable[memoManag.code.h]=hel;
                //pgb sartu prozesuan
                memoManag.pgb=&pageTable;

                //sartu prozesuan informazioa 
                P.memoMana=memoManag;

                //mutex
                pthread_mutex_lock(&beg_Sche);
                listaSartu(q,P);
                pthread_mutex_unlock(&beg_Sche);
              i++;
              //listaInprimatu(q);
          }
       }
    }
}