#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "main.h"

// Size of the buffer
#define SIZE 1000

// Special marker used to indicate end of the producer data
#define END_MARKER -1

// Buffer, shared resource
char buffer[SIZE];
char buffer1[SIZE];
char buffer2[SIZE];

// Number of items in the buffer, shared resource
int count = 0;
// Index where the producer will put the next item
int prod_idx = 0;
// Index where the consumer will pick up the next item
int con_idx = 0;

int plus_idx = 0;

int new_idx = 0;

int line_counter = 0;

int total_chars = 0;

int chars_outputed = 0;

// Initialize the mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize the condition variables
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]){
    srand(time(0));
    // Create the producer and consumer threads
    pthread_t p1, p2, p3, p4;
    pthread_create(&p1, NULL, input_thread, NULL);
    pthread_create(&p2, NULL, line_sep_thread, NULL);
    pthread_create(&p3, NULL, plus_thread, NULL);

    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    pthread_join(p3, NULL);
    pthread_create(&p4, NULL, output_thread, NULL);
    pthread_join(p4, NULL);

    return 0;
}

/*void *output_thread(void *args) - this is the output thread that is
 * responsible for outputing the 80 lines to stdout
 */
void *output_thread(void *args){
	pthread_mutex_lock(&mutex);
    int num_lines = (total_chars -5) / 80; // -5 to get rid of the DONE
    //printf("total_chars: %d num_lines: %d", total_chars, num_lines);

    if(num_lines == 0) exit(0);

    else{
    	for(int i = 0; i < num_lines; i++){
    		for (int j = 0; j < 80; ++j){
    			if(buffer2[j+(80*i) +5] == '$') {
    				printf("\n");
    				exit(0);
    			}
    			printf("%c", buffer2[j+(80*i)]);
    		}
    		printf("\n");
    	}
    }
    pthread_mutex_unlock(&mutex);
    //printf("ENDING OUTPUT THREAD\n");
    pthread_exit(0);
}

/*
 *  this fucntion outputs the line from the thread
 */
char output_line(){
	char buf[80];

	char value = 'a';

	printf("BUFFER2: \n");
    for(int j = 0; j < 80; j++){
        printf("%c", buffer2[j+chars_outputed]);

    }
	chars_outputed = chars_outputed + 80;

	return value;
}

/* void *plus_thread(void *args) - this is the plus thread that is
 * responsible for checking two '+' and replacing by '^'.
 */
void *plus_thread(void *args){
    char value;
    while (value != '$'){
      // Lock the mutex before checking where there is space in the buffer
      pthread_mutex_lock(&mutex);
      while (count == SIZE){
      	// Buffer is full. Wait for the consumer to signal that the buffer has space
        pthread_cond_wait(&empty, &mutex);
        //printf("plus buffer waititng.... \n");
		}
      value = produce_plus();
      if((value>=32 && value<=126)){
      	total_chars++;
      }
	  //printf("plus value:  %c\n", value);
      // Signal to the consumer that the buffer is no longer empty
      pthread_cond_signal(&full);
      // Unlock the mutex

      pthread_mutex_unlock(&mutex);

    }
    //printf("ENDING PLUS THREAD\n");
    pthread_exit(0);
}

/* int check_buffer(char buf[1000]) - checks for DONE char
 *  if found returns 1 else returns 0
 */
int check_buffer(char buf[1000]){
    if(sizeof(buf) < 4) return 0;

    else{
        if(strstr(buf, "DONE\n") != NULL) return 1;
        else return 0;
    }
}

/*  int check_pluss(int idx) - checks for idx and idx+1 for '+'
 *  if found 1 is returned else 0
 */
int check_pluss(int idx){
    if(SIZE - idx >= 2){
        if(buffer1[idx] == '+' && buffer1[idx+1] == '+') return 1;
    }
    return 0;
}

/* char produce_plus() - responsible for checking index's and calling check_plus
 *  from the plus thread
 */
char produce_plus(){
    char value;
    size_t chars;
    if(check_pluss(plus_idx)) {
        value =  '^';
        buffer2[plus_idx] = value;
        plus_idx = (plus_idx + 2) % SIZE;
    }
    else {
        value = buffer1[plus_idx];
        buffer2[plus_idx] = value;
        plus_idx = (plus_idx + 1) % SIZE;
    }

    // Increment the index where the next item will be put. Roll over to the start of the buffer if the item was placed in the last slot in the buffer
    count--;
    return value;
}

/*
 Produce an item. Produces a random integer between [0, 1000] unless it is the last item to be produced in which case the value -1 is returned.
*/
char produce_input(){
    char value;
    size_t chars;
    if(check_buffer(buffer)) {
        value =  '$';
    }
    else {
        value = getchar();
    }
    buffer[prod_idx] = value;
    // Increment the index where the next item will be put. Roll over to the start of the buffer if the item was placed in the last slot in the buffer
    prod_idx = (prod_idx + 1) % SIZE;
    count++;
    return value;
}

/*
 Function that the producer thread will run. Produce an item and put in the buffer only if there is space in the buffer. If the buffer is full, then wait until there is space in the buffer.
*/
void *input_thread(void *args){
    char value;
    while (value != '$'){
      // Lock the mutex before checking where there is space in the buffer
      pthread_mutex_lock(&mutex);
      while (count == SIZE)
        // Buffer is full. Wait for the consumer to signal that the buffer has space
        pthread_cond_wait(&empty, &mutex);

      value = produce_input();
      //printf("PROD %c\n", value);
      // Signal to the consumer that the buffer is no longer empty
      pthread_cond_signal(&full);
      // Unlock the mutex

      pthread_mutex_unlock(&mutex);
    }
    //printf("ENDING INPUT THREAD\n");
    pthread_exit(0);
}

/*
 Get the next item from the buffer
*/
char consume_input(){
    char value = buffer[con_idx];


    if(value == '\n') buffer1[con_idx] = ' ';
    else{
        buffer1[con_idx] = value;
    }

    if(check_buffer(buffer1)) {
        value =  '$';
    }

    // Increment the index from which the item will be picked up, rolling over to the start of the buffer if currently at the end of the buffer
    con_idx = (con_idx + 1) % SIZE;
    count--;
    return value;
}

/*
 Function that the consumer thread will run. Get  an item from the buffer if the buffer is not empty. If the buffer is empty then wait until there is data in the buffer.
*/
void *line_sep_thread(void *args){
    char value;
    // Continue consuming until the END_MARKER is seen
    while (value != '$')
    {
      // Lock the mutex before checking if the buffer has data
      pthread_mutex_lock(&mutex);
      while (count == 0)
        // Buffer is empty. Wait for the producer to signal that the buffer has data
        pthread_cond_wait(&full, &mutex);

      value = consume_input();

     // printf("CONS %c\n", value);
      // Signal to the producer that the buffer has space
      pthread_cond_signal(&empty);
      // Unlock the mutex
      pthread_mutex_unlock(&mutex);
    }
    //printf("ENDING LINE SEPERATOR THREAD\n");
    pthread_exit(0);
}

