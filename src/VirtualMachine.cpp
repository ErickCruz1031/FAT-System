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
void ParseDateTimeTest();
void ParseDateTime(SVMDateTimeRef datetime_ref, unsigned short date, unsigned short time, unsigned int hundredth);
void AddStdinFiles();
TVMStatus MainRead(int filedescriptor, void *data, int *length);
void VMCallback(void *calldata, int result);
void VMModCallback(void *calldata, int result);
bool Allocate(TVMMemoryPoolID pool_index, TVMMemorySize size, void **pointer);
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
    // vector<uint8_t> Info;
    uint8_t *Info;
    // int sectorID;
 };
 struct FATEntry 
{
    uint16_t current;
    uint16_t next;
};
struct RootEntry
{
    bool free;
    char shortName[VM_FILE_SYSTEM_SFN_SIZE];
    char longName[VM_FILE_SYSTEM_MAX_PATH]; // offset 0, bytes 11
    bool read_only;
    bool is_dir;
    unsigned char attributes;
    unsigned int filesize;
    uint16_t first_cluster_number;
    bool dirty;
    int offset;
    SVMDateTime create_datetime; 
    SVMDateTime access_datetime; 
    SVMDateTime modify_datetime;

    // longEntries

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

struct File
{
    int fileID;
    int start_cluster_ID;
    // int dir_cluster_ID;
    string abs_path;
    int size;
    // creation_date;
    // last_modified;
    // last_accessed;
    // dirty bit
    // RD_only
};
struct Directory {
    int directoryID;
    string abs_path;
    vector<Directory> dir_path;
    int start_cluster_ID;
    vector<RootEntry> entries;
    bool open;
    int curr_read_entry;
};

vector<Directory> dir_list;
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

// vector<string> cwd; 
string cwd;
int cwd_offset;
File curr_dir;

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

    ParseFAT();
    AddStdinFiles();
    

