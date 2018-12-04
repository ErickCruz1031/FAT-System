#ifndef VIRTUALMACHINE
#define VIRTUALMACHINE

#include <iostream>
using namespace std;
#include "VirtualMachine.h"
#include "Machine.h"
#include <vector>
#include <deque>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;
#ifdef __cplusplus
extern "C"{
#endif
    TVMMainEntry VMLoadModule(const char*);
    void VMUnloadModule(void);
#ifdef __cplusplus
}
#endif

#endif

#define VM_THREAD_PRIORITY_LOWEST       ((TVMThreadPriority)0x00)
#define VM_TIMEOUT_INF                  ((TVMTick)-2)
//#define VM_MEMORY_POOL_ID_INVALID               ((TVMMemoryPoolID)1)
void Schedule();
void Skeleton(void *param);
void AlarmCallback(void *calldata);
void VMFileWriteCallback(void *calldata, int result);
void VMFileOpenCallback(void *calldata, int result);
void infiniteLoop(void *param);
void ParseFAT();
void AddStdinFiles();
TVMStatus MainRead(int filedescriptor, void *data, int *length);
void VMCallback(void *calldata, int result);
void VMModCallback(void *calldata, int result);
bool Allocate(TVMMemoryPoolID pool_index, TVMMemorySize size, void **pointer);
int openCluster();
//void offsetIndex(int* cluster, int* sector, int*byte, int fd);
TVMStatus MainWrite(int filedescriptor, void *data, int *length);
void offsetIndex(int* cluster, int* sector, int*byte, int fd, int original_offset);


 struct ThreadControlBlock;
 struct MutexControlBlock;

 struct ThreadControlBlock{
     TVMThreadEntry entry;
     void *param;
     TVMThreadID threadID; 
     TVMThreadPriority priority;
     TVMThreadState state;
     SMachineContext context_ref;
     TVMMemorySize memsize;
     uint8_t *stack_ptr;
     int processData;
     TVMTick rem_sleep;
     deque<MutexControlBlock> thread_mutex_list;
     TVMMutexID wait_mutexID;
 };

 struct MutexControlBlock{
    bool is_alive;
    TVMMutexID mutexID;
    TVMThreadID owner_thread_ID;
    deque<ThreadControlBlock> waiting_queue;
 };


 struct Block{
    bool is_free; 
    uint8_t *base;  
    TVMMemorySize size;
 };

 struct MemoryPoolControlBlock
 { 
    bool is_alive;
    uint8_t *memory;
    TVMMemoryPoolID ID;
    TVMMemorySize size;
    TVMMemorySize bytes_left;
    vector<Block> free_blocks;
    vector<Block> allocated_blocks;

 };



vector<ThreadControlBlock> thread_list;
TVMThreadID current_thread_id;
TVMThreadID idle_id = 1;
vector<ThreadControlBlock> inf_sleep_threads; 
deque<ThreadControlBlock> sleeping_threads_list;
vector<MemoryPoolControlBlock> Pools;
int tick_ms;
TVMTick tick_count = 0;
deque<MutexControlBlock> mutex_list;
const TVMMemoryPoolID VM_MEMORY_POOL_ID_SYSTEM = 1;

//Project 4:
 struct Sector
 {
    vector<uint8_t> Info;
    int ID;
    // int sectorID;
 };
 struct FATEntry 
{
    short current;
    short next;
};

struct Cluster
{
    int clusterID; 
    vector<Sector> cluster_data;
    bool dirty; // check  if it has been modified

    // sectors
    // offset
    // way to clear cluster
};

struct RootEntry
{
    bool free;
    char shortName [11]; // offset 0, bytes 11
    bool read_only;
    bool is_dir;
    int filesize;
    short first_cluster_number;
    bool dirty;
    int offset;
    uint16_t creation_date;
    uint16_t creation_time;
    uint16_t last_modify_date;
    uint16_t last_modify_time;
    uint16_t last_access_date;
    vector<Cluster> Sectors;
    bool seek;
    int pointer_offset;
    // longEntries

};


struct File
{
	int fileID;
    int start_cluster_ID;
    char *abs_path;
    int size;
    // creation_date;
    // last_modified;
    // last_accessed;
    // dirty bit
    // RD_only
};
// int fat_fd;
// uint8_t sec_per_clus;
// uint16_t fat_size;
// uint8_t num_fat;
// uint16_t root_entry_cnt;
// int num_data_sectors;
// int num_data_clusters;
// uint16_t num_total_sectors;
// int root_dir_sectors;
// int fat_begin;
// int root_begin;
// int data_begin;

// change types:
int fat_fd;
int sec_per_clus;
int fat_size;
int num_fat;
int root_entry_cnt;
int num_data_sectors;
int num_data_clusters;
int num_total_sectors;
int root_dir_sectors;
int fat_begin;
int root_begin;
int data_begin;

vector<FATEntry> FAT_Table;
vector<RootEntry> root_entries;
vector<Cluster> data_clusters;
vector<File> files_list;

void (*actual_entry)(void *);

vector<deque<ThreadControlBlock> > ready_threads_list(4); 

#ifndef NULL
#define NULL (void *)0
#endif

// TVMStatus VMStart(int tickms, int argc, char *argv[]){

TVMStatus VMStart(int tickms, TVMMemorySize heapsize,TVMMemorySize sharedsize, const char* mount, int argc, char* argv[]){

    // CHANGE

    /* VMStart() starts the virtual machine by loading the module 
    specified by argv[0]. The argc and argvare passed directly 
    into the VMMain() function that exists in the loaded module. 
    The time in milliseconds of the virtual machine tick is specified 
    by the tickms parameter.The heap size of the virtual machine is 
    specified by heapsize. The heap is accessed by the applications 
    through the VM_MEMORY_POOL_ID_SYSTEM memory pool. The size of the 
    shared memory space between the virtual machine and the machine 
    is specified by the sharedsize. */

    current_thread_id = 0;
    tick_ms = tickms; 
    TVMThreadID temp = 0;
    SMachineContext other;

    vector<Block> free_blocks_one;
    vector<Block> free_blocks_two;

    vector<Block> alloc_blocks_one;
    vector<Block> alloc_blocks_two;

    // cout << "heap size is " << heapsize << " and shared it " << sharedsize << "\n";


    //Create the shared memory pools between VM and Machine
    uint8_t* ptr = (uint8_t *) MachineInitialize(sharedsize);
    Block first_block = {
        true,
        ptr,
        sharedsize,
    };
    free_blocks_one.push_back(first_block);
    MemoryPoolControlBlock shared_pool = {
        true,
        ptr,
        0,
        sharedsize,
        sharedsize,
        free_blocks_one,
        alloc_blocks_one,
    };
    Pools.push_back(shared_pool);



    //Create the system pool 
    
    uint8_t* sys_ptr = (uint8_t *) (malloc(sizeof(uint8_t) * heapsize));

    Block first_sys_block = {
        true,
        sys_ptr,
        heapsize,
    };

    free_blocks_two.push_back(first_sys_block);
    MemoryPoolControlBlock sys_pool = 
    {
        true,
        sys_ptr,
        1,
        heapsize,
        heapsize,
        free_blocks_two,
        alloc_blocks_two,
    };

    Pools.push_back(sys_pool);

    // cout << "THE SYSZE IS " << Pools[1].bytes_left << "\n";

    MachineEnableSignals();

    useconds_t microsecs = tickms * 1000;

    MachineRequestAlarm(microsecs, AlarmCallback, NULL);

    deque<MutexControlBlock> main_mutex_queue;
    // main application's control block
    ThreadControlBlock main_TCB = { 
        NULL, // entry
        NULL, // param
        temp, // threadID
        VM_THREAD_PRIORITY_NORMAL, // priority
        VM_THREAD_STATE_RUNNING, // state
        other,//Context ref
        0, // memsize
        NULL, // stack_ptr
        -1, //Process data
        0, // rem_sleep
        main_mutex_queue, //thread_mutex_list
        NULL, //wait_mutexID
    };
    current_thread_id = 0;
    thread_list.push_back(main_TCB);
    
    //Set up the idle thread
    TVMMemorySize mem = 0x1000000;
    TVMThreadID test_threadid;

    VMThreadCreate(infiniteLoop, NULL, mem, 0, &test_threadid);
    VMThreadActivate(1);
    
    //void *calldata = 0;

    // OpenFAT
    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);
    void *calldata = &thread_list[current_thread_id].threadID;
    MachineFileOpen(mount, O_RDWR, 0644, VMCallback, calldata);
    thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
    Schedule(); 
    MachineResumeSignals(sigset);
    fat_fd = thread_list[current_thread_id].processData;
    cout << "FD IS " <<fat_fd << "\n";

    ParseFAT();
    AddStdinFiles();
    return VM_STATUS_SUCCESS;

    TVMMainEntry vm_main = VMLoadModule(argv[0]);
    
    if (vm_main == NULL){
        return VM_STATUS_FAILURE;
    }
    
    vm_main(argc, argv);

    // cout << "YO WE ARE HERE BACK\n";
    // free allocated memory
    /*
    int thread_list_size = thread_list.size();
    for (int i = 0; i < thread_list_size; i++){
        free(thread_list[i].stack_ptr);
    }
    */

    return VM_STATUS_SUCCESS;
}


// void OpenFAT();

void AddStdinFiles(){
	File stdin_file = {
		0,
	    0,
	    "0",
	    0,
	};

	File stdout_file = {
		1, 
		1, 
		"1",
		1,
	};

	File stderror_file = {
		2, 
		2, 
		"2", 
		2,
	};
	files_list.push_back(stdin_file);
	files_list.push_back(stdout_file);
	files_list.push_back(stderror_file);
	return;
}

