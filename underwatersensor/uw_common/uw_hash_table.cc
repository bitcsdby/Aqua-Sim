
#include "tclcl.h"
#include "uw_hash_table.h"



void UW_Hash_Table::PutInHash(int addr)
{
  bool exist=false;
  int index=0;
  for (int i=0;i<current_index;i++){
    if(table[i].node==addr) {
      index=i;
      exist=true;
    }
  }

  if(exist) table[index].num++;     
  else {
    table[current_index].node=addr;
    table[current_index].num=1;
    current_index++;
  }
}

int 
UW_Hash_Table::node(int index)
{
  return table[index].node;
}

int 
UW_Hash_Table::number(int index)
{
  return table[index].num;
}
