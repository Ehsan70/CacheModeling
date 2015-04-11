# CacheModeling
This assignment uses SimpleScalar to analyze miss rate for Direct Mapped , 4-way Set Associative and 8-way Set Associative caches.  

## Part (a): Instruction Cache
Model the following two instruction cache configurations: 
*a direct-mapped cache with 32KB capacity and 64-byte blocks;  
*a 4-way set associative cache with 32KB capacity and 32-byte blocks and LRU replacement. 

##Part (b): Data Cache
Model an 8-way set associative data cache with 16KB capacity and 64-byte blocks, assuming a writeback
policy with write allocate for store instructions and LRU replacement. Measure the miss rate for load
instructions (measured as the number of load misses divided by the number of load instructions executed)
and store instructions (measured as the number of store instruction misses divided by the number of store
instructions executed); also measure the ratio of the total number of writeback events (recall a writeback
can be triggered by either a loads or a store) divided by the total number of store instructions.


##Part (c): Next Block Instruction Prefetch
Extend the set associative instruction cache in part (a)(ii) to prefetch the next consecutive block into the
instruction cache when an instruction cache access misses. 

# How to run the code? 
* Clone the code and 'make' the files.
* Go to any of the folders (./go , ./vpr , ./gcc , ./fppp)
* Execute the command that is saved on README file in each file. 
  * One way is to directly send the command to bash, by executing bash README   