void ParseFAT(){

    // Parse BPB

    uint8_t Buffer[512];
    int length = 512;

    MainRead(fat_fd, Buffer, &length);

    memcpy(&fat_size, &Buffer[22], 2);
    memcpy(&sec_per_clus, &Buffer[13], 1);
    memcpy(&num_fat, &Buffer[16], 1);
    memcpy(&root_entry_cnt, &Buffer[17], 2);
    memcpy(&num_total_sectors, &Buffer[19], 2);
    if (num_total_sectors == 0){
    	memcpy(&num_total_sectors, &Buffer[32], 4);
    }

    //update globals
    root_dir_sectors = (root_entry_cnt * 32 + 511) / 512;
    num_data_sectors = num_total_sectors - (1 + num_fat * fat_size + root_dir_sectors);

    cout << "fatsz: " << fat_size << "\n";
    cout << "sec_per_clus: " << sec_per_clus << "\n";
    cout << "num_fat: " << num_fat << "\n";
    cout << "root_entry_cnt: " << root_entry_cnt << "\n";
    cout << "num_total_sectors: " << num_total_sectors << "\n";
    cout << "root_dir_sectors: " << root_dir_sectors << "\n";
    cout << "num_data_sectors: " <<  num_data_sectors << "\n";

    // Parse FAT

    int root_dir_sectors = (root_entry_cnt * 32 + 511) / 512;
    int bytes_to_read = (num_fat * fat_size + root_dir_sectors) * 512; // changed to read data sectors as well
    uint8_t * data = (uint8_t *) malloc(bytes_to_read * sizeof(void));
    MainRead(fat_fd, data, &bytes_to_read);

    cout << "returned from the other read\n";

    int i = 0;
    int count = 0;
    int num_entries = (fat_size * 512) / 16;

    while(count < num_entries)
    {
        short temp;
        FATEntry entry;
        memcpy(&temp, &data[i], 2);
        entry.current = count;
        entry.next = temp;
        FAT_Table.push_back(entry);
        //cout << temp << "\n";
        i+= 2;
        count +=1;
    }
    cout << "FAT size is " << fat_size << "\n";

    int next_start = (512 * fat_size) * 2;//Skip over the 2 fat tables

    //cout << "This is the cccc " << p << "\n";
    // cout << "Num_entries is " << num_entries << "\n";
    // cout << "Size of the table is " << FAT_Table.size() << "\n";

    //start reading the directory
    // cout << "Directory...\n";
    int root_cnt = 0;
    int j = 0;

    // while (root_cnt < root_entry_cnt)
    // {
    //     char name [8];
    //     memcpy(&name, &data[next_start + j], 8);
    //     cout << name << "\n";
    //     cout << "Next...\n";
    //     j += 32;
    //     root_cnt++;

    // }

    // Parse Root Dir
    
    char *shortName = new char[11];
    // uint8_t *shortName = (uint8_t *) malloc(sizeof(uint8_t) * 11);
    bool free;
    bool is_dir;
    bool read_only;
    int filesize;
    uint8_t dirty_bit;
    bool dirty;
    uint8_t attribute;
    short first_cluster_number;
    // short creation_date;
    // short creation_time;
    // short write_date;
    // short write_time;
    uint16_t creation_date;
    uint16_t creation_time;
    uint16_t last_modify_date;
    uint16_t last_modify_time;
    uint16_t last_access_date;

    //update globals
    root_begin = (num_fat * fat_size) * 512; // changed
    data_begin = root_begin + (root_entry_cnt * 32);

    cout << "root begin: " << root_begin << "\n";
    cout << "data begin: " << data_begin << "\n";

    //for (int j = num_fat * fat_size * 512; j < bytes_to_read - num_data_sectors * 512; j+=32){
    int k = 0;
    int root_count = 0;
    bool long_entry = true;
    while (root_count < root_entry_cnt){
        //cout << "looping\n";
        memcpy(shortName, &data[root_begin + k], 11);
        memcpy(&attribute, &data[root_begin + k + 11], 1);
/*
        if (attribute && 0x01 | attribute && 0x02 | attribute && 0x04 | attribute && 0x08){
        	short_entry_offset += 32; 
    		continue;
        }

*/     	
        if (long_entry == true)
        {
        	if (shortName[0] & 0x40)
        	{
        		long_entry = false;
        	 	k+=32;
        		root_count+=1;
        		continue;
        	}
        	else
        	{
        		//ong_entry = false;
        	 	k+=32;
        		root_count+=1;
        		continue;

        	}
        	
        }
		// char name[11];
        if (shortName[0] == 0xE00){
            free = true;
        }
        else{
            free = false;
         //    char * new_name = new char [11];
        	// memcpy(new_name, shortName, 11);
        	//cout << "name: " << (char *)shortName << "\n";
        	
        	
            for (int i = 0; i < 11; i++){
            	cout <<shortName[i] ;
            }
            cout << "\n";
            // for (int l = 0; l < 11; l++){

            // 	memcpy(&name[l], &shortName[l], 1);
            // 	// cout << "name[" << l << "]: " << shortName[l] << "\n";
            // 	// cout << "ascii: " << (int) shortName[l] << "\n";
            // }
           // cout << "name: " << shortName << "\n";
            
        }

        

        memcpy(&attribute, &data[root_begin + k + 11], 1);
        read_only = attribute && 0x1;
        is_dir = attribute && 0x10;
        memcpy(&filesize, &data[root_begin + k + 28], 4);
        memcpy(&first_cluster_number, &data[root_begin + k + 26], 2);
        memcpy(&creation_date, &data[root_begin + k + 16], 2);
        memcpy(&creation_time, &data[root_begin + k + 14], 2);
        memcpy(&last_modify_date, &data[root_begin + k + 24], 2);
        memcpy(&last_modify_time, &data[root_begin + k + 22], 2);
        memcpy(&last_access_date, &data[root_begin + k + 18], 2);


        // if (!free){
        	// cout << "shortName: " << shortName << "\n";
        	// cout << "filesize: " << filesize << "\n";
        	// if (is_dir){
        	// 	cout << "is_dir: true\n";
        	// }
        	// else{
        	// 	cout << "is_dir: false\n";
        	// }
        	// cout << "first_cluster_number: " << first_cluster_number << "\n";
        	// cout << "creation date: " << creation_date << "\n";
        	// cout << "creation time: " << creation_time << "\n";
        	// cout << "last_modify_date: " << last_modify_date << "\n";
        	// cout << "last_modify_time: " << last_modify_time << "\n";
        	// cout << "last access date: " << last_access_date << "\n";
        	
        // }

        dirty = false; // TODO: may need to change
        /*
        RootEntry new_root_entry = {
            free,
            *shortName,
            read_only,
            is_dir,
            filesize,
            first_cluster_number,
            dirty,
            k, 
            creation_date, 
   			creation_time, 
		    last_modify_date,
		    last_modify_time,
		    last_access_date,

        };
        */
        RootEntry new_entry;
        int mod_index = root_entries.size();
        new_entry.free = free;
        for(int i = 0; i < 11; i++)
        {
            new_entry.shortName[i] = shortName[i];
        }
        new_entry.read_only = read_only;
        new_entry.is_dir = is_dir;
        new_entry.filesize = filesize;
        new_entry.first_cluster_number = first_cluster_number;
        new_entry.dirty = dirty;
        new_entry.offset = k;
        new_entry.creation_date = creation_date;
        new_entry.creation_time = creation_time;
        new_entry.last_modify_date = last_access_date;
        new_entry.last_modify_time = last_modify_time;
        new_entry.last_access_date = last_access_date;

        root_entries.push_back(new_entry);
        root_entries[mod_index].seek = false;
        root_entries[mod_index].pointer_offset = 0;

        cout << "Before IT IS : ";
        cout << string(root_entries[0].shortName) << "\n";
        long_entry = true;
        k+=32;
        root_count+=1;


    }

    // Parse Data ?

       //update globals
    root_begin = (1 + (num_fat * fat_size)) * 512; // changed
    data_begin = root_begin + (root_entry_cnt * 32);
    //TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor)
    cout << "AFTER IT IS : ";
    cout << string(root_entries[0].shortName) << "\n";
    cout << "The size os " << root_entries.size();
    int fd;
    VMFileOpen("LONGTEST.TXT", O_CREAT, 0600, &fd);
    VMFileOpen("OTHER.TXT", O_CREAT, 0600, &fd);
    
    
    
    

    cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
   // cout << "The size of the vector is " << data_clusters.size() << "\n";
    for(int i = 0; i < data_clusters.size(); i++)
    {
         cout << "The size of the vector is " << data_clusters.size() << "\n";
        for(int j = 0; j < data_clusters[i].cluster_data.size(); j++)
        {
            cout << "j is " << j << "\n";
            cout << "i is " << i << " \n";
            cout << "THIS IS CLUSTER " << data_clusters[i].clusterID << "\n";
            cout << "Size of the cluster data is " << data_clusters[i].cluster_data.size() << "\n";
            cout << "The size of this sector is  " << data_clusters[i].cluster_data[j].Info.size() << "\n";
            cout << "Sector number is " << data_clusters[i].cluster_data[j].ID << "\n";;
            for(int k = 0; k < data_clusters[i].cluster_data[j].Info.size(); k+=16)
            {
                
                
                for(int p = 0; p < 16; p++){
                        printf("%02X ", data_clusters[i].cluster_data[j].Info[k+p]);
                    }
                for(int p = 0; p < 16; p++){
                        if(isprint(data_clusters[i].cluster_data[j].Info[k+p])){
                            printf("%c",data_clusters[i].cluster_data[j].Info[k+p]);
                        }
                        else{
                            printf(".");
                        }
                    }
                    printf("\n");



                /*
                for(int i = 0; i < 512; i += 16){
                    for(int j = 0; j < 16; j++){
                        printf("%02X ", cluster_data[i+j]);
                    }
                    for(int j = 0; j < 16; j++){
                        if(isprint(cluster_data[i+j])){
                            printf("%c",cluster_data[i+j]);
                        }
                        else{
                            printf(".");
                        }
                    }
                    printf("\n");
                }
                */

            }
        }
    }
    uint8_t buff[1024];
    int h = 1024;
    //Need a system to tell which are system fd's and which are not
    fd = fd + 100;
    //TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset)
    int d;
    //Adding this doesn't seem to work???
    //VMFileSeek(fd, 1024, 0, &d);
    VMFileRead(fd, buff, &h);
    VMFileSeek(fd, 1024, 0, &d);
    cout << "RETURNED FROM READ\n";
    cout << "h is " << h << "\n";
    for(int i = 0; i < h; i += 16){
        if (i == 512)
        {
            printf("\n");
        }
        for(int j = 0; j < 16; j++){
            printf("%02X ", buff[i+j]);
        }
        for(int j = 0; j < 16; j++){
            if(isprint(buff[i+j])){
                printf("%c",buff[i+j]);
            }
            else{
                printf(".");
            }
        }
        printf("\n");
    }


    int Length = 23;
    char* b = "I LITERALLY HATE THIS!\n";
    //fd = fd + 100;
    cout << "Before calling it is " << fd << "\n";
    VMFileWrite(fd ,b,&Length);

    cout << "OMGGGGGGGGGGGGG\n";
   // cout << "The size of the vector is " << data_clusters.size() << "\n";
    for(int i = 0; i < data_clusters.size(); i++)
    {
         cout << "The size of the vector is " << data_clusters.size() << "\n";
        for(int j = 0; j < data_clusters[i].cluster_data.size(); j++)
        {
           
            for(int k = 0; k < data_clusters[i].cluster_data[j].Info.size(); k+=16)
            {
                
                
                for(int p = 0; p < 16; p++){
                        printf("%02X ", data_clusters[i].cluster_data[j].Info[k+p]);
                    }
                for(int p = 0; p < 16; p++){
                        if(isprint(data_clusters[i].cluster_data[j].Info[k+p])){
                            printf("%c",data_clusters[i].cluster_data[j].Info[k+p]);
                        }
                        else{
                            printf(".");
                        }
                    }
                    printf("\n");



                /*
                for(int i = 0; i < 512; i += 16){
                    for(int j = 0; j < 16; j++){
                        printf("%02X ", cluster_data[i+j]);
                    }
                    for(int j = 0; j < 16; j++){
                        if(isprint(cluster_data[i+j])){
                            printf("%c",cluster_data[i+j]);
                        }
                        else{
                            printf(".");
                        }
                    }
                    printf("\n");
                }
                */

            }
            cout << "\n";
        }
    }
    for(int i = 0; i < 50; i++)
    {
        cout << FAT_Table[i].current << " points to " << FAT_Table[i].next << "\n";
    }
    return;
}

