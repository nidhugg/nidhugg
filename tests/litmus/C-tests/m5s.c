// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/m5s.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r2_0; 
volatile int atom_1_r2_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];

  int v4_r2 = vars[0];
  int v64 = (v2_r1 == 1);
  atom_1_r1_1 = v64;
  int v65 = (v4_r2 == 0);
  atom_1_r2_0 = v65;
  int v66 = (v4_r2 == 2);
  atom_1_r2_2 = v66;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 2;
  return NULL;
}

void *t3(void *arg){
label_4:;
  vars[1] = 2;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[0] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_1_r2_0 = 0;
  atom_1_r2_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v5 = atom_1_r1_1;
  int v6 = atom_1_r2_0;
  int v7 = vars[0];
  int v8 = (v7 == 1);
  int v9 = vars[1];
  int v10 = (v9 == 1);
  int v11_conj = v8 & v10;
  int v12_conj = v6 & v11_conj;
  int v13_conj = v5 & v12_conj;
  int v14 = atom_1_r1_1;
  int v15 = atom_1_r2_0;
  int v16 = vars[0];
  int v17 = (v16 == 1);
  int v18 = vars[1];
  int v19 = (v18 == 2);
  int v20_conj = v17 & v19;
  int v21_conj = v15 & v20_conj;
  int v22_conj = v14 & v21_conj;
  int v23 = atom_1_r1_1;
  int v24 = atom_1_r2_0;
  int v25 = vars[0];
  int v26 = (v25 == 2);
  int v27 = vars[1];
  int v28 = (v27 == 1);
  int v29_conj = v26 & v28;
  int v30_conj = v24 & v29_conj;
  int v31_conj = v23 & v30_conj;
  int v32 = atom_1_r1_1;
  int v33 = atom_1_r2_0;
  int v34 = vars[0];
  int v35 = (v34 == 2);
  int v36 = vars[1];
  int v37 = (v36 == 2);
  int v38_conj = v35 & v37;
  int v39_conj = v33 & v38_conj;
  int v40_conj = v32 & v39_conj;
  int v41 = atom_1_r1_1;
  int v42 = atom_1_r2_2;
  int v43 = vars[0];
  int v44 = (v43 == 1);
  int v45 = vars[1];
  int v46 = (v45 == 1);
  int v47_conj = v44 & v46;
  int v48_conj = v42 & v47_conj;
  int v49_conj = v41 & v48_conj;
  int v50 = atom_1_r1_1;
  int v51 = atom_1_r2_2;
  int v52 = vars[0];
  int v53 = (v52 == 1);
  int v54 = vars[1];
  int v55 = (v54 == 2);
  int v56_conj = v53 & v55;
  int v57_conj = v51 & v56_conj;
  int v58_conj = v50 & v57_conj;
  int v59_disj = v49_conj | v58_conj;
  int v60_disj = v40_conj | v59_disj;
  int v61_disj = v31_conj | v60_disj;
  int v62_disj = v22_conj | v61_disj;
  int v63_disj = v13_conj | v62_disj;
  if (v63_disj == 1) assert(0);
  return 0;
}