    // return VM_STATUS_SUCCESS;

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
        // cout << temp << "\n";
        i+= 2;
        count +=1;
    }
    // cout << "FAT size is " << fat_size << "\n";

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
    
    // uint8_t *shortName = (uint8_t *) malloc(sizeof(uint8_t) * 11);
    bool free;
    bool is_dir;
    bool read_only;
    int filesize;
    uint8_t dirty_bit;
    bool dirty;
    uint8_t attribute;
    uint16_t first_cluster_number;
    // short creation_date;
    // short creation_time;
    // short write_date;
    // short write_time;
    unsigned short creation_date;
    unsigned short creation_time;
    unsigned short last_modify_date;
    unsigned short last_modify_time;
    unsigned short last_access_date;
    uint8_t create_time_hundredth;

    //update globals
    root_begin = (1 + num_fat * fat_size) * 512; // changed
    data_begin = root_begin + (root_entry_cnt * 32);

    int parse_root_begin = num_fat * fat_size * 512;
    // int parse_data_begin = parse_root_begin + (root_entry_cnt * 32);

    cout << "root begin: " << root_begin << "\n";
    cout << "data begin: " << data_begin << "\n";

    //for (int j = num_fat * fat_size * 512; j < bytes_to_read - num_data_sectors * 512; j+=32){
    int k = 0;
    int root_count = 0;
    bool long_entry = true;
    while (root_count < root_entry_cnt){
        char shortName[VM_FILE_SYSTEM_SFN_SIZE];
        char longName[VM_FILE_SYSTEM_MAX_PATH];
        memcpy(shortName, &data[parse_root_begin + k], 11);
        memcpy(&attribute, &data[parse_root_begin + k + 11], 1);
/*
        if (attribute && 0x01 | attribute && 0x02 | attribute && 0x04 | attribute && 0x08){
            short_entry_offset += 32; 
            continue;
        }
*/      
        
        if (long_entry == true)
        {
            if (shortName[0] & 0x40) // if bit 6 is set
            {
                long_entry = false; 

                k+=32;
                root_count+=1;
                // cout << "name: " << (char *)shortName << "\n";
                // cout << "end\n";
                continue;
            }
            else
            {
                //ong_entry = false;
                k+=32;
                root_count+=1;
                // cout << "name: " << (char *)shortName << "\n";
                // cout << "hi\n";
                continue;

            }
            
        }
        cout << "name: " << (char *)shortName << "\n";

        // char name[11];
        if (shortName[0] == 0x00){
            free = true;
            break; 
        }
        else{
            free = false;
         //    char * new_name = new char [11];
            // memcpy(new_name, shortName, 11);

            

            // for (int i = 0; i < 11; i++){
            //     cout << "letter: " << shortName[i] << "\n";
            // }
            // for (int l = 0; l < 11; l++){

            //  memcpy(&name[l], &shortName[l], 1);
            //  // cout << "name[" << l << "]: " << shortName[l] << "\n";
            //  // cout << "ascii: " << (int) shortName[l] << "\n";
            // }
           // cout << "name: " << shortName << "\n";
            
        }

        

        memcpy(&attribute, &data[parse_root_begin + k + 11], 1);
        read_only = attribute && 0x1;
        is_dir = attribute && 0x10;
        memcpy(&filesize, &data[parse_root_begin + k + 28], 4);
        memcpy(&first_cluster_number, &data[parse_root_begin + k + 26], 2);
        memcpy(&creation_date, &data[parse_root_begin + k + 16], 2);
        memcpy(&creation_time, &data[parse_root_begin + k + 14], 2);
        memcpy(&last_modify_date, &data[parse_root_begin + k + 24], 2);
        memcpy(&last_modify_time, &data[parse_root_begin + k + 22], 2);
        memcpy(&last_access_date, &data[parse_root_begin + k + 18], 2);
        memcpy(&create_time_hundredth, &data[parse_root_begin + k + 13], 1);
        
        SVMDateTime create_datetime;
        SVMDateTime access_datetime;
        SVMDateTime modify_datetime;

        unsigned int create_hundredth = (unsigned int)create_time_hundredth;
        ParseDateTime(&create_datetime, creation_date, creation_time, create_hundredth);
        ParseDateTime(&access_datetime, last_access_date, last_modify_time, 0);
        ParseDateTime(&modify_datetime, last_modify_date, last_modify_time, 0);

        // if (!free){
            // cout << "shortName: " << shortName << "\n";
            // cout << "filesize: " << filesize << "\n";
            // if (is_dir){
            //  cout << "is_dir: true\n";
            // }
            // else{
            //  cout << "is_dir: false\n";
            // }
            // cout << "first_cluster_number: " << first_cluster_number << "\n";
            // cout << "creation date: " << creation_date << "\n";
            // cout << "creation time: " << creation_time << "\n";
            // cout << "last_modify_date: " << last_modify_date << "\n";
            // cout << "last_modify_time: " << last_modify_time << "\n";
            // cout << "last access date: " << last_access_date << "\n";
            
        // }

        dirty = false; // TODO: may need to change

        // RootEntry new_root_entry = {
        //     free,
        //     shortName,
        //     longName,
        //     read_only,
        //     is_dir,
        //     attribute,
        //     filesize,
        //     first_cluster_number,
        //     dirty,
        //     k, 
        //     create_datetime, 
        //     access_datetime,
        //     modify_datetime,
        //     // bool free;
        //     // char *shortName; // offset 0, bytes 11
        //     // bool read_only;
        //     // bool is_dir;
        //     // int filesize;
        //     // short first_cluster_number;
        //     // uint8_t dirty;
        //     // int offset;
        // };

        RootEntry new_root_entry;

        memcpy(longName, new_root_entry.longName, 256);
        memcpy(longName, new_root_entry.shortName, 13);
        new_root_entry.free = free;
        // new_root_entry.shortName = shortName;
        // new_root_entry.longName = longName;
        new_root_entry.read_only = read_only,
        new_root_entry.is_dir = is_dir;
        new_root_entry.attributes = attribute;
        new_root_entry.filesize = filesize;
        new_root_entry.first_cluster_number = first_cluster_number;
        new_root_entry.dirty = dirty;
        new_root_entry.offset = k;
        new_root_entry.create_datetime = create_datetime;
        new_root_entry.access_datetime = access_datetime;
        new_root_entry.modify_datetime = modify_datetime;



        root_entries.push_back(new_root_entry);
        long_entry = true;
        k+=32;
        root_count+=1;


    }

    // cout << "hi\n";
    cwd.push_back('/'); // add root directory
    cwd.push_back('\0');

    // char *hello = new char[5];
    // VMDirectoryCurrent(hello);

    // cout << "after call: " << hello << "\n";

    // Parse Data ?
    

    // DirEntry curr_entry;

    // vector<string> path_name;
    // string root_name ("/");
    // path_name.push_back(root_name);

    // for (int root_index = 0; root_index < root_entry_cnt; root_index++){
    //  curr_entry = root_entries[root_index];
    //  dir_list[0].entries.push_back(curr_entry);
    //  if (curr_entry.is_dir){
    //      RecurseDirectories(curr_entry, root_name);
    //  }
        
    // }    

    // go through the subdirectories

    // uint8_t *cluster_data = (uint8_t *) malloc(sizeof(uint8_t) * sec_per_clus * 512);

    // int clusterID = 0;
    // for (int data_index = data_begin; data_index < num_total_sectors * 512; data_index += sec_per_clus * 512){
    //  uint8_t *cluster_data = (uint8_t *) malloc(sizeof(uint8_t) * sec_per_clus * 512);
    //  memcpy(cluster_data, &data[data_begin + data_index], sec_per_clus * 512); // changed from &clusterdata to clusterdata
    //  Cluster new_cluster = {
    //      clusterID,
    //      cluster_data,
    //      false, // may change
    //  };
    //  data_clusters.push_back(new_cluster);
    //  clusterID++;
    // }
    
    return;
}