TVMStatus VMTickMS(int *tickmsref){
    //cout << "In tick MS\n";
    if (tickmsref == NULL){
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    *tickmsref = tick_ms;
    return VM_STATUS_SUCCESS;

}

TVMStatus VMTickCount(TVMTickRef tickref){
    //cout << "In tick count\n";
    if (tickref == NULL){
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    MachineEnableSignals();
    *tickref = tick_count;
    return VM_STATUS_SUCCESS;

}

TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
    
    if (entry == NULL || tid == NULL){
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    // cout << "Going in the size is " << Pools[1].allocated_blocks.size()  << "\n";

    SMachineContext new_context;
    uint8_t* stackaddr;

    if (int(thread_list.size() == 1))
    {
        stackaddr = (uint8_t *) (malloc(sizeof(uint8_t) * memsize));

    }
    else//Take memopry from system pool
    {
        // cout << "Made it from the other one for " << int(thread_list.size()) << "\n";
        VMMemoryPoolAllocate(1, memsize, (void**)&stackaddr);

    }

    
    *tid = int(thread_list.size());
    
    // cout << "creating thread " << *tid << "\n";
    deque<MutexControlBlock> new_mutex_queue;
    // initialize new thread control block
    ThreadControlBlock new_thread_TCB = {
        entry,
        param,
        *tid,
        prio,
        VM_THREAD_STATE_DEAD,
        new_context,
        memsize,
        stackaddr,
        -1,
        0,
        new_mutex_queue,
        NULL,
    };
    
    thread_list.push_back(new_thread_TCB);
    // cout << "Size of sys pool is " << Pools[1].allocated_blocks.size() << "\n";

    return VM_STATUS_SUCCESS;

}

TVMStatus VMThreadDelete(TVMThreadID thread){

    if (thread >= thread_list.size())
    {
        return VM_STATUS_ERROR_INVALID_ID;
    }
    else if (thread_list[thread].state != VM_THREAD_STATE_DEAD)
    {
        return VM_STATUS_ERROR_INVALID_STATE;
    }

    return VM_STATUS_SUCCESS;

}

TVMStatus VMThreadActivate(TVMThreadID thread){ // ? Disable signals?

    if (thread >= thread_list.size()){
        return VM_STATUS_ERROR_INVALID_ID;
    }
    else if (thread_list[thread].state != VM_THREAD_STATE_DEAD){
        return VM_STATUS_ERROR_INVALID_STATE;
    }
    else{
        TVMThreadState thread_priority = thread_list[thread].priority;
        thread_list[thread].state = VM_THREAD_STATE_READY; 
        ready_threads_list[thread_priority].push_back(thread_list[thread]); 

        MachineContextCreate(&thread_list[thread].context_ref, Skeleton, thread_list[thread].param, thread_list[thread].stack_ptr, thread_list[thread].memsize);
        
        if (thread_priority > thread_list[current_thread_id].priority){
            // cout << "scheduling from activate\n";
            Schedule();

        }
    }
    
    return VM_STATUS_SUCCESS;

}

TVMStatus VMThreadTerminate(TVMThreadID thread){

    // CHANGE

    /* VMThreadTerminate()terminates the thread specified by 
    threadparameter in the virtual machine. After termination the 
    thread enters the state VM_THREAD_STATE_DEAD, and must release
    any mutexes that it currently holds. The termination of a thread 
    an trigger another thread to be scheduled. */

    // cout << "WE ARE IN TERMINATEEEEE\n";

    if (thread == 1)
    {
        return VM_STATUS_SUCCESS;
    }
    
    if (thread >= thread_list.size()){
        return VM_STATUS_ERROR_INVALID_ID;
    }
    
    if (thread_list[thread].state == VM_THREAD_STATE_DEAD){
        return VM_STATUS_ERROR_INVALID_STATE;
    }

    TVMThreadPriority thread_prio = thread_list[thread].priority;
    deque<ThreadControlBlock> thread_prio_queue = ready_threads_list[thread_prio];
    int prio_size = thread_prio_queue.size();

    thread_list[thread].state = VM_THREAD_STATE_DEAD;

    for (int i = 0; i < prio_size; i++){
        if (thread_prio_queue[i].threadID == thread){
            ready_threads_list[thread_prio].erase(ready_threads_list[thread_prio].begin() + i);
            break;
        }
    }

        // release mutexes that the thread owns
    deque<MutexControlBlock> thread_mutex_list = thread_list[thread].thread_mutex_list;
    int thread_mutex_list_size = thread_mutex_list.size();
    for (int j = 0; j < thread_mutex_list_size; j++){
        mutex_list[thread_mutex_list[j].mutexID].owner_thread_ID = NULL;
    }

    Schedule();

    return VM_STATUS_SUCCESS;

}

TVMStatus VMThreadID(TVMThreadIDRef threadref){

    if (threadref == NULL){
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    *threadref = current_thread_id;
    return VM_STATUS_SUCCESS;

}

TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref){
    
    if (thread >= thread_list.size()){
        return VM_STATUS_ERROR_INVALID_ID;
    }
    if (stateref == NULL){
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    switch (thread_list[thread].state) {
        case VM_THREAD_STATE_DEAD:
            *stateref = VM_THREAD_STATE_DEAD;
            return VM_STATUS_SUCCESS;
        case VM_THREAD_STATE_WAITING:
            *stateref = VM_THREAD_STATE_WAITING;
            return VM_STATUS_SUCCESS;
        case VM_THREAD_STATE_READY:
            *stateref = VM_THREAD_STATE_READY;
            return VM_STATUS_SUCCESS;
        case VM_THREAD_STATE_RUNNING:
            *stateref = VM_THREAD_STATE_RUNNING;
            return VM_STATUS_SUCCESS;
        default: 
            return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    return VM_STATUS_SUCCESS;
}

TVMStatus VMThreadSleep(TVMTick tick){

    if (tick == VM_TIMEOUT_INFINITE){
        if (thread_list[current_thread_id].wait_mutexID != NULL){ // ? 
            return VM_STATUS_SUCCESS;
        } 
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    else if (tick == VM_TIMEOUT_IMMEDIATE){
        thread_list[current_thread_id].state = VM_THREAD_STATE_READY;

        ready_threads_list[thread_list[current_thread_id].priority].push_back(thread_list[current_thread_id]);

        Schedule();

        return VM_STATUS_SUCCESS;
    }

    thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
    thread_list[current_thread_id].rem_sleep = tick;
    sleeping_threads_list.push_back(thread_list[current_thread_id]);
    Schedule();

    return VM_STATUS_SUCCESS;
    
}

int openCluster()
{
    for(int i = 0; i < FAT_Table.size(); i++)
    {
        if (FAT_Table[i].next = 0)
        {
            return i;
        }
    }
    return -1;
}

TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor){
    
    if (filedescriptor == NULL|| filename == NULL){
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    cout << "The flags areeeee " << flags << "\n";
    if (flags && O_CREAT)
    {
        cout << "1sttttt\n";

    } 
    if (flags && O_TRUNC)
    {
        cout << "2nddddd\n";
    }
    if (flags && O_RDWR)
    {
        cout << "3rddddd\n";
    }

    bool found = false;
    int index;
    string name = string(filename);
    cout << "Looking for " << name << "\n";
    for (int i = 0; i < root_entries.size(); i++)
    {
        int k = 0;//K is for the name input
        int j = 0;//J is for the one in the root directory
        int count = 0;
        cout << "We are going to " << string(root_entries[i].shortName) << "\n";
        string root_name = string(root_entries[i].shortName);
        cout << "STRING NOW is " << root_name << "\n";
        while (k < name.length() && j < 11)
        {

            cout << "Comparing "<< root_name[j] << " and " << name[k] << "\n";
            if (root_name[j] == 32)
            {
                j++;//Do not increment k
                continue;
            }
            if (name[k] == 46)
            {
                k++;
                count++;
                continue;
            }
            if (root_name[j] == name[k])//Skip periods
            {
                cout << "Equal and count is "<< count << "\n";
                count++;
                if (count == name.length())//Getting rid of period/extension characters
                {
                    index = i;
                    found = true;
                    break;
                }
            }
            j++;
            k++;
        }
        if (found)
        {
            break;
        }
    }//Check to see if its already in the root directory
    cout << "Out of that loop\n";
    if (!found)
    {
        cout << "Not found\n";
        RootEntry New;
        SVMDateTime Time;
        New.free = false;
        //New.shortName = new char[11];
        for(int i = 0; i < 11 && i < name.length(); i++)
        {
            New.shortName[i] = name[i];
        }
        New.read_only = false; //???
        New.is_dir = false;
        New.filesize = false;
        New.first_cluster_number = openCluster();
        New.dirty = false;
        New.offset = 0;
        New.seek = false;
        New.pointer_offset = 0;
        VMDateTime(&Time);
        index = root_entries.size();
        root_entries.push_back(New);
        //Just make a SVMDATETIME for each root entry and parse before copying it back into the FAT image

        //uint16_t creation_date;
        //uint16_t creation_time;
        //uint16_t last_modify_date;
        //uint16_t last_modify_time;
        //uint16_t last_access_date;
        // longEntries
        //Maybe when we open we load al clusters of file into mem

    }

    int current = root_entries[index].first_cluster_number;
    cout << "USing this one for the calcs: " << string(root_entries[index].shortName) << "\n";
    *filedescriptor = index;
    int next;

    //uint8_t *cluster_data = (uint8_t *) malloc(sizeof(uint8_t) * sec_per_clus * 512);
    if (found)
    {
        cout << "FOUND\n";
        cout << "first cluster is " << current << "\n";
        cout << "DATA starts at byte " << data_begin<< "\n"; 
        cout << "THis is sector " << data_begin / 512 << "\n";
        cout << "FD is " << fat_fd << "\n";
        int sec_count = data_begin / 512;
        cout << "The size of this file is " << root_entries[index].filesize << "\n";
        int clusterID = 0;
        bool finished = false;
        TMachineSignalStateRef sigset = NULL;
        //uint8_t *cluster_data = (uint8_t *) malloc(sizeof(uint8_t) * sec_per_clus * 512);
        //uint8_t *cluster_data = (uint8_t *) malloc(sizeof(uint8_t) * 512);
        //uint8_t *original = cluster_data;
        MachineSuspendSignals(sigset);
        while (current != -1)
        {
            uint8_t *cluster_data;
            vector<Sector> Data;
         
            for(int i = 0; i < FAT_Table.size(); i++)
            {
                if (FAT_Table[i].current == current)
                {
                    next = FAT_Table[i].next;
                    if (next == -1)
                    {
                        finished = true;
                    }
                    break;

                }
            }//Get Next 
            cout << "Current cluster is " << current << " and next is " << next << "\n";
            //Seek to the cluster before reading
            //MachineSuspendSignals(sigset);
            void *calldata = &thread_list[current_thread_id].threadID;
            int cluster_location = data_begin + ((current - 2) * (sec_per_clus * 512));//Offset from 0
            //Added the -2
            int loc = cluster_location / 512;
            cout << "Data begin is " << data_begin << "\n";
            cout << "cluster location is " << cluster_location << "\n";
            //fat_fd;
            //Seek to the cluster
            MachineFileSeek(fat_fd, cluster_location, SEEK_SET, VMModCallback, calldata);
            thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
            Schedule();
            //MachineFileRead(filedescriptor, new_data, 512, VMCallback, calldata);
            cout << "THE SEEK RETURNED " << thread_list[current_thread_id].processData << "\n";
            for (int k = 0; k < sec_per_clus; k++)
            {
                Sector Temp;
                Data.push_back(Temp);//The index of it is k 
                cout << "This is sector " << loc + k << "\n";
                //uint8_t *cluster_data = (uint8_t *) malloc(sizeof(uint8_t) * (sec_per_clus * 512));
                
                VMMemoryPoolAllocate(0, 512, (void**)&cluster_data);
                MachineFileRead(fat_fd, cluster_data, 512, VMModCallback, calldata); //Read the actual cluster
                thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
                Schedule();
                cout << "Read is " << thread_list[current_thread_id].processData << "\n";
                //cout << (char*)cluster_data << "\n";
                //cout << "Returned and got this\n";
                /*
                for(int i = 0; i < 512; i += 16){
                    for(int j = 0; j < 16; j++){
                        printf("%02X ", cluster_data[i+j]);
                    }
                    for(int j = 0; j < 16; j++){
                        if(isprint(cluster_data[i+j])){
                            printf("%c",cluster_data[i+j]);
                        }
                        else{
                            printf(".");
                        }
                    }
                    printf("\n");
                }
                */
                for(int i = 0; i < 512; i++)
                {
                    Data[k].Info.push_back(cluster_data[i]);
                }
                Data[k].ID = loc + k; //Put its sector number in there
               // cout << "\n";
                //cluster_data = cluster_data + 512;
            }//Loop to read a sector at a time
            cout << "--------------------------------\n";
            //cout << cluster_data << "\n";
            
            

            //memcpy(cluster_data, &data[data_begin + data_index], sec_per_clus * 512); // changed from &clusterdata to clusterdata
            Cluster new_cluster = {
                current,
                Data,
                false, // may change
            };
            //cout << "CLuster is this: \n";
            //cout << (char*)cluster_data << "\n";
            data_clusters.push_back(new_cluster);
            VMMemoryPoolDeallocate(0,cluster_data);
            current = next;
            //clusterID++;
        }   
        MachineResumeSignals(sigset);
    }
    else
    { //Do this actually
        vector<Sector> Data;
        Cluster new_cluster = {
                current,
                Data,
                false, // may change
        };
        data_clusters.push_back(new_cluster);

    }



  /*  
    MachineSuspendSignals(sigset);
    void *calldata = &thread_list[current_thread_id].threadID;
    MachineFileOpen(filename, flags, mode, VMModCallback, calldata);
    thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
    Schedule(); 
    MachineResumeSignals(sigset);
    *filedescriptor = thread_list[current_thread_id].processData;
*/
    if (*filedescriptor < 0){
        return VM_STATUS_FAILURE;
    }
    
    return VM_STATUS_SUCCESS;
}
TVMStatus VMFileClose(int filedescriptor){

    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);
    void *calldata = &thread_list[current_thread_id].threadID;
    MachineFileClose(filedescriptor, VMCallback, calldata);
    thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
    Schedule(); 
    MachineResumeSignals(sigset);

    return VM_STATUS_SUCCESS;
}   

TVMStatus MainRead(int filedescriptor, void *data, int *length){
	cout << "in vmfileread\n";
    if (data == NULL || length == NULL)
    {
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);


    if (*length > 512)
    {
        int times = *length / 512; 
        int left_over = *length % 512; 
        int j = 1;

        for(; j <= times; j++)
        {
            uint8_t* new_data;
        
            VMMemoryPoolAllocate(0, 512, (void**)&new_data);
            void *calldata = &thread_list[current_thread_id].threadID;
    
            MachineFileRead(filedescriptor, new_data, 512, VMCallback, calldata);
        
            thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
            Schedule();

            memcpy(data, new_data, 512);

            VMMemoryPoolDeallocate(0, new_data);
            data = data + 512;
        }


        uint8_t* new_data;
        
        VMMemoryPoolAllocate(0, left_over, (void**)&new_data);
        void *calldata = &thread_list[current_thread_id].threadID;
        
        MachineFileRead(filedescriptor, new_data, left_over, VMCallback, calldata);
        thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
        Schedule();

        memcpy(data, new_data, left_over);

        VMMemoryPoolDeallocate(0, new_data); 


     
    }
    else
    {
        cout << "less than or equal to 512 bytes\n";

        uint8_t* new_data;
        
        VMMemoryPoolAllocate(0, *length, (void**)&new_data);
        
        void *calldata = &thread_list[current_thread_id].threadID;
    
        MachineFileRead(filedescriptor, new_data, *length, VMCallback, calldata);

        thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;

        Schedule();

        *length = thread_list[current_thread_id].processData;

        cout << "length is " << *length << "\n";

        memcpy(data, new_data, thread_list[current_thread_id].processData);

        VMMemoryPoolDeallocate(0, new_data); 

    }
    
    MachineResumeSignals(sigset);

	return VM_STATUS_SUCCESS;
}
void offsetIndex(int* cluster, int* sector, int*byte, int fd, int original_offset)
{
    cout << "Initial offset is " << root_entries[fd].pointer_offset << "\n";
    int counter = 0;
    int offset = original_offset;
    int cluster_bytes = sec_per_clus * 512; //Num of bytes per cluster
    while(offset > cluster_bytes)
    {
        offset = offset - cluster_bytes;
        counter++;
    }//Get the index of which cluter this offset is at 
    *cluster = counter;
    offset = original_offset;
    int sectors_occ = offset / 512; //Number of sectors that it takes up
    int bit = offset % 512;
    /*
    if (bit > 0)
    {
        sectors_occ++;
    }//If there is a remainder then it starts on the next sector
    */
    *sector = sectors_occ;
    *byte = bit;



}
TVMStatus VMFileRead(int filedescriptor, void *data, int *length){

    cout << "In FVMFileRead\n";
    cout << "fd is " << filedescriptor << "\n";
    //int total_read = 0;
    if (filedescriptor < 3){
    	MainRead(filedescriptor, data, length);
    }
   
    else{
    	// read from FAT file system
    	// all we have is the file descriptor.
    	// File : fileID is same as fd, start cluster number, use this to go to the correct first FAT
    	// then iterate through FAT to find next, go through storing pntrs
    	// then go through data clusters, reading all into data

    	vector<int> data_pointers;

    	// get start cluster from fd:
        cout << "Going to check\n";
        filedescriptor = filedescriptor - 100; //Decode the file descriptor
    	//File read_file = files_list[filedescriptor];
    	unsigned int current = root_entries[filedescriptor].first_cluster_number;
        unsigned int next;

        //Set the offset to the bottom of the file
        root_entries[filedescriptor].pointer_offset = root_entries[filedescriptor].filesize;
        
        cout << "About to loop and first is " << current << "\n";
        int num_bytes = *length;
        *length = 0;
    	// go to entry in fat table associated with the start cluster
    	// add to data pointer list
    	//FATEntry curr_entry = FAT_Table[start_cluster_ID];
    	//while(curr_entry.current < 0xFFF8){ // not EOC, see pg. 17 fatgen103
        while(current < 0xFF8){ 
            cout << "Current is " << current <<" and next is ";
    		data_pointers.push_back(current);
    		current = FAT_Table[current].next;
            cout << current << "\n";
    	}
        cout << "Found the end\n";
        for(int i = 0; i < data_pointers.size(); i++)
            {
                cout << data_pointers[i];
            }
        cout << "\n";
    	// go through data region with data pointers, read into data 
        
    	int num_ptrs = data_pointers.size();
    	int curr_cluster_num;
    	Cluster curr_cluster;
        cout << "Reading this amount: " << num_bytes << "\n";
        int cluster_index = 0;
        int sector_index = 0;
        int byte_index = 0;
        
        //C
        offsetIndex(&cluster_index, &sector_index, &byte_index, filedescriptor, root_entries[filedescriptor].pointer_offset);
    	//for(int ptr_index = 0; ptr_index < num_ptrs; ptr_index++){
        cout << "Cluster index is " << cluster_index << "\n";
        cout << "Sector index is " << sector_index << "\n";
        cout << "Byte is " << byte_index << "\n";
        cout << "List containts: ";

        cout << "\n";
        for(int ptr_index = cluster_index; ptr_index < num_ptrs; ptr_index++){
    		// find data cluster
    		curr_cluster_num = data_pointers[ptr_index];
            cout << "Cluster " << curr_cluster_num << "\n";
    		//curr_cluster = data_clusters[curr_cluster_num];
            //The indexes are not in order
            int i = 0;
            for(; i < data_clusters.size(); i++)
            {
                if (curr_cluster_num == data_clusters[i].clusterID)
                {
                    break;
                }
            }
            cout << "About to start reading\n";
            cout << "Cluster list is " << data_clusters[i].cluster_data.size() << "\n";
            
            //for(int j = 0; j < data_clusters[i].cluster_data.size(); j++)
            for(int j = sector_index; j < data_clusters[i].cluster_data.size(); j++)
            {
                cout << "Sector list is " << data_clusters[i].cluster_data[j].Info.size() << "\n";
                vector<uint8_t> content;
                int count = 0;
                //for(int k = 0; (k < (data_clusters[i].cluster_data[j].Info.size())) && (k < 512) && (num_bytes > 0); k++)
                for(int k = byte_index; (k < (data_clusters[i].cluster_data[j].Info.size())) && (k < 512) && (num_bytes > 0); k++)
                {
                    //data = data_clusters[i].cluster_data[j].Info[k];
                    //cout << "k is " << k << "\n";
                    //cout << "Size is " << data_clusters[i].cluster_data[j].Info.size() << "\n";
                    //cout << "Copying " << (int)data_clusters[i].cluster_data[j].Info[k] << "\n";
                    if (data == NULL)
                    {
                        cout << "NOO\n";
                    }
                    
                    //cout << "After \n";
                    content.push_back(data_clusters[i].cluster_data[j].Info[k]);
                    //!!!!!!!!! uncomment thisssssss
                    //root_entries[filedescriptor].pointer_offset++; //Each time you read a byte, you move over the pointer
                    count++;
                    //data = data + 1;
                    num_bytes --;
                }
                cout << "Read a total of " << count << "\n";
                //total_read += count;
                *length = *length + count;
                uint8_t* ptr = content.data();
                memcpy(data, ptr, count);
                data = data + count;
                
            }

    		// read data from cluster into data
    		//memcpy(data, curr_cluster.cluster_data, 512);
    		//data = data + 512;
    	}
    	
    }
    //*length = total_read;

}
TVMStatus VMFileWrite(int filedescriptor, void *data, int *length){
    if (filedescriptor < 3)
    {
        TVMStatus Var = MainWrite(filedescriptor, data, length);
        return Var;

    }

    else
    {
        int num_bytes = *length;
        filedescriptor = filedescriptor - 100;
        int filesize = root_entries[filedescriptor].filesize;
        

        vector<int> data_pointers;

    	// get start cluster from fd:
        cout << "Going to check\n";
        //filedescriptor = filedescriptor - 100; //Decode the file descriptor
    	//File read_file = files_list[filedescriptor];
    	unsigned int current = root_entries[filedescriptor].first_cluster_number;
        cout << "The first one is "<< current << "\n";
        cout << "The fd is " << filedescriptor << "\n";
        cout << "it is actually " << root_entries[filedescriptor].first_cluster_number << "\n";
        unsigned int next;

    	// go to entry in fat table associated with the start cluster
    	// add to data pointer list
    	//FATEntry curr_entry = FAT_Table[start_cluster_ID];
    	//while(curr_entry.current < 0xFFF8){ // not EOC, see pg. 17 fatgen103

        //If file has no allocated clusters????
        while(current < 0xFF8){ 
            cout << "Current is " << current << "\n";
    		data_pointers.push_back(current);
    		current = FAT_Table[current].next;
    	}
        //Check when data pointers is 1 but it actually has 0
        int total_bytes = data_pointers.size() * (sec_per_clus * 512); //Number of total bytes allocated for the file
        int left = total_bytes - filesize;
        cout << "Got after the loop\n";
        int cluster_index = 0;
        int sector_index = 0;
        int byte_index = 0;

        //Check if offset is at end of filesize
        if (left > num_bytes)//No need to allocate new containers
        {
           cout << "More left than size\n";

            //offsetIndex(&cluster_index, &sector_index, &byte_index, filedescriptor, filesize);
            
            offsetIndex(&cluster_index, &sector_index, &byte_index, filedescriptor, root_entries[filedescriptor].pointer_offset);//Get where you are moving teh items from


            int curr_cluster_num;
            int copy_offset = root_entries[filedescriptor].pointer_offset + num_bytes; //Where the first character will go -> Maybe get rid of the plus one
            int copy_count = 0;
            cout << "Printing\n";
            for(int i = 0; i < data_pointers.size(); i++)
            {
                cout << data_pointers[i];
            }
            cout << "\n";

            cout << "We went to offset " << root_entries[filedescriptor].pointer_offset << "\n";
            cout << "Sector i is " << sector_index << "\n";
            cout << "byte i is " << byte_index << "\n";
            cout << "clust is " << cluster_index << "\n";
            for(int ptr_index = cluster_index; ptr_index < data_pointers.size(); ptr_index++){
                curr_cluster_num = data_pointers[ptr_index];
                int i = 0;
                for(; i < data_clusters.size(); i++)
                {
                    if (curr_cluster_num == data_clusters[i].clusterID)
                    {
                        break;
                    }
                }
                cout << "About to start reading\n";
                cout << "Cluster list is " << data_clusters[i].cluster_data.size() << "\n";
                
                
                //for(int j = 0; j < data_clusters[i].cluster_data.size(); j++)
                int j = sector_index;
                for(; j < data_clusters[i].cluster_data.size(); j++)
                {
                    cout << "Sector list is " << data_clusters[i].cluster_data[j].Info.size() << "\n";
                    vector<uint8_t> content;
                    int count = 0;
                    //for(int k = 0; (k < (data_clusters[i].cluster_data[j].Info.size())) && (k < 512) && (num_bytes > 0); k++)
                    for(int k = byte_index; (k < (data_clusters[i].cluster_data[j].Info.size())) && (k < 512) && (num_bytes > 0); k++)
                    {
                       // int cluster_copy;
                       // int sector_copy;
                        //int byte_copy;

                        //offsetIndex(&cluster_copy, &sector_copy, &byte_copy, filedescriptor, copy_offset);
                        uint8_t B;
                        
                        memcpy(&B , data, 1);
                        cout << "Copying " << (char)B << "\n";
                        data = data + 1;
                        data_clusters[i].cluster_data[j].Info[k] = B;
                       
                        //data_clusters[cluster_copy].cluster_data[sector_copy].Info[byte_copy] = data_clusters[i].cluster_data[j].Info[k];
                       // uint8_t B;
                       // memcpy(&B , data, 1);
                       // data = data + 1;
                        //data_clusters[i].cluster_data[j].Info[k] = B;
                        copy_count++;
                        copy_offset++;

                        //content.push_back(data_clusters[i].cluster_data[j].Info[k]);
                        //root_entries[filedescriptor].pointer_offset++; //Each time you read a byte, you move over the pointer
                        count++;
                        //data = data + 1;
                        num_bytes --;
                    }
                    cout << "Read a total of " << count << "\n";
                    //total_read += count;
                    //*length = *length + count;
                    //uint8_t* ptr = content.data();
                   // memcpy(data, ptr, count);
                   // data = data + count;
                
                }


            }
            //Check if after you went through all the clusters there are still bytes to be read
            if (num_bytes > 0)
            {
                while (num_bytes > 0)
                {

                
                    int open = openCluster();
                    int last = data_pointers[data_pointers.size() - 1];
                    FAT_Table[last].next = open;
                    FAT_Table[open].next = -1;
                    data_pointers.push_back(open); //In case of more iterations
                    vector<Sector> Extra; //Extra cluster needed
                    for(int i = 0; i < sec_per_clus; i++)
                    {
                        Sector New;
                        Extra.push_back(New);
                    }
                    for(int j = 0; j < Extra.size(); j++)
                    {
                        for(int k = 0; k < 512 && num_bytes > 0; k++)
                        {
                            uint8_t B;
                            memcpy(&B , data, 1);
                            cout << "Copying " << (char)B << "\n";
                            data = data + 1;
                            Extra[j].Info.push_back(B);
                            num_bytes--;
                        }
                    }

                    Cluster new_cluster = {
                    open,
                    Extra,
                    false, // may change
                    };
                    //cout << "CLuster is this: \n";
                    //cout << (char*)cluster_data << "\n";
                    data_clusters.push_back(new_cluster);
                }
            }

            cout << "We wrotttte " << copy_count << "\n";
            cout << "We were given " << *length << "\n";

    	}


        
        
    }


    
    
    return VM_STATUS_SUCCESS;

}

TVMStatus MainWrite(int filedescriptor, void *data, int *length){

    // CHANGE
    /* MachineFileRead and MachineFileWrite functions require that 
    shared memory locations be used to transfer data to/from the 
    machine. In addition the maximum amount of data transferred 
    must be limited to 512 bytes.VMFileRead and VMFileWrite must
    still be able to transfer up any number of bytes specified. */

    // cout << "In write from " << current_thread_id << "\n";

    if (data == NULL || length == NULL)
    {
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);
    if (*length > 512)
    {

        cout << "Length is more than expected\n";
        int times = *length / 512; //How many iterations of 512 we have to do 
        int left_over = *length % 512; //Remainder
        int j = 1;
        for(; j <= times; j++)
        {
            uint8_t* new_data;
            //cout << "Calling it \n";
            VMMemoryPoolAllocate(0, 512, (void**)&new_data);
            void *calldata = &thread_list[current_thread_id].threadID;
            memcpy(new_data, data , 512);
            MachineFileWrite(filedescriptor, new_data, 512, VMCallback, calldata);
            if (thread_list[current_thread_id].processData < 0){
                MachineResumeSignals(sigset);
                return VM_STATUS_FAILURE;
            }
            thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
            Schedule();
            VMMemoryPoolDeallocate(0, new_data);
            data = data + 512;
            // data = data + (512 + j); // ?
        }

        uint8_t* new_data;
        //cout << "Calling it \n";
        VMMemoryPoolAllocate(0, left_over, (void**)&new_data);
        void *calldata = &thread_list[current_thread_id].threadID;
        memcpy(new_data, data, left_over);
        MachineFileWrite(filedescriptor, new_data, left_over, VMCallback, calldata);
        thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
        Schedule();
        VMMemoryPoolDeallocate(0, new_data); // ? go before schedule?


        
    }
    else
    {
        uint8_t* new_data;
        //cout << "Calling it \n";
        VMMemoryPoolAllocate(0, *length, (void**)&new_data);
        //cout << "Returned\n";
        void *calldata = &thread_list[current_thread_id].threadID;
        //memcpy(Pools[0].allocated_blocks[0].base, data, *length);
        memcpy(new_data, data, *length);
        //Pools[0].allocated_blocks[0].is_free = false;
        //cout << "ABOUT TO CALL\n";
        MachineFileWrite(filedescriptor, new_data, *length, VMCallback, calldata);
        //MachineFileWrite(filedescriptor, data, *length, VMCallback, calldata);
        thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;

        // cout << "blocking for thread " << current_thread_id << "'s file write call\n";
        Schedule();
        VMMemoryPoolDeallocate(0, new_data); // ? go before schedule?

    }

    MachineResumeSignals(sigset);
    //} 
    //MachineResumeSignals(sigset);
    //cout << "Returning from write\n";
return VM_STATUS_SUCCESS;

}

TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset){

    if (filedescriptor < 3)
    {
        TMachineSignalStateRef sigset = NULL;
        MachineSuspendSignals(sigset);

        void *calldata = &thread_list[current_thread_id].threadID;
        MachineFileSeek(filedescriptor, offset, whence, VMModCallback, calldata);
        thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
        Schedule(); 
        MachineResumeSignals(sigset);

        if (newoffset != NULL)
        {
            *newoffset = thread_list[current_thread_id].processData;
        }

        return VM_STATUS_SUCCESS;
    } //If the fd is less than 3, then do it as before
    else//If this is a file that is in the FAT image
    {
        vector<int> data_pointers;

    	// get start cluster from fd:
        cout << "Going to check\n";
        filedescriptor = filedescriptor - 100; //Decode the file descriptor
        if (filedescriptor >= root_entries.size())
        {
            return VM_STATUS_FAILURE;
        }




    	//File read_file = files_list[filedescriptor];
    	int current = root_entries[filedescriptor].first_cluster_number;
        int next;

        while(current != -1){ 
    		data_pointers.push_back(current);
    		current = FAT_Table[current].next;
    	}
        //Maybe it has zero allocated???
        int total_bytes = data_pointers.size() * (sec_per_clus * 512);

        if ((whence + offset) > root_entries[filedescriptor].filesize && (whence + offset) > total_bytes)
        {
            return VM_STATUS_FAILURE;
        }
        else
        {
            root_entries[filedescriptor].pointer_offset = whence + offset;
            root_entries[filedescriptor].seek = true;
            *newoffset = whence + offset;
        }
        /*
        int cluster_bytes = sec_per_clus * 512; //Number of bytes in one cluster
    	//File read_file = files_list[filedescriptor];
    	int current = root_entries[filedescriptor].first_cluster_number;
        int next;
        
        cout << "About to loop and first is " << current << "\n";
    	// go to entry in fat table associated with the start cluster
    	// add to data pointer list
    	//FATEntry curr_entry = FAT_Table[start_cluster_ID];
    	//while(curr_entry.current < 0xFFF8){ // not EOC, see pg. 17 fatgen103
        while(current != -1){ 
    		data_pointers.push_back(current);
    		current = FAT_Table[current].next;
    	}
        cout << "Found the end\n";
        int file_bytes = data_pointers.size() * cluster_bytes;
        */


    }

}

void VMModCallback(void *calldata, int result){

    TVMThreadID * ref = (TVMThreadID *) calldata;
    thread_list[*ref].state = VM_THREAD_STATE_READY;
    thread_list[*ref].processData = result; 
    ready_threads_list[thread_list[*ref].priority].push_back(thread_list[*ref]);
    Schedule();
    return;

}

void VMCallback(void *calldata, int result){

    TVMThreadID * ref = (TVMThreadID *) calldata;
    thread_list[*ref].state = VM_THREAD_STATE_READY;
    thread_list[*ref].processData = result;
    ready_threads_list[thread_list[*ref].priority].push_back(thread_list[*ref]);
    // cout << "Called schedule from callback for " << *ref << "\n";

    Schedule();
    return;

}
 
 void Schedule(){

     // cout << "In schedule from " << current_thread_id << "\n";
     for (int i = 3; i >= 0; i--)
     {
         int numerator = ready_threads_list[i].size();
         if (numerator > 0) 
         {
             TVMThreadID oldID = current_thread_id; 
             current_thread_id = ready_threads_list[i].front().threadID;
             thread_list[current_thread_id].state = VM_THREAD_STATE_RUNNING;
             if (current_thread_id != oldID)
             {
                 if (i != 0) 
                 {
                     ready_threads_list[i].pop_front();
                     if (thread_list[oldID].state ==  VM_THREAD_STATE_RUNNING && oldID != 1)
                     {
                        thread_list[oldID].state =  VM_THREAD_STATE_READY;
                         
                        ready_threads_list[thread_list[oldID].priority].push_back(thread_list[oldID]);
                         
                     }
                 }
                 actual_entry = thread_list[current_thread_id].entry;
                 // cout << "About to switch to " << current_thread_id << "\n";
                 // cout << "Size of the alloc is " << Pools[1].allocated_blocks.size() << "\n";
                 // cout << "THE SIZE OF THE POOOOOLS IOS "<<  Pools.size() << "\n";
                 for (int i = 0; i < (int) Pools[1].allocated_blocks.size(); i++)
                 {
                    // cout << "Block " << i << " is " << Pools[1].allocated_blocks[i].size << "\n";
                 }
                 MachineContextSwitch(&thread_list[oldID].context_ref, &thread_list[current_thread_id].context_ref);
                 // cout << "Returned right away\n";
                 return;

             }

         }

     }
     
     return;

 }

void infiniteLoop(void *param){

    bool done = false;
    int counter = 0;
    MachineEnableSignals();

    while (!done)
    {
        counter++;
    }
    
    return;

}

void Skeleton(void *param){

    // cout << "IN SKELETON\n";
    MachineEnableSignals();
    actual_entry(param); 
    VMThreadTerminate(current_thread_id);
    
    return;

}

void AlarmCallback(void *calldata){

    int sleeping_size = sleeping_threads_list.size();
    tick_count++; 
    vector<int> to_delete;
    bool is_ready_updated = false;
    TVMMutexID mutexID;
    TVMThreadID sleep_threadID;
    // cout << "In callback\n";
    for (int i = 0; i < sleeping_size; i++){ 
        mutexID = sleeping_threads_list[i].wait_mutexID;
        sleep_threadID = sleeping_threads_list[i].threadID;
        if (mutexID != NULL){
            // thread is sleeping because it is waiting for a mutex
            // check if the mutex is available or not
            // todo: set a max # ticks a thread can be skipped over waiting for a mutex
            if (mutex_list[mutexID].owner_thread_ID == NULL){
                // mutex is unlocked, give it to the current thread if it at the front of the queue

                if (mutex_list[mutexID].waiting_queue.front().threadID == sleep_threadID){
                    // if the curr sleeping thread is first in line for the free mutex...
                    // 1) NULLify the TCB's wait_mutexID
                    // 2) set MCB owner_thread_ID to curr sleeping thread
                    // 3) remove thread from list of threads waiting for mutex in the MCB
                    // 4) add mutex to the list of mutexes that threads own in the TCB
                    // 5) remove thread from sleeping list
                    // 6) set the thread to READY and add to ready queue
                    //cout << "Thread " << mutex_list[mutexID].waiting_queue.front().threadID << "is getting mutex!\n";
                    thread_list[sleep_threadID].wait_mutexID = NULL;
                    mutex_list[mutexID].owner_thread_ID = sleep_threadID;
                    mutex_list[mutexID].waiting_queue.pop_front();
                    thread_list[sleep_threadID].thread_mutex_list.push_back(mutex_list[mutexID]);
                    to_delete.push_back(i);
                    is_ready_updated = true; 
                    thread_list[sleep_threadID].state = VM_THREAD_STATE_READY;
                    ready_threads_list[sleeping_threads_list[i].priority].push_back(thread_list[sleep_threadID]);
                    continue;
                }
                // otherwise, keep waiting
            }   
        }
        sleeping_threads_list[i].rem_sleep--; 
        thread_list[sleeping_threads_list[i].threadID].rem_sleep--; 
        if (sleeping_threads_list[i].rem_sleep == 0){ 
            // cout << "In teh other if\n";
            
            is_ready_updated = true;
            thread_list[sleeping_threads_list[i].threadID].state = VM_THREAD_STATE_READY; 

            ready_threads_list[sleeping_threads_list[i].priority].push_back(thread_list[sleeping_threads_list[i].threadID]);
            
            to_delete.push_back(i);
        }

    }

    vector<TVMThreadID>inf_to_delete;

    TVMMutexID inf_mutexID;
    TVMThreadID inf_threadID;

    int inf_size = inf_sleep_threads.size();
    for (int k = 0; k < inf_size; k++){
        // cout << "In the other looop\n";
        inf_threadID = inf_sleep_threads[k].threadID;
        inf_mutexID = inf_sleep_threads[k].wait_mutexID;
        // if a thread is in inf_sleep_threads, its wait_mutexID is not NULL, no check needed
        if (mutex_list[inf_mutexID].owner_thread_ID == NULL){ // desired mutex is unlocked
            if (mutex_list[inf_mutexID].waiting_queue.front().threadID == inf_threadID){
                thread_list[inf_threadID].wait_mutexID = NULL;
                mutex_list[inf_mutexID].owner_thread_ID = sleep_threadID;
                mutex_list[inf_mutexID].waiting_queue.pop_front();
                thread_list[inf_threadID].thread_mutex_list.push_back(mutex_list[inf_mutexID]);
                inf_to_delete.push_back(k);
                is_ready_updated = true; 
                thread_list[inf_threadID].state = VM_THREAD_STATE_READY;
                ready_threads_list[inf_sleep_threads[k].priority].push_back(thread_list[inf_threadID]);
            }
            continue;
        }   
    }
    // cout << "Finally here\n";

    int to_delete_size = to_delete.size();
    for (int n = 0; n < to_delete_size; n++){
        sleeping_threads_list.erase(sleeping_threads_list.begin() + to_delete[n]);
    }
    //int inf_delete_size = inf_to_delete.size();
    for (int l = 0; l < (int)inf_to_delete.size(); l++){
        inf_sleep_threads.erase(inf_sleep_threads.begin() + inf_to_delete[l]);
    }

    if (ready_threads_list[thread_list[current_thread_id].priority].size() > 0 || is_ready_updated){
        Schedule();
    }

    return;
}


// PROJECT 3: Memory Pools and Mutexes

TVMStatus VMMemoryPoolCreate(void *base, TVMMemorySize size, TVMMemoryPoolIDRef memory){
    /* VMMemoryPoolCreate() creates a memory pool from an array of 
    memory. The base and size of the memory array are specified by 
    base and size parameters respectively. The memory pool identifier 
    is put into the location specified by the memory parameter. */

    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);

    if (base == NULL || memory == NULL || size == 0){
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }


    TVMMemoryPoolID new_poolID = Pools.size();
    // uint8_t* ptr = (uint8_t *) (malloc(sizeof(uint8_t) * size));
    // base  = ptr;
    uint8_t* ptr = (uint8_t*)base; // ?

    *memory = new_poolID;

    vector<Block> free_blocks;
    vector<Block> alloc_blocks;


    Block first_block = {
        true,
        ptr,
        size,
    };

    free_blocks.push_back(first_block);

    MemoryPoolControlBlock new_pool = {
        true, 
        ptr,
        new_poolID,
        // size - 32, // pointer takes 32 bits (4 bytes), size is in bits
        // size - 32,
        size,
        size,
        free_blocks,
        alloc_blocks,
    };

    Pools.push_back(new_pool);
    MachineResumeSignals(sigset);
    /* Upon successful creation of the memory pool, 
    VMMemoryPoolCreate() will return VM_STATUS_SUCCESS. If the base
    or memory are NULL, or size is zero VM_STATUS_ERROR_INVALID_PARAMETER 
    is returned. */
    return VM_STATUS_SUCCESS;
}

TVMStatus VMMemoryPoolDelete(TVMMemoryPoolID memory){
    /* VMMemoryPoolDelete() deletes a memory pool that has no memory 
    allocated from the pool. The memory pool to delete is specified 
    by the memory parameter. */
    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);

    if (memory >= Pools.size() || Pools[memory].is_alive == false){
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    else if (Pools[memory].allocated_blocks.size() > 0){
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_STATE;
    }
    
    Pools[memory].is_alive = false;
    MachineResumeSignals(sigset);

    /* Upon successful deletion of the memory pool, VMMemoryPoolDelete() 
    will return VM_STATUS_SUCCESS. If the memory pool specified by memory 
    is not a valid memory pool, VM_STATUS_ERROR_INVALID_PARAMETER is 
    returned. If any memory has been allocated from the pool and not 
    deallocated, then VM_STATUS_ERROR_INVALID_STATE is returned. */
    return VM_STATUS_SUCCESS;
}

TVMStatus VMMemoryPoolQuery(TVMMemoryPoolID memory, TVMMemorySizeRef bytesleft){
    /* VMMemoryPoolQuery() queriesa memory pool for the available 
    memory left inthe pool. The memory pool to query is specified by 
    the memoryparameter.The space left unallocated in the memory pool 
    is placed in the location specified by bytes left.*/
    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);

    if (bytesleft == NULL || memory >= Pools.size() || Pools[memory].is_alive == false)
    {
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    // cout << "bytes left are " << Pools[memory].bytes_left << "\n";
    *bytesleft = Pools[memory].bytes_left;

    MachineResumeSignals(sigset);

    /* Upon successful queryingof the memory pool, VMMemoryPoolQuery()
     will return VM_STATUS_SUCCESS. If the memory pool specified by 
     memoryis not a valid memory poolor bytesleftis NULL, 
     VM_STATUS_ERROR_INVALID_PARAMETER is returned. */
    return VM_STATUS_SUCCESS;
}

TVMStatus VMMemoryPoolAllocate(TVMMemoryPoolID memory, TVMMemorySize size, void **pointer){
    /* VMMemoryPoolAllocate() allocates memory from the memory pool. 
    he memory pool to allocate from is specified by the memory 
    parameter. The size of the allocation is specified by size and 
    the base of the allocated array is put in the location specified by 
    pointer.The allocated size will be rounded to the next multiple of 
    64 bytes that is greater than or equal to the sizeparameter. */

    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);

    if (size == 0 || pointer == NULL || memory >= Pools.size() || Pools[memory].is_alive == false){
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    bool status = Allocate(memory, size, pointer);

    if (!status){
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INSUFFICIENT_RESOURCES;
    }

    MachineResumeSignals(sigset);
     // if (!status)
     // {
         // cout << "THERE WAS AN ERROR " << size <<  "but we have " << Pools[1].bytes_left << "\n";
     // }
    /* Upon successful allocation from the memory pool, 
    VMMemoryPoolAllocate() will return VM_STATUS_SUCCESS. 
    If the memory pool specified by memory is not a valid memory pool, 
    size is zero, or pointer is NULL, VM_STATUS_ERROR_INVALID_PARAMETER 
    is returned. If the memory pool does not have sufficient memory 
    to allocate the array of size bytes, 
    VM_STATUS_ERROR_INSUFFICIENT_RESOURCES is returned. */
    return VM_STATUS_SUCCESS;
}

bool Allocate(TVMMemoryPoolID pool_index, TVMMemorySize size, void **pointer){

    if (Pools[pool_index].bytes_left < size){
        return false;
    }

    if (size % 64 != 0){
        int size_div_64 = size / 64; 
        int factor_64 = size_div_64 + 1;
        size = 64 * factor_64;
    }
    //MemoryPoolControlBlock pool = Pools[pool_index];
    //vector<Block> free_blocks = pool.free_blocks;
    // cout << "Iterating for size " << Pools[pool_index].free_blocks.size() << "\n";
    for (int i = 0; i < Pools[pool_index].free_blocks.size(); i++){
        if (Pools[pool_index].free_blocks[i].is_free == true && Pools[pool_index].free_blocks[i].size >= size){

            *pointer = Pools[pool_index].free_blocks[i].base; // ?
            // cout << "Bytes is " << Pools[pool_index].bytes_left;

            Pools[pool_index].bytes_left = Pools[pool_index].bytes_left - size; // update bytes left

            Block new_block = {
                false, // is_free
                Pools[pool_index].free_blocks[i].base, // base
                size,
            };

            if (Pools[pool_index].free_blocks[i].size  == size){   
                Pools[pool_index].free_blocks.erase(Pools[pool_index].free_blocks.begin() + i); // erase block from free_blocks
            }
            else{
                Pools[pool_index].free_blocks[i].base = Pools[pool_index].free_blocks[i].base + size;
                Pools[pool_index].free_blocks[i].size = Pools[pool_index].free_blocks[i].size - size;
            }
            Pools[pool_index].allocated_blocks.push_back(new_block); // add to list of allocated blocks in MemoryPoolBlock
            return true;
        }
    }
    return false; // unable to allocate memory
}

TVMStatus VMMemoryPoolDeallocate(TVMMemoryPoolID memory, void *pointer){
    /* VMMemoryPoolDeallocate() deallocates memory to the memory pool. 
    The memory pool to deallocate tois specified by the memory parameter. 
    The base of the previously allocated array is specified by pointer. */
    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);

    if (pointer == NULL || memory >= Pools.size() || Pools[memory].is_alive == false){
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    else if (pointer < Pools[memory].memory || pointer >= Pools[memory].memory + Pools[memory].size){
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    // vector<Blockk> alloc_blocks_list = Pools[memory].allocated_blocks
    // int alloc_blocks_list_size = alloc_blocks_list.size();
    // for (int j = 0; j < alloc_pools_list_size; j++){
    //     if (alloc_blocks_list[j].base == pointer){

    //     }
    // }


    // cout << "Deallocatinggggggggg\n";
    bool free_blocks_around = false;
    bool found_block = false;

    for (int i = 0; i < Pools[memory].allocated_blocks.size(); i++)
    {
        if (pointer == Pools[memory].allocated_blocks[i].base)
        {
            found_block = true;

            int allocated_size = Pools[memory].allocated_blocks[i].size;
            Pools[memory].bytes_left += allocated_size;
            for (int j = 0; j < (int)Pools[memory].free_blocks.size(); j++)
            {
                int size = Pools[memory].free_blocks[j].size;
                if (Pools[memory].free_blocks[j].base + size  == pointer)
                {
                    // cout << "Reached the first if\n";
                    free_blocks_around = true;
                    Pools[memory].free_blocks[j].size += allocated_size;
                    Pools[memory].allocated_blocks.erase(Pools[memory].allocated_blocks.begin() + i);


                    for (int k = 0; k < (int)Pools[memory].free_blocks.size(); k++)
                    {
                        if (k == j)
                        {
                            continue;
                        }
                        else if (Pools[memory].free_blocks[j].base + Pools[memory].free_blocks[j].size == Pools[memory].free_blocks[k].base)
                        {
                            Pools[memory].free_blocks[j].size += Pools[memory].free_blocks[k].size;
                            Pools[memory].free_blocks.erase(Pools[memory].free_blocks.begin() + k);
                            break;

                        }
                    }

                    // break;

                }
                else if (Pools[memory].allocated_blocks[i].base + Pools[memory].allocated_blocks[i].size == Pools[memory].free_blocks[j].base)
                {
                    free_blocks_around = true;
                    // cout << "We are in the else\n";
                    Pools[memory].free_blocks[j].base = Pools[memory].allocated_blocks[i].base;
                    Pools[memory].free_blocks[j].size += Pools[memory].allocated_blocks[i].size;
                    Pools[memory].allocated_blocks.erase(Pools[memory].allocated_blocks.begin() + i);

                    // break;

                }
                break;
            }

            if (!free_blocks_around)
            {
                // cout << "None of the statements happened\n";
                Pools[memory].allocated_blocks[i].is_free = true;
                // cout << "First\n";
                Pools[memory].free_blocks.push_back(Pools[memory].allocated_blocks[i]);
                // cout << "Second\n";
                Pools[memory].allocated_blocks.erase(Pools[memory].allocated_blocks.begin() + i);
                // cout << "We out\n";

            }

            break;

        }

    }

    if (!found_block){
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    MachineResumeSignals(sigset);
    /* Upon successful deallocation from the memory pool, 
    VMMemoryPoolDeallocate() will return VM_STATUS_SUCCESS. If the 
    memory pool specified by memory is not a valid memory pool, or 
    pointer is NULL, VM_STATUS_ERROR_INVALID_PARAMETER is returned. 
    If pointer does not specify a memory location that was previously 
    allocated from the memory pool, VM_STATUS_ERROR_INVALID_PARAMETER 
    is returned. */
    return VM_STATUS_SUCCESS;
}       

TVMStatus VMMutexCreate(TVMMutexIDRef mutexref){ // should disable signals before calling
    /* VMMutexCreate()creates a mutex in the virtual machine. Once 
    created the mutex is in the unlocked state. The mutex identifier 
    is put into the location specified by the mutexref parameter. */
    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);

    if (mutexref == NULL){
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    TVMMutexID new_mutex_ID = mutex_list.size(); 
    deque<ThreadControlBlock> new_waiting_list; // list of waiting threads

    // cout << "created mutex " << new_mutex_ID << "\n";
    MutexControlBlock new_mutex = {
        true, // is_alive
        new_mutex_ID, // mutexID
        NULL, // owner_thread_ID; NULL --> unlocked
        new_waiting_list, // waiting_queue
    };

    mutex_list.push_back(new_mutex); 
    *mutexref = new_mutex_ID; //'The mutex identifier is put into the location specified by the mutexref parameter'

    MachineResumeSignals(sigset);
    /* Upon successful creation of the thread VMMutexCreate() returns 
    VM_STATUS_SUCCESS. VMMutexCreate() returns 
    VM_STATUS_ERROR_INVALID_PARAMETER if either mutexref is NULL. */
    return VM_STATUS_SUCCESS;
}

TVMStatus VMMutexDelete(TVMMutexID mutex){ // should disable signals before calling
    // VMMutexDelete()deletes the unlocked mutex specified by 
    // mutexparameter from the virtual machine. 
    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);

    if (mutex >= mutex_list.size() || mutex_list[mutex].is_alive == false){
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_ID;
    }
    else if (mutex_list[mutex].owner_thread_ID != NULL){
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_STATE; // mutex is currently held by thread, locked
    }

    mutex_list[mutex].is_alive = false;

    // mutex_list.erase(mutex_list.begin() + mutex); // erasing things from mutex_list will nullfiy meaning of thread_ID as index

    MachineResumeSignals(sigset);
    /* Upon successful deletion of the thread from the virtual 
    machine, VMMutexDelete() returns VM_STATUS_SUCCESS. If the mutex 
    specified by the threadidentifier mutex does not exist, 
    VM_STATUS_ERROR_INVALID_ID is returned. If the mutex does exist, 
    but is currently held by a thread, VM_STATUS_ERROR_INVALID_STATE is returned. */
    return VM_STATUS_SUCCESS;
}

TVMStatus VMMutexQuery(TVMMutexID mutex, TVMThreadIDRef ownerref){ // should disable signals before calling
    /* VMMutexQuery()retrieves the owner of the mutex specified by 
    mutex and places the thread identifier of owner in the location 
    specified by ownerref. If the mutex is currently unlocked, 
    VM_THREAD_ID_INVALID returned as the owner. */

    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);

    if (mutex >= mutex_list.size() || mutex_list[mutex].is_alive == false){
        return VM_STATUS_ERROR_INVALID_ID;
    }
    else if (ownerref == NULL){
        MachineResumeSignals(sigset);

        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (mutex_list[mutex].owner_thread_ID == NULL){
        *ownerref = VM_THREAD_ID_INVALID;
    }
    else{
        *ownerref = mutex_list[mutex].owner_thread_ID;
    }
    MachineResumeSignals(sigset);

    /* Upon successful querying of the mutex owner from the virtual 
    machine, VMMutexQuery() returns VM_STATUS_SUCCESS. If the mutex 
    specified by the mutex identifier mutexdoes not exist, 
    VM_STATUS_ERROR_INVALID_IDis returned. If the parameter ownerref
    is NULL, VM_STATUS_ERROR_INVALID_PARAMETER is returned.*/
    return VM_STATUS_SUCCESS;
}

TVMStatus VMMutexAcquire(TVMMutexID mutex, TVMTick timeout){ // should disable signals before calling
    /* VMMutexAcquire() attempts to lock the mutex specified by 
    mutex waiting up to timeout ticks. If timeout is specified as 
    VM_TIMEOUT_IMMEDIATE the current returns immediately if the mutex 
    is already locked. If timeout is specified as VM_TIMEOUT_INFINITE
    the thread will block until the mutex is acquired.*/
    // ? Does it lock the mutex using the currently running thread?
    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);
    // cout << "in VMMutexAcquire for mutex " << mutex << " and thread " << current_thread_id << "\n";

    if (mutex > mutex_list.size() || mutex_list[mutex].is_alive == false){
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_ID;
    }

    if (timeout == VM_TIMEOUT_IMMEDIATE){
        // TODO: current returns immediately if the mutex is already locked
        if (mutex_list[mutex].owner_thread_ID != NULL){
            MachineResumeSignals(sigset);
            return VM_STATUS_FAILURE; // ? does not mention what is returned
        }
        else{ 
            // TODO: timeout is immediate and mutex_list is not already locked 
            // ? give the lock to the currently running thread
            mutex_list[mutex].owner_thread_ID = current_thread_id; // set current thread as owner of mutex
            thread_list[current_thread_id].thread_mutex_list.push_back(mutex_list[mutex]); // push mutex onto the current thread's mutex list
        }
    }
    else if (timeout == VM_TIMEOUT_INFINITE){
        /*
            thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
            thread_list[current_thread_id].wait_mutexID = mutex;
            mutex_list[mutex].waiting_queue.push_back(thread_list[current_thread_id]); // add to list of threads waiting for mutex
            inf_sleep_threads.push_back(thread_list[current_thread_id]);
            //cout << "Yessssir\n";
            Schedule();
        */ 
        // TODO: block until mutex is acquired AKA set state to waiting and schedule
        
        if (mutex_list[mutex].owner_thread_ID == NULL){
            // give mutex lock to currently running thread
            //cout << "Infinite\n";
            mutex_list[mutex].owner_thread_ID = current_thread_id;
            thread_list[current_thread_id].thread_mutex_list.push_back(mutex_list[mutex]);

        }
        else{
            thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
            thread_list[current_thread_id].wait_mutexID = mutex;
            mutex_list[mutex].waiting_queue.push_back(thread_list[current_thread_id]); // add to list of threads waiting for mutex
            inf_sleep_threads.push_back(thread_list[current_thread_id]);
            //cout << "Yessssir\n";
            Schedule();
        }
        
        // ? Do something in Alarm Callback to set it to something other than waiting?
    }   
    else{
        // TODO: normal timeout input
        //cout << "ohh noo\n";
        thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
        thread_list[current_thread_id].wait_mutexID = mutex;
        mutex_list[mutex].waiting_queue.push_back(thread_list[current_thread_id]);
        sleeping_threads_list.push_back(thread_list[current_thread_id]);
        VMThreadSleep(timeout);
    }

    MachineResumeSignals(sigset);
    //cout << "Leaving\n";
    /* Upon successful acquisition of the currently running thread, 
    VMMutexAcquire() returns VM_STATUS_SUCCESS. If the timeout 
    expires prior to the acquisition of the mutex, VM_STATUS_FAILURE 
    is returned. If the mutex specified by the mutex identifier mutex
    does not exist, VM_STATUS_ERROR_INVALID_ID is returned. */
    return VM_STATUS_SUCCESS;
}

TVMStatus VMMutexRelease(TVMMutexID mutex){ // should disable signals before calling
    /* VMMutexRelease()releases the mutex specified by the mutex 
    parameter that is currently held by the running thread. 
    Release of the mutex may cause another higher priority thread 
    to be scheduled if it acquires the newly released mutex.*/

    // cout << "releasing mutex " << mutex << "\n";

    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);

    if (mutex > mutex_list.size() || mutex_list[mutex].is_alive == false){
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_ID;
    }
    else if (mutex_list[mutex].owner_thread_ID != current_thread_id){
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_STATE;
    }

    mutex_list[mutex].owner_thread_ID = NULL;

    int thread_mutex_list_size = thread_list[current_thread_id].thread_mutex_list.size();
    for (int i = 0; i < thread_mutex_list_size; i++){
        if (thread_list[current_thread_id].thread_mutex_list[i].mutexID == mutex){
            thread_list[current_thread_id].thread_mutex_list.erase(thread_list[current_thread_id].thread_mutex_list.begin() + i);
            break;
        }
    }

    MachineResumeSignals(sigset);

    /* Upon successful release of the mutex, VMMutexRelease() returns 
    VM_STATUS_SUCCESS. If the mutex specified by the mutex identifier 
    mutex does not exist, VM_STATUS_ERROR_INVALID_ID is returned. 
    If the mutex specified by the mutex identifier mutex does exist, 
    but is not currently held by the running thread, 
    VM_STATUS_ERROR_INVALID_STATE is returned.*/
    return VM_STATUS_SUCCESS;
}

TVMStatus VMDirectoryOpen(const char *dirname, int *dirdescriptor){
    return VM_STATUS_SUCCESS;
}

TVMStatus VMDirectoryClose(int dirdescriptor){
    return VM_STATUS_SUCCESS;
}

TVMStatus VMDirectoryRead(int dirdescriptor, SVMDirectoryEntryRef dirent){
    return VM_STATUS_SUCCESS;
}

TVMStatus VMDirectoryRewind(int dirdescriptor){
    return VM_STATUS_SUCCESS;
}

TVMStatus VMDirectoryCurrent(char *abspath){
    return VM_STATUS_SUCCESS;
}

TVMStatus VMDirectoryChange(const char *path){
    return VM_STATUS_SUCCESS;
}

// Extra Credit: 
// TVMStatus VMDirectoryCreate(const char *dirname){
//     return VM_STATUS_SUCCESS;
// }

// TVMStatus VMDirectoryUnlink(const char *path){
//     return VM_STATUS_SUCCESS;
// }

