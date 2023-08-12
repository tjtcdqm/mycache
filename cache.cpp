#include<iostream>
#include<cmath>
#include<vector>
#include<string>
#include<ctime>
#include<cstdlib>
using namespace std;
#define ASSOCIATITIVIES 1//组相联
#define MAX_NUM_OF_CHECK 1100000//检索次数
#define SIZE_OF_CACHE 16384//bytes
#define SIZE_OF_BLOCK 8//bytes
#define NUM_OF_BLOCK ((SIZE_OF_CACHE)/(SIZE_OF_BLOCK))
#define NUM_OF_SET ((NUM_OF_BLOCK)/(ASSOCIATITIVIES))//组的总数
typedef bool valid_flag;
typedef bool bit;
typedef int control_write_mode;

typedef struct {
    //char data[8];
    valid_flag valid;//有效位
    control_write_mode con_write;//控制cache写入替换模式
    vector<bit> tag;//有128个块时，即2的七次方时，tag位有32-2-1-7=22位
}Block;

typedef struct{
    vector<Block> b;
}Cache;
typedef struct{
    vector<bit> offset;//字节偏移
    vector<bit> which_bytes;//字偏移
    vector<bit> index;//索引
    vector<bit> tag;//标志位
    //unsigned int bits_of_indx=log2(NUM_OF_BLOCK);
    //unsigned int bits_of_tag=32-2-log2((SIZE_OF_BLOCK)/4)-bits_of_indx;
}Address;

unsigned int change_hex_string_to_num(string& hex_string);//将一个合法16进制字符串转换为数字
void load_address(Address& add,string& hex_string);//将16进制字符串装填address
unsigned int change_index_To_num(Address& address);//将一个地址的索引变为一个数字
void check(Address& address,Cache& cache,string& control_string);//根据address检索cache中的块是否可用，不可用时，将进行写cache操作，同时定义关联度
void inital_address(Address& add);//初始化address空间，给他的每个位分配大小
void inital_cache(Cache& cache);//初始化cache
int write_for_cache(Address& address,Cache& cache,int start_of_associal_set,string& control_string);//给cache写

int main(){
    Address address;
    Cache cache;
    inital_address(address);
    inital_cache(cache);
    string alter_rule="FIFO";
    //cout<<"请输入替换规则";
    //cin>>alter_rule;
    srand(time(nullptr));
    for(int i=0;i<MAX_NUM_OF_CHECK;++i){
        string add;
        cin>>add;
        load_address(address,add);
        check(address,cache,alter_rule);
    }
    return 0;
}
void inital_address(Address& add){
    add.offset.resize(2,0);//地址必有两位字节偏移
    add.which_bytes.resize(log2((SIZE_OF_BLOCK)/4),0);//字偏移为数据块中所有的字个数，取2的对数
    add.index.resize(log2(NUM_OF_SET),0);//索引为组的总数取2的对数
    add.tag.resize(32-2-log2((SIZE_OF_BLOCK)/4)-log2(NUM_OF_SET),0);//剩下的就是标志位
}

void inital_cache(Cache& cache){
    cache.b.resize(NUM_OF_BLOCK);//一个cache有多少数据块
    for(int i=0;i<NUM_OF_BLOCK;++i){
        cache.b[i].valid=0;//有效位
        cache.b[i].con_write=0;
        cache.b[i].tag.resize(32-2-log2((SIZE_OF_BLOCK)/4)-log2(NUM_OF_SET),0);//cache块的标志位
    }
}

