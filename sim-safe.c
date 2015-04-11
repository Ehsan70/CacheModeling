/* sim-safe.c - sample functional simulator implementation */
	
/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "options.h"
#include "stats.h"
#include "sim.h"


#define  DIRECT_MAPPED_CACHE TRUE
#define  SET_ASSOCIATIVE_CACHE FALSE


static counter_t g_icache_miss;
static counter_t g_icache4_miss;
static counter_t g_icache8_miss;
static counter_t g_load_inst;
static counter_t g_store_inst;
static counter_t g_load_miss;
static counter_t g_store_miss;

/*
 * This file implements a functional simulator.  This functional simulator is
 * the simplest, most user-friendly simulator in the simplescalar tool set.
 * Unlike sim-fast, this functional simulator checks for all instruction
 * errors, and the implementation is crafted for clarity rather than speed.
 */

/* simulated registers */
static struct regs_t regs;

/* simulated memory */
static struct mem_t *mem = NULL;

/* track number of refs */
static counter_t sim_num_refs = 0;

/* maximum number of inst's to execute */
static unsigned int max_insts;

/*counter for instruction cache misses*/
static counter_t g_icache_miss;

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-safe: This simulator implements a functional simulator.  This\n"
"functional simulator is the simplest, most user-friendly simulator in the\n"
"simplescalar tool set.  Unlike sim-fast, this functional simulator checks\n"
"for all instruction errors, and the implementation is crafted for clarity\n"
"rather than speed.\n"
		 );

  /* instruction limit */
  opt_reg_uint(odb, "-max:inst", "maximum number of inst's to execute",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL);

}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  /* nada */
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{

	stat_reg_counter(sdb, "sim_num_icache_miss",
 			"total number of instruction cache misses",
 			&g_icache_miss, 0, NULL);
	
	stat_reg_formula(sdb, "sim_icache_miss_rate",
 			"instruction cache miss rate (percentage)",
 			"100*(sim_num_icache_miss / sim_num_insn)", NULL);

        stat_reg_counter(sdb, "sim_num_icache4_miss",
                        "total number of 4-way set-assoc cache misses",
                        &g_icache4_miss, 0, NULL);

        stat_reg_formula(sdb, "sim_icache4_miss_rate",
                        "4-way set-assoc cache miss rate (percentage)",
                        "100*(sim_num_icache4_miss / sim_num_insn)", NULL);
       /* stat_reg_counter(sdb, "sim_num_icache8_miss",
                        "total number of 8-way set-assoc cache misses",
                        &g_icache8_miss, 0, NULL);*/

	
        stat_reg_counter(sdb, "sim_num_load",
                        "total number of load instructions",
                        &g_load_inst, 0, NULL);
        stat_reg_counter(sdb, "sim_num_store",
                        "total number of store instructions",
                        &g_store_inst, 0, NULL);
        stat_reg_counter(sdb, "sim_load_miss",
                        "total number of store instructions",
                        &g_load_miss, 0, NULL);
        stat_reg_counter(sdb, "sim_store_miss",
                        "total number of store instructions",
                        &g_store_miss, 0, NULL);
        stat_reg_formula(sdb, "sim_icache8_miss_rate",
                        "8-way set-assoc cache miss rate (percentage)",
                        "100*((sim_load_miss + sim_store_miss) / sim_num_insn)", NULL);
        stat_reg_formula(sdb, "sim_load_miss_rate",
                        "load miss rate with 8-way set-assoc (percentage)",
                        "100*(sim_load_miss / sim_num_load)", NULL);
        stat_reg_formula(sdb, "sim_store_miss_rate",
                        "store miss rate with 8-way set-assoc (percentage)",
                        "100*(sim_store_miss / sim_num_store)", NULL);
        stat_reg_formula(sdb, "sim_writeback_rate",
                        "writeback ratio",
                        "((sim_num_load + sim_num_store) / sim_num_store)", NULL);


  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions executed",
		   &sim_num_insn, sim_num_insn, NULL);
  stat_reg_counter(sdb, "sim_num_refs",
		   "total number of loads and stores executed",
		   &sim_num_refs, 0, NULL);
  stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       &sim_elapsed_time, 0, NULL);
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);
  ld_reg_stats(sdb);
  mem_reg_stats(mem, sdb);
}

