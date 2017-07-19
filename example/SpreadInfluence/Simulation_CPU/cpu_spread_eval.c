#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <igraph.h>
#include <time.h>

char** split(char *string, char *del){
	char **str_res=(char**)malloc(1);
	char *token = strtok(string,del);
	int count=0;
	while (token!=NULL) {
		str_res = realloc (str_res, sizeof (char*) * ++count);
		str_res[count-1] = token;
		token = strtok(NULL,del);
	}
	return str_res;
}

int active (igraph_t graph,int inf_array[], int trs_array[],int act_array[],int res_array[],int num,int n_iter){
	int num_act_v;
	igraph_vs_t vs;
	igraph_vit_t vit;
	igraph_vs_t vs_all;
	igraph_vit_t vit_all;
	if (n_iter == 0){
		igraph_vs_all(&vs_all);
		igraph_vit_create(&graph,vs_all,&vit_all);
		while (!IGRAPH_VIT_END(vit_all)) {
			int s_v = inf_array[IGRAPH_VIT_GET(vit_all)];
			if(s_v>=trs_array[IGRAPH_VIT_GET(vit_all)])
			{
				res_array[IGRAPH_VIT_GET(vit_all)]=1;
			}
			IGRAPH_VIT_NEXT(vit_all);
		}
		igraph_vit_destroy(&vit_all);
		igraph_vs_destroy(&vs_all);
	}else{
		igraph_vs_all(&vs_all);
		igraph_vit_create(&graph,vs_all,&vit_all);
		while(!IGRAPH_VIT_END(vit_all)){
			if(act_array[IGRAPH_VIT_GET(vit_all)]!=1){
				int s_v = inf_array[IGRAPH_VIT_GET(vit_all)];
				igraph_vs_adj(&vs,IGRAPH_VIT_GET(vit_all),IGRAPH_ALL);
				igraph_vit_create(&graph,vs,&vit);
				while(!IGRAPH_VIT_END(vit)){
					if(act_array[IGRAPH_VIT_GET(vit)]==1){
						s_v++;
					}
					IGRAPH_VIT_NEXT(vit);
				}
				igraph_vit_destroy(&vit);
				igraph_vs_destroy(&vs);
				if(s_v >= trs_array[IGRAPH_VIT_GET(vit_all)]){
					res_array[IGRAPH_VIT_GET(vit_all)]=1;
				}
			}
			IGRAPH_VIT_NEXT(vit_all);
		}
		igraph_vit_destroy(&vit_all);
		igraph_vs_destroy(&vs_all);
	}
	num_act_v=0;
	for (int i = 0; i < num; i++)
	{
		if(res_array[i]==1){
			act_array[i]=1;
			num_act_v++;
		}
	}
	return num_act_v;
}

void main(int argc, char *argv[])
{

	if(argc <3){
		printf("Error: missing input parameter\n run: ./spread <graph_filepath> <influence sequence> <T>");
		exit(1);
	}
	/* code */
	igraph_t graph;
	FILE *fd;
	igraph_vs_t vs;
	igraph_vit_t vit;
	igraph_vs_t vs_all;
	igraph_vit_t vit_all;
	char **inf_seq;
	char **thr_seq;
	int T, num_v,num_e,num_act_v,num_act_v_prec;

	fd = fopen(argv[1],"r");

	igraph_read_graph_ncol(&graph,fd,NULL,1,IGRAPH_ADD_WEIGHTS_NO,IGRAPH_UNDIRECTED);

	fclose(fd);

	inf_seq = split(argv[2],",");
	thr_seq = split(argv[3],",");


	num_v =igraph_vcount(&graph);
	num_e =igraph_ecount(&graph);

	int inf_v[num_v]; // vertex's influence values
	int trs_v[num_v]; // vertex's threshold values
	int act_v[num_v]; // vertex's activation values

	for (int i = 0; i < num_v; i++)
	{
		inf_v[i]=0;
		trs_v[i]=0;
		act_v[i]=0;
	}

	srand(time(NULL));


	for (int i = 0; i < num_v; ++i)
	{
		inf_v[i]=atoi(inf_seq[i]);
	}

	for (int i = 0; i < num_v; ++i)
	{
		trs_v[i]=atoi(thr_seq[i]);
	}

	

	/* CALCULATE A0*/

	int * res = (int*)calloc(num_v, sizeof(int));

	num_act_v = active(graph,inf_v,trs_v,act_v,res,num_v,0);
	num_act_v_prec=0;

	/*SPREAD DIFFUSION*/

	while(num_act_v_prec!=num_act_v){
		num_act_v_prec = num_act_v;
		num_act_v = active(graph,inf_v,trs_v,act_v,res,num_v,1);
	}
	/*RESULT*/
	printf("final_active_node %d\n",num_act_v);
	igraph_destroy(&graph);
	exit(0);
}
