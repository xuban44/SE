#include "globals.h"
#include <stdlib.h>
#include <stdio.h>

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