/* initialize the simulator */
void
sim_init(void)
{
  sim_num_refs = 0;

  /* allocate and initialize register file */
  regs_init(&regs);

  /* allocate and initialize memory space */
  mem = mem_create("mem");
  mem_init(mem);
}

/* load program into simulated state */
void
sim_load_prog(char *fname,		/* program to load */
	      int argc, char **argv,	/* program arguments */
	      char **envp)		/* program environment */
{
  /* load program text and data, set up environment, memory, and regs */
  ld_load_prog(fname, argc, argv, envp, &regs, mem, TRUE);
}

/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)		/* output stream */
{
  /* nothing currently */
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)		/* output stream */
{
  /* nada */
}

/* un-initialize simulator-specific state */
void
sim_uninit(void)
{
  /* nada */
}


/*
 * configure the execution engine
 */

/*
 * precise architected register accessors
 */

/* next program counter */
#define SET_NPC(EXPR)		(regs.regs_NPC = (EXPR))

/* current program counter */
#define CPC			(regs.regs_PC)

/* general purpose registers */
#define GPR(N)			(regs.regs_R[N])
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))

#if defined(TARGET_PISA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_L(N)		(regs.regs_F.l[(N)])
#define SET_FPR_L(N,EXPR)	(regs.regs_F.l[(N)] = (EXPR))
#define FPR_F(N)		(regs.regs_F.f[(N)])
#define SET_FPR_F(N,EXPR)	(regs.regs_F.f[(N)] = (EXPR))
#define FPR_D(N)		(regs.regs_F.d[(N) >> 1])
#define SET_FPR_D(N,EXPR)	(regs.regs_F.d[(N) >> 1] = (EXPR))

/* miscellaneous register accessors */
#define SET_HI(EXPR)		(regs.regs_C.hi = (EXPR))
#define HI			(regs.regs_C.hi)
#define SET_LO(EXPR)		(regs.regs_C.lo = (EXPR))
#define LO			(regs.regs_C.lo)
#define FCC			(regs.regs_C.fcc)
#define SET_FCC(EXPR)		(regs.regs_C.fcc = (EXPR))

#elif defined(TARGET_ALPHA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_Q(N)		(regs.regs_F.q[N])
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = (EXPR))
#define FPR(N)			(regs.regs_F.d[(N)])
#define SET_FPR(N,EXPR)		(regs.regs_F.d[(N)] = (EXPR))

/* miscellaneous register accessors */
#define FPCR			(regs.regs_C.fpcr)
#define SET_FPCR(EXPR)		(regs.regs_C.fpcr = (EXPR))
#define UNIQ			(regs.regs_C.uniq)
#define SET_UNIQ(EXPR)		(regs.regs_C.uniq = (EXPR))

#else
#error No ISA target defined...
#endif

/* precise architected memory state accessor macros */
#define READ_BYTE(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_BYTE(mem, addr))
#define READ_HALF(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_HALF(mem, addr))
#define READ_WORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_WORD(mem, addr))
#ifdef HOST_HAS_QWORD
#define READ_QWORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_QWORD(mem, addr))
#endif /* HOST_HAS_QWORD */

#define WRITE_BYTE(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_BYTE(mem, addr, (SRC)))
#define WRITE_HALF(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_HALF(mem, addr, (SRC)))
#define WRITE_WORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_WORD(mem, addr, (SRC)))
#ifdef HOST_HAS_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_QWORD(mem, addr, (SRC)))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)

#define DNA         (0)

/* general register dependence decoders */
#define DGPR(N)         (N)
#define DGPR_D(N)       ((N) &~1)

/* floating point register dependence decoders */
#define DFPR_L(N)       (((N)+32)&~1)
#define DFPR_F(N)       (((N)+32)&~1)
#define DFPR_D(N)       (((N)+32)&~1)

/* miscellaneous register dependence decoders */
#define DHI         (0+32+32)
#define DLO         (1+32+32)
#define DFCC            (2+32+32)
#define DTMP            (3+32+32)

/*
Used to represent a single cache block. Interestingly, we do not need to put actual 
instructions in the cache since we are only interested in determining the miss rate for
the cache (hence there are no fields for “data” or “instructions”)
*/
struct block {
 int m_valid; // is block valid?
 md_addr_t m_tag; // tag used to determine whether we have a cache hit
};



