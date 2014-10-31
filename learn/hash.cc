#include <stdio.h>
#include <ha0ha.h>

/**
 * 实验InnoDB中Hash表的使用
 *
 * 从标准输入流中读取一个整数
 * 如果这个整数在Hash表中，就将其删除
 * 如果不在Hash表中，就将其删除
 */

/**
 * Hash表所存储的数据结构，里面必须有一个成员，数据类型为HashNode的指针
 * 这是因为InnoDB的哈希表使用链地址法来解决Hash冲突
 */
struct HashNode {
  int _val;
  HashNode * _hash_next;
};

int space_id = 0; //用作Hash表的标识

int main() {

  int val;
  hash_table_t * h = hash_create(4096); //创建Hash表

  if (h == NULL) {
    return 1;
  }

  while (scanf("%d", &val) > 0) {

    HashNode * node = NULL;
    //在Hash表中查找，将结果放到node指针中
    HASH_SEARCH(_hash_next, h, space_id, HashNode*, node, ut_ad(1), node->_val == val);
    if (node) {
      printf("Found %d, now delete it\n", val);
      //将记录从Hash表中删除
      HASH_DELETE(HashNode, _hash_next, h, space_id, node);
      delete node;
    } else {
      node = new HashNode();
      node->_val = val;
      printf("Insert %d\n", val);
      //将记录添加到Hash表中
      HASH_INSERT(HashNode, _hash_next, h, space_id, node);
    }
  }

  //销毁Hash表
  hash_table_free(h);

  //销毁Hash表的时候，并不会销毁Hash表所存的指针。这里由于程序即将退出，所以不再释放
  return 0;
}
