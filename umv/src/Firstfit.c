#include <stdio.h> 
#include <malloc.h> 
 
/* #define maxp 10 //maxp:maximum no of processes 
#define maxmsz 100 //maxmsz:maximum memory size  */

struct memory{ 
	int bsz;//bsz:block size 
	int bs;//bs:block start 
	int be;//be:block end 
	int ap;//ap:allocated process 
	int flag;//for allocated or not 
	struct memory*link; 
}*start,*trav,*prev; 

int pq[maxp];//pq:process queue 
int f=-1,r=-1; 

void enqueue(int ps) 
{ 
	/*ps:process size*/ 
	if((r+1)%maxp==f) { 
		printf("\nqueue is full, no new process can be loaded"); 
	} else { 
		r=(r+1)%maxp; 
		pq[r]=ps; 
	} 
} 

int dequeue() 
{ 
	int ps; 
	if(f==r) { 
		printf("queue empty,no process left\n"); 
		return -1; 
	} else { 
		f=(f+1)%maxp; 
		ps=pq[f]; 
		return ps; 
	} 
} 

void first_fit(int ps,int pid) 
{ 
	/*pid:process id*/ 
	struct memory *block; 
	block=(struct memory*)malloc(sizeof(struct memory)); 
	trav=start; 

	/*search the memory block large enough*/ 
	while(trav!=NULL) { 
		if(trav->flag==0 && trav->bsz>=ps) { 
			break; 
		} 
		
		prev=trav; 
		trav=trav->link; 
	} 

	if(trav==NULL) { 
		printf("No large enough block is found\n"); 
	} else {/*if enough memory is found*/ 
		dequeue();//remove process from queue 
		block->bsz=ps; 
		block->bs=trav->bs; 
		block->be=trav->bs+ps-1; //anus size defininition 
		block->ap=pid; 
		block->flag=1; 
		block->link=trav; 
		trav->bsz=trav->bsz-ps; 
		trav->bs=block->be+1; 
		
		if(trav==start) {/*if first block is large enough*/ 
			start=block; 
		} else { 
			prev->link=block; 
		} 
	} 
} 

void dealocate_memory(int pid) 
{ 
	trav=start; 

	while(trav!=NULL && trav->ap!=pid) { 
		trav=trav->link; 
	} 

	if(trav==NULL) { 
		printf("This process is not in memory at all\n"); 
	} else { 
		if(trav->link->flag==0) { 
			trav->bsz=trav->bsz+trav->link->bsz; //anus link definition 
			trav->be=trav->link->be; 
			prev=trav->link; 
			trav->link=prev->link; 
			prev->link=NULL; 
			free(prev); 
		} 

		trav->ap=-1; 
		trav->flag=0; 
	} 
} 

void show_mem_status() 
{ 

	int c=1,free=0,aloc=0; 
	trav=start; 
	
	printf("Memory Status is::\n"); 
	printf("Block BSZ BS BE Flag process\n"); 
	
	while(trav!=NULL) { 
		printf("%d %d %d %d %d %d\n",c++,trav->bsz,trav->bs,trav->be,trav->flag,trav->ap); 

		if(trav->flag==0) { 
			free=free+trav->bsz; 
		} else { 
			aloc=aloc+trav->bsz; 
		} 
		
		trav=trav->link; 
	} 

	printf("Free memory= %d\n",free); 
	printf("Allocated memory= %d\n",aloc); 

} 

int main() 
{ 
	int ch,ps; 
	char cc; 

	/*the size of the total memory size is defined i.e. largest block or hole*/ 
	/*......................................................................*/ 
	start=(struct memory*)malloc(sizeof(struct memory)); 
	start->bsz=maxmsz; 
	start->bs=0; 
	start->be=maxmsz; 
	start->ap=-1; 
	start->flag=0; 
	start->link=NULL; 
	/*......................................................................*/ 
	
	while(1) { 
		printf("\n\t1. Enter process in queue\n"); 
		printf("\t2. Allocate memory to process from queue\n"); 
		printf("\t3. Show memory staus\n"); 
		printf("\t4. Deallocate memory of processor\n"); 
		printf("\t5. Exit\n"); 
		printf("Enter your choice: "); 
		scanf("%d",&ch); 

		switch(ch) { 
			case 1: 
				do { 
					printf("Enter the size of the process: "); 
					scanf("%d",&ps); 
					enqueue(ps); 
					printf("Do you want to enter another process(y/n)?: "); 
					scanf("%c",&cc); 
					scanf("%c",&cc); 
				} while(cc=='y'); 
			break; 
			case 2: 
				do { 
					ps=pq[f+1]; 
					if(ps!=-1) { 
						first_fit(ps,f+1); 
						show_mem_status(); 
					} 
			
					printf("Do you want to allocate mem to another process(y/n)?: "); 
					scanf("%c",&cc); 
					scanf("%c",&cc); 
 				} while(cc=='y'); 
			break; 
			case 3: 
				show_mem_status(); 
			break; 
			case 4: 
				do { 
					printf("Enter the process Id: "); 
					scanf("%d",&ps); 
					dealocate_memory(ps); 
					show_mem_status(); 
					printf("Do you want to enter another process(y/n)?: "); 
					scanf("%c",&cc); 
					scanf("%c",&cc); 
				} while(cc=='y'); 
 			break; 
			case 5: 
			break; 
			default: 
 				printf("\nEnter valid choice!!\n"); 
 			} 
 
		if(ch==5) 
		break; 
	} 
}