/*
contains a pointer to an array of blocks (called “m_tag_array”) along with some
additional fields we will use to help us determine the cache set index and the tag for each cache acc$
*/
	struct cache {
		struct block *m_tag_array;
		unsigned m_total_blocks;
 		unsigned m_set_shift;
 		unsigned m_set_mask;
 		unsigned m_tag_shift;
		unsigned m_nways;
		unsigned m_timestamp;
};



/*
This function is called with three parameters: The cache to access, the starting
address of the memory reference, and a pointer to the miss counter to update on a cache miss. 

It computes an index to select a cache block by first shifting the address to
the right by “c->m_set_shift” bits, and then bit-wise AND-ing the result with “c->m_set_mask”. 
*/
void cache_access( struct cache *c, unsigned addr, counter_t *miss_counter )
{
	 unsigned index, tag;
 	index = (addr>>c->m_set_shift)&c->m_set_mask;
 	tag = (addr>>c->m_tag_shift);
 	assert( index < c->m_total_blocks );
 	if(!(c->m_tag_array[index].m_valid&&(c->m_tag_array[index].m_tag==tag))) {
 		*miss_counter = *miss_counter + 1;
 		c->m_tag_array[index].m_valid = 1;
 		c->m_tag_array[index].m_tag = tag;
 }
}

void set_assoc_cache( struct cache *c, unsigned addr, counter_t *miss_counter, unsigned timestamp, int blocksize){

	unsigned index, tag, i;
	unsigned hit = 0;
	int LRU[c->m_nways];
	int blockLRU = 0;
        index = (addr>>c->m_set_shift)&c->m_set_mask;
        tag = (addr>>c->m_tag_shift);
        assert( index < c->m_total_blocks );

	for(i=0; i<c->m_nways; i++){//for loop to iterate over the 4 block locations in each set
	//we use i*blocksize to offset into the array to the start of each of these blocks.
	//there are 4 blocks per set, so we search the first block of each set, and then the
	//second block of each set and so on. 
		if((c->m_tag_array[index+i*blocksize].m_valid&&(c->m_tag_array[index+i*blocksize].m_tag==tag))){
			hit = 1; //set hit to 1 so it doesn't enter the if statement below
			c->m_timestamp = timestamp; //update timestamp
		}
		else{
			LRU[i] = timestamp; //LRU[1-4] corresponds to block locations 1-4. we set each cache with a timestamp when it misses to calculate the last used to help with eviction later
		}
	}

	if( hit==0){ // made it through the for loop with no hit, so a miss in the cache
		*miss_counter += 1; // update miss counter, either load or store. 
		//*icache8_miss += 1; // update icache 8 miss counter
		for( i=0; i<c->m_nways-1; i++){
			if(LRU[i] < LRU[i+1]) // compare block timestamps for least recently used
				blockLRU = i; // block i(1-4) was the last recently used
		}
		//use blockLRU as well as the current index to find the block to evict from that set.
		c->m_tag_array[index+blockLRU*blocksize].m_valid = 1; 
		c->m_tag_array[index+blockLRU*blocksize].m_tag = tag;
	
	}
}
	

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  md_inst_t inst;
  register md_addr_t addr;
  enum md_opcode op;
  register int is_write;
  enum md_fault_type fault;

	/* Allocates memory for an instruction cache and initializes it to all zeros*/
  struct cache *icache = (struct cache *) calloc( sizeof(struct cache), 1 );
  
	/* Allocates 512 blocks for our cache and again initializes their contents to all zeros.
	We need 512 blocks since each block is 64 bytes and the total capacity of the cache is 32KB. 	*/
	icache->m_tag_array = (struct block *) calloc( sizeof(struct block), 512 );

  /*Records how many blocks there are for assertion checking purposes (see the “assert()” 
	statement in “cache_access()”).*/
	icache->m_total_blocks = 512;
  
	/*Since each block is 64B, we need to shift the address by 6 bits. */
	icache->m_set_shift = 6;
	
	/* The mask used to compute the index for a direct mapped cache needs to be one less than 
	the number of blocks which is 512-1=511.
 	This can also be written as (1<<9)-1, which emphasizes that we are going to use 9 bits of 
	the address for our cache index.*/
  icache->m_set_mask = (1<<9)-1;
  icache->m_tag_shift = 15;

	struct cache *icache4 = (struct cache *) calloc (sizeof(struct cache), 1);
