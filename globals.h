#include <pthread.h>

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
int mug;//0 edo 1 balioak izango du.
int quantum;//prozezu bakoitzeko denbora
int memoria;//zenbateko memoria edukiko duen
int luzeraTLB;//TLB-aren luzera

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