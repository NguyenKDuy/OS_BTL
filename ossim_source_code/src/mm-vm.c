//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
static pthread_mutex_t mm_lock;

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct rg_elmt)
{

  int vmaid = rg_elmt.vmaid;
  if (rg_elmt.rg_start >= rg_elmt.rg_end && vmaid == 0) {
    return -1; 
  }
  // Traverse the free list to insert or merge the region
  if (rg_elmt.rg_start < rg_elmt.rg_end && vmaid == 1 ){
    return -1;
  }

  if (vmaid == 0) {
    struct vm_rg_struct **rg_node = &mm->mmap->vm_freerg_list;
    while (*rg_node) {
      struct vm_rg_struct *current = *rg_node;
      
      // Insert the new region in sorted order
      if (OVERLAP(current->rg_start, current->rg_end, rg_elmt.rg_start, rg_elmt.rg_end))  {
        printf("*Double free fault\n");
        return -1;
      }
      if (INCLUDE(current->rg_start, current->rg_end, rg_elmt.rg_start, rg_elmt.rg_end)) {
        printf("*Double free fault\n");
        return -1;
      }
      if (((current->rg_start == rg_elmt.rg_start) && (current->rg_end == rg_elmt.rg_end))){
        printf("*Double free fault\n");
        return -1;
      }
      rg_node = &current->rg_next;
      
    }
     
    // Append the new region to the end of the list
    struct vm_rg_struct *new_node = malloc(sizeof(struct vm_rg_struct));
    *new_node = rg_elmt;
    new_node->rg_next = NULL;
    *rg_node = new_node;

    return 0;
  }
  else {
    struct vm_rg_struct **rg_node = &mm->mmap->vm_next->vm_freerg_list;
    while (*rg_node) {
      struct vm_rg_struct *current = *rg_node;
      // Insert the new region in sorted orde
      if (OVERLAP(current->rg_start, current->rg_end, rg_elmt.rg_start, rg_elmt.rg_end))  {
        printf("*Double free fault\n");
        return -1;
      }
      if (INCLUDE(current->rg_start, current->rg_end, rg_elmt.rg_start, rg_elmt.rg_end)) {
        printf("*Double free fault\n");
        return -1;
      }
      if (((current->rg_start == rg_elmt.rg_start) && (current->rg_end == rg_elmt.rg_end))){
        printf("*Double free fault\n");
        return -1;
      }
      rg_node = &current->rg_next;
    }
    struct vm_rg_struct *new_node = malloc(sizeof(struct vm_rg_struct));
    *new_node = rg_elmt;
    new_node->rg_next = NULL;
    *rg_node = new_node;
    return 0;
  }
}

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma= mm->mmap;

  if(mm->mmap == NULL)
    return NULL;

  int vmait = 0;
  
  while (vmait < vmaid)
  {
    if(pvma == NULL)
	  return NULL;

    vmait++;
    pvma = pvma->vm_next;
  }

  return pvma;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
  
  /*Allocate at the toproof */
  /* TODO: get_free_vmrg_area FAILED handle the region management (Fig.6)*/

  /* TODO retrive current vma if needed, current comment out due to compiler redundant warning*/
  /*Attempt to increate limit to get space */
  struct vm_rg_struct rgnode;
  pthread_mutex_lock(&mm_lock);
  if (caller->mm->symrgtbl[rgid].rg_end != caller->mm->symrgtbl[rgid].rg_start) {
    printf("Region is used, free first\n");
     pthread_mutex_unlock(&mm_lock);
    return -1;
  }
  if (vmaid == 0) {
    if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
    {
      caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
      caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
      caller->mm->mmap->sbrk = rgnode.rg_start + size;
      caller->mm->symrgtbl[rgid].vmaid = vmaid;

      *alloc_addr = rgnode.rg_start;
      printf("finish alloc to region=%d size=%d adrress form %ld to %ld at process %d\n",rgid,size,rgnode.rg_start,rgnode.rg_end,caller->pid);fflush(stdout);
      pthread_mutex_unlock(&mm_lock);
      return 0;
    }
    
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
    int old_sbrk = cur_vma->sbrk;
    int remain = cur_vma->vm_end - old_sbrk;
    int inc_sz = PAGING_PAGE_ALIGNSZ(size -remain);
    int inc_limit_ret;
    /* TODO retrive old_sbrk if needed, current comment out due to compiler redundant warning*/
    /* TODO: commit the limit increment */
    inc_vma_limit(caller, vmaid, inc_sz, &inc_limit_ret);
    caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
    caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;
    caller->mm->symrgtbl[rgid].vmaid = vmaid;
    cur_vma->sbrk = old_sbrk + size;
    /* TODO: commit the allocation address */
    printf("[IVMA] finish alloc to region=%d size=%d adrress form %d to %d at process %d\n",rgid,size,old_sbrk,(int)(old_sbrk + size),caller->pid);fflush(stdout);
    *alloc_addr = old_sbrk;
  }
  else if (vmaid == 1) {
    if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
    {
      
      caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
      caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
      caller->mm->mmap->vm_next->sbrk = rgnode.rg_start - size;
      caller->mm->symrgtbl[rgid].vmaid = vmaid;

      *alloc_addr = rgnode.rg_start;
      printf("finish malloc to region=%d size=%d adrress form %ld to %ld at process %d\n",rgid,size,rgnode.rg_start,rgnode.rg_end,caller->pid);fflush(stdout);
      pthread_mutex_unlock(&mm_lock);
      return 0;
    }
    
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
    int old_sbrk = cur_vma->sbrk;
    int remain = old_sbrk - cur_vma->vm_end;
    int inc_sz = PAGING_PAGE_ALIGNSZ(size -remain);
    int inc_limit_ret;
    /* TODO retrive old_sbrk if needed, current comment out due to compiler redundant warning*/
    //Delete not enough regions in free lists
    
    inc_vma_limit(caller, vmaid, inc_sz, &inc_limit_ret);

    /* TODO: commit the limit increment */
    caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
    caller->mm->symrgtbl[rgid].rg_end = old_sbrk-size;
    caller->mm->symrgtbl[rgid].vmaid = vmaid;
    /* TODO: commit the allocation address */
    printf("[IVMA] finish malloc to region=%d size=%d adrress form %d to %d at process %d\n",rgid,size,old_sbrk,(int)(old_sbrk - size),caller->pid);fflush(stdout);
    *alloc_addr = old_sbrk;
    cur_vma->sbrk = old_sbrk - size;
  }
  pthread_mutex_unlock(&mm_lock);
  return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __free(struct pcb_t *caller, int rgid)
{
  // Dummy initialization for avoding compiler dummay warning
  // in incompleted TODO code rgnode will overwrite through implementing
  // the manipulation of rgid later
  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return -1;
  /* TODO: Manage the collect freed region to freerg_list */
  pthread_mutex_lock(&mm_lock);
  struct vm_rg_struct * rgnode = get_symrg_byid(caller->mm, rgid);
  /*enlist the obsoleted memory region */
  if (enlist_vm_freerg_list(caller->mm, *rgnode) != -1) {
    if (rgnode->vmaid == 1) {
      printf("\t*Finish free region=%d size=%li adrress form %ld to %ld at process %u\n",rgid,rgnode->rg_start - rgnode->rg_end,rgnode->rg_start,rgnode->rg_end,caller->pid);fflush(stdout);
    }
    else {
        printf("\t*Finish free region=%d size=%li adrress form %ld to %ld at process %u\n",rgid,rgnode->rg_end - rgnode->rg_start,rgnode->rg_start,rgnode->rg_end,caller->pid);fflush(stdout);
    }
    rgnode->rg_start = rgnode->rg_end = -1;
    printf("List of free rg vma0\n");
    print_list_rg(caller->mm->mmap->vm_freerg_list);
    printf("List of free rg vma1\n");
    print_list_rg(caller->mm->mmap->vm_next->vm_freerg_list);
    pthread_mutex_unlock(&mm_lock);
    return 0;
  }
  else {
    pthread_mutex_unlock(&mm_lock);
    return -1;
  }
  
}
int __dump(struct pcb_t *caller, int rgid) {
  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return -1;
  pthread_mutex_lock(&mm_lock);
  printf("=======[BEGIN]DUMP=======\n");
  struct vm_rg_struct * rgnode = get_symrg_byid(caller->mm, rgid);
  print_list_rg (rgnode);


  struct vm_rg_struct *rg = caller->mm->mmap->vm_freerg_list;
   if (rg == NULL) {printf("No free region in vma0\t"); return -1;}
   int count0 = 0;
   while (rg != NULL)
   {
       count0++;
       rg = rg->rg_next;
   }
   printf("Number of free region in vma0=%d\t",count0);

  rg=caller->mm->mmap->vm_next->vm_freerg_list;
   if (rg == NULL) {printf("No free region in vma1\n"); return -1;}
   int count1 = 0;
   while (rg != NULL)
   {
       count1++;
       rg = rg->rg_next;
   }
   printf("Number of free region in vma1=%d\n",count1);
   pthread_mutex_unlock(&mm_lock);
   printf("=======[END]DUMP=======\n");
   return 0;
}

