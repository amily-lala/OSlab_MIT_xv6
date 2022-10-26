struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;    // device num
  uint blockno; // 缓存的磁盘块号
  struct sleeplock lock; 
  uint refcnt; // 引用次数
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE]; // content
  
  uint recent_used_time; //最近被访问时间
  int bucketNum;  // 所属桶号
};

