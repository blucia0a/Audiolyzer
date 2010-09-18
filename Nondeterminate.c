/*
 *Nondeterminate - A piece of music to be played by a computer running 
 *                 Audiolyzer.
 *
 *Copyright Brandon Lucia 2010
 *blucia@gmail.com
 ***************************** 
 *
 *This piece of music is composed in four movements, and can be played by
 *compiling this program w/ -O0, and running it under Pin (http://pintool.org)
 *with Audiolyzer as the pintool (Audiolyzer is part of this project).
 *
 *A more thorough discussion of the piece and its structure and its main idea
 *can be found at http://catspajamas.tumblr.com
 *
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUMTHREADS 16
#define P2_NUMTHREADS 32
#define P3_NUMTHREADS 32 
#define P4_NUMTHREADS 64 

#define OUTER 5 
#define SHORT 2 
void SET_SD_SAMPLE_RATE(unsigned long r)  __attribute__ ((noinline));
void SET_SD_SAMPLE_RATE(unsigned long r){
  fprintf(stderr,"updating the sample rate!!\n");
}

pthread_mutex_t globLock;
pthread_barrier_t globBar;

pthread_mutex_t phase_1Lock;
pthread_barrier_t phase_1Bar;

int sum = 0;

pthread_mutex_t phase_2aLock;
pthread_mutex_t phase_2bLock;

pthread_barrier_t phase_2aBar;
pthread_barrier_t phase_2bBar;

int suma = 0;
int sumb = 0;

#define NUMP3GROUPS 8
volatile int p3CurGroup = 0;
pthread_barrier_t phase_3Bars[NUMP3GROUPS];
pthread_mutex_t phase_3Locks[NUMP3GROUPS];



int done = 0;

void *phase_4(void *v){

  pthread_barrier_wait(&phase_1Bar);

  fprintf(stderr,"DONE WITH BARRIER!\n");
  int i = 0;
  for(i = 0; i < 33; i++){
    pthread_mutex_lock(&phase_1Lock);
    sum ++; 
    pthread_mutex_unlock(&phase_1Lock);
    usleep(1000000 - (100*i*i));
  }
  pthread_mutex_lock(&globLock);
  done++;
  pthread_mutex_unlock(&globLock);
}

void *phase_1(void *v){

  pthread_barrier_wait(&phase_1Bar);

  fprintf(stderr,"DONE WITH BARRIER!\n");
  int i = 0;
  for(i = 0; i < 50; i++){
    pthread_mutex_lock(&phase_1Lock);
    sum ++; 
    pthread_mutex_unlock(&phase_1Lock);
    usleep(1000 - (10*i));
  }
  pthread_mutex_lock(&globLock);
  done++;
  pthread_mutex_unlock(&globLock);
}

void *phase_2a(void *v){

  pthread_barrier_wait(&globBar);
  pthread_barrier_wait(&phase_2bBar);//means b goes first
  int t;
  for(t = 0; t < OUTER; t++){

    fprintf(stderr,"A is waiting\n");
    pthread_barrier_wait(&phase_2aBar);
    fprintf(stderr,"A is Running\n");
    int i = 0;
    for(i = 0; i < 10; i++){
      pthread_mutex_lock(&phase_2aLock);
      suma ++; 
      pthread_mutex_unlock(&phase_2aLock);
      usleep(1000 - (10*i*i));
    }

    if(t < OUTER - 1){ 
      pthread_barrier_wait(&phase_2bBar);
    }

  }
  pthread_mutex_lock(&globLock);
  done++;
  pthread_mutex_unlock(&globLock);
}

void *phase_2b(void *v){
  pthread_barrier_wait(&globBar);
  int t;
  for(t = 0; t < OUTER; t++){

    fprintf(stderr,"B is waiting\n");
    pthread_barrier_wait(&phase_2bBar);
    fprintf(stderr,"B is running\n");
    int i;
    for(i = 0; i < 10; i++){
      pthread_mutex_lock(&phase_2bLock);
      sumb ++; 
      pthread_mutex_unlock(&phase_2bLock);
      usleep(1000 - (10*i*i));
    }
    pthread_barrier_wait(&phase_2aBar);
    //fprintf(stderr,"B is waiting for 5s\n");
    //usleep(5000000);
    //fprintf(stderr,"B is done waiting\n");

  }
  fprintf(stderr,"B is done\n");

  pthread_mutex_lock(&globLock);
  done++;
  pthread_mutex_unlock(&globLock);
}



void *phase_3(void *v){

  int *tidp = (int*)v;
  int tid = *tidp;
  int groupID = 0;
  int g;
  for(g = 0; g < NUMP3GROUPS; g++){
    if(g * (P3_NUMTHREADS / NUMP3GROUPS) > tid){
      break;
    }
    groupID = g;
  }

  fprintf(stderr,"tid was %d\n",tid);
  fprintf(stderr,"groupID was %d\n",groupID);

  pthread_barrier_wait(&globBar);

  int t;
  for(t = 0; t < SHORT; t++){


    while(1){    
      if(p3CurGroup != groupID){
        pthread_barrier_wait(&globBar);
      }else{
        break; 
      }
    }

    fprintf(stderr,">>>>>>>>>>>>>>>>thread %d in group %d is running<<<<<<<<<<<<<<<<\n",tid,groupID);
    
    int i;
    for(i = 0; i < 5; i++){
      pthread_mutex_lock(&phase_3Locks[groupID]);
      sumb ++; 
      pthread_mutex_unlock(&phase_3Locks[groupID]);
      usleep(1000 - (10*i*i));
    }

    if(tid % (P3_NUMTHREADS / NUMP3GROUPS) == 0){
      fprintf(stderr,"tid %d incremented group count\n",tid);
      if(groupID < NUMP3GROUPS-1){
        pthread_mutex_lock(&globLock);
        p3CurGroup++;
        pthread_mutex_unlock(&globLock);
      }else{
        pthread_mutex_lock(&globLock);
        p3CurGroup = 0;
        pthread_mutex_unlock(&globLock);
      }
    }
    if(t < SHORT - 1){
      pthread_barrier_wait(&globBar);
    }else{
      pthread_mutex_lock(&globLock);
      done = P3_NUMTHREADS;
      pthread_mutex_unlock(&globLock);
    }

  }
  fprintf(stderr,"%d is done\n",tid);

  pthread_mutex_lock(&globLock);
  done++;
  pthread_mutex_unlock(&globLock);
}




int main(int argc, char *argv[]){

  pthread_mutex_init(&globLock,NULL);
  pthread_mutex_init(&phase_1Lock,NULL);
  pthread_barrier_init(&phase_1Bar,NULL,NUMTHREADS);  
  pthread_t spawned[NUMTHREADS];
  
  fprintf(stderr,"Phase 1\n");
  //goto phase2;
  goto phase3;
  //goto phase4;
  int i;
  for(i = 0; i < NUMTHREADS; i++){

    pthread_mutex_lock(&globLock);
    fprintf(stderr,"creating thread %d\n",i);
    pthread_create(&spawned[i],NULL,phase_1,NULL); 
    fprintf(stderr,"created thread %d\n",i);
    pthread_mutex_unlock(&globLock);     

  }

  while(1){
    pthread_mutex_lock(&globLock);
    if(done == NUMTHREADS){
      pthread_mutex_unlock(&globLock);
      break;
    }
    usleep(5000);
    pthread_mutex_unlock(&globLock);
    usleep(50);
  }
 
  for(i = 0; i < NUMTHREADS; i++){
    pthread_join(spawned[i],NULL);
  }
  done = 0;

phase2:
  /*End Phase 1*/

  fprintf(stderr,"Phase 2\n");
  pthread_mutex_init(&phase_2aLock,NULL);
  pthread_mutex_init(&phase_2bLock,NULL);
  pthread_barrier_init(&phase_2aBar,NULL,P2_NUMTHREADS);  
  pthread_barrier_init(&phase_2bBar,NULL,P2_NUMTHREADS);  
  pthread_barrier_init(&globBar,NULL,P2_NUMTHREADS);  
  pthread_t spawned2[P2_NUMTHREADS];

  for(i = 0; i < P2_NUMTHREADS; i++){
    fprintf(stderr,"Spawning 2!\n");

    pthread_mutex_lock(&globLock);
    fprintf(stderr,"creating thread %d\n",i);

    if(i < P2_NUMTHREADS / 2){
      pthread_create(&spawned2[i],NULL,phase_2a,NULL); 
    }else{
      pthread_create(&spawned2[i],NULL,phase_2b,NULL); 
    }

    fprintf(stderr,"created thread %d\n",i);
    pthread_mutex_unlock(&globLock);     

  }

  while(1){
    //pthread_mutex_lock(&globLock);
    if(done == P2_NUMTHREADS){break;}
    usleep(5000);
    //pthread_mutex_unlock(&globLock);
    usleep(50);
  }

