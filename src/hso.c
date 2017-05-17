#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <zmq.h>

#include "libs/zhelpers.h"

#define VERBOSE 1
#define VERBOSE_MASTER 1
#define VERBOSE_THREAD 0

typedef struct cpu_thread_data {
	char *params;
	char *model;
	char *result;
} cpu_thread_data;

typedef struct gpu_thread_data {
	char *params;
	char *model;
	int gpu_id;
	char *result;
} gpu_thread_data;

char** split(char *string, char *del,int * size){
	char **str_res=(char**)malloc(1);
	char *token = strtok(string,del);
	int count=0;
	while (token!=NULL) {
		str_res = realloc (str_res, sizeof (char*) * ++count);
		str_res[count-1] = token;
		token = strtok(NULL,del);
	}
	*size=count;
	return str_res;
}

void *gpufunction(void *arg){
	char line[5000];
	char *command;
	gpu_thread_data *mtdata=(gpu_thread_data *)arg;
	strcpy(mtdata->result,"");
	if(strcmp(mtdata->params,"null")!=0){
		int command_size=((7+strlen(mtdata->model)+strlen(mtdata->params)*sizeof(char))+sizeof(int));
		command = (char *) realloc(command,command_size);
		sprintf(command,"./%s \"%s\" %d",mtdata->model,mtdata->params,mtdata->gpu_id);
		if(VERBOSE_THREAD){
			printf("Run gpu simulation: %s\n",command);
		}
		/* Open the command for reading. */
		FILE *fd = popen(command, "r");
		if (fd == NULL) {
			printf("Failed to run simulation: %s\n",command);
			exit(1);
		}
		/* Read the output a line at a time - output it. */
		while (fgets(line, sizeof(line), fd) != NULL) {
		}
		pclose(fd);

		mtdata->result = (char*) realloc (mtdata->result,(strlen(line)+1)*sizeof(char));
		strcpy(mtdata->result,line);
	}else{
		mtdata->result =(char*) realloc(mtdata->result,5*sizeof(char));
		strcpy(mtdata->result,"null");
	}
	pthread_exit(NULL);
}

void *cpufunction (void *arg){
	char line[5000];
	char *command;
	cpu_thread_data *mtdata=(cpu_thread_data *)arg;
	strcpy(mtdata->result,"");
	if(strcmp(mtdata->params,"null")!=0){
		int command_size=(6+strlen(mtdata->model)+strlen(mtdata->params))*sizeof(char);
		command = (char *) realloc(command,command_size);
		sprintf(command,"./%s \"%s\"",mtdata->model,mtdata->params);
		if (VERBOSE_THREAD) {
			printf("Run cpu simulation: %s\n",command);
		}
		FILE *fd = popen(command,"r");
		if (fd == NULL) {
			printf("Failed to run simulation: %s\n",command);
			exit(1);
		}
		/* Read the output a line at a time - output it. */
		while (fgets(line, sizeof(line), fd) != NULL) {
		}
		int len = strlen(line);
		if(line[len-1]=='\n')
			line[len-1]='\0';
		pclose(fd);
		mtdata->result = (char*) realloc (mtdata->result,(strlen(line)+1)*sizeof(char));
		strcpy(mtdata->result,line);
	}else{
		mtdata->result = (char*) realloc (mtdata->result,5*sizeof(char));
		strcpy(mtdata->result,"null");
	}
	pthread_exit(NULL);
}

int numGpus(){
	char result[100];
	FILE *fd = popen("lspci | grep NVIDIA |wc -l", "r");
	if(fd == NULL){
		printf("Failed to run \"lspci | grep NVIDIA |wc -l\"\n");
		exit(1);
	}
	fgets(result, sizeof(result), fd);
	pclose(fd);
	return atoi(result);
}

int numCpus(){
	FILE *fd = popen("lscpu | grep Socket", "r");
	if (fd == NULL) {
		printf("Failed to run \"lscpu | grep Socket\"\n" );
		exit(1);
	}
	char tmp[30];
	fgets(tmp, sizeof(tmp), fd);
	pclose(fd);
	int size;
	char ** res = split(tmp,":",&size);
	int nSock = atoi(res[1]);
	return nSock;
}

char* getAddress(){
	int n;
    	struct ifreq ifr;
    	char array[] = "eth0";
 
	n = socket(AF_INET, SOCK_DGRAM, 0);
    	//Type of address to retrieve - IPv4 IP address
    	ifr.ifr_addr.sa_family = AF_INET;
    	//Copy the interface name in the ifreq structure
    	strncpy(ifr.ifr_name , array , IFNAMSIZ - 1);
    	ioctl(n, SIOCGIFADDR, &ifr);
    	close(n);
	return inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr);
}

