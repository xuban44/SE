#include "globals.h"
#include "lista.h"
#include <stdlib.h>
#include <math.h>
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
    int bukatua=0;//bukatu duen prozesua ala ez jakiteko

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
                        bukatua=1;
                    }
                    //bukatzean TLB-tik borratu eta memoria nagusitik
                    if(bukatua==1){
                    	bitMapaTLB[pro.memoMana.code.h]=0;
                    	bitMapaTLB[pro.memoMana.data.h]=0;
                    	//memoria nagusitik borratu
                    	for(int j=0; j<5;j++){
                    		bitMapaMemoria[pro.memoMana.code.h+j]=0;
                    		bitMapaMemoria[pro.memoMana.data.h+j]=0;
                    	}

                    }
					bukatua=0;
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

                    //PTBR=pro.memoMana.pgb;
                }
                else{//hau milisegunduro egiten da. Linuxekoak bezala. Corea geldirik badago milisegunduro orekatzen da.
                    orekatu();
                }
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