int pgdump(struct pcb_t *proc, uint32_t reg_index) {
  return __dump(proc, reg_index);
}


/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;

  /* By default using vmaid = 0 */
  return __alloc(proc, 0, reg_index, size, &addr);
}

/*pgmalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify vaiable in symbole table)
 */
int pgmalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;

  /* By default using vmaid = 1 */
  return __alloc(proc, 1, reg_index, size, &addr);
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int pgfree_data(struct pcb_t *proc, uint32_t reg_index)
{
   return __free(proc, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte = mm->pgd[pgn];
 
  if (!PAGING_PTE_PAGE_PRESENT(pte))
  { /* Page is not online, make it actively living */
    int vicpgn, swpfpn; 
    int vicfpn;
    uint32_t vicpte;

    int tgtfpn = PAGING_PTE_SWP(pte);//the target frame storing our variable
    // enlist_pgn_node(&caller->mm->fifo_pgn,pgn);
    if(find_victim_page(caller->mm, &vicpgn) == -1) return -1;

    /* Get free frame in MEMSWP */
    if(MEMPHY_get_freefp(caller->active_mswp, &swpfpn) == -1) return -1;

    vicpte = mm->pgd[vicpgn];
    vicfpn = PAGING_FPN(vicpte);

    /* Do swap frame from MEMRAM to MEMSWP and vice versa*/
    /* Copy victim frame to swap */
    __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
    printf("\t*Swapping out vtcfpn: %d, swapping in swpfpn: %d", vicfpn, swpfpn);
    /* Copy target frame from swap to mem */
    __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);

    /* Update page table */
    pte_set_swap(&mm->pgd[vicpgn], 0, swpfpn);

    /* Update its online status of the target page */
    pte_set_fpn(&mm->pgd[pgn], vicfpn);
    //pte_set_fpn() & mm->pgd[pgn];
    pte_set_fpn(&pte, tgtfpn);

    enlist_pgn_node(&caller->mm->fifo_pgn,pgn);
  }
  *fpn = PAGING_PTE_FPN(pte);

  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess 
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_read(caller->mram,phyaddr, data);

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess 
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;
  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */
  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_write(caller->mram,phyaddr, value);

   return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __read(struct pcb_t *caller, int rgid, int offset, BYTE *data)
{
  pthread_mutex_lock(&mm_lock);
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  int vmaid = currg->vmaid;

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */
  {
    pthread_mutex_unlock(&mm_lock);
    return -1;
  }
  pg_getval(caller->mm, currg->rg_start + offset, data, caller);
  pthread_mutex_unlock(&mm_lock);

  return 0;
}


/*pgwrite - PAGING-based read a region memory */
int pgread(
		struct pcb_t * proc, // Process executing the instruction
		uint32_t source, // Index of source register
		uint32_t offset, // Source address = [source] + [offset]
		uint32_t destination) 
{

  BYTE data;
  int val = __read(proc, source, offset, &data);
  // destination = (uint32_t) data;
  

#ifdef IODUMP
  printf("read region=%d offset=%d value=%d\n", source, offset, data);
  if(!pgalloc(proc,sizeof(BYTE),destination)){
    if(!__write(proc,destination,0,data))
      printf("save read value from src region=%d to dst region=%d\n",source,destination);
  }
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __write(struct pcb_t *caller, int rgid, int offset, BYTE value)
{ 
  pthread_mutex_lock(&mm_lock);
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  int vmaid = currg->vmaid;

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */
	{  
    pthread_mutex_unlock(&mm_lock);
    return -1;
  }
  pg_setval(caller->mm, currg->rg_start + offset, value, caller);
  pthread_mutex_unlock(&mm_lock);
  return 0;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(
		struct pcb_t * proc, // Process executing the instruction
		BYTE data, // Data to be wrttien into memory
		uint32_t destination, // Index of destination register
		uint32_t offset)
{
  int val = __write(proc, destination, offset, data);
#ifdef IODUMP
  printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  return val;
}


/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  int pagenum, fpn;
  uint32_t pte;


  for(pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte= caller->mm->pgd[pagenum];

    if (!PAGING_PTE_PAGE_PRESENT(pte))
    {
      fpn = PAGING_PTE_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else {
      fpn = PAGING_PTE_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);    
    }
  }

  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct* get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_rg_struct * newrg;
  /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  newrg = malloc(sizeof(struct vm_rg_struct));

  /* TODO: update the newrg boundary */
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + alignedsz;
  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  struct mm_struct *mm = caller->mm;
  /* TODO validate the planned memory area is not overlapped */
  for (int i = 0 ; i < PAGING_MAX_SYMTBL_SZ; i++) {
    struct vm_rg_struct *rg = &mm->symrgtbl[i];
    if (rg->rg_start == -1 || rg->rg_end == -1) 
      continue;
    if (rg->vmaid != vmaid) 
      continue;
    if (rg->rg_start == vmastart && rg->rg_end == vmaend)
      continue;
    if (OVERLAP(rg->rg_start, rg->rg_end, vmastart, vmaend)) 
      return -1;
  }
  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size 
 *@inc_limit_ret: increment limit return
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz, int* inc_limit_ret)
{
  struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage =  inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  
  int old_end = cur_vma->vm_end;
  /*Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0) {
    printf ("\tOverlap regions in VMA");
    return -1; /*Overlap and failed allocation */
  }
  /* TODO: Obtain the new vm area based on vmaid */
  if (vmaid==0) {
    cur_vma->vm_end +=inc_amt;
    cur_vma->sbrk += inc_amt;
  }
  else if(vmaid==1) {
    cur_vma->vm_end-=inc_amt;
    cur_vma->sbrk-=inc_amt;
  }

  *inc_limit_ret = cur_vma->sbrk; 
  if (vm_map_ram(caller, area->rg_start, area->rg_end, 
                    old_end, incnumpage , newrg, vmaid) < 0)
    return -1; /* Map the memory to MEMRAM */

  return 0;
}

/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *retpgn) 
{
  struct pgn_t *pg = mm->fifo_pgn;
  struct pgn_t *prev = NULL;
  /* TODO: Implement the theorical mechanism to find the victim page */
if (!pg) return -1;
  while (pg->pg_next)
  {
    prev = pg;
    pg = pg->pg_next;
  }
  // pg is victim_page
  if (!prev) // Size of fifo_pgn = 1
  {
    mm->fifo_pgn = NULL;
  }
  else
  {
    prev->pg_next = NULL;
  }
  *retpgn = pg->pgn;
  free(pg);

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size 
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;
  if (rgit == NULL) 
    return -1;
  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;
  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL && rgit->vmaid == vmaid)
  {
    if (vmaid == 0) {
      if (rgit->rg_start + size <= rgit->rg_end)
      { /* Current region has enough space */
        newrg->rg_start = rgit->rg_start;
        newrg->rg_end = rgit->rg_start + size;
        /* Update left space in chosen region */
        if (rgit->rg_start + size < rgit->rg_end)
        {
          rgit->rg_start = rgit->rg_start + size;
          break;
        }
        else
        { /*Use up all space, remove current node */
          /*Clone next rg node */
          struct vm_rg_struct *nextrg = rgit->rg_next;

          /*Cloning */
          if (nextrg != NULL)
          {
            rgit->rg_start = nextrg->rg_start;
            rgit->rg_end = nextrg->rg_end;

            rgit->rg_next = nextrg->rg_next;

            free(nextrg);
            break;
          }
          else
          { /*End of free list */
            rgit->rg_start = rgit->rg_end = 0;	//dummy, size 0 region
            rgit->rg_next = NULL;
            break;
          }
        }
      }
      else
      {
        rgit = rgit->rg_next;	// Traverse next rg
      }
    }
    else {
      if (rgit->rg_end + size <= rgit->rg_start)
      { /* Current region has enough space */
        newrg->rg_start = rgit->rg_start;
        newrg->rg_end = rgit->rg_start - size;
        /* Update left space in chosen region */
        if (rgit->rg_end + size < rgit->rg_start)
        {
          rgit->rg_start = rgit->rg_start - size;
          break;
        }
        else
        { /*Use up all space, remove current node */
          /*Clone next rg node */
          struct vm_rg_struct *nextrg = rgit->rg_next;

          /*Cloning */
          if (nextrg != NULL)
          {
            rgit->rg_start = nextrg->rg_start;
            rgit->rg_end = nextrg->rg_end;

            rgit->rg_next = nextrg->rg_next;

            free(nextrg);
            break;
          }
          else
          { /*End of free list */
            rgit->rg_start = rgit->rg_end = 0;	//dummy, size 0 region
            rgit->rg_next = NULL;
            break;
          }
        }
      }
      else
      {
        rgit = rgit->rg_next;	// Traverse next rg
      }
    }
  }
 if(newrg->rg_start == -1) // new region not found
   return -1;

 return 0;
}

//#endif
