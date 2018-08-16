/*
 *  HUFFMAN CODING
 *  @leba39
 */

//I N C L U D E s
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

//C O N S T A N T s   &   D E F I N E s
const int BUFF_SIZE = 16;

//S T R U C T s
struct map_char{
    unsigned char ascii;
    unsigned int freq;
    struct map_char* next;
};


//F U N C T I O N s
FILE* open_file(char* path, char* mode);
int  read_file(FILE* fp, struct map_char** map, unsigned char* size);
void add_map(struct map_char** head, unsigned char new_data);
void check_map(struct map_char** head, unsigned char data, unsigned char* size);
void print_map(struct map_char* head, unsigned char size);
void free_map(struct map_char** head);

int main(){


    //VARs
    FILE* fp                = NULL;
    struct map_char* list   = NULL;
    unsigned char n_items   = 0;

    //OPEN
    if((fp = open_file("file.txt","r"))==NULL){
        fprintf(stdout,"Error opening file.\n");
        return -1;
    }

    //PROCESS
    if(read_file(fp,&list,&n_items)!=0){
        fprintf(stdout,"Error reading and mapping file.\n");
        return -1;
    }

    //DEBUG
    print_map(list,n_items);

    //FREE
    free_map(&list);
    
    return 0;
}

FILE* open_file(char* path, char* mode){

    //VARs
    FILE* fp = NULL;
    
    //OPEN
    fp = fopen(path,mode);
    if(!fp){
        perror("Error in open_file");
    }else{
        fprintf(stdout,"File opened!\n");
    }
    return fp;
}

int read_file(FILE* fp, struct map_char** map, unsigned char* size){

    if(!fp||!map||!size)  return -1;

    //VARs
    int nbytes, exit = 0;
    char buffer[BUFF_SIZE];   
        //need to find opt value for buffer_size to reduce system calls

    //TELL
    fseek(fp,0L,SEEK_END);
    nbytes = ftell(fp);
    fprintf(stdout,"File -> No. bytes: %d\n",nbytes);
    rewind(fp);

    //BUFFERED STREAM
    fprintf(stdout,"File -> Reading file using buffer of: %d (bytes)\n",BUFF_SIZE);
    memset(buffer,'\0',sizeof(char)*BUFF_SIZE);
    while((nbytes=fread(buffer,sizeof(unsigned char),BUFF_SIZE,fp)!=0)){

        for(int i=0; i<BUFF_SIZE; i++){
            if(buffer[i]!='\0'){
                
                check_map(map, buffer[i], size);
                char s[3];
                if(buffer[i]==10){
                    s[0] = 'L';
                    s[1] = 'F';
                    s[2] = '\0';
                }else{
                    s[0] = buffer[i];
                    s[1] = '\0';
                    s[2] = '\0';
                }
                fprintf(stdout,"Letra: %5s\tDec: %5u\tHex:%5x\n",s,buffer[i],buffer[i]);
            }else{
                break;
            }    
        }
        memset(buffer,'\0',sizeof(unsigned char)*BUFF_SIZE);
        exit++;
    }

    //STOPPED READING
    if(feof(fp)!=0){
        fprintf(stdout,"File -> File read successfully in %d passes.\n",exit);
    }else{
        fprintf(stdout,"File -> Couldnt process file.\n");
        if(ferror(fp)!=0)   fprintf(stderr,"read_file ferror.\n");
    }
    //CLOSING FILE POINTER.
    if(fclose(fp) == 0){
        fprintf(stdout,"File -> File closed.\n");
    }else{
        //sets errno.
        perror("closing file in read_file");
        return -2;
    }
    
    return 0;
}

void add_map(struct map_char** head, unsigned char new_data){

    if(!head)   return;
    //ALLOCATE NODE
    struct map_char* new = (struct map_char*)malloc(sizeof(struct map_char));
    new->ascii           = new_data;
    new->freq            = 1;
    if(!(*head)){   
        //empty list
        new->next        = NULL;
    }else{
        new->next        = *head;
    }
    *head = new;
    
    return;
}

void check_map(struct map_char** head, unsigned char data, unsigned char* size){

    if(!head)   return; //NULL
    
    //VARs
    bool found = false;
    unsigned char len = 0;
    //LOOP
    for(struct map_char* node=*head; node!=NULL; node=node->next){
        len++;
        if(data==node->ascii){
            node->freq++;
            found = true;
            break;
        }
    }
    if(!found)  add_map(head,data);
    *size = len;

    return;
}

void print_map(struct map_char* head, unsigned char size){
    
    if(!head){
        fprintf(stdout,"Empty map!\n");
        return;
    }

    fprintf(stdout,"%10s\t|%10s\t|%10s\n","ASCII","FREQ","HEX");
    for(struct map_char* node=head; node!=NULL; node=node->next){
        fprintf(stdout,"%10d\t|%10d\t|%10x\n",node->ascii,node->freq,node->ascii);
    }
    fprintf(stdout,"\tNo. different elements:  %10d\n",size);    

    return;
}

void free_map(struct map_char** head){

    if(!head||!(*head))  return;

    //one element.
    if(!(*head)->next)  goto LAST_NODE;

    //multiple elements
    struct map_char* pointer=*head;
    do{
        *head = (*head)->next;
        free(pointer);
        pointer = *head;
    }while(pointer->next != NULL);
    
    LAST_NODE:
    //last one
    free(*head);
    *head = NULL;

    return;
}