void ParseDateTimeTest(){
    unsigned short date = (unsigned short) 0b0000100100001101;
    unsigned short time = (unsigned short) 0b0000100011100001;

    SVMDateTime datetime;
    ParseDateTime(&datetime, date, time, 0);
    printf("%04d/%02d/%02d", datetime.DYear, datetime.DMonth, datetime.DDay);

    return;
}
void ParseDateTime(SVMDateTimeRef datetime_ref, unsigned short date, unsigned short time, unsigned int hundredth){
    // parse the date into month, day, year
    unsigned int year;
    unsigned char month;
    unsigned char day;
    year = (unsigned int) (date & 0b1111111);
    month = (unsigned char) ((date >> 7) & 0b1111);
    day = (unsigned char) ((date >> 11) & 0b11111);

    // parse the time into hour, minute, second, hundredth
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
    unsigned char hundred;

    hour = (unsigned char) (time & 0b11111);
    minute = (unsigned char) ((time >> 6) & 0b111111);
    second = (unsigned char) ((time >> 11) & 0b11111);

    datetime_ref->DYear = year;
    datetime_ref->DMonth = month;
    datetime_ref->DDay = day;
    datetime_ref->DHour = hour;
    datetime_ref->DMinute = minute;
    datetime_ref->DSecond = second;
    datetime_ref->DHundredth = (unsigned char) hundred;

}

