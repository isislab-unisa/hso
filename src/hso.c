#define _GNU_SOURCE
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

#define VERBOSE  0
#define VERBOSE_MASTER 1
#define VERBOSE_THREAD 0

typedef struct cpu_thread_data {
	int index;
	char *params;
	char *model;
	char *result;
} cpu_thread_data;

typedef struct gpu_thread_data {
	int index;
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
		//strcpy(list_output,"");
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
		int len = strlen(line);
		if(line[len-1]=='\n')
			line[len-1]='\0';
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
	FILE *fd = popen("lspci | grep NVIDIA | wc -l", "r");
	if(fd == NULL){
		printf("Failed to run \"nvidia-smi -L | wc -l\"\n");
		exit(1);
	}
	fgets(result, sizeof(result), fd);
	if (atoi(result)!=0){
		FILE *fd2 = popen("nvidia-smi -L | wc -l", "r");
		if(fd2 == NULL){
			printf("Failed to run \"nvidia-smi -L | wc -l\"\n");
			exit(1);
		}	
		fgets(result, sizeof(result), fd);
		pclose(fd2);	
		return atoi(result);
	}
	pclose(fd);
	return 0;
}

int numCpus(){
	FILE *fd = popen("grep -i \"physical id\" /proc/cpuinfo | sort -u | wc -l", "r");
	if (fd == NULL) {
		printf("Failed to run grep -i \"physical id\" /proc/cpuinfo | sort -u | wc -l\n" );
		exit(1);
	}
	char tmp[30];
	fgets(tmp, sizeof(tmp), fd);
	pclose(fd);
	int size;
	return atoi(tmp);
}

void *context,*evalQ,*outQ;

