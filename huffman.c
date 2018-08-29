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
#include <assert.h>

//C O N S T A N T s   &   D E F I N E s
#define CODE 9              //8+nullbyte
#define ID_LEN 2            //magic string len
const int BUFF_SIZE = 16;   //buffered reader

//S T R U C T s
struct bst_node{
    unsigned char ascii;
    unsigned int freq;
    struct bst_node* left;
    struct bst_node* right;
};

struct map_char{
    unsigned char ascii;
    unsigned int freq;
    struct map_char* next;
    struct bst_node* node;
};

struct map_prefix{
    unsigned char ascii;
    unsigned char prefix_dec;    
    unsigned char prefix_len;
    char prefix[CODE];
    struct map_prefix* next;
};

//F U N C T I O N s
FILE* open_file(char* path, char* mode);
int read_file(FILE* fp, struct map_char** map, unsigned long* size);
int list_len(struct map_char* head);
int write_id(FILE* fp);
int write_header(FILE* fp, struct map_prefix* list, unsigned char* list_len, unsigned long* n_bytes);
unsigned int calc_freq(struct bst_node* node);
unsigned char str_to_dec(char str[], unsigned char len);
void add_map(struct map_char** head, unsigned char new_data, unsigned int freq, struct bst_node* node);
void check_map(struct map_char** head, unsigned char data);
void print_map(struct map_char* head, unsigned char size);
void free_map(struct map_char** head);
void delete_map(struct map_char** head, struct map_char* node);
void build_tree(struct bst_node** root, struct map_char** head);
void cp_val(struct bst_node* tree_node, struct map_char* list_item, bool leaf);
void print_tree(struct bst_node* bst, int indent);
void free_tree(struct bst_node* root);
void add_prefix(unsigned char ascii, char* prefix, struct map_prefix** head);
void build_prefixes(struct bst_node* root, struct map_prefix** list, char prefix[]);
void free_prefix(struct map_prefix** head);
void print_prefix(struct map_prefix* head);

int main(){


    //VARs
    FILE* fp_in             = NULL;
    FILE* fp_out            = NULL;
    struct map_char* list   = NULL;
    struct bst_node* root   = NULL;
    struct map_prefix* code = NULL;
    unsigned char n_items   = 0;
    unsigned char n_prefix  = 0;
    unsigned long n_bytes   = 0;
    char code_tmp[CODE]     = {'\0'};   //same as memset-ing it later


    //OPEN
    if((fp_in = open_file("file.txt","r"))==NULL){
        fprintf(stdout,"Error opening file.\n");
        return -1;
    }

    //PROCESS
    if(read_file(fp_in,&list,&n_bytes)!=0){
        fprintf(stdout,"Error reading and mapping file.\n");
        return -1;
    }
    n_items = list_len(list);

    //DEBUG
    fprintf(stdout,"List -> Frequencies sorted.\n");
    print_map(list,n_items);

    //BINARY TREE
    build_tree(&root,&list);
    assert(list_len(list)==1);  //should be one. this way free_map doesn't need a multiple-item list casuistic althought we have it covered    

    //DEBUG
    fprintf(stdout,"BST -> Printing tree:\n");
    print_tree(root,1);

    //HUFFMAN PREFIX
    fprintf(stdout,"Prefix -> Building prefixes...\n");
    build_prefixes(root, &code, code_tmp);

    //ASSERT
    for(struct map_prefix* pointer=code; pointer!=NULL; pointer = pointer->next)    n_prefix++;
    assert(n_prefix==n_items);

    //DEBUG 
    print_prefix(code);

    //OUTPUT
    fp_in = NULL;          //input already closed
    if((fp_out = open_file("compressed.bin","w"))==NULL){
        fprintf(stdout,"Error opening output file.\n");
        return -1;
    }   

    //SIGN FILE
    if((write_id(fp_out)!=0)){
        fprintf(stdout,"Error writing file identifiers.\n");
        if(fclose(fp_out)!=0){
            perror("Error closing output file in write_id");
        }
        return -1;      
    }  
    
    //WRITE HEADER INFO. NO BYTES AND PREFIX TABLE
    if((write_header(fp_out, code, &n_prefix, &n_bytes)!=0)){
        fprintf(stdout,"Error writing file headers.\n");
        return -1;
    }

    //OPEN INPUT FOR 2ND PASS
    if((fp_in = open_file("file.txt","r"))==NULL){
        fprintf(stdout,"Error opening file.\n");
        return -1;
    }

    //COMPRESS
    


    //TODO: acordarse de cerrar el archivos
    //TODO: cambiar return codes en caso de error para el main y cambiar stdouts por stderrs donde hiciese falta
    
    //FREE
    free_tree(root);    //this order is important, so that when we go into free_map the node pointed by our last item (list) would be already 
    free_map(&list);    //freed by this function (free_tree). thats why we were getting an invalid freed showing up in our valgrind tests.
    free_prefix(&code);


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
        fprintf(stdout,"File -> %s opened.\n",(strcmp(mode,"r")==0) ? "(input)":"(output)");
    }
    return fp;
}

