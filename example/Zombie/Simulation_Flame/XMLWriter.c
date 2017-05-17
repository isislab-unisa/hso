#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

char** split(char *string, char *del){
  char **str_res;
  char* token = strtok(string,del);

  int count=0;
   while (token!=NULL) {
     str_res = realloc (str_res, sizeof (char*) * ++count);
     str_res[count-1]=token;
     token = strtok(NULL,del);
   }
   return str_res;
}

static float random_float(){
    const float min=-1;
    const float max=1;
    return (max-min)*((float) rand()/RAND_MAX)+ min;
}

int writer(char* filepath,int human, int zombie, char *h_step, char *z_step){

    xmlDocPtr doc=NULL;
    xmlNodePtr root_node=NULL;
    xmlNodePtr agent_node=NULL;
    char buff[256];
    int i,j;

    LIBXML_TEST_VERSION;

    doc = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, BAD_CAST "states");
    xmlDocSetRootElement(doc,root_node);
    xmlNewChild(root_node,NULL,BAD_CAST "itno",BAD_CAST "0");
    xmlNewChild(root_node,NULL,BAD_CAST "environment",NULL);
    srand(time(NULL));
    int count=0;
    for (i=0;i<(human+zombie);i++){
            sprintf(buff,"%d",count);
            agent_node = xmlNewNode(NULL, BAD_CAST "xagent");
            xmlNewChild(agent_node,NULL,BAD_CAST "name",BAD_CAST "Agent");
            xmlNewChild(agent_node,NULL,BAD_CAST "id",BAD_CAST buff);
            sprintf(buff,"%f",random_float());
            xmlNewChild(agent_node,NULL,BAD_CAST "x",BAD_CAST buff);
            sprintf(buff,"%f",random_float());
            xmlNewChild(agent_node,NULL,BAD_CAST "y",BAD_CAST buff);
            //sprintf(buff,"%f",random_float());
            xmlNewChild(agent_node,NULL,BAD_CAST "z",BAD_CAST "0");
            sprintf(buff,"%f",random_float());
            xmlNewChild(agent_node,NULL,BAD_CAST "fx",BAD_CAST buff);
            sprintf(buff,"%f",random_float());
            xmlNewChild(agent_node,NULL,BAD_CAST "fy",BAD_CAST buff);
            //sprintf(buff,"%f",random_float());
            xmlNewChild(agent_node,NULL,BAD_CAST "fz",BAD_CAST "0");
            if(i<human){
                    xmlNewChild(agent_node,NULL,BAD_CAST "type",BAD_CAST "0");
                    xmlNewChild(agent_node,NULL,BAD_CAST "step_size", BAD_CAST h_step);
                }else{
                    xmlNewChild(agent_node,NULL,BAD_CAST "type",BAD_CAST "1");
                    xmlNewChild(agent_node,NULL,BAD_CAST "step_size",BAD_CAST z_step);
                    }
            xmlAddChild(root_node,agent_node);
            count++;
        }


    xmlSaveFormatFileEnc(filepath,doc,"UTF-8",1);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    xmlMemoryDump();
    return 0;
}


int main(int argc, char *argv[]){
		char *filepath = (char*)malloc((strlen(argv[1])+1)*sizeof(char));
		strcpy(filepath,argv[1]);
		char *input=(char*)malloc((strlen(argv[2])+1)*sizeof(char));
		strcpy(input,argv[2]);
    		char **str=split(input,",");
		writer(filepath,atoi(str[0]),atoi(str[1]),str[2],str[3]);
	}
