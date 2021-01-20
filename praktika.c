#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "globals.h"
#include "lista.h"
#include "clock.h"
#include "timera.h"
#include "core.h"

void sortu_hariak(int maiz){//clock, timer, process generator, Schedule Dispatcher, orekatu eta coreak

    int i, err, hari_kop;
    pthread_t *hariak;
    struct Informazioa *h_p;
    hari_kop=5;//5 hari egingo dira

    hariak = malloc(hari_kop * sizeof(pthread_t));
    h_p = malloc(hari_kop * sizeof(struct Informazioa));

    i=0;//hasieratu

    for(int j = 0; j < hari_kop; j++){//pasatzeko informazioa sortu
        h_p[j].maiztasuna = maiz;
    }

    //clock haria sortu eta clock prozesua eman
    err = pthread_create(&hariak[i], NULL, clocka, (void *)&h_p[i]);

    if(err > 0){
        fprintf(stderr, "Errore bat gertatu da clock haria sortzean.\n");
        exit(1);
    }
    i++;//hurrengo harira pasa

    //timer haria sortu eta timer prozesua eman
    err = pthread_create(&hariak[i], NULL, timera, (void *)&h_p[i]);;

    if(err > 0){
        fprintf(stderr, "Errore bat gertatu da timer haria sortzean.\n");
        exit(1);
    }
    i++;//hurrengo harira pasa

    //process generator haria sortu eta loader prozesua eman
    err = pthread_create(&hariak[i], NULL, loader, (void *)&h_p[i]);;

    if(err > 0){
        fprintf(stderr, "Errore bat gertatu da process generator haria sortzean.\n");
        exit(1);
    }
    i++;//hurrengo harira pasa

    //Scheduler/Dispatcher haria sortu eta scheduler_Dispatcher prozesua eman
    err = pthread_create(&hariak[i], NULL, scheduler_Dispatcher, (void *)&h_p[i]);

    if(err > 0){
        fprintf(stderr, "Errore bat gertatu da Scheduler/Dispatcher haria sortzean.\n");
        exit(1);
    }
    i++;

	//erekatu haria sortu eta prozesua eman
    err = pthread_create(&hariak[i], NULL, oreka, (void *)&h_p[i]);

    if(err > 0){
        fprintf(stderr, "Errore bat gertatu da orekatu haria sortzean.\n");
        exit(1);
    }

    //hariak hasieratu
    for(i = 0;i < hari_kop;i++){
        pthread_join(hariak[i], NULL);
    }

    //core hariak sortu. Hauek core bakoitzaren prozesuak kudeatuko dituzte
    pthread_t *core_thread;
    core_thread = malloc(corekop * sizeof(pthread_t));
    struct Informazioa *c_p;
    c_p = malloc(corekop * sizeof(struct Informazioa));
    for(int r = 0; r < corekop; r++){//pasatzeko informazioa sortu
        c_p[r].id = r;
    }
    for(int j=0;j<corekop;j++){
        err = pthread_create(&core_thread[j], NULL, core, (void *)&c_p[j]);

        if(err > 0){
            fprintf(stderr, "Errore bat gertatu da core hariak sortzean.\n");
            exit(1);
        }
    }

    //coreak
     for(int z = 0;z < corekop;z++){
        pthread_join(core_thread[z], NULL);
    }
    

    free(hariak);
    free(h_p);
    free(core_thread);
}

