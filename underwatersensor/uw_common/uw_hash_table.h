

#ifndef ns_uw_hash_table_h
#define ns_uw_hash_table_h

#include "config.h"
#include "tclcl.h"

#define TABLE_SIZE 20

struct value_record{
  int node;
  int num;
};


class UW_Hash_Table {
 public:
  UW_Hash_Table()
{
  current_index=0;
for (int i=0;i<TABLE_SIZE;i++)
{
table[i].node=-1;
 table[i].num=-1;
  }  
}

  int current_index;
  void PutInHash(int addr);
int  node(int);
int  number(int);
  value_record table[TABLE_SIZE];
};



#endif