void *context,*evalQ,*outQ;

int main(int argc, char *argv[]){
	float balance_factor=0.5;
	int numtasks,rank;
	MPI_Status stat;

	//initialize MPI
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	char *buff,*cpu_model,*gpu_model;
	int cpu_model_size,gpu_model_size;
	int ncpu[numtasks], ngpu[numtasks],rcpu[numtasks],rgpu[numtasks];
	int number[2];
	if(rank==0){ // master
		ncpu[0]=numCpus();
		ngpu[0]=numGpus();
		for (int i=1;i<numtasks;i++){
			MPI_Recv(&number,2,MPI_INT, i ,0,MPI_COMM_WORLD,&stat);
			ncpu[i]=number[0];
			ngpu[i]=number[1];
		}
		if (VERBOSE_MASTER) {
			printf("MASTER: Discovery risorse effettuata:\n");
			for(int i =0; i<numtasks;i++){
				printf("CPU rank %d = %d -- GPU rank %d =%d\n",i,ncpu[i],i,ngpu[i]);
			}
		}
		char *ipaddr=getAddress();
		char *path_eval=(char *) malloc((strlen(ipaddr)+12)*sizeof(char));
		sprintf(path_eval,"tcp://%s:5555");
		if (VERBOSE_MASTER) {
			printf("MASTER: path della coda di ascolto %s\n",path_eval);
		}
		char *path_out;
		context= zmq_ctx_new();
		evalQ = zmq_socket(context,ZMQ_PAIR);
		int rc = zmq_bind(evalQ,path_eval);
		if(rc==-1){
			printf("MASTER: error zeroMQ EVAL\n");
			exit(1);
		}
		char **evalQstr;
		int split_size=0;
		char have_cpu, have_gpu;
		while(1){ //receive the messages by the algorithm optimization
			buff=s_recv(evalQ);
			int size=atoi(buff);
			buff=(char*) malloc ((size+1)*sizeof(char));
			zmq_recv(evalQ,buff,size,0);
			buff[size]='\0'; //aggiungere carattere di terminazione alla stringa
			if(buff==NULL){
				printf("MASTER: error in receiving the request\n");
				exit(1);
			}
			if (VERBOSE) {
				printf("MASTER: riceuto %s\n",buff);
			}
			evalQstr = split(buff,"::",&split_size);
			if(strcmp(evalQstr[0],"INIT_OUT")==0){
				int size = strlen(evalQstr[1])+strlen(evalQstr[2])+strlen(evalQstr[3])+3;
				path_out=(char *) malloc(size*sizeof(char));
				strcpy(path_out,evalQstr[1]);
				strcat(path_out,":");
				strcat(path_out,evalQstr[2]);
				strcat(path_out,":");
				strcat(path_out,evalQstr[3]);
				if (VERBOSE_MASTER) {
					printf("MASTER: path della coda di output %s\n",path_out );
				}
				outQ = zmq_socket(context,ZMQ_PAIR);
				int rc2 = zmq_connect(outQ,path_out);
				if(rc2==-1){
					printf("MASTER: Error zeroMQ OUTPUT\n");
					exit(1);
				}
				continue;
			}else if(strcmp(evalQstr[0],"INIT_CPU")==0){
				cpu_model_size=strlen(evalQstr[1]);
				cpu_model = (char*) malloc((cpu_model_size+1)*sizeof(char));
				strcpy(cpu_model,evalQstr[1]);
				buff=s_recv(evalQ);
				size=atoi(buff);
				buff=(char*) malloc ((size+1)*sizeof(char));
				zmq_recv(evalQ,buff,size,0);
				buff[size]='\0'; //aggiungere carattere di terminazione alla stringa
				if(buff==NULL){
					printf("MASTER: error in receiving the request\n");
					exit(1);
				}

				evalQstr = split(buff,"::",&split_size);
				gpu_model_size=strlen(evalQstr[1]);
				gpu_model = (char*) malloc((gpu_model_size+1)*sizeof(char));
				strcpy(gpu_model,evalQstr[1]);
				char *models = (char*) malloc((cpu_model_size+gpu_model_size+14)*sizeof(char));
				sprintf(models,"::MODEL::%s::%s::",cpu_model,gpu_model);
				if (VERBOSE) {
					printf("MASTER: aggiornati i modelli delle simulazioni\nmodello CPU: %s\nmodello GPU: %s\n ",cpu_model,gpu_model);
				}
				int models_size =strlen(models)+1;
				for (int i=1;i<numtasks;i++){ //send the paht of model to slave
					MPI_Send(&models_size,1,MPI_INT,i,0,MPI_COMM_WORLD);
					MPI_Send(models,models_size,MPI_CHAR,i,0,MPI_COMM_WORLD);
				}
				if (VERBOSE) {
					printf("MASTER: propagazione modelli agli slave effettuata\n");
				}
				continue;
			} else {
				int n_params = atoi(evalQstr[2]);
				char **list_params = split(evalQstr[1],";",&split_size);
				int simgpu,simcpu;
				if(have_cpu && !have_gpu){
					simcpu=n_params;
					simgpu=0;
				}
				if(!have_cpu && have_gpu){
					simgpu=n_params;
					simcpu=0;
				}
				if(have_cpu && have_gpu){
					simcpu=n_params*balance_factor;
					simgpu = n_params-simcpu;
				}
				if (VERBOSE) {
					printf("MASTER: # simulazioni CPU: %d, # simulazioni GPU: %d\n",simcpu,simgpu);
				}
				char *params_split[numtasks];
				for (int i=0;i<numtasks;i++){
					params_split[i]= (char*) malloc(sizeof(char));
					strcpy(params_split[i],"");
				}

				int count = 0;
				while (count<n_params) { //finchè ci sono ancora parametri da asseggnare
					for(int i = 0;i<numtasks;i++){ //sorta di round robin sui task disponibili
						params_split[i]=(char*) realloc(params_split[i],(strlen(params_split[i])+6)*sizeof(char));
						strcat(params_split[i],":CPU:");
						for(int j=0;j<ncpu[i];j++){
							if(simcpu>0 && count<n_params){ //se ho ancora dei parametri di simulazioni da testare su cpu
								params_split[i]=(char*)realloc(params_split[i],(strlen(params_split[i])+strlen(list_params[count])+1)*sizeof(char));
								strcat(params_split[i],list_params[count]);
								params_split[i]=(char*)realloc(params_split[i],(strlen(params_split[i])+2)*sizeof(char));
								strcat(params_split[i],";");
								count++;
								simcpu--;
							}else{
								params_split[i]=(char*)realloc(params_split[i],(strlen(params_split[i])+5)*sizeof(char));
								strcat(params_split[i],"null");
								params_split[i]=(char*)realloc(params_split[i],(strlen(params_split[i])+2)*sizeof(char));
								strcat(params_split[i],";");
							}
						}
						params_split[i]=(char*)realloc(params_split[i],(strlen(params_split[i])+6)*sizeof(char));
						strcat(params_split[i],":GPU:");
						for(int j=0;j<ngpu[i];j++){
							if(simgpu>0 && count<n_params){ //se ho ancora dei parametri di simulazioni da testare su gpu
								params_split[i]=(char*) realloc(params_split[i],(strlen(params_split[i])+strlen(list_params[count])+1)*sizeof(char));
								strcat(params_split[i],list_params[count]);
								params_split[i]=(char*) realloc(params_split[i],(strlen(params_split[i])+2)*sizeof(char));
								strcat(params_split[i],";");
								count++;
								simgpu--;
							}else{
								params_split[i]=(char*) realloc(params_split[i],(strlen(params_split[i])+5)*sizeof(char));
								strcat(params_split[i],"null");
								params_split[i]=(char*) realloc(params_split[i],(strlen(params_split[i])+2)*sizeof(char));
								strcat(params_split[i],";");
							}
						}
						params_split[i]=(char*) realloc(params_split[i],(strlen(params_split[i])+3)*sizeof(char));
						strcat(params_split[i],":-");
					}
				}
				if (VERBOSE) {
					printf("MASTER:\n");
					for (int i = 0; i < numtasks; i++) {
						printf("parametri simulazioni rank %d: %s\n",i,params_split[i]);
					}
				}
				for(int i=1;i<numtasks;i++){ // invia i parametri delle simulazioni agli slave
					int size = strlen(params_split[i])+1;
					MPI_Send(&size,1,MPI_INT,i,0,MPI_COMM_WORLD);
					MPI_Send(params_split[i],size,MPI_CHAR,i,0,MPI_COMM_WORLD);
				}
				if(VERBOSE){
					printf("MASTER: parametri inviati agli slave\n");
				}
				char *list_output; //stringa da inviare su outQ
				list_output = (char*) malloc(9*sizeof(char));
				strcpy(list_output,"OUTPUT::");

				/*INIZIO COMPUTAZIONE LOCALE*/
				if (VERBOSE) {
						printf("MASTER: inizio computazione locale\n");
				}
				char ** iteration_param = split(params_split[0],"-",&split_size);
				for(int i = 0;i<split_size;i++){
					int temp_size;
					char** temp_split = split(iteration_param[i],":",&temp_size);
					int num_param_cpu;
					char** cpu_params = split(temp_split[1],";",&num_param_cpu);
					int num_param_gpu;
					char** gpu_params = split(temp_split[3],";",&num_param_gpu);
					pthread_t cpu_thread[num_param_cpu];
					pthread_t gpu_thread[num_param_gpu];
					cpu_thread_data data_cpu[num_param_cpu];
					gpu_thread_data data_gpu[num_param_gpu];
					for(int j=0;j<num_param_cpu;j++){
						data_cpu[j].params = (char*)malloc((strlen(cpu_params[j])+1)*sizeof(char));
						strcpy(data_cpu[j].params, cpu_params[j]);
						data_cpu[j].model = (char*)malloc((strlen(cpu_model)+1)*sizeof(char));
						strcpy(data_cpu[j].model , cpu_model);
						data_cpu[j].result = (char*)malloc(sizeof(char));
					}
					for(int j=0;j<num_param_gpu;j++){
						data_gpu[j].params = (char*)malloc((strlen(gpu_params[j])+1)*sizeof(char));
						strcpy(data_gpu[j].params, gpu_params[j]);
						data_gpu[j].model = (char*)malloc((strlen(gpu_model)+1)*sizeof(char));
						strcpy(data_gpu[j].model , gpu_model);
						data_gpu[j].gpu_id=j;
						data_gpu[j].result=(char*) malloc(sizeof(char));
					}
					for(int j=0;j<num_param_cpu;j++){
						pthread_create(&cpu_thread[j],NULL,cpufunction,(void*) &data_cpu[j]);
					}
					for(int j=0;j<num_param_gpu;j++){
						pthread_create(&gpu_thread[j],NULL,gpufunction,(void*) &data_gpu[j]);
					}
					for(int j=0;j<num_param_cpu;j++){
						pthread_join(cpu_thread[j],NULL);
					}

					for(int j=0;j<num_param_gpu;j++){
						pthread_join(gpu_thread[j],NULL);
					}
					for(int j=0;j<num_param_cpu;j++){
						if(strcmp(data_cpu[j].result,"null")!=0){
							int tmp_size;
							char **tmp = split(data_cpu[j].result," ",&tmp_size);
							list_output=(char*) realloc(list_output,(strlen(list_output)+strlen(tmp[1])+1)*sizeof(char));
							strcat(list_output,tmp[1]);
							list_output=(char*) realloc(list_output,(strlen(list_output)+2)*sizeof(char));
							strcat(list_output,";");
						}
					}
					for(int j=0;j<num_param_gpu;j++){
						if(strcmp(data_gpu[j].result,"null")!=0){
							int tmp_size;
							char **tmp =split(data_gpu[j].result," ",&tmp_size);
							list_output=(char*) realloc(list_output,(strlen(list_output)+strlen(tmp[1])+1)*sizeof(char));
							strcat(list_output,tmp[1]);
							list_output=(char*) realloc(list_output,(strlen(list_output)+2)*sizeof(char));
							strcat(list_output,";");
						}
					}

				}
				/*FINE COMPUTAZIONE LOCALNE*/
				if (VERBOSE) {
					printf("MASTER: fine computazione locale\n");
					printf("MASTER: output locale: %s\n",list_output);
					printf("MASTER: raccolta output degli slave\n");
				}
				for(int i=1;i<numtasks;i++){
					int size=0;
					MPI_Recv(&size,1,MPI_INT,i,0,MPI_COMM_WORLD,&stat);
					char * result=(char*)malloc(size*sizeof(char));
					MPI_Recv(result,size,MPI_CHAR,i,0,MPI_COMM_WORLD,&stat);
					list_output=(char*) realloc(list_output,(strlen(list_output)+strlen(result)+1)*sizeof(char));
					strcat(list_output,result);
				}
				int size=strlen(list_output);
				if(list_output[size-1]==';'){
					list_output[size-1]='\0';
				}
				list_output=(char*) realloc(list_output,(strlen(list_output)+3)*sizeof(char));
				strcat(list_output,"::");
				if (VERBOSE) {
					printf("MASTER output globale: %s\n",list_output );
				}
				buff=(char*) malloc (sizeof(int));
				size=strlen(list_output);
				sprintf(buff,"%d",size);
				zmq_send(outQ,buff,sizeof(int),0);
				zmq_send(outQ,list_output,strlen(list_output),0);
			}
			free(buff);
		}

	}else{ //slave
		number[0]=numCpus();
		number[1]=numGpus();
		MPI_Send(&number,2,MPI_INT,0,0,MPI_COMM_WORLD);
		//retrieve the parameter for simulations
		while(1){
			char *output;
			output = (char *) malloc(sizeof(char));
			strcpy(output,"");
			int size;
			MPI_Recv(&size,1,MPI_INT,0,0,MPI_COMM_WORLD,&stat);
			char *listparams=(char*)malloc((size)*sizeof(char));
			MPI_Recv(listparams,size,MPI_CHAR,0,0,MPI_COMM_WORLD,&stat);
			if(strstr(listparams,"MODEL")!=NULL){
				int models_size;
				char** models=split(listparams,"::",&models_size);
				cpu_model_size = strlen(models[1]);
				cpu_model=(char*)malloc((cpu_model_size+1)*sizeof(char));
				strcpy(cpu_model,models[1]);
				gpu_model_size = strlen(models[2]);
				gpu_model = (char*) malloc((gpu_model_size+1)*sizeof(char));
				strcpy(gpu_model,models[2]);
				if (VERBOSE) {
					printf("RANK %d: ho ricevuto i modelli %s - %s\n",rank,cpu_model,gpu_model);
				}
				continue;
			}
			int split_size;
			char ** iteration_param = split(listparams,"-",&split_size);

			for(int i = 0;i<split_size;i++){
				int temp_size;
				char** temp_split = split(iteration_param[i],":",&temp_size);
				int num_param_cpu;
				char** cpu_params = split(temp_split[1],";",&num_param_cpu);
				int num_param_gpu;
				char** gpu_params = split(temp_split[3],";",&num_param_gpu);
				pthread_t cpu_thread[num_param_cpu];
				pthread_t gpu_thread[num_param_gpu];
				cpu_thread_data data_cpu[num_param_cpu];
				gpu_thread_data data_gpu[num_param_gpu];
				for(int j=0;j<num_param_cpu;j++){
					data_cpu[j].params = (char*)malloc((strlen(cpu_params[j])+1)*sizeof(char));
					strcpy(data_cpu[j].params,cpu_params[j]);
					data_cpu[j].model = (char*)malloc((strlen(cpu_model)+1)*sizeof(char));
					strcpy(data_cpu[j].model , cpu_model);
					data_cpu[j].result=(char*) malloc(sizeof(char));
				}
				for(int j=0;j<num_param_gpu;j++){
					data_gpu[j].params = (char*)malloc((strlen(gpu_params[j])+1)*sizeof(char));
					strcpy(data_gpu[j].params ,gpu_params[j]);
					data_gpu[j].model = (char*)malloc((strlen(gpu_model)+1)*sizeof(char));
					strcpy(data_gpu[j].model , gpu_model);
					data_gpu[j].gpu_id=j;
					data_gpu[j].result=(char*)malloc(sizeof(int));
				}
				for(int j=0;j<num_param_cpu;j++){
					pthread_create(&cpu_thread[j],NULL,cpufunction,(void*) &data_cpu[j]);
				}
				for(int j=0;j<num_param_gpu;j++){
					pthread_create(&gpu_thread[j],NULL,gpufunction,(void*) &data_gpu[j]);
				}

				for(int j=0;j<num_param_cpu;j++){
					pthread_join(cpu_thread[j],NULL);
				}
				for(int j=0;j<num_param_gpu;j++){
					pthread_join(gpu_thread[j],NULL);
				}

				for(int j=0;j<num_param_cpu;j++){
					if(strcmp(data_cpu[j].result,"null")!=0){
						int tmp_size;
						char **tmp = split(data_cpu[j].result," ",&tmp_size);
						output = (char*) realloc(output,(strlen(output)+strlen(tmp[1])+1)*sizeof(char));
						strcat(output,tmp[1]);
						output = (char*) realloc(output,(strlen(output)+2)*sizeof(char));
						strcat(output,";");
					}
				}
				for(int j=0;j<num_param_gpu;j++){
					if(strcmp(data_gpu[j].result,"null")!=0){
						int tmp_size;
						char **tmp =split(data_gpu[j].result," ",&tmp_size);
						output = (char*) realloc(output,(strlen(output)+strlen(tmp[1])+1)*sizeof(char));
						strcat(output,tmp[1]);
						output = (char*) realloc(output,(strlen(output)+2)*sizeof(char));
						strcat(output,";");
					}
				}
			}
			int res_size = strlen(output)+1;
			MPI_Send(&res_size,1,MPI_INT,0,0,MPI_COMM_WORLD);
			MPI_Send(output,res_size,MPI_CHAR,0,0,MPI_COMM_WORLD);
		}
	}
	MPI_Finalize();
	exit(0);
}