int main(int argc, char *argv[]){
	int numtasks,rank;
	MPI_Status stat;
	//initialize MPI
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	char *buff,*cpu_model,*gpu_model;
	int cpu_model_size,gpu_model_size;
	int ncpu[numtasks], ngpu[numtasks],rcpu[numtasks],rgpu[numtasks],num_cpu,num_gpu;
	int number[2];
	if(rank==0){ // master
		ncpu[0]=0;
		ngpu[0]=0;
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
		int fd;
		struct ifreq ifr;
		fd = socket(AF_INET, SOCK_DGRAM, 0);
		/* I want to get an IPv4 IP address */
 		ifr.ifr_addr.sa_family = AF_INET;
 		/* I want IP address attached to "eth0" */
 		strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
		ioctl(fd, SIOCGIFADDR, &ifr);
		close(fd);
		char* path_eval= (char*)malloc(26*sizeof(char));
		sprintf(path_eval,"tcp://%s:5555",inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
		
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
			buff[size]='\0'; 
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
			}else if(strcmp(evalQstr[0],"INIT_NUM")==0){
				int t_cpu = atoi(evalQstr[1]);
				int t_gpu = atoi(evalQstr[2]);
				num_cpu = t_cpu;
				num_gpu = t_gpu;
				
				for(int i=0;i<numtasks;i++){
					rcpu[i]=0;
					rgpu[i]=0;
				}
				have_cpu=1;
				have_gpu=1;
				if(t_cpu==0){
					have_cpu=0;
				}
				if(t_gpu==0){
					have_gpu=0;
				}
				while(t_cpu>0 || t_gpu>0){
				    for(int i=0;i<numtasks;i++){
				        if(t_cpu>0 && rcpu[i]<ncpu[i]){
				            rcpu[i]++;
				            t_cpu--;
				        }
				        if (t_gpu>0 && rgpu[i]<ngpu[i]){
				            rgpu[i]++;
				            t_gpu--;
				        }
				    }
				}
				if(VERBOSE_MASTER){
						printf("MASTER: aggiornate risorse disponibili\n");
				}
				continue;
			}
			else if(strcmp(evalQstr[0],"INIT_CPU")==0){
				cpu_model_size=strlen(evalQstr[1]);
				cpu_model = (char*) malloc((cpu_model_size+1)*sizeof(char));
				strcpy(cpu_model,evalQstr[1]);
				buff=s_recv(evalQ);
				size=atoi(buff);
				buff=(char*) malloc ((size+1)*sizeof(char));
				zmq_recv(evalQ,buff,size,0);
				buff[size]='\0';
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
				if (VERBOSE_MASTER) {
					printf("MASTER: aggiornati i modelli delle simulazioni\nmodello CPU: %s\nmodello GPU: %s\n ",cpu_model,gpu_model);
				}
				int models_size =strlen(models)+1;
				for (int i=1;i<numtasks;i++){ //send the paht of model to slave
					MPI_Send(&models_size,1,MPI_INT,i,0,MPI_COMM_WORLD);
					MPI_Send(models,models_size,MPI_CHAR,i,0,MPI_COMM_WORLD);
				}
				if (VERBOSE_MASTER) {
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
					printf("num_cpu %d num_gpu %d\n",num_cpu,num_gpu);
					int tmp = n_params /(num_cpu+num_gpu);
					simcpu=num_cpu*tmp;
					simgpu = n_params-simcpu;
				}
				if (VERBOSE_MASTER) {
					printf("MASTER: # simulazioni CPU: %d, # simulazioni GPU: %d\n",simcpu,simgpu);
				}
				char *params_split[numtasks];
				for (int i=0;i<numtasks;i++){
					params_split[i]= (char*) malloc(sizeof(char));
					strcpy(params_split[i],"");
				}

				char *tmp_cpu_params[numtasks];
				char *tmp_gpu_params[numtasks];

				int a_taskcpu[numtasks];
				for (int i=0;i<numtasks;i++){
					a_taskcpu[i]=0;
				}
				int a_taskgpu[numtasks];
				for (int i=0;i<numtasks;i++){
					a_taskgpu[i]=0;
				}

				for(int i=0;i<numtasks;i++){
					tmp_cpu_params[i]=(char*)malloc(sizeof(char));
					strcpy(tmp_cpu_params[i],"");
				}
				
				for(int i=0;i<numtasks;i++){
					tmp_gpu_params[i]=(char*)malloc(sizeof(char));
					strcpy(tmp_gpu_params[i],"");
				}

				int count = 0;
				while (count<n_params) { 
					for(int i=1;i<numtasks;i++){
						if(a_taskcpu[i] == ncpu[i] && a_taskgpu[i] == ngpu[i]){
							params_split[i] = (char*) realloc(params_split[i],(strlen(params_split[i])+6)*sizeof(char));
							strcat(params_split[i],":CPU:");
							if(strcmp(tmp_cpu_params[i],"")==0){
								params_split[i]=(char*) realloc(params_split[i],(strlen(params_split[i])+6)*sizeof(char));
								strcat(params_split[i],"none;");
							}else{
								params_split[i] = (char*) realloc(params_split[i],(strlen(params_split[i])+strlen(tmp_cpu_params[i])+1)*sizeof(char));
								strcat(params_split[i],tmp_cpu_params[i]);
							}
							params_split[i] = (char*) realloc(params_split[i],(strlen(params_split[i])+6)*sizeof(char));
							strcat(params_split[i],":GPU:");
							if(strcmp(tmp_gpu_params[i],"")==0){
								params_split[i]= (char*) realloc(params_split[i],(strlen(params_split[i])+6)*sizeof(char));
								strcat(params_split[i],"none;");
							}else{
								params_split[i] = (char*) realloc(params_split[i],(strlen(params_split[i])+strlen(tmp_gpu_params[i])+1)*sizeof(char));
								strcat(params_split[i],tmp_gpu_params[i]);
							}
							params_split[i] = (char*) realloc(params_split[i],(strlen(params_split[i])+3)*sizeof(char));
							strcat(params_split[i],":-");
							tmp_cpu_params[i]=(char*)malloc(sizeof(char));
							tmp_gpu_params[i]=(char*)malloc(sizeof(char));
							strcpy(tmp_cpu_params[i],"");
							strcpy(tmp_gpu_params[i],"");
							a_taskcpu[i]=0;
							a_taskgpu[i]=0;
							continue;
						}else{
							if(a_taskcpu[i]<ncpu[i]){
								if(simcpu>0 && count<n_params && a_taskcpu[i]<rcpu[i]){
									char *tmp_value=(char*) malloc((sizeof(int)+2+strlen(list_params[count]))*sizeof(char));
									sprintf(tmp_value,"%d#%s",count,list_params[count]);
									tmp_cpu_params[i]= (char*) realloc(tmp_cpu_params[i],(strlen(tmp_cpu_params[i])+strlen(tmp_value)+1)*sizeof(char));
									strcat(tmp_cpu_params[i],tmp_value);
									tmp_cpu_params[i]= (char*) realloc(tmp_cpu_params[i],(strlen(tmp_cpu_params[i])+2)*sizeof(char));
									strcat(tmp_cpu_params[i],";");
									simcpu--;
									count++;
								}else{
									tmp_cpu_params[i]= (char*) realloc(tmp_cpu_params[i],(strlen(tmp_cpu_params[i])+6)*sizeof(char));
									strcat(tmp_cpu_params[i],"null;");
								}
								a_taskcpu[i]++;
							}
							if(a_taskgpu[i]<ngpu[i]){
								if(simgpu>0 && count<n_params && a_taskgpu[i]<rgpu[i]){
									char *tmp_value=(char*) malloc((sizeof(int)+2+strlen(list_params[count]))*sizeof(char));;
									sprintf(tmp_value,"%d#%s",count,list_params[count]);
									tmp_gpu_params[i]= (char*) realloc(tmp_gpu_params[i],(strlen(tmp_gpu_params[i])+strlen(tmp_value)+1)*sizeof(char));
									strcat(tmp_gpu_params[i],tmp_value);
									tmp_gpu_params[i]= (char*) realloc(tmp_gpu_params[i],(strlen(tmp_gpu_params[i])+2)*sizeof(char));
									strcat(tmp_gpu_params[i],";");
									simgpu--;
									count++;
								}else{
									tmp_gpu_params[i]= (char*) realloc(tmp_gpu_params[i],(strlen(tmp_gpu_params[i])+6)*sizeof(char));
									strcat(tmp_gpu_params[i],"null;");
								}
								a_taskgpu[i]++;
							}
						}
					}
				}

				for(int i=1;i<numtasks;i++){
					if(strlen(tmp_cpu_params[i])>0 || strlen(tmp_gpu_params[i])>0){
						params_split[i]= (char*) realloc(params_split[i],(strlen(params_split[i])+ 6)*sizeof(char));
						strcat(params_split[i],":CPU:");
						int temp;
						char **tmp = split(tmp_cpu_params[i],";",&temp);
						if(ncpu[i]==0){
							params_split[i]= (char*) realloc(params_split[i],(strlen(params_split[i])+6)*sizeof(char));
							strcat(params_split[i],"none;");
						}else{
							for(int j=0;j<ncpu[i];j++){
								if(j<temp){
									params_split[i]= (char*) realloc(params_split[i],(strlen(params_split[i])+strlen(tmp[j])+1)*sizeof(char));
									strcat(params_split[i],tmp[j]);
									params_split[i]= (char*) realloc(params_split[i],(strlen(params_split[i])+2)*sizeof(char));
									strcat(params_split[i],";");
								}else{
									params_split[i]= (char*) realloc(params_split[i],(strlen(params_split[i])+6)*sizeof(char));
									strcat(params_split[i],"null;");
								}
							}
						}
						params_split[i]=(char*) realloc(params_split[i],(strlen(params_split[i])+6)*sizeof(char));
						strcat(params_split[i],":GPU:");
						tmp = split(tmp_gpu_params[i],";",&temp);

						if(ngpu[i]==0){
							params_split[i]= (char*) realloc(params_split[i],(strlen(params_split[i])+6)*sizeof(char));
							strcat(params_split[i],"none;");
						}else{
							for(int j=0;j<ngpu[i];j++){
								if(j<temp){
									params_split[i]= (char*) realloc(params_split[i],(strlen(params_split[i])+strlen(tmp[j])+1)*sizeof(char));
									strcat(params_split[i],tmp[j]);
									params_split[i]= (char*) realloc(params_split[i],(strlen(params_split[i])+2)*sizeof(char));
									strcat(params_split[i],";");
								}else{
									params_split[i]= (char*) realloc(params_split[i],(strlen(params_split[i])+6)*sizeof(char));
									strcat(params_split[i],"null;");
								}
							}

						}
						params_split[i]= (char*) realloc(params_split[i],(strlen(params_split[i])+3)*sizeof(char));
						strcat(params_split[i],":-");
					}
				}
				if (VERBOSE) {
					printf("MASTER:\n");
					for (int i = 0; i < numtasks; i++) {
						printf("parametri simulazioni rank %d: %s\n",i,params_split[i]);
					}
				}
				for(int i=1;i<numtasks;i++){ 
					int size = strlen(params_split[i])+1;
					MPI_Send(&size,1,MPI_INT,i,0,MPI_COMM_WORLD);
					MPI_Send(params_split[i],size,MPI_CHAR,i,0,MPI_COMM_WORLD);
				}
				if(VERBOSE){
					printf("MASTER: parametri inviati agli slave\n");
				}
				char *list_output;
				list_output = (char*) malloc(9*sizeof(char));
				strcpy(list_output,"OUTPUT::");

				char *array_output[n_params];
				char *array_result[n_params];

				for(int i = 0;i<numtasks;i++){
					array_result[i]= (char *) malloc (sizeof(char));
					strcpy(array_result[i],"");
				}
				
				for(int i=1;i<numtasks;i++){
					int size=0;
					MPI_Recv(&size,1,MPI_INT,i,0,MPI_COMM_WORLD,&stat);
					array_result[i]=(char*)malloc(size*sizeof(char));
					MPI_Recv(array_result[i],size,MPI_CHAR,i,0,MPI_COMM_WORLD,&stat);
					if (VERBOSE)
					{
						printf("MASTER ho ricevuto da rank %d: %s\n",i,array_result[i]);

					}
				}

				for(int i=1;i<numtasks;i++){
					int result_split_size;
					char **result_split = split(array_result[i],";",&result_split_size);
					for (int j = 0; j < result_split_size ; j++)
					{
						int tmp_size;
						char **tmp_split = split(result_split[j],"#",&tmp_size);
						int index = atoi(tmp_split[0]);
						array_output[index]=(char*)malloc((strlen(tmp_split[1])+1)*sizeof(char));
						strcpy(array_output[index],tmp_split[1]);
					}
				}
				for (int i = 0; i < n_params; i++)
				{
					list_output = (char*) realloc(list_output,(strlen(list_output)+strlen(array_output[i])+1)*sizeof(char));
					strcat(list_output,array_output[i]);
					list_output = (char*) realloc(list_output,(strlen(list_output)+2)*sizeof(char));
					strcat(list_output,";");
				}

				int size=strlen(list_output);
				if(list_output[size-1]==';'){
					list_output[size-1]='\0';
				}
				list_output=(char*) realloc(list_output,(strlen(list_output)+3)*sizeof(char));
				strcat(list_output,"::");
				printf("%s\n",list_output);
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
		
		number[1]=numGpus();
		if(number[1]!=0){
			number[0]=0;
		}else{
			number[0]=numCpus();
		}
		
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
					if((strcmp(cpu_params[j],"null")!=0) && (strcmp(cpu_params[j],"none")!=0) ){
						int tmp_size;
						char **t_split = split(cpu_params[j],"#",&tmp_size);
						data_cpu[j].index=atoi(t_split[0]);
						data_cpu[j].params = (char*)malloc((strlen(t_split[1])+1)*sizeof(char));
						strcpy(data_cpu[j].params, t_split[1]);
					}else{
						data_cpu[j].index=-1;
						data_cpu[j].params = (char*)malloc((5)*sizeof(char));
						strcpy(data_cpu[j].params, "null");
					}
					
					data_cpu[j].model = (char*)malloc((strlen(cpu_model)+1)*sizeof(char));
					strcpy(data_cpu[j].model , cpu_model);
					data_cpu[j].result=(char*) malloc(sizeof(char));
				}
				for(int j=0;j<num_param_gpu;j++){
					if((strcmp(gpu_params[j],"null")!=0) && (strcmp(gpu_params[j],"none")!=0)){
						int tmp_size;
						char **t_split = split(gpu_params[j],"#",&tmp_size);
						data_gpu[j].index=atoi(t_split[0]);
						data_gpu[j].params = (char*)malloc((strlen(t_split[1])+1)*sizeof(char));
						strcpy(data_gpu[j].params, t_split[1]);
					}else{
						data_gpu[j].index=-1;
						data_gpu[j].params = (char*)malloc((5)*sizeof(char));
						strcpy(data_gpu[j].params, "null");
					}
					
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
						char *tmp_value=(char*) malloc((sizeof(int)+2+strlen(tmp[1]))*sizeof(char));;
						sprintf(tmp_value,"%d#%s",data_cpu[j].index,tmp[1]);
						output = (char*) realloc(output,(strlen(output)+strlen(tmp_value)+1)*sizeof(char));
						strcat(output,tmp_value);
						output = (char*) realloc(output,(strlen(output)+2)*sizeof(char));
						strcat(output,";");
					}
				}
				for(int j=0;j<num_param_gpu;j++){
					if(strcmp(data_gpu[j].result,"null")!=0){
						int tmp_size;
						char **tmp =split(data_gpu[j].result," ",&tmp_size);
						char *tmp_value=(char*) malloc((sizeof(int)+2+strlen(tmp[1]))*sizeof(char));;
						sprintf(tmp_value,"%d#%s",data_gpu[j].index,tmp[1]);
						output = (char*) realloc(output,(strlen(output)+strlen(tmp_value)+1)*sizeof(char));
						strcat(output,tmp_value);
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