/*

void RecurseDirectories(DirEntry curr_entry, vector<string> path_name){
    string filename(curr_entry.shortName);
    int dirID = dir_list.size();
    
    vector<int> data_pointers;

    FATEntry curr_fat = FAT_Table[first_cluster_number];

    while (curr_entry.current < 0xFFF8){
        data_pointers.push_back(curr_fat.current);
        curr_fat = FAT_Table[curr_fat.next];
    }

    int num_data_pointers = data_pointers.size(); // also equals to the number of clusters
    // int num_bytes_in_dir = num_data_pointers * sec_per_clus * 512;
    int bytes_per_clus = 512 * sec_per_clus;
    int curr_ptr;
    int actual_cluster_num;
    int cluster_byte_offset;
    int new_offset;
    int length;
    int num_entries = bytes_per_clus / 32; // ?

    bool free;
    bool is_dir;
    bool read_only;
    int filesize;
    uint8_t dirty_bit;
    bool dirty;
    uint8_t attribute;
    uint16_t first_cluster_number;
    uint16_t creation_date;
    uint16_t creation_time;
    uint16_t last_modify_date;
    uint16_t last_modify_time;
    uint16_t last_access_date;

    vector<DirEntry> dir_entries;

    bool done;
    bool long_entry;
    for(int i = 0; i < num_data_pointers; i++){
        uint8_t *dir_data = (uint8_t *) malloc(sizeof(uint8_t) * sec_per_clus * 512);
        curr_ptr = data_pointers[i];
        actual_cluster_num = curr_ptr - 2;
        cluster_byte_offset = data_begin + actual_cluster_num * bytes_per_clus; // bytes past beginning of image
        VMFileSeek(fat_fd, cluster_byte_offset, 0, &new_offset);
        length = bytes_per_clus;
        MainRead(fat_fd, dir_data, &length);

        // with current cluster, read 32-bit entries
        // for (int j = 0; j < num_entries; j++){

        // }
        done = false;
        
        DirEntry newEntry; 
        
        long_entry = false;

        for(int i = 0; i < root_entry_cnt ; i+= 32){
            // uint8_t *shortName = new uint8_t[11];
            // memcpy(shortName, &dir_data[i], 11);

            // if (shortName[0] == 0xE00){
            //  free = true;
            // }
            // else{
            //  free = false;

            // }

            // memcpy(&read_nly)
            uint8_t *shortName = new uint8_t[11];

            memcpy(shortName, &dir_data[i], 11);
    /*
            if (attribute && 0x01 | attribute && 0x02 | attribute && 0x04 | attribute && 0x08){
                short_entry_offset += 32; 
                continue;
            }
        *
            if (long_entry == true)
            {
                if (shortName[0] & 0x40)
                {
                    long_entry = false;
                    k+=32;
                    // root_count+=1;
                    continue;
                }
                else
                {
                    //ong_entry = false;
                    k+=32;
                    // root_count+=1;
                    continue;

                }
                
            }

            // char name[11];
            if (shortName[0] == 0x00){
                free = true;
                break;
            }
            else{
                free = false;
             //    char * new_name = new char [11];
                // memcpy(new_name, shortName, 11);
                cout << "name: " << (char *)shortName << "\n";
                

                for (int i = 0; i < 11; i++){
                    cout << "letter: " << shortName[i] << "\n";
                }
                // for (int l = 0; l < 11; l++){

                //  memcpy(&name[l], &shortName[l], 1);
                //  // cout << "name[" << l << "]: " << shortName[l] << "\n";
                //  // cout << "ascii: " << (int) shortName[l] << "\n";
                // }
               // cout << "name: " << shortName << "\n";
                
            }

            

            memcpy(&attribute, &data[i + 11], 1);
            read_only = attribute && 0x1;
            is_dir = attribute && 0x10;
            memcpy(&filesize, &data[i + 28], 4);
            memcpy(&first_cluster_number, &data[i + 26], 2);
            memcpy(&creation_date, &data[i + 16], 2);
            memcpy(&creation_time, &data[i + 14], 2);
            memcpy(&last_modify_date, &data[i + 24], 2);
            memcpy(&last_modify_time, &data[i + 22], 2);
            memcpy(&last_access_date, &data[i + 18], 2);


            // if (!free){
                // cout << "shortName: " << shortName << "\n";
                // cout << "filesize: " << filesize << "\n";
                // if (is_dir){
                //  cout << "is_dir: true\n";
                // }
                // else{
                //  cout << "is_dir: false\n";
                // }
                // cout << "first_cluster_number: " << first_cluster_number << "\n";
                // cout << "creation date: " << creation_date << "\n";
                // cout << "creation time: " << creation_time << "\n";
                // cout << "last_modify_date: " << last_modify_date << "\n";
                // cout << "last_modify_time: " << last_modify_time << "\n";
                // cout << "last access date: " << last_access_date << "\n";
                
            // }

            dirty = false; // TODO: may need to change

            DirEntry new_entry = {
                free,
                (char *)shortName,
                read_only,
                is_dir,
                filesize,
                first_cluster_number,
                dirty,
                i, 
                creation_date, 
                creation_time, 
                last_modify_date,
                last_modify_time,
                last_access_date,

            }

            dir_entries.push_back(new_entry);
        }

    }       

    vector<string> add_to_path = path_name.push_back(filename);
    Directory new_dir = {
        dirID,
        add_to_path,
        curr_entry.start_cluster_ID,
        dir_entries,
    }

    dir_list.push_back(new_dir);

    int num_dir_entries = dir_entries.size();
    for (int dir_index = 0; dir_index < num_dir_entries; dir_index++){
        if (dir_entries[dir_index].is_dir){
            RecurseDirectories(dir_entries[dir_index], add_to_path);
        }
    }
}

*/


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

TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor){
    
    if (filedescriptor == NULL|| filename == NULL){
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }

    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);
    void *calldata = &thread_list[current_thread_id].threadID;
    MachineFileOpen(filename, flags, mode, VMModCallback, calldata);
    thread_list[current_thread_id].state = VM_THREAD_STATE_WAITING;
    Schedule(); 
    MachineResumeSignals(sigset);
    *filedescriptor = thread_list[current_thread_id].processData;

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

TVMStatus VMFileRead(int filedescriptor, void *data, int *length){

    if (filedescriptor < 3){
        MainRead(filedescriptor, data, length);
    }
    else{
        // read from FAT file system
        // all we have is the file descriptor.
        // File : fileID is same as fd, start cluster number, use this to go to the correct first FAT
        // then iterate through FAT to find next, go through storing pntrs
        // then go through data clusters, reading all into data

        TMachineSignalStateRef sigset = NULL;
        MachineSuspendSignals(sigset);

        vector<int> data_pointers;

        // get start cluster from fd:
        File read_file = files_list[filedescriptor];
        int start_cluster_ID = read_file.start_cluster_ID;

        // go to entry in fat table associated with the start cluster
        // add to data pointer list
        FATEntry curr_entry = FAT_Table[start_cluster_ID];
        while(curr_entry.current < 0xFFF8){ // not EOC, see pg. 17 fatgen103
            data_pointers.push_back(curr_entry.current);
            curr_entry = FAT_Table[curr_entry.next];
        }

        // go through data region with data pointers, read into data 
        int num_ptrs = data_pointers.size();
        int curr_cluster_num;
        Cluster curr_cluster;
        for(int ptr_index = 0; ptr_index < num_ptrs; ptr_index++){
            // find data cluster
            curr_cluster_num = data_pointers[ptr_index];
            for(int i = 0; i < data_clusters.size(); i++){
                if (data_clusters[i].clusterID == curr_cluster_num){
                    curr_cluster = data_clusters[i];
                }
            }
            
            // TODO: change to put data into a vector of sectors
            memcpy(data, curr_cluster.cluster_data[ptr_index].Info, 512); //?
            data = data + 512;
        }
        // *length = length;
        MachineResumeSignals(sigset);
        return VM_STATUS_SUCCESS;
        
    }

}