void check(Address& address,Cache& cache,string& control_string){
    static unsigned int hit_on =0;//命中次数
    static unsigned int missing =0;//缺失次数
    static unsigned int total_check =0;//总共检索次数
    static control_write_mode con_LRU=0;
    ++total_check;
    int idx_of_associal_set = change_index_To_num(address);//组的索引
    int idx_of_blocks = idx_of_associal_set*ASSOCIATITIVIES;//组中第一个块的索引
    int i=0;
    int idx_control=-1;
    for(;i<ASSOCIATITIVIES;++i){
        //命中
        unsigned int n=idx_of_blocks+i;
        if(cache.b[n].valid==1&&address.tag==cache.b[n].tag){
            cout<<"bingo!!!"<<endl<<"numbers of hitting:"<<++hit_on<<endl;
            idx_control=n;
            break;
        }
    }
    //没有命中
    if(i==ASSOCIATITIVIES){
        cout<<"missing,ready to write into cache: "<<endl;
        cout<<"numbers of missing: "<<++missing<<endl;
        idx_control=write_for_cache(address,cache,idx_of_blocks,control_string);//返回写位置的下标
    }
    if(control_string=="LRU"){
        cache.b[idx_control].con_write=con_LRU++;//由于LRU规则在使用后就要更新，使用包括写和命中，所以直接在check函数管理LRU的写替换模式
    }
    cout<<"Your hitting rate is "<<hit_on*1.0/total_check<<endl<<"Your missing rate is "<<missing*1.0/total_check<<endl;;
}
int write_for_cache(Address& address,Cache& cache,int start_of_associal_set,string& control_string){
    int i=0;
    int idx=start_of_associal_set;
    int idx_control=0;
    static control_write_mode con_FIFO=0;//由于FIFO只有在写入时需要，而在使用时不需要维护，所以在写入cache函数管理FIFO,即写入先后顺序
    control_write_mode alter_mode=MAX_NUM_OF_CHECK;

    for(;i<ASSOCIATITIVIES;++i,++idx){
        //如果有空位，则直接写入，不用替换
        if(cache.b[idx].valid==0){
            cache.b[idx].valid=1;
            cache.b[idx].tag=address.tag;
            if(control_string=="FIFO"){
                cache.b[idx].con_write=con_FIFO++;
           }
           idx_control=idx;
            break;
        }
        else{
            //为可能的替换做准备
            control_write_mode num=0;
            if((num=min(cache.b[idx].con_write,alter_mode))!=alter_mode){
                //无论那个替换规则，都是找到control_write最小的位置，即组中control_write最小的位置
                alter_mode=num;
                idx_control=idx;
            }
        }
    }
    //没有空位
    if(i==ASSOCIATITIVIES){

        if(control_string=="LRU"||control_string=="FIFO"){
            cache.b[idx_control].valid=1;
            cache.b[idx_control].tag=address.tag;
        }
        if(control_string=="FIFO"){
            cache.b[idx_control].con_write=con_FIFO++;
        }
        else if(control_string=="random"){
            //随机替换不需要管理写替换规则
            int random = rand()%(ASSOCIATITIVIES);
            cache.b[start_of_associal_set+random].valid=1;
            cache.b[start_of_associal_set+random].tag=address.tag;
        }
    }
    return idx_control;
}

void load_address(Address& address,string& hex_string){
    unsigned int add=change_hex_string_to_num(hex_string);
    for(auto it=address.offset.rbegin();it!=address.offset.rend();++it){
        //cout<<(add&1);
        *it = add&1;
        add=add>>1;
    }
    for(auto it=address.which_bytes.rbegin();it!=address.which_bytes.rend();++it){
        *it = add&1;
        add=add>>1;
    }
    for(auto it=address.index.rbegin();it!=address.index.rend();++it){
        *it = add&1;
        add=add>>1;
    }
    for(auto it=address.tag.rbegin();it!=address.tag.rend();++it){
        *it = add&1;
        add=add>>1;
    }
}

unsigned int change_hex_string_to_num(string& hex_string){
    int n=hex_string.size();
    unsigned int res=0;
    int epx=1;
    for(int i=n-1;i>=0;--i){
        char digit = hex_string[i];
        if(digit>='a'&&digit<='f'){
            res+=epx*(digit-'a'+10);
	}
        else{
            res+=epx*(digit-'0');
        }
        epx*=16;
    }
    return res;
}

unsigned int change_index_To_num(Address& address){
    int epx = 1;
    unsigned int res=0;
    int n=address.index.size();
    for(int i=n-1;i>=0;--i){
        res+=address.index[i]*epx;
        epx*=2;
    }
    return res;
}