phase3:
  done = 0; 
  pthread_barrier_init(&globBar,NULL,P3_NUMTHREADS);  
  pthread_mutex_init(&globLock,NULL);  
  fprintf(stderr,"Phase 3\n");

  int l;
  for(l = 0; l < NUMP3GROUPS; l++){
    pthread_mutex_init(&phase_3Locks[l], NULL);
    pthread_barrier_init(&phase_3Bars[l], NULL, P3_NUMTHREADS / NUMP3GROUPS);  
  }

  pthread_t spawned3[P3_NUMTHREADS];

  for(i = 0; i < P3_NUMTHREADS; i++){

    pthread_mutex_lock(&globLock);

    fprintf(stderr,"creating thread %d\n",i);
    int *arg = (int*)malloc(sizeof(int));
    *arg = i;
    pthread_create(&spawned3[i],NULL,phase_3,(void*)arg); 
    fprintf(stderr,">>>created thread<<< %d\n",i);

    pthread_mutex_unlock(&globLock);     

  }

  while(1){
    //pthread_mutex_lock(&globLock);
    if(done == P3_NUMTHREADS){break;}
    usleep(5000);
    //pthread_mutex_unlock(&globLock);
    usleep(50);
  }

phase4: 
  done = 0; 
  //phase 4
  //pthread_mutex_init(&globLock,NULL);
  pthread_mutex_init(&phase_1Lock,NULL);
  pthread_barrier_init(&phase_1Bar,NULL,P4_NUMTHREADS);  
  pthread_t spawned4[P4_NUMTHREADS];
  
  fprintf(stderr,"Phase 4\n");
  for(i = 0; i < P4_NUMTHREADS; i++){

    pthread_mutex_lock(&globLock);
    fprintf(stderr,"creating thread %d\n",i);
    pthread_create(&spawned4[i],NULL,phase_4,NULL); 
    fprintf(stderr,"created thread %d\n",i);
    pthread_mutex_unlock(&globLock);     

  }

  while(1){
    if(done == P4_NUMTHREADS){
      break;
    }
    usleep(500);
  }
 
  for(i = 0; i < P4_NUMTHREADS; i++){
    pthread_join(spawned4[i],NULL);
  }
  done = 0;
  
  //done
  for(i = 0; i < 1000; i++){
    pthread_mutex_lock(&globLock);
    pthread_mutex_unlock(&globLock); 
  }
}
