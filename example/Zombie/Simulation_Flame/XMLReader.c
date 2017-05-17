#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>


int reader(char *docname){

	int num_human=0;
	int num_zombie=0;
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(docname);

	if (doc == NULL ) {
		fprintf(stderr,"Document not parsed successfully. \n");
		exit(-1);
	}

	cur = xmlDocGetRootElement(doc);

	if (cur == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		exit(-1);
	}

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if (!(xmlStrcmp(cur->name, (const xmlChar *)"xagent"))){
			xmlChar *key;
			xmlNodePtr cur1 = cur->xmlChildrenNode;
			while (cur1 != NULL) {
			    if ((!xmlStrcmp(cur1->name, (const xmlChar *)"type"))) {
		            if((!xmlStrcmp(cur1->xmlChildrenNode->content,(const xmlChar *)"0")))
				        	num_human++;
		            else
		              num_zombie++;

		 	    }
			cur1 = cur1->next;
			}
		}
	  cur = cur->next;
	}

	xmlFreeDoc(doc);
	return num_human;
}

int main(int argc,char *argv[]){
	printf("human_count %d", reader(argv[1]));
	exit(0);
}
