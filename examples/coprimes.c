#include <stdio.h>
#include <stdlib.h>

#define GENERATOR_IMPLEMENTATION
#include "generator.h"

void* coprimes(void *imax_arg){
    unsigned long m,n,aux;
    unsigned long imax = (unsigned long)imax_arg;
    int br;

    if(imax<2){ return NULL; }

    m=2;
    n=1;
    yield(m);
    yield(n);

    if(imax<3){ return NULL; }

    br=0;
    while(m!=1){
        aux=(m<<1)-n;
        if(aux<=imax && br<1){
            aux = m;
            m=(m<<1)-n;
            n=aux;
            br=0;
            yield(m);
            yield(n);
            continue;
        }
        aux=(m<<1)+n;
        if(aux<=imax && br<2){
            aux = m;
            m=(m<<1)+n;
            n=aux;
            br=0;
            yield(m);
            yield(n);
            continue;
        }
        aux=m+(n<<1);
        if(aux<=imax && br<3){
            m=m+(n<<1);
            br=0;
            yield(m);
            yield(n);
            continue;
        }

        br=3;
        while(br==3){
            if((n<<1)>=m){
                aux=n;
                n=(n<<1)-m;
                m=aux;
                br=1;
            }else if(3*n>=m){
                aux=n;
                n=m-(n<<1);
                m=aux;
                br=2;
            }else{
                m-=n<<1;
                br=3;
            }
        }
        
        if(m==1 && br==1){
            m = 3;
            n = 1;
            br=0;
            yield(m);
            yield(n);
        }
    }

    return NULL;
}

int main(void)
{
    int n = 4096;
    void* base = malloc(n);

    Generator g = generator_create(coprimes, (void*)5, base, n);
    while (has_next(g)) {
        void *a = next(&g);
        void *b = next(&g);
        if (has_next(g)) {
            printf("%ld , %ld\n", (long)a, (long)b);
        }
    }

    free(generator_destroy(&g));

    return 0;
}

