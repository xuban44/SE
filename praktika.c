#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>


//prozesuaren informazioa
struct PCB{
    int pid;//id-a
    int lehentasuna;//0tik-200era. 200 lehentasun gehien eta 0 gutxien
    int x_denb;//exekuzio denbora
    char ego;//egoera. X->exekutatzen. I->itxaroten.
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
        if(begiratu->balioa.lehentasuna < pro.lehentasuna){//lehentasun handiena badu
            tmp->next=begiratu;
            q->aurrena=tmp;
            q->zenbat++;
        }
        else{
            int jarraitu=1;//0 denean whileatik aterako da
            while(jarraitu){
                if(begiratu->next==NULL){
                    jarraitu=0;
                }
                else{
                    if(begiratu->next->balioa.lehentasuna < pro.lehentasuna){//bilatu txikiagoa
                        jarraitu=0;
                    }
                    else{
                        begiratu=begiratu->next;//hurrengoa
                    }
                }
                    
            }
            tmp->next=begiratu->next;
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

void listaLehentasunakIgo(process_queue *q){//bakoitzari lehentasuna igotzen zaio.+1. Maximoa 200.
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
process_queue *coreak;
//denbora kontrolatzeko
int tik;
int corekop;//zenbat kore dituen ordenagailuak.
int mug;//0 edo 1 balioa izango du.
int quantum;//prozezu bakoitzeko denbora
//mutex-ak
pthread_mutex_t kont_tik;
pthread_mutex_t beg_Sche;
//funtzioak
void sortu_hariak(int maiz);
void *clocka (void *hari_param);
void *timera (void *hari_param);
void *process_generator(void *hari_param);
void *scheduler_Dispatcher(void *hari_param);
void *core(void *hari_param);


void sortu_hariak(int maiz){//clock, timer, process generator, Schedule Dispatcher

    int i, err, hari_kop;
    pthread_t *hariak;
    pthread_t *core_thread;
    struct Informazioa *h_p;
    hari_kop=4;//4 hari egingo dira

    hariak = malloc(hari_kop * sizeof(pthread_t));
    h_p = malloc(hari_kop * sizeof(struct Informazioa));

    i=0;//hasieratu

    for(int j = 0; j < hari_kop-1; j++){//pasatzeko informazioa sortu
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

    //process generator haria sortu eta process_generator prozesua eman
    err = pthread_create(&hariak[i], NULL, process_generator, (void *)&h_p[i]);;

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

    //core hariak sortu. Hauek core bakoitzaren prozesuak kudeadtuko dituzte
    core_thread = malloc(corekop * sizeof(pthread_t));
    struct Informazioa *c_p;
    c_p = malloc(corekop * sizeof(struct Informazioa));
    for(int r = 0; r < hari_kop-1; r++){//pasatzeko informazioa sortu
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
     for(int z = 0;z < corekop;z++)
        pthread_join(core_thread[z], NULL);

    //hariak
    for(i = 0;i < hari_kop;i++)
        pthread_join(hariak[i], NULL);

    free(hariak);
    free(h_p);


}

void *clocka (void *hari_param){
    tik=0;
    while(1){
        pthread_mutex_lock(&kont_tik);
        tik++;
        pthread_mutex_unlock(&kont_tik);
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

void *process_generator(void *hari_param){
    //batzuetan prozesu bat sortzen du ditu
    int zenbakia;
    struct PCB P;//prozesua sortzeko
    int i=0;//zenbatgarren prozesua den jakiteko
    while(1){
        if(q->zenbat<1000){//aldioro 1000 prozesu baino gehiago ez eduki. Memoria kontrolatzeko eta ez betetzeko.
    	   zenbakia = rand() % 100000;// %0,001 eko probabilitadea jarri dut
    	   if(zenbakia==0){
    		  P.pid=i;
              P.x_denb=rand() % 400 + 1;//exekuzoa dnebora bat eman auzaz.
              P.ego='I';
              pthread_mutex_lock(&beg_Sche);
              listaSartu(q,P);
              pthread_mutex_unlock(&beg_Sche);
    		  i++;
    		  listaInprimatu(q);
    	}
        }
    }
}

void *scheduler_Dispatcher(void *hari_param){
    //Erabaki zein nukleori pasa prozesua. lehentasun handieneko prozesua hartzen du.
    struct PCB P;//hartuko den prozesua
    int txikiena=0;
    while(1){
        if(!listaHutsa(q)){
            pthread_mutex_lock(&beg_Sche);
            P=listaHartu(q);//prozesu bat hartzen du.Lehentasunaren arabera.
            pthread_mutex_unlock(&beg_Sche);
            //begiratu zer coretan dauden prozesu gutxien
            for(int i=1;i<corekop;i++){
                if(coreak[i].zenbat>coreak[txikiena].zenbat){
                    txikiena=i;
                }
            }
            //bertan sartu prozesua;
            listaSartu(&coreak[txikiena],P);
       }
    }
}
void *core(void *hari_param){
    int semaforoa=0;//0 eta 1 balioak hartuko ditu. Kontatzeko erabiltzen da
    int kont;//zenbat denbora doan kontatuko du.
    struct Informazioa *param;
    param = (struct Informazioa *)hari_param;
    int id = param->id;
    struct PCB pro;//prozesatzen ari den balioa
    int prozesatzen=0;//lanean ari den ala ez jakiteko
    while(1){
        if(semaforoa!=mug){//maiztasuna betetzean begiratu
            if(prozesatzen){//lanean ari bada corea.
                semaforoa=mug;
                kont++;
                if(kont==quantum){
                    if(coreak[id].aurrena->balioa.x_denb>quantum){//kenketa egin denbora gehiago behar badu
                        coreak[id].aurrena->balioa.x_denb=-quantum;
                        listaSartu(&coreak[id],pro);
                    }
                    listaLehentasunakIgo(&coreak[id]);//geldirik zeudenei lehentsunak igo
                    prozesatzen=0;//ez dago inor prozesatzen, hurrengo itzulian hartuko du bat.
                }
            }
            else{//hau milisegunduro egiten da. Linuxekoak bezala. Corea geldirik badago milisegunduro begiratzen du
                if(!listaHutsa(&coreak[id])){//begiratu prozesurik badagoen hartzeko
                    pro=listaHartu(&coreak[id]);
                    pro.ego='X';//egoera aldatu
                    prozesatzen=1;//suposatzen da lehenengoa hartu duela. Lehentasunaren arabera ordenatuak daudelako
                }
            }
        }
    }    
}   

int main(int argc, char *argv[]){
	
	char *c;
	int maiztasuna;
	if(argc!=2){
		printf("maiztasuna bakarrik sartu behar da\n");
		exit(1);
	}

    //quantumaren balioa eman. Linuxen bezala 200 jarriko dut
    quantum=200;
    //suposatuko dugu 3 core dituela gure ordenagailuak. Agindua sistema operatibo bakoitzero desberdina delako.
    //gehiago hartzen baditut arazoak ematen dizkit ordenagailuak.
    corekop = 3;
    //listak hasieratu
    q=malloc(sizeof(process_queue));
    coreak=malloc(corekop*sizeof(process_queue));
    for(int i=0;i<corekop;i++){//core bakoitzeko lista bat
        listaHasieratu(&coreak[i]);
    }
    //mutexak sortu
    pthread_mutex_init(&kont_tik, NULL);
    pthread_mutex_init(&beg_Sche,NULL);
    //Process_Queue sortu
    listaHasieratu(q);
    //maiztasuna hartu
    maiztasuna = strtol(argv[1], &c, 10);
    //hariask sortu
    sortu_hariak(maiztasuna);
    //programa bukatzean aldagaia borratu
    pthread_mutex_destroy(&kont_tik);
    pthread_mutex_destroy(&beg_Sche);
    //programa bukatzean Queue borratu
    //askQueue(&p);
}