int read_file(FILE* fp, struct map_char** map, unsigned long* size){

    if(!fp||!map||!size)  return -1;

    //VARs
    int nbytes, exit = 0;
    char buffer[BUFF_SIZE];   
        //need to find opt value for buffer_size to reduce system calls

    //TELL
    fseek(fp,0L,SEEK_END);
    *size = ftell(fp);
    fprintf(stdout,"File -> No. bytes: %ld\n",*size);
    rewind(fp);

    //BUFFERED STREAM
    fprintf(stdout,"File -> Reading file using buffer of: %d (bytes)\n",BUFF_SIZE);
    memset(buffer,'\0',sizeof(char)*BUFF_SIZE);
    while((nbytes=fread(buffer,sizeof(unsigned char),BUFF_SIZE,fp)!=0)){

        for(int i=0; i<BUFF_SIZE; i++){
            if(buffer[i]!='\0'){
                
                check_map(map, buffer[i]);
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

void add_map(struct map_char** head, unsigned char new_data, unsigned int freq, struct bst_node* node){

    //adds at the front of the list
    if(!head)   return;
    
    //VARs
    int i;
    bool end = true;
    struct map_char *new, *pointer;

    //ALLOCATE NODE
    new = (struct map_char*)malloc(sizeof(struct map_char));
    new->ascii  = new_data;
    new->freq   = freq;
    new->node   = node;

    if(!(*head)){   
        //empty list
        new->next   = NULL;
        *head       = new;
        return;
    }
    if(!(*head)->next){
        //one element
        if((*head)->freq <= freq){
            new->next       = NULL;
            (*head)->next   = new;        
        }else{
            new->next       = *head;
            *head           = new;
        }
        return;
    }
        //multiple elements. scan.
    i = 0;
    pointer = *head;
    while(pointer->next != NULL){
        if(pointer->freq > freq){
            end = false;
            break;
        }
        pointer = pointer->next;
        i++;
    }

    if(end && pointer->freq <= freq){
        //insert as last node
        pointer->next = new;
        new->next   = NULL;        
    }else{
        
        new->next   = pointer;  //next element
        pointer     = *head;    //reset
        for(int j=0; j<(i-1); j++)  pointer = pointer->next;

        if(i!=0){               //previous element
            pointer->next   = new;
        }else{
            *head           = new;        
        }
    }

    return;
}

void check_map(struct map_char** head, unsigned char data){

    if(!head)   return; //NULL
    
    //VARs
    bool found = false;
    unsigned char len = 0;

    //LOOP
    for(struct map_char* node=*head; node!=NULL; node=node->next){
        len++;
        if(data==node->ascii){
            unsigned int freq_tmp = node->freq;
            delete_map(head,node);
            add_map(head,data,++freq_tmp,NULL); 
            found = true;
            break;
        }
    }
    if(!found)  add_map(head,data,1,NULL);   //fresh node. freq=1.

    return;
}

void print_map(struct map_char* head, unsigned char size){
    
    if(!head){
        fprintf(stdout,"Empty map!\n");
        return;
    }
    fprintf(stdout,"List -> Printing table:\n");
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
        free((*head)->node);           
        (*head)->node = NULL;
        free(pointer);
        pointer = *head;
    }while(pointer->next != NULL);
    
    LAST_NODE:
    //last one
        /*unnecessary since we already freed node in the free_tree function. we need to call free_tree then free_map...
         otherwise valgrind would show an invalid free warning, although it isnt a proper error and no leaks would be found*/
    //free((*head)->node);              
    //(*head)->node = NULL;
    free(*head);
    *head = NULL;

    return;
}

void delete_map(struct map_char** head, struct map_char* node){

    if(!head||!(*head)||!node)  return;
   
    //VARs
    int i;
    bool found                  = false;
    struct map_char *pointer    = *head;
    

    if(!pointer->next && pointer==node){
        //one element
        free(pointer->node);
        pointer->node = NULL;
        free(pointer);
        *head = NULL;
        return;
    }  
    
        //multiple elements. scan.
    i = 0;
    while(pointer->next != NULL){
        if(pointer == node){
            found = true;
            break;
        }
        pointer = pointer->next;
        i++;
    }

    if(found && i==0){
        //first element
        *head = pointer->next;
        free(pointer->node);
        pointer->node = NULL;
        free(pointer);
        pointer = NULL;
        return;
    }

    if(found){
        //in between
        struct map_char *prev = *head;
        for(int j=0; j<(i-1); j++)  prev = prev->next;
        prev->next = prev->next->next;
        free(pointer->node);
        pointer->node = NULL;
        free(pointer);
        pointer = NULL;
        return;
    }

    if(!found && pointer==node){
        //last element.
        free(pointer->node);
        pointer->node = NULL;
        free(pointer);
        pointer = *head;
        for(int j=0; j<(i-1); j++)  pointer = pointer->next;
        pointer->next = NULL;
    }
    return;
}

int list_len(struct map_char* head){

    if(!head)   return -1;
        
    //VAR
    int len = 0;
    for(struct map_char* node=head; node!=NULL; node=node->next)   len++;
    return len;
}

void build_tree(struct bst_node** root, struct map_char** head){

    if(!root||!head||!(*head)) return;

    //VARs
    struct bst_node *parent, *min_node1, *min_node2;
    int len;

    len = list_len(*head);
 
    fprintf(stdout,"BST -> Building Huffman tree...\n");   
    if(len==-1)    return;
    if(len==1){
        //ALLOC
        parent          = (struct bst_node*)malloc(sizeof(struct bst_node));
        min_node1       = (struct bst_node*)malloc(sizeof(struct bst_node));
   
        cp_val(min_node1, *head, true);
      
        parent->ascii   = 0;        //serves as flag. parent node.
        parent->left    = min_node1;//could be right, doesnt matter
        parent->right   = NULL;
        parent->freq    = calc_freq(parent);

    }else if(len>1){
    
        //VAR
        int freq1, freq2;
        struct map_char *min_1, *min_2;
        //build tree
        do{
            //next two from sorted list
            min_1 = *head;
            min_2 = (*head)->next;

            //ALLOC
            parent      = (struct bst_node*)malloc(sizeof(struct bst_node));
            min_node1   = (struct bst_node*)malloc(sizeof(struct bst_node));
            min_node2   = (struct bst_node*)malloc(sizeof(struct bst_node));
           
            cp_val(min_node1,min_1, (min_1->ascii!=0));
            cp_val(min_node2,min_2, (min_2->ascii!=0));
            freq1 = calc_freq(min_node1);
            freq2 = calc_freq(min_node2);

            parent->ascii   = 0;        //serves as flag. parent node.
            if(freq1 < freq2){
                parent->left    = min_node1;
                parent->right   = min_node2;
            }else{
                parent->left    = min_node2;
                parent->right   = min_node1;
            }    
            parent->freq        = freq1+freq2;

            delete_map(head, min_1);
            delete_map(head, min_2);
            add_map(head, parent->ascii, parent->freq, parent);
        }while(list_len(*head)!=1);
    }

    *root = parent;
    return;
}

void cp_val(struct bst_node* tree_node, struct map_char* list_item, bool leaf){

    if(!tree_node||!list_item)    return;
    tree_node->ascii = list_item->ascii;
    tree_node->freq  = list_item->freq;
    if(leaf){
        tree_node->right = NULL;
        tree_node->left  = NULL;
    }else{
        tree_node->right = list_item->node->right;
        tree_node->left  = list_item->node->left;
    }   
    return;
}

unsigned int calc_freq(struct bst_node* node){
    
    if(!node)   return 0;
    
    if(!(node->left)&&!(node->right)){
        return node->freq;
    }else{
        return (calc_freq(node->left)+calc_freq(node->right));
    } 
}


void print_tree(struct bst_node* bst, int indent){
    
    if(!bst){
        fprintf(stdout, "Empty tree.\n");
        return;
    }
    for(int i=0; i<indent; i++)     fprintf(stdout, "\t");
    if(bst->ascii==0){
        fprintf(stdout, "freq:%u (NODE)\n", bst->freq);
    }else{
        fprintf(stdout, "freq:%u dec:%d (LEAF)\n", bst->freq, bst->ascii);
    }
    if(bst->left)       print_tree(bst->left,  indent+1);
    if(bst->right)      print_tree(bst->right, indent+1);
}

void free_tree(struct bst_node* root){

    if(!root)   return;

    //recurse down left subtrees
    free_tree(root->left);
    root->left = NULL;
    //recurse down right subtrees
    free_tree(root->right);
    root->right = NULL;
    //free node
    free(root);
    root = NULL;

    return;
}

void add_prefix(unsigned char ascii, char* prefix, struct map_prefix** head){

    if(!head)   return;
    
    //ALLOC
    struct map_prefix* new = (struct map_prefix*)malloc(sizeof(struct map_prefix));
    new->ascii  = ascii;
    strcpy(new->prefix, prefix);
    new->prefix_len = strlen(new->prefix);
    new->prefix_dec = str_to_dec(new->prefix, new->prefix_len);
    new->next   = *head;
    *head       = new;
    
    return;
}

void build_prefixes(struct bst_node* root, struct map_prefix** list, char prefix[]){

    if(!root||!list)    return;
    
    //VARs
    const char* move_left   = "0";
    const char* move_right  = "1";
    int index;
    
    if(!root->left&&!root->right){
        //leaf
        add_prefix(root->ascii, prefix, list);
    }else{
        //append and recurse down left
        strcat(prefix,move_left);
        build_prefixes(root->left,list,prefix);

        //clean
        index           = strlen(prefix);
        prefix[index-1] = '\0';           

        //append and recurse down right
        strcat(prefix,move_right);
        build_prefixes(root->right,list,prefix);

        //clean
        index           = strlen(prefix);
        prefix[index-1] = '\0';     
    }
    
    return;
}

void free_prefix(struct map_prefix** head){

    if(!head||!(*head))  return;

    //one element.
    if(!(*head)->next)  goto LAST_NODE;

    //multiple elements
    struct map_prefix* pointer=*head;
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

unsigned char str_to_dec(char str[], unsigned char len){

    if(!str||len==0)    return 0;

    unsigned char pos = 0;
    unsigned char dec = 0;
    for(int i=len; i>0; i--){
        dec += (str[i-1]=='1') ? 1<<pos : 0;
        pos++;
    }
    
    return dec;
}

void print_prefix(struct map_prefix* head){
    
    if(!head){
        fprintf(stdout,"Empty map_prefix!\n");
        return;
    }
    fprintf(stdout,"Prefix -> Printing table:\n");
    fprintf(stdout,"%10s\t|%10s\t|%10s\t|%10s\t|%10s\n","ASCII_DEC","PREFIX","PREFIX_DEC","PREFIX_HEX","PREFIX_LEN");
    for(struct map_prefix* node=head; node!=NULL; node=node->next){
        fprintf(stdout,"%10d\t|%10s\t|%10d\t|%10x\t|%10d\n",node->ascii,node->prefix,node->prefix_dec,node->prefix_dec,node->prefix_len);
    }

    return;
}

int write_id(FILE* fp){

    if(!fp) return -1;
    
    //VARs
    const unsigned char ID[ID_LEN] = {244,245};  //ids

    fprintf(stdout,"File -> (output) writing identifiers...\n");
    if(fwrite(ID,sizeof(unsigned char),ID_LEN,fp)!=ID_LEN){ //should be equal to ID_len because sizeof(unsigned char) is 1 byte
        return -1;
    }
    return 0;
}

int write_header(FILE* fp, struct map_prefix* list, unsigned char* list_len, unsigned long* n_bytes){

    if(!fp) return -1;
    
    //VAR
    unsigned char count = 0;


    fprintf(stdout,"File -> (output) writing headers...\n");
    //file should be already open with cursor at the end after writing identifiers
    if(ftell(fp)!=ID_LEN){
        if(fseek(fp, 0L, SEEK_END)!=0){
            perror("Error seeking end of output file in write_header.");
            return -1;
        }
    }

    //original file no bytes
    if(fwrite(n_bytes,sizeof(unsigned long),1,fp)!=1){ 
        return -1;
    }

    //prefix table
    if(fwrite(list_len,sizeof(unsigned char),1,fp)!=1){
        return -1;
    }

    for(struct map_prefix* pointer=list; pointer!=NULL; pointer=pointer->next){

        //ascii (dec), prefix (dec) and prefix len
        if(fwrite(&(pointer->ascii),sizeof(unsigned char),1,fp)!=1)         return -1;
        if(fwrite(&(pointer->prefix_dec),sizeof(unsigned char),1,fp)!=1)    return -1;
        if(fwrite(&(pointer->prefix_len),sizeof(unsigned char),1,fp)!=1)    return -1;
        count++;
    }
    assert(*list_len==count);   //just to reassure procedure

    return 0;
}
