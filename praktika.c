#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

//memoriako 
struct Datua{
	char d;
};
typedef struct Datua Datua;
//memoriaren helbideak
struct Helbidea{
	int h;
};
typedef struct Helbidea Helbidea;

//memory management datu egitura sortu 
struct mm{
	Helbidea **pgb;
	Helbidea code;
	Helbidea data;
	int offsetC; //luzera jakiteko
	int offsetD;
};
typedef struct mm mm;

//prozesuaren informazioa
struct PCB{
    int pid;//id-a
    int lehentasuna;//0tik-200era. 200 lehentasun gehien eta 0 gutxien
    int x_denb;//exekuzio denbora
    char ego;//egoera. X->exekutatzen. I->itxaroten.
    mm memoMana;
};

//hariak hartuko duten informazioa
struct Informazioa{
    int maiztasuna;//pasa behar den maiztasuna
    int id;//zein den jakiteko
};

//nodoak sortu eta bakoitzean PCB izan
struct node{
    struct PCB balioa;
    struct node *next;
};
typedef struct node node;//izena aldatu

//prozesuen lista dinamikoa sortu. Prozesuak lehentasunaren arabera ordenatuak egongo dira
struct process_queue{
    int zenbat;
    node *aurrena;
};
typedef struct process_queue process_queue;//izena aldatu

//hasieratu lista
void listaHasieratu(process_queue *q){
    q->zenbat = 0;
    q->aurrena = NULL;
}

//begiratu lista hutsa dagoen ala ez
int listaHutsa(process_queue *q){
    return(q->aurrena == NULL);
}

int listaZenbat(process_queue *q){
    return q->zenbat;
}
void listaInprimatu(process_queue *q){
    node *begiratu;
    begiratu = malloc(sizeof(node));
    begiratu = q->aurrena;
    int jarraitu = q->zenbat;
    while(jarraitu!=0){
        printf("%d.",begiratu->balioa.pid);
        begiratu=begiratu->next;
        jarraitu--;
    }
}

//sartu listan PCB bat. Lehentasunaren arabera ordenatua
void listaSartu(process_queue *q, struct PCB pro){
    node *tmp;
    tmp = malloc(sizeof(node));
    tmp->balioa = pro;
    tmp->next = NULL;
    if(listaHutsa(q)){
        q->aurrena = tmp;
        q->zenbat++;
    }
    else{
        node *begiratu;
        begiratu = malloc(sizeof(node));
        begiratu = q->aurrena;
        if(begiratu->balioa.lehentasuna < pro.lehentasuna){//lehentasun handiena badu, lehenengo kasuan aurrena aldatu
            tmp->next=begiratu;
            q->aurrena=tmp;
            q->zenbat++;
        }
        else{
            int jarraitu=1;//0 denean whileatik aterako da
            int aldatu=0;
            while(jarraitu){
                if(begiratu->next==NULL){
                    jarraitu=0;
                }
                else{
                    if(begiratu->next->balioa.lehentasuna < pro.lehentasuna){//bilatu txikiagoa
                        jarraitu=0;
                        aldatu=1;
                    }
                    else{
                        begiratu=begiratu->next;//hurrengoa
                    }
                }
                    
            }
            if(aldatu){
            	tmp->next=begiratu->next;
        	}
            begiratu->next=tmp;
            q->zenbat++;
        }
    }
}

struct PCB listaHartu(process_queue *q){
    struct PCB pro;
    //badakigu ez dela hutsa eta ez da arazorik egongo
    pro=q->aurrena->balioa;
    q->aurrena=q->aurrena->next;
    q->zenbat--;
    return pro;
}

void listaLehentasunakIgo(process_queue *q){//bakoitzari lehentasuna igotzen zaio.+1. Maximoa 200. Horrela badakit prozesu bat ez dela exekutatu gabe geratuko.
    node *begiratu;
    begiratu = malloc(sizeof(node));
    begiratu = q->aurrena;
    int jarraitu = q->zenbat;
    while(jarraitu!=0){
        begiratu->balioa.lehentasuna=+1;
        begiratu=begiratu->next;
        jarraitu--;
    }
}

//ALDAGAI GLOBALAK
process_queue *q;//prozesuak sortzean process-generator-ek hemen utziko ditu.
process_queue *coreak; //coreak
Datua *memoriaNagusia; //memoria nagusia
Helbidea *pageTable; //Page table
int *bitMapaMemoria;
Helbidea *TLB1;//TLB. helbideak gordetzeko.
Helbidea *TLB2;//TLB. helbideak gordetzeko. aurreko helbidearen itzulpena
int *bitMapaTLB;

