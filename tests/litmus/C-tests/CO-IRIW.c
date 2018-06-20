/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[1]; 
atomic_int atom_1_r0_2; 
atomic_int atom_1_r1_2; 
atomic_int atom_3_r0_2; 
atomic_int atom_3_r1_2; 
atomic_int atom_3_r1_1; 
atomic_int atom_3_r0_1; 
atomic_int atom_3_r0_0; 
atomic_int atom_3_r1_0; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r0_1; 
atomic_int atom_1_r0_0; 
atomic_int atom_1_r1_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r0 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v4_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v164 = (v2_r0 == 2);
  atomic_store_explicit(&atom_1_r0_2, v164, memory_order_seq_cst);
  int v165 = (v4_r1 == 2);
  atomic_store_explicit(&atom_1_r1_2, v165, memory_order_seq_cst);
  int v172 = (v4_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v172, memory_order_seq_cst);
  int v173 = (v2_r0 == 1);
  atomic_store_explicit(&atom_1_r0_1, v173, memory_order_seq_cst);
  int v174 = (v2_r0 == 0);
  atomic_store_explicit(&atom_1_r0_0, v174, memory_order_seq_cst);
  int v175 = (v4_r1 == 0);
  atomic_store_explicit(&atom_1_r1_0, v175, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v6_r0 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v8_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v166 = (v6_r0 == 2);
  atomic_store_explicit(&atom_3_r0_2, v166, memory_order_seq_cst);
  int v167 = (v8_r1 == 2);
  atomic_store_explicit(&atom_3_r1_2, v167, memory_order_seq_cst);
  int v168 = (v8_r1 == 1);
  atomic_store_explicit(&atom_3_r1_1, v168, memory_order_seq_cst);
  int v169 = (v6_r0 == 1);
  atomic_store_explicit(&atom_3_r0_1, v169, memory_order_seq_cst);
  int v170 = (v6_r0 == 0);
  atomic_store_explicit(&atom_3_r0_0, v170, memory_order_seq_cst);
  int v171 = (v8_r1 == 0);
  atomic_store_explicit(&atom_3_r1_0, v171, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r0_2, 0);
  atomic_init(&atom_1_r1_2, 0);
  atomic_init(&atom_3_r0_2, 0);
  atomic_init(&atom_3_r1_2, 0);
  atomic_init(&atom_3_r1_1, 0);
  atomic_init(&atom_3_r0_1, 0);
  atomic_init(&atom_3_r0_0, 0);
  atomic_init(&atom_3_r1_0, 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r0_1, 0);
  atomic_init(&atom_1_r0_0, 0);
  atomic_init(&atom_1_r1_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v9 = atomic_load_explicit(&atom_1_r0_2, memory_order_seq_cst);
  int v10 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v11 = atomic_load_explicit(&atom_3_r0_2, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v14_disj = v12 | v13;
  int v15_conj = v11 & v14_disj;
  int v16 = atomic_load_explicit(&atom_3_r0_1, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v19_disj = v17 | v18;
  int v20_conj = v16 & v19_disj;
  int v21 = atomic_load_explicit(&atom_3_r0_0, memory_order_seq_cst);
  int v22 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v23 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v24 = atomic_load_explicit(&atom_3_r1_0, memory_order_seq_cst);
  int v25_disj = v23 | v24;
  int v26_disj = v22 | v25_disj;
  int v27_conj = v21 & v26_disj;
  int v28_disj = v20_conj | v27_conj;
  int v29_disj = v15_conj | v28_disj;
  int v30_conj = v10 & v29_disj;
  int v31 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v32 = atomic_load_explicit(&atom_3_r0_2, memory_order_seq_cst);
  int v33 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v34 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v35_disj = v33 | v34;
  int v36_conj = v32 & v35_disj;
  int v37 = atomic_load_explicit(&atom_3_r0_1, memory_order_seq_cst);
  int v38 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v39_conj = v37 & v38;
  int v40 = atomic_load_explicit(&atom_3_r0_0, memory_order_seq_cst);
  int v41 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v42 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v43 = atomic_load_explicit(&atom_3_r1_0, memory_order_seq_cst);
  int v44_disj = v42 | v43;
  int v45_disj = v41 | v44_disj;
  int v46_conj = v40 & v45_disj;
  int v47_disj = v39_conj | v46_conj;
  int v48_disj = v36_conj | v47_disj;
  int v49_conj = v31 & v48_disj;
  int v50_disj = v30_conj | v49_conj;
  int v51_conj = v9 & v50_disj;
  int v52 = atomic_load_explicit(&atom_1_r0_1, memory_order_seq_cst);
  int v53 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v54 = atomic_load_explicit(&atom_3_r0_2, memory_order_seq_cst);
  int v55 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v56_conj = v54 & v55;
  int v57 = atomic_load_explicit(&atom_3_r0_1, memory_order_seq_cst);
  int v58 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v59 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v60_disj = v58 | v59;
  int v61_conj = v57 & v60_disj;
  int v62 = atomic_load_explicit(&atom_3_r0_0, memory_order_seq_cst);
  int v63 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v64 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v65 = atomic_load_explicit(&atom_3_r1_0, memory_order_seq_cst);
  int v66_disj = v64 | v65;
  int v67_disj = v63 | v66_disj;
  int v68_conj = v62 & v67_disj;
  int v69_disj = v61_conj | v68_conj;
  int v70_disj = v56_conj | v69_disj;
  int v71_conj = v53 & v70_disj;
  int v72 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v73 = atomic_load_explicit(&atom_3_r0_2, memory_order_seq_cst);
  int v74 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v75 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v76_disj = v74 | v75;
  int v77_conj = v73 & v76_disj;
  int v78 = atomic_load_explicit(&atom_3_r0_1, memory_order_seq_cst);
  int v79 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v80 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v81_disj = v79 | v80;
  int v82_conj = v78 & v81_disj;
  int v83 = atomic_load_explicit(&atom_3_r0_0, memory_order_seq_cst);
  int v84 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v85 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v86 = atomic_load_explicit(&atom_3_r1_0, memory_order_seq_cst);
  int v87_disj = v85 | v86;
  int v88_disj = v84 | v87_disj;
  int v89_conj = v83 & v88_disj;
  int v90_disj = v82_conj | v89_conj;
  int v91_disj = v77_conj | v90_disj;
  int v92_conj = v72 & v91_disj;
  int v93_disj = v71_conj | v92_conj;
  int v94_conj = v52 & v93_disj;
  int v95 = atomic_load_explicit(&atom_1_r0_0, memory_order_seq_cst);
  int v96 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v97 = atomic_load_explicit(&atom_3_r0_2, memory_order_seq_cst);
  int v98 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v99 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v100_disj = v98 | v99;
  int v101_conj = v97 & v100_disj;
  int v102 = atomic_load_explicit(&atom_3_r0_1, memory_order_seq_cst);
  int v103 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v104 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v105_disj = v103 | v104;
  int v106_conj = v102 & v105_disj;
  int v107 = atomic_load_explicit(&atom_3_r0_0, memory_order_seq_cst);
  int v108 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v109 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v110 = atomic_load_explicit(&atom_3_r1_0, memory_order_seq_cst);
  int v111_disj = v109 | v110;
  int v112_disj = v108 | v111_disj;
  int v113_conj = v107 & v112_disj;
  int v114_disj = v106_conj | v113_conj;
  int v115_disj = v101_conj | v114_disj;
  int v116_conj = v96 & v115_disj;
  int v117 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v118 = atomic_load_explicit(&atom_3_r0_2, memory_order_seq_cst);
  int v119 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v120 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v121_disj = v119 | v120;
  int v122_conj = v118 & v121_disj;
  int v123 = atomic_load_explicit(&atom_3_r0_1, memory_order_seq_cst);
  int v124 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v125 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v126_disj = v124 | v125;
  int v127_conj = v123 & v126_disj;
  int v128 = atomic_load_explicit(&atom_3_r0_0, memory_order_seq_cst);
  int v129 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v130 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v131 = atomic_load_explicit(&atom_3_r1_0, memory_order_seq_cst);
  int v132_disj = v130 | v131;
  int v133_disj = v129 | v132_disj;
  int v134_conj = v128 & v133_disj;
  int v135_disj = v127_conj | v134_conj;
  int v136_disj = v122_conj | v135_disj;
  int v137_conj = v117 & v136_disj;
  int v138 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v139 = atomic_load_explicit(&atom_3_r0_2, memory_order_seq_cst);
  int v140 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v141 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v142_disj = v140 | v141;
  int v143_conj = v139 & v142_disj;
  int v144 = atomic_load_explicit(&atom_3_r0_1, memory_order_seq_cst);
  int v145 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v146 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v147_disj = v145 | v146;
  int v148_conj = v144 & v147_disj;
  int v149 = atomic_load_explicit(&atom_3_r0_0, memory_order_seq_cst);
  int v150 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v151 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v152 = atomic_load_explicit(&atom_3_r1_0, memory_order_seq_cst);
  int v153_disj = v151 | v152;
  int v154_disj = v150 | v153_disj;
  int v155_conj = v149 & v154_disj;
  int v156_disj = v148_conj | v155_conj;
  int v157_disj = v143_conj | v156_disj;
  int v158_conj = v138 & v157_disj;
  int v159_disj = v137_conj | v158_conj;
  int v160_disj = v116_conj | v159_disj;
  int v161_conj = v95 & v160_disj;
  int v162_disj = v94_conj | v161_conj;
  int v163_disj = v51_conj | v162_disj;
  if (v163_disj == 0) assert(0);
  return 0;
}
