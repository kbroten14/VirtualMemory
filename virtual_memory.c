/**
 *
 *  Kevin Broten
 *  Operating Systems 
 *  Homework 6
 *  Virtual Memory
 *
 */ 


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// number of characters to read for each line from input file
#define BUFFER_SIZE         32

// number of bytes to read
#define CHUNK               256

// mask to determine page number from logical address
#define PAGE_MASK			0xff00

// mask to determine offset from logical address
#define OFFSET_MASK			0x00ff


FILE    *address_file;
FILE    *backing_store;

// how we store reads from input file
char    address[BUFFER_SIZE];

// the buffer containing reads from backing store
signed char     buffer[CHUNK];

// the value of the byte (char) in memory
signed char     value;



// Allocates memory for an N x M 2D array
signed char **get_two_dimensional(int n, int m)
{
    int i;
    signed char **array;
    array = (signed char **) malloc(n * sizeof(signed char *));
    for(i = 0; i < n; i++)
        array[i] = (signed char *) malloc(m * sizeof(signed char));

    return array;
}


// Initializes the 2D array, assigning each spot in the array the value of -1
void init_two_dimensional(signed char** array, int n, int m)
{
    int i, j;
    for(i = 0; i < n; i++)
        for(j = 0; j < m; j++)
            array[i][j] = -1;
}


// Allocates memory for a one dimensional array of a specified size
signed char *get_one_dimensional(int size)
{
    signed char *array;
    array = malloc(size * sizeof(signed char));

    return array;
}


// Initializes the one dimensional array, assigning each spot the value of -1
void init_one_dimensional(signed char* array, int size)
{
    int i;
    for(i = 0; i < size; i++)
        array[i] = -1;
}


// Searches through the TLB and returns the frame number if it exists in the TLB
//      Returns -1 if the page number is not in the TLB
int check_tlb(signed char** tlb, int page_number)
{
    int i, frame_number;
    frame_number = -1;
    for(i = 0; i < 16; i++)
        if(tlb[i][0] == page_number)
            frame_number = tlb[i][1];

    return frame_number;
}


// Looks in the page table at the specified page number and returns the value in that location
//      Returns frame number or -1 if the frame number is not in the page table
int check_page_table(signed char* page_table, int page_number)
{
    int frame_number;
    frame_number = page_table[page_number];

    return frame_number;
}

// Seeks and reads through backing store for the page number and loads it into a buffer
//      Grabs the offset value from the buffer and returns that value
signed char get_value_from_backing_store(int page_number, int offset)
{
    signed char value;

    if (fseek(backing_store, page_number * 256, SEEK_SET) != 0) // seek page number in backing store 
    {
        fprintf(stderr, "Error seeking in backing store\n");
        return -1;
    }

    if (fread(buffer, sizeof(char), CHUNK, backing_store) == 0) // read CHUNK from backing store into buffer
    {
        fprintf(stderr, "Error reading from backing store\n");
        return -1;
    }

    value = buffer[offset];

    return value;
}

// Gets the specified frame from physical memory
int get_frame(signed char* physical_mem, int frame_number)
{
    int frame;
    frame = physical_mem[frame_number];

    return frame;
}


// Gets the value from the specified frame
signed char get_value_from_frame(int frame)
{
    signed char value;
    value = (signed char) frame;

    return value;
}


// Gets the physical address from the specified frame number and offset
int get_physical_address(int frame_number, int offset)
{
    int physical_address;
    physical_address = (frame_number << 8) | offset;

    return physical_address;
}



int main(int argc, char *argv[])
{
	signed char *physical_mem = get_one_dimensional(256); // allocate physical memory
	signed char *page_table = get_one_dimensional(256); // allocate page table
	signed char **tlb = get_two_dimensional(16, 2); // allocate TLB

    init_one_dimensional(physical_mem, 256); // initialize physical memory
    init_one_dimensional(page_table, 256); // initialize page table
    init_two_dimensional(tlb, 16, 2); // initialize TLB
	

    // perform basic error checking
    if (argc != 3) {
        fprintf(stderr,"Usage: ./vm [backing store] [input file]\n");
        return -1;
    }

    // open the file containing the backing store
    backing_store = fopen(argv[1], "rb");
    
    if (backing_store == NULL) { 
        fprintf(stderr, "Error opening %s\n",argv[1]);
        return -1;
    }

    // open the file containing the logical addresses
    address_file = fopen(argv[2], "r");

    if (address_file == NULL) {
        fprintf(stderr, "Error opening %s\n",argv[2]);
        return -1;
    }

    int tlb_hit = 0; //records total tlb misses
    int page_fault = 0; //records total page faults
    int addresses = 0; //records total addresses read in

    int next_open_frame = 0; //holds the next open frame in physical memory
    int tlb_position = 0; //holds the next x position in the tlb to write to

    while ( fgets(address, BUFFER_SIZE, address_file) != NULL) {
        int logical_address = atoi(address); //current read address
        int page_number = (logical_address & PAGE_MASK) >> 8; //address page number
        int offset = logical_address & OFFSET_MASK; //address offset

        addresses += 1;

        int frame_number = check_tlb(tlb, page_number); // checks TLB for frame number, frame_number = -1 when frame not in TLB         
        int physical_address;
        int frame;
        if(frame_number > -1) // if frame number not in TLB
        {
            tlb_hit++;
            frame = get_frame(physical_mem, frame_number); // get the frame
            value = get_value_from_frame(frame); // read value from frame
            physical_address = get_physical_address(frame_number, offset); // get physical address 
        }
        else // else check the page table
        {
            frame_number = check_page_table(page_table, page_number); // check the page table

            if(frame_number > -1) // if page table at page number has a frame number
            {
                frame = get_frame(physical_mem, frame_number); // get the frame
                value = get_value_from_frame(frame); // read value from frame
                physical_address = get_physical_address(frame_number, offset); // get the physical address  
            }
            else // else page fault occurs and value is grabbed from backing store
            { 
                page_fault++;

                value = get_value_from_backing_store(page_number, offset); // grab value from backing store
                frame_number = next_open_frame; // get the next frame open number
                physical_mem[frame_number] = value; // write value to next open frame number in physical memory
                tlb[tlb_position][0] = page_number; // update TLB
                tlb[tlb_position][1] = frame_number; // update TLB
                page_table[page_number] = frame_number; // update page table

                physical_address = get_physical_address(frame_number, offset); // get physical address

                if(next_open_frame == 255)
                    next_open_frame = 0; // next open frame to write to starts back at 0 when physical memory is full
                else
                    next_open_frame++; // otherwise increments next open frame to the next frame in physical memory

                if(tlb_position == 15)
                    tlb_position = 0; // next tlb spot becomes 0 when tlb is full
                else
                    tlb_position++; // increment tlb spot
            }
        }
        
        printf("Virtual Address: %d  Physical Address: %d  Value: %d\n", logical_address, physical_address, value);
    }

    printf("TLB Hits: %d\n", tlb_hit);
    printf("TLB Hit Rate: %f\n", (double) tlb_hit/addresses);
    printf("Page Faults: %d\n", page_fault);
    printf("Page Fault Rate: %f\n", (double) page_fault/addresses);



    fclose(address_file);
    fclose(backing_store);

    return 0;
}