//process_queue *lis;//hemen gorde mugituko diren prozesuak
//denbora kontrolatzeko
int tik;
int corekop;//zenbat kore dituen ordenagailuak.
int mug=0;//0 edo 1 balioak izango du.
int quantum;//prozezu bakoitzeko denbora
int memoria;//zenbateko memoria edukiko duen
int memNon=0;//memoria nagusian non sartu jakiteko
int luzeraTLB;

//mutex-ak
pthread_mutex_t kont_tik;
pthread_mutex_t beg_Sche;
pthread_mutex_t kont_core;
//funtzioak
void sortu_hariak(int maiz);
void *clocka (void *hari_param);
void *timera (void *hari_param);
void *loader(void *hari_param);
void *scheduler_Dispatcher(void *hari_param);
void *core(void *hari_param);
void *oreka(void *hari_param);
Helbidea MMU(Helbidea hel);
Helbidea TLB(Helbidea hel);

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

void *timera (void *hari_param){
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
void *core(void *hari_param){

	 //erregistroak
	int *PTBR;
	Datua IR;
	Helbidea PC;

    int semaforoa=0;//0 eta 1 balioak hartuko ditu. Kontatzeko erabiltzen da
    int kont=0;//zenbat denbora doan kontatuko du.
    struct Informazioa *param;
    param = (struct Informazioa *)hari_param;
    int id = param->id;
    struct PCB pro;//prozesatzen ari den balioa
    int prozesatzen=0;//lanean ari den ala ez jakiteko
    int luzera;//jakiteko kodea eta datuen luzera memorian
    int i;//aldagai laguntzaile bat

    while(1){
        if(semaforoa!=mug){//maiztasuna betetzean begiratu
            if(prozesatzen){//lanean ari bada corea.
                semaforoa=mug;
                kont++;
                if(kont==quantum){
                	kont=0;
                    if(pro.x_denb>quantum){//kenketa egin denbora gehiago behar badu
                        pro.x_denb=-quantum;
                        if(pro.lehentasuna>5){//begiratu lehentasuna ez dela 5 baino txikiagoa kenketa egiterakoan
                            pro.lehentasuna=-5;//5 kendu
                        }
                        pthread_mutex_lock(&kont_core);
                        listaSartu(&coreak[id],pro);
                        pthread_mutex_unlock(&kont_core);
                    }
                    pthread_mutex_lock(&kont_core);
                    pro.ego='I';
                    listaLehentasunakIgo(&coreak[id]);//geldirik zeudenei lehentsunak igo
                    pthread_mutex_unlock(&kont_core);
                    //exekutatu
                    //aurrena datuak
                    //begiratu zein den helbide fisikoa
                    PC=MMU(pro.memoMana.data);
                    luzera=pro.memoMana.offsetD;
                    for(int z=0; z<luzera; z++){
                    	IR=memoriaNagusia[PC.h+z];
                    	//suposatzen da emen exekutatu egiten dituela prozesuak
                    }
                    //ondoren kodea
                    //begiratu zein den helbide fisikoa
                    PC=MMU(pro.memoMana.code);
                    luzera=pro.memoMana.offsetC;
                    for(int k=0; k<luzera; k++){
                    	IR=memoriaNagusia[PC.h+k];
                    	//suposatzen da emen exekutatu egiten dituela prozesuak
                    }
                    prozesatzen=0;//ez dago inor prozesatzen, hurrengo itzulian hartuko du bat.
                }
            }
            else{//saiatu prozesu bat hartzen. Prozesua hartzean memoria fisikoan jarri.
                if(!listaHutsa(&coreak[id])){//begiratu prozesurik badagoen hartzeko
                    pthread_mutex_lock(&kont_core);
                    if(!listaHutsa(&coreak[id])){
                        pro=listaHartu(&coreak[id]);
                        pro.ego='X';//egoera aldatu
                        prozesatzen=1;//suposatzen da lehenengoa hartu duela. Lehentasunaren arabera ordenatuak daudelako
                    }
                    pthread_mutex_unlock(&kont_core);

                    PTBR=pro.memoMana.pgb;
                }
                else{//hau milisegunduro egiten da. Linuxekoak bezala. Corea geldirik badago milisegunduro orekatzen da.
                    orekatu();
                }
            }
        }
    }    
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

Helbidea MMU(Helbidea hel){
	return TLB(hel);
}
Helbidea TLB(Helbidea hel){//hemengo memoria 6koa egingo dut. Gardekinetan agertzen den bezala
	int aur=0; //badagoen ala ez jakiteko
	for(int i=0;i<luzeraTLB;i++){
		if(bitMapaTLB[i]==1){
			if(TLB1[i].h==hel.h){
				return TLB2[i];
			}

		}
	}
	//ez badago TLB-an pageTAble-an begiratu eta TLB-an sartu. Ausaz batengatik ordezkatu.
	int kendu = random() % luzeraTLB;
	TLB1[kendu]=hel;
	TLB2[kendu]=pageTable[hel.h];
	return pageTable[hel.h];
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