TVMStatus VMFileWrite(int filedescriptor, void *data, int *length){

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
    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);
    // iterate through files, 
    if (dir_list.size() >= 1){ // already created
        MachineResumeSignals(sigset);
        return VM_STATUS_FAILURE; // should only have root directory 
    }
    cout << "opening directory with dirname " << dirname << "\n";
    string str_dirname(dirname);
    int str_dirname_size = str_dirname.size();
    char *abs_path = (char *) malloc (sizeof(char) * (cwd.size() + str_dirname_size + 1));
    if (VM_STATUS_SUCCESS == VMFileSystemGetAbsolutePath(abs_path, cwd.c_str(), dirname)){
        if (abs_path != "/"){
            cout << "path isn't /\n";
            MachineResumeSignals(sigset);
            return VM_STATUS_FAILURE;
        }
        else{
            cout << "path is /";
            // vector<string> root_path;
            // root_path.append("/");
    
            // cwd = root_begin;
            // change to be more generic if doing extra credit
            vector<Directory> root_dir_vec;
            Directory root = {
                0,
                cwd,
                root_dir_vec,
                0, // may change 
                root_entries,
                false,
                0,
            };

            *dirdescriptor = 0;

            dir_list.push_back(root);
        }
    }   
    else{
        cout << "problem resolving path\n";
        MachineResumeSignals(sigset);
        return VM_STATUS_FAILURE;
    }

    MachineResumeSignals(sigset);
    return VM_STATUS_SUCCESS;
}

TVMStatus VMDirectoryClose(int dirdescriptor){
    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);

    int dir_size = dir_list.size();
    if (dirdescriptor >= dir_size){
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    Directory curr_directory = dir_list[dirdescriptor];
    int curr_entry_index = curr_directory.curr_read_entry;
    
    dir_list[dirdescriptor].open = false;

    MachineResumeSignals(sigset);
    return VM_STATUS_SUCCESS;
}

TVMStatus VMDirectoryRead(int dirdescriptor, SVMDirectoryEntryRef dirent){
    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);
    cout << "calling vm directory read\n";

    //TODO check if there is nothing left to read
    int dir_size = dir_list.size();
    if (dirdescriptor >= dir_size){
        cout << "not a valid dirdescriptor\n";
        return VM_STATUS_ERROR_INVALID_ID;
    }
    Directory curr_directory = dir_list[dirdescriptor];
    int curr_entry_index = curr_directory.curr_read_entry;
    vector<RootEntry> entries = curr_directory.entries;
    RootEntry curr_entry = entries[curr_entry_index];
    if (curr_entry_index >= entries.size()){
        cout << "nothing left to read\n";
        return VM_STATUS_FAILURE; 
    }
    if (!curr_directory.open){
        cout << "failed b/c trying to read before opening directory";
        return VM_STATUS_FAILURE;
    }

    // char short_char_array[13];
    // char long_char_array[256];
    // string short_str(curr_entry.shortName);
    // string long_str(curr_entry.longName);
    // short_str.push_back('\0');
    // long_str.push_back('\0');
    // const char *short_char_ptr = short_str.c_str();
    // const char *long_char_ptr = long_str.c_str();
    // strncpy(short_char_array, short_char_ptr, 12);
    // strncpy(long_char_array, long_char_ptr, 255);
    // long_char_array[255] = '\0';
    // short_char[12] = '\0';
