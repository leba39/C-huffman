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
#define CACHE_SIZE 2        //cache len in compress
#define BYTE_BITS 8         //bits in a byte
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
int compress(FILE* fp_in, FILE* fp_out, struct map_prefix* root);
int list_len(struct map_char* head);
int write_id(FILE* fp);
int write_header(FILE* fp, struct map_prefix* list, unsigned char* list_len, unsigned long* n_bytes);
int decompress(FILE* fp_in, FILE* fp_out, struct map_prefix** root, unsigned char n_prefix, unsigned long n_bytes_file);
unsigned int calc_freq(struct bst_node* node);
unsigned char str_to_dec(char str[], unsigned char len);
unsigned char decode(unsigned char encoded_byte,struct map_prefix** root,unsigned char* dec_leftover,unsigned char* len_leftover,FILE* fp,unsigned char cache[]);
unsigned long get_filesize(FILE* fp_in);
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
void read_prefixes(FILE* fp, struct map_prefix** code, unsigned char* num);
void add_prefix_table(unsigned char ascii, unsigned char prefix_dec, unsigned char prefix_len, struct map_prefix** head);
struct map_prefix* check_byte(unsigned char byte_dec, unsigned char byte_len, struct map_prefix** root);
struct map_prefix* get_prefix(struct map_prefix* root, unsigned char ascii_dec);
bool fill_cache(unsigned char cache[], struct map_prefix* byte_node);
bool verify_file(FILE* fp_in);