//initialize tag array to 1024 because we have 32kb and each block is 32bytes. (32kb/32b)=1024
	icache4->m_tag_array = (struct block *) calloc (sizeof(struct block), 1024);
	icache4->m_total_blocks = 1024; //32kb/32b=1024
	icache4->m_set_shift = 7; // 2^5=32(byte blocks) and 2^2=4(way). 5+2=7 so shift 7 bits
	icache4->m_set_mask = (1<<8)-1; //1set=4blocks so 1024/4=256 so we have 256 sets. thus 1<<8
	icache4->m_tag_shift = 14; //lower 7+7=14 used for offset and index so shift by 15 for tag
	icache4->m_nways = 4;

        struct cache *icache8 = (struct cache *) calloc (sizeof(struct cache), 1);
//initialize tag array to 256 because we have 16kb and each block is 64bytes so (16kb/64b)=256
        icache8->m_tag_array = (struct block *) calloc (sizeof(struct block), 256);
        icache8->m_total_blocks = 256; //16kb/64b=256
        icache8->m_set_shift = 9; //2^6=64(bytes) and 2^3=8(way). 6+3=9 so shift 9
        icache8->m_set_mask = (1<<5)-1; //1set=4blocks,256/4=32 so  we have 32 sets. thus 1<<5
        icache8->m_tag_shift = 14; //lower 9+5=14 used for offset and index so shift by 15 for tag
        icache8->m_nways = 8;

  fprintf(stderr, "sim: ** starting functional simulation **\n");

  /* set up initial default next PC */
  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);


  while (TRUE)
    {
      /* maintain $r0 semantics */
      regs.regs_R[MD_REG_ZERO] = 0;
#ifdef TARGET_ALPHA
      regs.regs_F.d[MD_REG_ZERO] = 0.0;
#endif /* TARGET_ALPHA */


        cache_access(icache, regs.regs_PC, &g_icache_miss); //moved this function here
	set_assoc_cache(icache4, regs.regs_PC, &g_icache4_miss, sim_num_insn, 32);

      /* get the next instruction to execute */
      MD_FETCH_INST(inst, mem, regs.regs_PC);

      /* keep an instruction count */
      sim_num_insn++;

      /* set default reference address and access mode */
      addr = 0; is_write = FALSE;

      /* set default fault - none */
      fault = md_fault_none;

      /* decode the instruction */
      MD_SET_OPCODE(op, inst);

      /* execute the instruction */
      switch (op)
	{
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	case OP:							\
          SYMCAT(OP,_IMPL);						\
          break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
        case OP:							\
          panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#define DECLARE_FAULT(FAULT)						\
	  { fault = (FAULT); break; }
#include "machine.def"
	default:
	  panic("attempted to execute a bogus opcode");
      }

      if (fault != md_fault_none)
	fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC);

      if (verbose)
	{
	  myfprintf(stderr, "%10n [xor: 0x%08x] @ 0x%08p: ",
		    sim_num_insn, md_xor_regs(&regs), regs.regs_PC);
	  md_print_insn(inst, regs.regs_PC, stderr);
	  if (MD_OP_FLAGS(op) & F_MEM)
	    myfprintf(stderr, "  mem: 0x%08p", addr);
	  fprintf(stderr, "\n");
	  /* fflush(stderr); */
	}

      if (MD_OP_FLAGS(op) & F_MEM)
	{
	  	sim_num_refs++;
	  	if (MD_OP_FLAGS(op) & F_STORE)
	    		is_write = TRUE;
	}
	
      if (MD_OP_FLAGS(op) & F_LOAD){
	// place function call here because we load misses for an 8-way cache
		set_assoc_cache(icache8, addr, &g_load_miss, sim_num_insn, 64);
        	g_load_inst++;
      	}

      if (MD_OP_FLAGS(op) & F_STORE){
	// place function call here to incrememnt the store miss variable when there is a miss in the 8-way cache for a store instruction. 
		set_assoc_cache(icache8, addr, &g_store_miss, sim_num_insn, 64);
      		g_store_inst++;
	}


      /* go to the next instruction */
      regs.regs_PC = regs.regs_NPC;
      regs.regs_NPC += sizeof(md_inst_t);

      /* finish early? */
      if (max_insts && sim_num_insn >= max_insts)
	return;
    }
}