/*
    SVMDirectoryEntry new_svm_dir = {
        curr_entry.longName, // TODO
        curr_entry.shortName,
        curr_entry.filesize, 
        curr_entry.attributes,
        curr_entry.create_datetime, 
        curr_entry.access_datetime,
        curr_entry.modify_datetime, 
    };
    */

    SVMDirectoryEntry new_svm_dir;
    memcpy(new_svm_dir.DLongFileName, curr_entry.longName, 13);
    
    //new_svm_dir.DLongFileName = curr_entry.longName;
    memcpy(new_svm_dir.DShortFileName,curr_entry.shortName,  256);
    //new_svm_dir.DShortFileName = curr_entry.shortName;
     new_svm_dir.DSize = curr_entry.filesize;
     new_svm_dir.DAttributes = curr_entry.attributes;
     new_svm_dir.DCreate = curr_entry.create_datetime;
     new_svm_dir.DAccess = curr_entry.access_datetime;
     new_svm_dir.DModify = curr_entry.modify_datetime;


    // dir_list[dirdescriptor].curr_read_entry++;

    *dirent = new_svm_dir;

    return VM_STATUS_SUCCESS;

}

TVMStatus VMDirectoryRewind(int dirdescriptor){
    cout << "calling vm directory rewind\n";
    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);
    int dir_size = dir_list.size();
    if (dirdescriptor >= dir_size){
        cout << "directory doesn't exist\n";
        MachineResumeSignals(sigset);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    Directory curr_dir = dir_list[dirdescriptor];
    if (!curr_dir.open){
        cout << "directory hasn't been opened yet\n";
        MachineResumeSignals(sigset);
        return VM_STATUS_FAILURE;
    }
    dir_list[dirdescriptor].curr_read_entry = 0; // rewind marker for the next entry to read back to 0

    MachineResumeSignals(sigset);
    return VM_STATUS_SUCCESS;
}

TVMStatus VMDirectoryCurrent(char *abspath){
    // int cwd_size = cwd.size();
    // string new_string;
    // for (int i = 0; i < cwd_size; i++){
    //  new_string = new_string + cwd[i];
    // }  


    // cout << "cwd: " << cwd << "\n";
    for (int i = 0; i < cwd.size(); i++){
        abspath[i] = cwd[i];
    }
    

    
    // cout << "current directory is " << abspath << "\n";
    return VM_STATUS_SUCCESS;
}

TVMStatus VMDirectoryChange(const char *path){
    TMachineSignalStateRef sigset = NULL;
    MachineSuspendSignals(sigset);
    cout << "calling VMDirectoryChange\n";
    // string new_path(path);
    // int path_len = new_path.size();
    // cwd.clear();
    // string path_dir;
    // for (int i = 0; i < path_len; i++){
    //  if (path[i] == "/"){
    //      if (i == 0){ // root dir
    //          cwd.push_back("/"); // root dir
    //          continue;
    //      }
    //      cwd.push_back(path_dir);
    //      path_dir.clear();
    //  }
    //  else{
    //      path_dir.push_back(path[i]);
    //  }
        
    // }
 //    return VM_STATUS_SUCCESS;

    // check if we change to the root directory. If we do, then the code works
    // 1) resolve to absolute path first:
    string str_path(path);
    int str_path_len = str_path.size();
    int cwd_len = cwd.size();
    char *abs_path = (char *)malloc(sizeof(char) * (str_path_len + cwd_len + 1));
    cout << "finished with malloc for space for absolute path\n";
    if (VM_STATUS_SUCCESS == VMFileSystemGetAbsolutePath(abs_path, cwd.c_str(), path)){
        if (abs_path != "/"){
            cout << "path is not /\n";
            MachineResumeSignals(sigset);
            return VM_STATUS_FAILURE; 
        }
        else{
            cout << "path is /\n";
        }
    }
    else {
        cout << "failed to resolve path\n";
        MachineResumeSignals(sigset);
        return VM_STATUS_FAILURE;
    }

    MachineResumeSignals(sigset);
    return VM_STATUS_SUCCESS;

}

// Extra Credit: 
// TVMStatus VMDirectoryCreate(const char *dirname){
//     return VM_STATUS_SUCCESS;
// }

// TVMStatus VMDirectoryUnlink(const char *path){
//     return VM_STATUS_SUCCESS;
// }