int main(int argc, char** argv){


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
    char *mode              = NULL;
    char *filename_in       = NULL;
    char *filename_out      = NULL;
    bool encode             = false;

    //CLA
    if(argc<3||argc>4){
        fprintf(stdout,"Use -> huffman [-e encode] [-d decode] input_filename output_filename\n");
        return -1;
    }
    mode        = argv[1];
    filename_in = argv[2];
    if(strcmp(mode,"-d")==0){       
        filename_out = (argc==4) ? argv[3] : "original.txt";
    }else if(strcmp(mode,"-e")==0){
        filename_out = (argc==4) ? argv[3] : "compressed.bin";
        encode = true; 
    }else{
        fprintf(stdout,"Use -> huffman [-e encode] [-d decode] input_filename output_filename\n");
        return -1;
    }

    if(encode)  goto ENCODE;
    
//DECODE:

    //OPEN
    if((fp_in = open_file(filename_in,"r"))==NULL){
        fprintf(stdout,"Error opening file.\n");
        return -1;
    }

    //VERIFY
    if(!verify_file(fp_in)){
        fprintf(stdout,"Couldnt identify file. Verify that it was compressed properly using huffman.\n");
        return -1;
    }
    fprintf(stdout,"Verify -> File identified correctly. Proceeding.\n");

    //ORIGINAL FILESIZE
    if((n_bytes=get_filesize(fp_in))==0){
        fprintf(stdout,"Empty or malformed original file. Cant decode further.\n");
        return -1;
    }
    fprintf(stdout,"File -> Uncompressed original filesize: %10ld\n",n_bytes);
   

    //READ HUFFMAN PREFIXES
    read_prefixes(fp_in, &code, &n_prefix);
    
    //ASSERT
    assert(code!=NULL);
    for(struct map_prefix* pointer=code; pointer!=NULL; pointer = pointer->next)    n_items++;
    assert(n_prefix==n_items);

    //DEBUG 
    print_prefix(code);
    fprintf(stdout,"Prefix -> Number of characters encoded: %10d\n",n_prefix);



    //PREPARE OUTPUT
    if((fp_out = open_file(filename_out,"w"))==NULL){
        fprintf(stdout,"Error opening output file.\n");
        return -1;
    }  


    //DECOMPRESS
    if(decompress(fp_in,fp_out,&code,n_prefix,n_bytes)!=0){
        fprintf(stdout,"Error decompressing input file.\n");
        if((fclose(fp_out)!=0)||fclose(fp_in)!=0){
            perror("Error closing output/input file(s) in decompress.");
        }
        return -1;
    }


    //FREE
    free_prefix(&code);


    return 0;

ENCODE:

    //OPEN
    if((fp_in = open_file(filename_in,"r"))==NULL){
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
    if((fp_out = open_file(filename_out,"w"))==NULL){
        fprintf(stdout,"Error opening output file.\n");
        return -1;
    }   

    //SIGN FILE
    if(write_id(fp_out)!=0){
        fprintf(stdout,"Error writing file identifiers.\n");
        if(fclose(fp_out)!=0){
            perror("Error closing output file in write_id");
        }
        return -1;      
    }  
    
    //WRITE HEADER INFO. NO BYTES AND PREFIX TABLE
    if(write_header(fp_out, code, &n_prefix, &n_bytes)!=0){
        fprintf(stdout,"Error writing file headers.\n");
        if(fclose(fp_out)!=0){
            perror("Error closing output file in write_header");
        }
        return -1;
    }

    //OPEN INPUT FOR 2ND PASS
    if((fp_in = open_file(filename_in,"r"))==NULL){
        fprintf(stdout,"Error opening file.\n");
        return -1;
    }

    //COMPRESS
    if(compress(fp_in, fp_out, code)!=0){
        fprintf(stdout,"Error compressing output file.\n");
        if((fclose(fp_out)!=0)||fclose(fp_in)!=0){
            perror("Error closing output/input file(s) in compress.");
        }
        return -1;
    }

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
    unsigned int nbytes, exit = 0;
    unsigned char buffer[BUFF_SIZE];   
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
    if(fclose(fp)==0){
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

struct map_prefix* get_prefix(struct map_prefix* root, unsigned char ascii_dec){

    if(!root)   return NULL;

    //VAR
    struct map_prefix* pointer = NULL;

    for(pointer=root; pointer!=NULL; pointer=pointer->next){
        if(pointer->ascii==ascii_dec)   break;
    }

    return pointer; //could be NULL if not found. that'd be an error.
}

int compress(FILE* fp_in, FILE* fp_out, struct map_prefix* root){

    if(!fp_in||!fp_out||!root)  return -1;

    //VARs
    int exit    = 0;
    bool full   = false;
    unsigned int nbytes;
    unsigned char buffer[BUFF_SIZE];                //need to find opt value for buffer_size to reduce system calls
    unsigned char cache[CACHE_SIZE] = {'\0','\0'};  //holds two bytes, we fill one, keep the leftovers in the other, write to output and clean

    //PREPARE INPUT
    if(ftell(fp_in)!=0) rewind(fp_in); //same as fseek(fp,0L,SEEK_END);

    //BUFFERED STREAM
    fprintf(stdout,"File -> Reading file using buffer of: %d (bytes)\n",BUFF_SIZE);
    memset(buffer,'\0',sizeof(char)*BUFF_SIZE);
    while((nbytes=fread(buffer,sizeof(unsigned char),BUFF_SIZE,fp_in)!=0)){     //fread not returning nbytes accordingly. only 1 or 0?
        struct map_prefix* byte_prefix  = NULL;
        for(int i=0; i<BUFF_SIZE; i++){
            
            if(buffer[i]=='\0')    break;   //until I fix fread return value
   
            //identify byte in prefix_table
            if((byte_prefix = get_prefix(root,buffer[i]))==NULL){   //it wont happen if we mapped correctly 
                fprintf(stdout,"Compress -> Cant map input to prefix_table.\n");
                exit = -1;
                goto STOP;
            }

            //fill cache with byte
            if((full=fill_cache(cache, byte_prefix))==true){

                //write first byte of cache
                if(fwrite(cache,sizeof(unsigned char),1,fp_out)!=1){
                    exit = -1;
                    goto STOP;
                }
    
                //swap positions
                cache[0] = cache[1];
                cache[1] = '\0';
            }
        }
        
        //prepare next buffer for next loop
        memset(buffer,'\0',sizeof(unsigned char)*BUFF_SIZE);
        exit++;
    }

    //write remnants of cache when we finish the loop if there are some
    if(!full){
        if(fwrite(cache,sizeof(unsigned char),1,fp_out)!=1){
            exit = -1;
            goto STOP;
        }
    }

    fprintf(stdout,"File -> File (output) compressed correctly.\n");

STOP:
    //STOPPED READING
    if(feof(fp_in)!=0){
        fprintf(stdout,"File -> File (input) read successfully in %d passes.\n",exit);
    }else{
        fprintf(stdout,"File -> Couldnt process input file.\n");
        if(ferror(fp_in)!=0)   fprintf(stderr,"compress ferror.\n");
    }
    //CLOSING FILE POINTER.
    if(fclose(fp_in)==0&&fclose(fp_out)==0){
        fprintf(stdout,"File -> File(s) (input|output) closed.\n");
    }else{
        //sets errno.
        perror("closing file(s) in compress");
        return -2;
    }
    if(exit==-1)    return -1;   

    return 0;
}

bool fill_cache(unsigned char cache[], struct map_prefix* byte_node){

    if(!cache||!byte_node)   return false;

    //VARs
    static unsigned char cursor     = BYTE_BITS;
    unsigned char byte_dec, byte_len, byte_buff;
    signed char pos;    
    
    //DEFINE
    byte_dec = byte_node->prefix_dec;
    byte_len = byte_node->prefix_len;

    pos = cursor-byte_len;

    if(pos>0){
        
        cursor      = pos;
        byte_buff   = byte_dec << pos;
        cache[0]    = cache[0] | byte_buff; 

        return false;
    }else if(pos<0){
        
        pos         = abs(pos);
        cursor      = BYTE_BITS-pos;
        byte_buff   = byte_dec >> pos;
        cache[0]    = cache[0] | byte_buff;
        cache[1]    = byte_dec << cursor;

    }else{
        //pos==0;
        cursor      = BYTE_BITS;
        cache[0]    = cache[0] | byte_dec;  //perfect fit
    }

    return true;
}

bool verify_file(FILE* fp_in){

    if(!fp_in)  return false;

    //VAR
    unsigned char ID[ID_LEN] = {0,0};
    
    rewind(fp_in); //just in case. same as fseek(fp_in,0L,SEEK_SET)
    if(fread(ID,sizeof(unsigned char),ID_LEN,fp_in)!=ID_LEN){
        fprintf(stdout,"File -> Couldnt read input file properly in verify_file.\n");
        return false;
    }

    return (ID[0]==244&&ID[1]==245);    //IDs f4 & f5
}

unsigned long get_filesize(FILE* fp_in){

    if(!fp_in)  return 0L;

    //VAR
    unsigned long original_filesize = 0L;

    assert(ftell(fp_in)==sizeof(unsigned char)*2);  //check that we've already verified the file    
    if(fread(&original_filesize,sizeof(unsigned long),1,fp_in)!=1){
        fprintf(stdout,"File -> Couldnt read original file size properly from input file in get_filesize.\n");
        return 0L;
    }

    return original_filesize;
}

void read_prefixes(FILE* fp, struct map_prefix** code, unsigned char* num){

    if(!fp||!code||!num)    return;
    if(*code)   free_prefix(code);  //clean up if somehow code has a list of prefixes in it
    
    //VAR
    unsigned char ascii, prefix_dec, prefix_len;

    //read number of prefixes ahead
    if(fread(num,sizeof(unsigned char),1,fp)!=1){
        fprintf(stdout,"File -> Couldnt read number of table items ahead in read_prefixes.\n");
        return;
    }
    
    //read prefixes and build table-list
    for(int i=0; i<(*num); i++){
   
        //ascii, prefix_dec and prefix_len     
        if(fread(&ascii,sizeof(unsigned char),1,fp)!=1||fread(&prefix_dec,sizeof(unsigned char),1,fp)!=1||fread(&prefix_len,sizeof(unsigned char),1,fp)!=1){
            fprintf(stdout,"File -> Couldnt read prefix item from input file properly.\n");
            return;
        }
        add_prefix_table(ascii, prefix_dec, prefix_len, code);
    }
    
    return;
}

void add_prefix_table(unsigned char ascii, unsigned char prefix_dec, unsigned char prefix_len, struct map_prefix** head){

    if(!head)   return;
    
    //ALLOC
    struct map_prefix* new = (struct map_prefix*)malloc(sizeof(struct map_prefix));
    new->prefix_len = prefix_len;
    new->prefix_dec = prefix_dec;
    new->ascii  = ascii;
    memset(&(new->prefix),'\0',CODE);  //unnecessary for decoding
    new->next   = *head;
    *head       = new;
    
    return;
}

int decompress(FILE* fp_in, FILE* fp_out, struct map_prefix** root, unsigned char n_prefix, unsigned long n_bytes_file){

    if(!fp_in||!fp_out||!root)  return -1;

    //VARs
    int exit    = 0;
    bool stop   = false;
    unsigned int nbytes;
    unsigned int filepos;
    unsigned long n_bytes_written = 0;
    unsigned char n_bytes_decoded = 0;
    unsigned char byte_dec_lo = 0;                  //leftovers
    unsigned char byte_len_lo = 0;
    unsigned char buffer[BUFF_SIZE];                //need to find opt value for buffer_size to reduce system calls
    unsigned char cache[BYTE_BITS];

    //PREPARE INPUT
    filepos = sizeof(unsigned long)+sizeof(unsigned char)*3*(n_prefix+1);   //2 bytes for ID, 4 bytes for filesize, 1 byte for no. prefix and 3x for prefix-item
    if(ftell(fp_in)!=filepos)   fseek(fp_in,(long)filepos,SEEK_SET);        //set position ready to decode if fp_in is not

    //BUFFERED STREAM
    fprintf(stdout,"File -> Reading file using buffer of: %d (bytes)\n",BUFF_SIZE);
    memset(buffer,'\0',sizeof(char)*BUFF_SIZE);
    while((nbytes=fread(buffer,sizeof(unsigned char),BUFF_SIZE,fp_in)!=0)){     //fread not returning nbytes accordingly. only 1 or 0?
        
        for(int i=0; i<BUFF_SIZE; i++){
            
            if(buffer[i]=='\0')    break;   //until I fix fread return value

            if((n_bytes_decoded=decode(buffer[i],root,&byte_dec_lo,&byte_len_lo,fp_out,cache))==0)    goto STOP;  //we should always decode at least 1
            
            //write only decoded bytes belonging to the original file, discarding those left for padding purposes
            if((n_bytes_written+n_bytes_decoded)>=n_bytes_file){
                //the rest is just padding
                n_bytes_decoded = n_bytes_file-n_bytes_written;
                stop = true;
            }
            if(fwrite(cache,sizeof(unsigned char),n_bytes_decoded,fp_out)!=n_bytes_decoded){
                fprintf(stdout,"Error writing to output file in decode.\n");
                goto STOP;
            }
            if(stop)    goto STOP;
        }

        //prepare next buffer for next loop
        memset(buffer,'\0',sizeof(unsigned char)*BUFF_SIZE);
        exit++;
    }
    fprintf(stdout,"File -> File (input) decompressed correctly.\n");


STOP:
    //STOPPED READING
    if(feof(fp_in)!=0){     //should be already at the end
        fprintf(stdout,"File -> File (input) read successfully in %d passes.\n",exit);
    }else{
        fprintf(stdout,"File -> Couldnt process input file.\n");
        if(ferror(fp_in)!=0)   fprintf(stderr,"compress ferror.\n");
    }
    //CLOSING FILE POINTER.
    if(fclose(fp_in)==0&&fclose(fp_out)==0){
        fprintf(stdout,"File -> File(s) (input|output) closed.\n");
    }else{
        //sets errno.
        perror("closing file(s) in decompress");
        return -2;
    }
    if(exit==-1)    return -1;   

    return 0;

}

unsigned char decode(unsigned char encoded_byte,struct map_prefix** root,unsigned char* dec_leftover,unsigned char* len_leftover,FILE* fp,unsigned char cache[]){

    if(!root||!dec_leftover||!len_leftover) return 0;

    //VARs
    struct map_prefix* pointer  = NULL;
    bool found                  = false;
    bool leftover               = false;
    unsigned char n_bytes_found = 0;
    unsigned char cursor        = 0;
    unsigned char byte          = encoded_byte;   
    unsigned char i, remaining_byte, len, compound = 0;

    memset(cache,'\0',sizeof(char)*BYTE_BITS);

SEARCH:
    found = false;
    remaining_byte = byte;
    
    for(i=BYTE_BITS-1; i>=cursor; i--){
 
        if(*len_leftover>0){
            //we need to match up leftovers from previous byte with the next one
            leftover    = true;
            byte        = remaining_byte >> i;       
            compound    = (*dec_leftover << (BYTE_BITS-i)) | byte;
            len         = *len_leftover+BYTE_BITS-i;
        }else{
            //append new bit
            leftover    = false;
            byte        = remaining_byte >> i;
            len         = BYTE_BITS-i;
        }

        //check if it is code for something
        if((pointer=check_byte((leftover) ? compound : byte,len,root))!=NULL){

            //write cache            
            cache[n_bytes_found] = pointer->ascii;           
               
            //update byte, cursor and no bytes found
            byte    = remaining_byte << (BYTE_BITS-i);
            cursor += BYTE_BITS-i;
            n_bytes_found++;
            found = true;
            if(leftover)    *len_leftover=0;    //we found missing part
            break;
        }
    }

    if(found)   goto SEARCH;    //keep searching

    //save leftovers for next call and return no bytes found
    *dec_leftover =  byte;
    *len_leftover =  BYTE_BITS-cursor;
    return n_bytes_found;
}

struct map_prefix* check_byte(unsigned char byte_dec, unsigned char byte_len, struct map_prefix** root){

    if(!root)   return NULL;

    //VAR
    struct map_prefix* pointer = NULL;

    for(pointer=*root; pointer!=NULL; pointer=pointer->next){
        if((pointer->prefix_dec==byte_dec)&&(pointer->prefix_len==byte_len))   break;
    }

    return pointer; //could be NULL if not found. that'd be an error.
}
