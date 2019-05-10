//project in embedded systems Dimitra Karatza 8828

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

//*****************write the files***********************************8
void writeFiles(double array[],int size){

  //write timestamps
  FILE *f1 = fopen("timestamps.txt", "w");
  if (f1 == NULL){
      printf("Error opening file!\n");
      exit(1);
  }
  int i;
  for(i=0;i<size;i++){
    fprintf(f1, "%f\n", array[i]);
  }
  fclose(f1);

  //write the time difference
  FILE *f2 = fopen("difference.txt", "w");
  if (f2 == NULL){
      printf("Error opening file!\n");
      exit(1);
  }
  for(i=1;i<size;i++){
    fprintf(f2, "%f\n", array[i]-array[i-1]);
  }
  fclose(f2);
}

//**************create graphs*****************************
void create_graphs(){
  int unused __attribute__((unused));
  // make plot of packages with plot.py
  unused = system("python plot.py");
}

//****************simple code for timestamps****************************
void timestamps(float t, float dt, int size){

  struct timeval tv;
  double array[size],ret,dif,sum=0;
  int pos=-1,i;
  for(i=0;i<size;i++){
    gettimeofday(&tv,NULL);
    ret=(double) tv.tv_sec + (double) tv.tv_usec*pow(10,-6);
    pos++;
    array[pos]=ret;
    //printf("%f\n", array[pos]);

    if(pos!=0){
      dif = array[pos]-array[pos-1];
      //printf("%f\n\n", dif);
    }
    usleep(dt*1000000);

  }

  writeFiles(array,size);
  create_graphs();

}

//****************improved code for timestamps****************************
void timestamps_improved(float t, float dt, int size){

  struct timeval tv;
  double array[size],ret,dif[size-1],sum=0;
  int pos=-1,i;
  for(i=0;i<size;i++){
    gettimeofday(&tv,NULL);
    ret=(double) tv.tv_sec + (double) tv.tv_usec*pow(10,-6);
    pos++;
    array[pos]=ret;
    //printf("%f\n", array[pos]);

    if(pos==0){
      usleep(dt*1000000);
    }else{
      dif[pos-1] = array[pos]-array[pos-1];
      sum=sum+dif[pos-1]-dt;
      //printf("%f\n\n", dif[pos-1]);
      if(dif[pos-1]!=dt){
        //usleep((2*dt-dif)*1000000);
        usleep((dt-(sum/pos))*1000000);
      }
    }

  }

  writeFiles(array,size);
  create_graphs();

}

//**************code without timestamps********************
void alarm_wakeup (int i){
   signal(SIGALRM,alarm_wakeup);
}

void no_timestamps(float t, float dt, int size){
  //initialization
  struct timeval tv;
  double array[size],ret,dif;
  int pos=-1;

  //set the timer
  struct itimerval tout_val;
  if(dt-(int)dt==0){
    tout_val.it_interval.tv_sec = dt;
    tout_val.it_interval.tv_usec = 0;
    tout_val.it_value.tv_sec = dt;
    tout_val.it_value.tv_usec = 0;
  }else{
    tout_val.it_interval.tv_sec = (int)dt;
    tout_val.it_interval.tv_usec = (dt-(int)dt)*1000000;
    tout_val.it_value.tv_sec = (int)dt;
    tout_val.it_value.tv_usec = (dt-(int)dt)*1000000;
  }

  setitimer(ITIMER_REAL, &tout_val,0);

  signal(SIGALRM,alarm_wakeup);

  //get the timestamps
  while (1){
    gettimeofday(&tv,NULL);
    ret=(double) tv.tv_sec + (double) tv.tv_usec*pow(10,-6);
    pos++;
    array[pos]=ret;
    //printf("%f\n", array[pos]);

    if(pos!=0){
      dif = array[pos]-array[pos-1];
      //printf("%f\n\n", dif);
    }

    if(pos==size-1){
      writeFiles(array,size);
      create_graphs();
      exit(0);
    }

    pause();

  }

}

//**************main function*****************************
int main(int argc, char **argv){

  float t=atof(argv[1]);
  float dt=atof(argv[2]);
  int size_array=(int) (t/dt);

  if(atoi(argv[3])==0){
    timestamps(t,dt,size_array);
  }else if(atoi(argv[3])==1){
    timestamps_improved(t,dt,size_array);
  }else if(atoi(argv[3])==2){
    no_timestamps(t,dt,size_array);
  }

  return 0;
}
