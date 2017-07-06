#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <igraph.h>
#include <cuda.h>
#include <time.h>

__global__ void actv(int *s, int *v, int *a, int *a_index,int *adj,int *d_result,int num_v,int i){
	int index = threadIdx.x + blockIdx.x * blockDim.x;
	if(index<num_v){
		if(i==0){
			if( s[index] >= v[index]){
			d_result[index]=1;
			}
		}else{
			if(a[index]==1){
				d_result[index]=1;
			}else{
				int sv = s[index];
				int j = a_index[index];
				while(adj[j]!=-1){
					if (a[adj[j]]==1){
						sv++;
					}
					j++;
				}
				if(sv>=v[index]){
					d_result[index]=1;
				}
			}
		}
		
	}
}

char** split(char *string, char *del){
	char **str_res=(char**)malloc(1);
	char *token = strtok(string,del);
	int count=0;
	while (token!=NULL) {
		str_res = (char**) realloc (str_res, sizeof (char*) * ++count);
		str_res[count-1] = token;
		token = strtok(NULL,del);
	}
	return str_res;
}

int main(int argc, char *argv[])
{

	if(argc <3){
		printf("Error: missing input parameter\n run: ./gpu_spread <graph_filepath> <influence sequence> <T>");
		exit(1);
	}
	/* code */
	igraph_t graph;
	FILE *fd;
	igraph_vs_t vs,vs_all;
	igraph_vit_t vit,vit_all;

	char **inf_seq, **trs_seq;
	int T, num_v,num_e;
	//igraph_integer_t max_degree;
	//igraph_matrix_t adjm;

	fd = fopen(argv[1],"r");

	igraph_read_graph_ncol(&graph,fd,NULL,1,IGRAPH_ADD_WEIGHTS_NO,IGRAPH_UNDIRECTED);
	//igraph_to_undirected(&graph, IGRAPH_TO_UNDIRECTED_COLLAPSE,0);
	fclose(fd);

	inf_seq = split(argv[2],",");
	//trs_seq = split(argv[3],",");
	T=atoi(argv[3]);

	num_v =igraph_vcount(&graph);
	num_e =igraph_ecount(&graph);

	int adj_index[num_v];
	int *adj=(int*) malloc(sizeof(int));

	int size=0;
	int index=0;
	igraph_vs_all(&vs_all);
	igraph_vit_create(&graph,vs_all,&vit_all);
	while (!IGRAPH_VIT_END(vit_all)) {
		adj_index[index]=size;
		igraph_vs_adj(&vs,IGRAPH_VIT_GET(vit_all),IGRAPH_ALL);
		igraph_vit_create(&graph,vs,&vit);
		while (!IGRAPH_VIT_END(vit)) {
			adj=(int*) realloc(adj,(size+1)*sizeof(int));
			adj[size]=IGRAPH_VIT_GET(vit);
			size++;
			IGRAPH_VIT_NEXT(vit);
		}
		adj=(int*) realloc(adj,(size+1)*sizeof(int));
		adj[size]=-1;
		size++;
		igraph_vit_destroy(&vit);
		igraph_vs_destroy(&vs);
		index++;
		IGRAPH_VIT_NEXT(vit_all);
	}
	igraph_vit_destroy(&vit_all);
	igraph_vs_destroy(&vs_all);


	int inf_v[num_v]; // vertex's influence values
	int trs_v[num_v]; // vertex's threshold values
	int act_v[num_v]; // vertex's activation values
	int res[num_v];

	int *d_inf_v;
	int *d_trs_v;
	int *d_act_v;
	int *d_index;
	int *d_adjlist;
	int *d_result;

	for (int i = 0; i < num_v; i++)
	{
		inf_v[i]=0;
		trs_v[i]=0;
		act_v[i]=0;
		res[i]=0;
	}

	srand(time(NULL));
	
	//char **inf_seq_split = split(inf_seq,",");
	for (int i = 0; i < num_v; ++i)
	{
		inf_v[i]=atoi(inf_seq[i]);
	}
	/*
	for (int i = 0; i < num_v; ++i)
	{
		trs_v[i]=atoi(trs_seq[i]);
	}*/

	for (int i = 0; i < T; i++)
	{
		int r = rand()%num_v+0;
		igraph_vs_adj(&vs,r,IGRAPH_ALL);
		igraph_vit_create(&graph,vs,&vit);
		if(trs_v[r]<IGRAPH_VIT_SIZE(vit)){
			trs_v[r]++;
		}
		igraph_vit_destroy(&vit);
		igraph_vs_destroy(&vs);

	}		
	

	//printf("numero di vertici %d e numero di archi %d\n",num_v,num_e);

	/* CALCULATE A0*/

	cudaMalloc((void **) &d_inf_v,num_v*sizeof(int));
	cudaMalloc((void **) &d_trs_v,num_v*sizeof(int));
	cudaMalloc((void **) &d_act_v,num_v*sizeof(int));
	cudaMalloc((void **) &d_index,num_v*sizeof(int));
	cudaMalloc((void **) &d_adjlist,size*sizeof(int));
	cudaMalloc((void **) &d_result,num_v*sizeof(int));

	cudaMemcpy(d_inf_v,inf_v,num_v*sizeof(int),cudaMemcpyHostToDevice);
	cudaMemcpy(d_trs_v,trs_v,num_v*sizeof(int),cudaMemcpyHostToDevice);
	cudaMemcpy(d_act_v,act_v,num_v*sizeof(int),cudaMemcpyHostToDevice);
	cudaMemcpy(d_result,res,num_v*sizeof(int),cudaMemcpyHostToDevice);
	cudaMemcpy(d_index,adj_index,index*sizeof(int),cudaMemcpyHostToDevice);
	cudaMemcpy(d_adjlist,adj,size*sizeof(int),cudaMemcpyHostToDevice);


	int thread = 8;
	int block;
	if(num_v%thread==0){
		block = (num_v/thread);
	}else{
		block = (num_v/thread)+1;
	}


	int num_act_v=0;

	actv<<<block,thread>>>(d_inf_v,d_trs_v,d_act_v,d_index,d_adjlist,d_result,num_v,0);
	cudaMemcpy(res,d_result,num_v*sizeof(int),cudaMemcpyDeviceToHost);

	for(int i=0;i<num_v;i++){
		if(res[i]==1){
			act_v[i]=1;
			num_act_v++;
		}
	}

	//printf("A0: active_node %d\n",num_act_v);

	int num_act_v_prec=0;
	int count = 1;
	while(num_act_v_prec!=num_act_v){
		num_act_v_prec = num_act_v;
		
		num_act_v=0;

		cudaMemcpy(d_act_v,act_v,num_v*sizeof(int),cudaMemcpyHostToDevice);
		cudaMemcpy(d_result,res,num_v*sizeof(int),cudaMemcpyHostToDevice);

		actv<<<block,thread>>>(d_inf_v,d_trs_v,d_act_v,d_index,d_adjlist,d_result,num_v,1);

		cudaMemcpy(res,d_result,num_v*sizeof(int),cudaMemcpyDeviceToHost);

		for (int i = 0; i < num_v; i++)
		{
			if(res[i]==1){
				act_v[i]=1;
				num_act_v++;
			}
		}
		//printf("A%d: active_node %d\n",count,num_act_v);
		count++;
	}
	/*RESULT*/
	printf("final_active_node %d\n",num_act_v );



	cudaFree(d_inf_v); cudaFree(d_trs_v); cudaFree(d_act_v); cudaFree(d_index); cudaFree(d_adjlist);cudaFree(d_result);
	igraph_destroy(&graph);
	return 0;
}