void orekatu(){
    process_queue *lis;//hemen gorde mugituko diren prozesuak
    lis=malloc(sizeof(process_queue));
    listaHasieratu(lis);
    struct PCB pro;
    //prozesuen batez bestekoa kalkulatu
    float batezBes=0;
    pthread_mutex_lock(&kont_core);//ez da soluzio onena baina ez dakit core bakoitzeko mutex bat sortzen. Hau da ezin ditut coreak independenteki gelditu. Batek beste guztiak geratuko ditu.
    for(int i=0;i<corekop;i++){
        batezBes=+coreak[i].zenbat;
    }
    batezBes=batezBes/corekop;
    batezBes=floor(batezBes);//beheraka biribildu beti sobran ibiltzeko.
    int zenMug;//zenbat mugitu bati jakiteko
    //aurrena prozesuak hartu batezBest baina gehiago dutenei
    for(int j=0;j<corekop;j++){
        if(coreak[j].zenbat>batezBes){
            zenMug=coreak[j].zenbat-batezBes;
            while(zenMug!=0){
                pro=listaHartu(&coreak[j]);
                listaSartu(lis,pro);
                zenMug--;
            }
        }
    }
    //ondoren prozesuak jarri denak berdin egoteko
    for(int z=0;z<corekop;z++){
        if(coreak[z].zenbat<batezBes){
            zenMug=batezBes-coreak[z].zenbat;
            while(zenMug!=0){
                pro=listaHartu(lis);
                listaSartu(lis,pro);
                zenMug--;
            }
        }
    }
    //sobratzen direnak sartu. Errorea batekoa izango da gehienez
    int non=0;
    while(!listaHutsa(lis)){
        pro=listaHartu(lis);
        listaSartu(&coreak[non],pro);
        non++;

    }
    pthread_mutex_unlock(&kont_core);

}
//200milisegunduro, quantum-ero, orekatu egiten ditu coretako ilarak.
void *oreka(void *hari_param){
    int semaforoa=0;//0 eta 1 balioak hartuko ditu. Kontatzeko erabiltzen da
    int kont;
    while(1){
        kont=0;
        if(semaforoa!=mug){
            kont++;
            if(kont==quantum){
                orekatu();
                kont=0;
            }
        }
    }
}

int main(int argc, char *argv[]){
  
	char *c;
	char *d;
	char *e;
  	int maiztasuna;
  	int tamPageTable;
  	if(argc!=3){
    	printf("maiztasuna eta core kopurua sartu behar dira\n");
    	exit(1);
  	}
 
    //quantumaren balioa eman. Linuxen bezala 200 jarriko dut
    quantum=200;
    //aldagaiak hartu. maiztasuna eta kore kopurua.
    maiztasuna = strtol(argv[1], &c, 10);
    corekop = strtol(argv[2], &d, 10);
    //memoria. 100 erabiliko da.
    memoria=1000;
    //memoria nagusia sortu
    memoriaNagusia = malloc(memoria*sizeof(Datua));
    //memoria nagusiaren bit mapa.bostnaka doazenez memoria/5
    int bitMapaTam = memoria/5;
    bitMapaMemoria = malloc(bitMapaTam*sizeof(int));
    //TLB sortu. Helbideak gordetzeko. 6-ko luzera izango du.2 TLB birtualekin eta fisikoekin
    luzeraTLB = 6;
    TLB1 = malloc(luzeraTLB*sizeof(Helbidea));
    TLB2 = malloc(luzeraTLB*sizeof(Helbidea));
    //bit mapa sortu TLB-rako.
    bitMapaTLB = malloc(luzeraTLB*sizeof(int));
    //Page Table sortu. Bakoitietan birtualak eta bikoitietan fisikoak
    tamPageTable = memoria/5;
    pageTable = malloc(tamPageTable*sizeof(Helbidea));
    //listak hasieratu
    q=malloc(sizeof(process_queue));
    coreak=malloc(corekop*sizeof(process_queue));
    for(int i=0;i<corekop;i++){//core bakoitzeko lista bat
        listaHasieratu(&coreak[i]);
    }
    //mutexak sortu
    pthread_mutex_init(&kont_tik, NULL);
    pthread_mutex_init(&beg_Sche,NULL);
    pthread_mutex_init(&kont_core,NULL);
    //Process_Queue sortu
    listaHasieratu(q);
    //hariask sortu
    sortu_hariak(maiztasuna);
    //programa bukatzean aldagaia borratu
    pthread_mutex_destroy(&kont_tik);
    pthread_mutex_destroy(&beg_Sche);
    pthread_mutex_destroy(&kont_core);
    //programa bukatzean Queue borratu
    //askQueue(&p);
}