#include "stdafx.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include "user_definition.h"
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;

typedef int Flag;
typedef int Status;

Status Cpu_Status=IDLE;
Status Disk1_Status=IDLE;
Status Disk2_Status=IDLE;

float ct[5000], ctt[5000];//sample value and duration
int cnt=0;//sample number counter

typedef struct

{
	float OccurTime;  
	float ReservedArrivalTime; 
	int NType;      

}Event,ElemType;

typedef struct LNode 

{
   ElemType data;
   LNode *next;

 }*Link,*Position;

struct LinkList 

 {
   Link head,tail; 
   int len; // length of the linklist
 };

typedef LinkList EventList; 

typedef struct

{
	float ArrivalTime;
	int Duration;

}QElemType;     //element in a queue

typedef struct QNode

{
   QElemType data;

   QNode *next;

}*QueuePtr;      

struct LinkQueue

{
	QueuePtr front,rear; 
};

EventList ev;

Event en; 

Event et;//temporary variable

LinkQueue Q_CPU;
LinkQueue Q_memory;
LinkQueue Q_disk1;
LinkQueue Q_disk2;
QElemType temp_record; //customer's record

/*important user's variables*/
long long holdrand[5];
int current_job_number=0;
float total_Cpu_time=0;
float total_Disk1_time=0;
float total_Disk2_time=0;
int arrival_lambda=1;
int simulated_time;
Flag end_flag=FALSE;
int finished_number=0;
float total_finished_time=0;

float average_memory_queue=0.0;
float total_memory_queue=0;
float previous_operating_memory_time=0.0;

float average_job=0.0;
float total_job=0.0;
float previous_operating_job_time=0.0;

float final_CPU_event_time = 0.0;
float final_disk1_event_time = 0.0;
float final_disk2_event_time =0.0;

float current_CPU_utilization;
float current_disk1_utilization;
float current_disk2_utilization;

streambuf *backup , *psbuf;
ofstream filestr;
char *filename;

void Start_Redirect(void)
{
	backup = std::cout.rdbuf();

	filestr.open(filename);

	psbuf = filestr.rdbuf();

	std::cout.rdbuf(psbuf);
}

void Close_Redirect(void)
{ 
	filestr.close(); 

	std::cout.rdbuf(backup);

}

//Type's value is ARRIVAL_TYPE, CPU_TYPE, DISK1_TYPE, DISK2_TYPE, CPU_PUT 4, respectively
int  __cdecl random(int Type)
{
	return (((holdrand[Type] = holdrand[Type] * 214013L + 2531011L) >> 16) & 0x7fff);
}

//Poisson returns time interval. lambda denotes arrival rate
//Type's value is ARRIVAL_TYPE, CPU_TYPE, DISK1_TYPE, DISK2_TYPE 
float Poisson(float lambda,int Type) 
{  
    float z;  
    do  
    {  z = ((float)random(Type))/RAND_MAX;  
    }while(z == 0 || z == 1);  
    return (-(1/lambda)*log(1-z));  
} 

int cmp(Event a,Event b)

 { // compare occuring time of events a and b, return -1, 0 or 1 

   if(a.OccurTime==b.OccurTime)

     return 0;
   
   else if(a.OccurTime<b.OccurTime)
	 
	  return -1;
   else

     return 1;

 }

Status InitList(LinkList &L)

{ // create an empty event list

   Link p;

   p=(Link)malloc(sizeof(LNode)); // generate the head node

   if(p)

   {

     p->next=NULL;

     L.head=L.tail=p;

     L.len=0;

     return OK;

   }

   else

     return ERROR;

}//InitList

Status ClearList ( LinkList &L )  
{   
	Link p; 

	while(L.head->next)
	{	
		p=L.head->next;

		L.head->next=p->next;
		free(p);
	}

	L.tail=L.head;

	L.len=0;

	return OK;
}

Status InitQueue(LinkQueue &Q)

{ // initiate an empty queue Q
   if(!(Q.front=Q.rear=(QueuePtr)malloc(sizeof(QNode))))

   exit(-1);

   Q.front->next=NULL;

   return OK;

}//InitQueue

Status ClearQueue(LinkQueue &Q)
{
	 if(Q.front->next== NULL)
	 return OK;
	 QueuePtr s = Q.front->next,p;
	 while(s)
	 {
	  p=s->next;
	  free(s);
	  s = p;
	 }
	 Q.rear =  Q.front;
	 Q.front->next=NULL;
	 return OK;
}
Status OrderInsert(LinkList &L,ElemType e,int (*comp)(ElemType,ElemType))

 { // Insert element e into a linklist L

   Link o,p,q;

   q=L.head;

   p=q->next;

   while(p!=NULL&&comp(p->data,e)<0) // p is not the tail and element is less than e

   {

     q=p;

     p=p->next;

   }

   o=(Link)malloc(sizeof(LNode)); // create a node

   o->data=e; // assignment

   q->next=o; // insertion

   o->next=p;

   L.len++; // linklist's length increases by one

   if(!p) // insert an element to the tail

     L.tail=o; // change the tail node

   return OK;

 }//OrderInsert

/*
Flag CPU_Random(void)
According to CPU output event's possibility distribution, return TO_DISK1£¬¨º?TO_DISK2£¬¨º?LEAVE
*/
Flag CPU_Random(void)
{
	int rand_num;
	rand_num=random(CPU_PUT)%10+1;//generate a random number between 1 and 10
	if(rand_num<=7)
		return TO_DISK1;     
	else if(rand_num>7&&rand_num<10)
		return TO_DISK2;	 
	else
		return LEAVE;		
}//CPU_Random

int QueueLength(LinkQueue Q)

{ // calculate the length of queue Q

   int i=0;

   QueuePtr p;

   p=Q.front;

   while(Q.rear!=p)

   {

     i++;

     p=p->next;

   }

   return i;

}//QueueLength

Status EnQueue(LinkQueue &Q,QElemType e)

{ // insert e to the tail of queue Q

   QueuePtr p;

   if(!(p=(QueuePtr)malloc(sizeof(QNode)))) // memory allocation fails
   exit(OVERFLOW);

   p->data=e;

   p->next=NULL;

   Q.rear->next=p;

   Q.rear=p;

   return OK;

}//EnQueue

Status DeQueue(LinkQueue &Q,QElemType &e)

 { // if queue is not empty, delete the head, return it with e, and returnOK; otherwise return ERROR and the head dequeues

   QueuePtr p;

   if(Q.front==Q.rear)

     return ERROR;

   p=Q.front->next;

   e=p->data;

   Q.front->next=p->next;

   if(Q.rear==p)

     Q.rear=Q.front;

   free(p);

   return OK;

 }//DeQueue

Status GetHead(LinkQueue Q,QElemType &e)

{ 

   QueuePtr p;

   if(Q.front==Q.rear)

     return ERROR;

   p=Q.front->next;

   e=p->data;

   return OK;

}//GetHead

 Position GetHead(LinkList L)

 { 
   return L.head;

 }//GetHead

Status QueueEmpty(LinkQueue Q)

 { // if Q is empty, return true; otherwise, return false

   if(Q.front==Q.rear)

     return TRUE;

   else

     return FALSE;

 }//QueueEmpty


 Status ListEmpty(LinkList L)

 { 
   if(L.len)

     return FALSE;

   else

     return TRUE;

 }//ListEmpty

Status DelFirst(LinkList &L,Link h,Link &q) 

{ 
   q=h->next;

   if(q) 

   {

     h->next=q->next;

     if(!h->next) 

       L.tail=h; 
	
// 	 cout<<L.len<<endl;

     L.len--;

     return OK;

   }

   else

     return FALSE;

}//DelFirst

ElemType GetCurElem(Link p)

{ 
   return p->data;

}//GetCurElem

void Re_initialization()

{	
	ClearList(ev);	

	en.OccurTime=0;

	en.ReservedArrivalTime=0;

	en.NType=0;

	OrderInsert(ev,en,cmp);//insert the first arrival event
	
    ClearQueue(Q_CPU);	

	ClearQueue(Q_memory);

    ClearQueue(Q_disk1);

	ClearQueue(Q_disk2);

	Cpu_Status=IDLE;
	Disk1_Status=IDLE;
	Disk2_Status=IDLE;

	current_job_number=0;

	end_flag=FALSE;

	total_Cpu_time=0;
	total_Disk1_time=0;
	total_Disk2_time=0;
	finished_number=0;
    total_finished_time=0;
	average_memory_queue=0.0;
	total_memory_queue=0;
	previous_operating_memory_time=0.0;
    average_job=0.0;
	total_job=0.0;
	previous_operating_job_time=0.0;
	final_CPU_event_time = 0.0;
	final_disk1_event_time = 0.0;
	final_disk2_event_time =0.0;
	current_CPU_utilization = 0.0;
	current_disk1_utilization = 0.0;
	current_disk2_utilization = 0.0;

}//Reinitialization
void Initialization()

{	
	InitList(ev);	//initiate event list

	en.OccurTime=0;
	
	en.ReservedArrivalTime=0;

	en.NType=0;

	OrderInsert(ev,en,cmp);

    InitQueue(Q_CPU);	

	InitQueue(Q_memory);

	InitQueue(Q_disk1);

	InitQueue(Q_disk2);

	holdrand[0]=time(NULL);  
	holdrand[1]=holdrand[0]+1;
	holdrand[2]=holdrand[1]+1;
	holdrand[3]=holdrand[2]+1;
	holdrand[4]=holdrand[3]+1;

	Cpu_Status=IDLE;
	Disk1_Status=IDLE;
	Disk2_Status=IDLE;

	current_job_number=0;

	end_flag=FALSE;

	total_Cpu_time=0;
	total_Disk1_time=0;
	total_Disk2_time=0;
	finished_number=0;
    total_finished_time=0;	
	average_memory_queue=0.0;
	total_memory_queue=0;
	previous_operating_memory_time=0.0;
	average_job=0.0;
	total_job=0.0;
	previous_operating_job_time=0.0;
	final_CPU_event_time = 0.0;
	final_disk1_event_time = 0.0;
	final_disk2_event_time =0.0;
	current_CPU_utilization = 0.0;
	current_disk1_utilization = 0.0;
	current_disk2_utilization = 0.0;

}//Initialization

void Arrival(void)
{	
		QElemType f;

		float intertime=0;		
		intertime=Poisson(arrival_lambda,ARRIVAL_TYPE);

		et.OccurTime=en.OccurTime+intertime;

		et.ReservedArrivalTime=et.OccurTime;

		et.NType=JOB_ARRIVAL;

		OrderInsert(ev,et,cmp);

	if(current_job_number<MAX_JOB_NUMBER)//there is space in memory
	{
		total_job += current_job_number*(en.OccurTime-previous_operating_job_time);
	
		ct[cnt]=current_job_number;
		ctt[cnt]=en.OccurTime-previous_operating_job_time;
		cnt++;
		
		previous_operating_job_time = en.OccurTime;	

		current_job_number++;

		if(Cpu_Status==IDLE) 
		{
			Cpu_Status=BUSY;			
			intertime=Poisson(CPU_LAMBDA,CPU_TYPE);

			total_Cpu_time +=intertime;

			et.OccurTime=en.OccurTime+intertime;

			et.ReservedArrivalTime=en.ReservedArrivalTime;

			et.NType=END_PROCESSING;

			OrderInsert(ev,et,cmp);
		}

		else//CPU is BUSY
		{
			f.ArrivalTime=en.ReservedArrivalTime;

			f.Duration=NULL;

			EnQueue(Q_CPU,f);
		}
	}
	
	else//memory is stuffed
	{
		f.ArrivalTime=en.ReservedArrivalTime;

		f.Duration=NULL;
	//	cout<<en.OccurTime<<"\t"<<QueueLength(Q_memory)<<endl;

		total_memory_queue += QueueLength(Q_memory)*(en.OccurTime-previous_operating_memory_time);

	//ct[cnt]=QueueLength(Q_memory);		
		//ctt[cnt]=en.OccurTime-previous_operating_memory_time;
		
		//cnt++;
		
		previous_operating_memory_time = en.OccurTime;

		EnQueue(Q_memory,f);

	//cout<<en.OccurTime<<"\t"<<QueueLength(Q_memory)<<endl;
	}
}
/**
This function process the leaving of an event.
It examines whether the memory queue is empty, first; then if the queue is not empty, dequeue the queue's head, 
and insert it to CPU queue; if not, release memory, increase the number of current jobs by one.
***/
void Leave(void)
{
	QElemType f;

	//ct[finished_number]=en.OccurTime-en.ReservedArrivalTime; 
	
	finished_number++;//finished_number	
	total_finished_time += en.OccurTime-en.ReservedArrivalTime; //	
	cout<<finished_number
		<<"\t"<< en.ReservedArrivalTime
		<<"\t"<< en.OccurTime
		<<"\t"<< en.OccurTime-en.ReservedArrivalTime<<endl; //	
	if(!QueueEmpty(Q_memory)&&!end_flag)//	{
//cout<<en.OccurTime<<"\t"<<QueueLength(Q_memory)<<endl;// 
		total_memory_queue += QueueLength(Q_memory)*(en.OccurTime-previous_operating_memory_time);// 
		//ct[cnt]=QueueLength(Q_memory);		//ctt[cnt]=en.OccurTime-previous_operating_memory_time;

		//cnt++;

		previous_operating_memory_time = en.OccurTime;
		
		DeQueue(Q_memory,temp_record);

	//cout<<en.OccurTime<<"\t"<<QueueLength(Q_memory)<<endl;// 
		f.ArrivalTime=temp_record.ArrivalTime;

		f.Duration=NULL;

		EnQueue(Q_CPU,f);
	}
	else
	{
		total_job += current_job_number*(en.OccurTime-previous_operating_job_time);
		ct[cnt]=current_job_number;
		ctt[cnt]=en.OccurTime-previous_operating_job_time;
		cnt++;

		previous_operating_job_time = en.OccurTime;
		current_job_number--;
	}
}
void ToDisk1(void)
{	
	QElemType f;

	float intertime=0;

	if(Disk1_Status==IDLE)//
	{
		Disk1_Status=BUSY;
					intertime=Poisson(DISK1_LAMBDA,DISK1_TYPE);
		total_Disk1_time+=intertime;

		et.OccurTime=en.OccurTime+intertime;
		et.ReservedArrivalTime=en.ReservedArrivalTime;

		et.NType=END_DISK1;

		OrderInsert(ev,et,cmp);

	}

	else
	{
		f.ArrivalTime=en.ReservedArrivalTime;

		f.Duration=NULL;

		EnQueue(Q_disk1,f);
	}
}

void ToDisk2(void)
{
	QElemType f;

	float intertime=0;

	if(Disk2_Status==IDLE)
	{
		Disk2_Status=BUSY;			
		intertime=Poisson(DISK2_LAMBDA,DISK2_TYPE);

		total_Disk2_time+=intertime;

		et.OccurTime=en.OccurTime+intertime;
		et.ReservedArrivalTime=en.ReservedArrivalTime;

		et.NType=END_DISK2;

		OrderInsert(ev,et,cmp);
	}

	else
	{
		f.ArrivalTime=en.ReservedArrivalTime;

		f.Duration=NULL;

		EnQueue(Q_disk2,f);
	}
}

void End_Processing(void)
{
	float intertime=0;

	final_CPU_event_time = en.OccurTime;

	if(final_CPU_event_time != 0)
	{
		current_CPU_utilization = total_Cpu_time/final_CPU_event_time;
	}

	switch(CPU_Random())
	{
		case LEAVE: Leave();break;
		case TO_DISK2: ToDisk2();break;
		case TO_DISK1: ToDisk1();break;
		default:break;
	}

	if(!QueueEmpty(Q_CPU)) 
	{	
		DeQueue(Q_CPU,temp_record);
		Cpu_Status=BUSY;			
		intertime=Poisson(CPU_LAMBDA,CPU_TYPE);
		total_Cpu_time +=intertime;

		et.OccurTime=en.OccurTime+intertime;
		et.ReservedArrivalTime=temp_record.ArrivalTime;

		et.NType=END_PROCESSING;
		OrderInsert(ev,et,cmp);
	}
	else
	{
		Cpu_Status=IDLE;
	}
}

void End_Disk1(void)
{
	float intertime=0;
		
	final_disk1_event_time = en.OccurTime;

	if(final_disk1_event_time != 0)
	{
		current_disk1_utilization = total_Disk1_time/final_disk1_event_time;
	}
	QElemType f;
	
	if(Cpu_Status==IDLE)
	{
		Cpu_Status=BUSY;

		intertime=Poisson(CPU_LAMBDA,CPU_TYPE);
		total_Cpu_time +=intertime;

		et.OccurTime=en.OccurTime+intertime;

	et.ReservedArrivalTime=en.ReservedArrivalTime;

		et.NType=END_PROCESSING;

		OrderInsert(ev,et,cmp);
	}
	else
	{
		f.ArrivalTime=en.ReservedArrivalTime;

		f.Duration=NULL;

		EnQueue(Q_CPU,f);	
	}

	if(!QueueEmpty(Q_disk1)) 
	{
		DeQueue(Q_disk1,temp_record);

		Disk1_Status=BUSY;
		intertime=Poisson(DISK1_LAMBDA,DISK1_TYPE);

		total_Disk1_time+=intertime;

		et.OccurTime=en.OccurTime+intertime;
		et.ReservedArrivalTime=temp_record.ArrivalTime;

		et.NType=END_DISK1;

		OrderInsert(ev,et,cmp);	
	}
	else
	{
		Disk1_Status=IDLE;
	}
}

void End_Disk2(void)
{
	float intertime=0;

	final_disk2_event_time = en.OccurTime;

	if(final_disk2_event_time != 0)
	{
		current_disk2_utilization = total_Disk2_time/final_disk2_event_time;
	}

	QElemType f;
	
	if(Cpu_Status==IDLE)
	{
		Cpu_Status=BUSY;
		intertime=Poisson(CPU_LAMBDA,CPU_TYPE);

		total_Cpu_time +=intertime;

		et.OccurTime=en.OccurTime+intertime;
		et.ReservedArrivalTime=en.ReservedArrivalTime;
		et.NType=END_PROCESSING;

		OrderInsert(ev,et,cmp);
	}
	else
	{
		f.ArrivalTime=en.ReservedArrivalTime;

		f.Duration=NULL;

		EnQueue(Q_CPU,f);	
	}

	if(!QueueEmpty(Q_disk2))
	{
		DeQueue(Q_disk2,temp_record);

		Disk2_Status=BUSY;			
		intertime=Poisson(DISK2_LAMBDA,DISK2_TYPE);

		total_Disk2_time+=intertime;

		et.OccurTime=en.OccurTime+intertime;
		et.ReservedArrivalTime=temp_record.ArrivalTime;

		et.NType=END_DISK2;

		OrderInsert(ev,et,cmp);	
	}
	else
	{
		Disk2_Status=IDLE;
	}
}

void Computer_System_Simulation()

{
	Link p;

	cout<<fixed;			

	cout<<setprecision(6);

	while(!ListEmpty(ev))

	  {		
		DelFirst(ev,GetHead(ev),p);
	   en.OccurTime=GetCurElem(p).OccurTime;

	   en.ReservedArrivalTime=GetCurElem(p).ReservedArrivalTime;

	   en.NType=GetCurElem(p).NType;

	   if(en.OccurTime >simulated_time  && end_flag == FALSE ) 
	   {
		   total_memory_queue += QueueLength(Q_memory)*(simulated_time-previous_operating_memory_time);
		   	
		   //ct[cnt]=QueueLength(Q_memory);
		   //ctt[cnt]=simulated_time-previous_operating_memory_time;		 
		   //cnt++;

		   end_flag=TRUE;			
	   }

	   switch(en.NType)
	   {
	   case JOB_ARRIVAL: 
		   if(	end_flag==FALSE)
		   {
			   Arrival();
		   }
		   break;
	   case END_PROCESSING: End_Processing();break;
	   case END_DISK1: End_Disk1();break;
	   case END_DISK2: End_Disk2();break;
	   default:break;	   
	   }   
  }//while

}//Computer_System_Simulation

int main()
{
	Initialization();   

	cout <<"Please input system simulated time : ";
	cin>>simulated_time;
	
	cout<<"when the maximum job number is "<<MAX_JOB_NUMBER
		<<" and the simulated time is "<<simulated_time
		<<"s,the result:\n\n";
	
	cout<<setw(40)<<"Total"<<setw(9)<<"Average"<<setw(9)<<"Average"<<setw(9)<<"Average"<<setw(12)<<"Utilization"<<endl;

	cout<<setw(6)<<"lambda"
		<<setw(5)<<" CPU"
		<<setw(11)<<" DISK1"
		<<setw(9)<<" DISK2"
		<<setw(9)<<"finished"
		<<setw(9)<<"finished"
		<<setw(9)<<"memory"
		<<setw(9)<<"job"
		<<setw(12)<<"of    "<<endl;	

	cout<<setw(40)<<"number"<<setw(9)<<"time"<<setw(9)<<"queue"<<setw(9)<<"number"<<setw(12)<<"memory   "<<endl;

    char str1[10][15]={"arrival_1.txt","arrival_2.txt","arrival_3.txt","arrival_4.txt",
					  "arrival_5.txt","arrival_6.txt","arrival_7.txt","arrival_8.txt",
					  "arrival_9.txt","arrival_10.txt"};

	char str2[10][15]={"arrival_1.xls","arrival_2.xls","arrival_3.xls","arrival_4.xls",
					  "arrival_5.xls","arrival_6.xls","arrival_7.xls","arrival_8.xls",
					  "arrival_9.xls","arrival_10.xls"};
	filename=&str1[0][0];

	float mct, sum;

	int i;
	
	for(arrival_lambda=1;arrival_lambda<=10;arrival_lambda++)
	{
	
	 Start_Redirect();

	 cnt=0;

	 Computer_System_Simulation(); 

	 Close_Redirect();

	 filename +=15;

	 average_memory_queue = total_memory_queue/simulated_time;

	 average_job = total_job/previous_operating_job_time;
	 	
		//mct=total_finished_time/finished_number;

	sum=0;

	 cout<<setw(4)<<fixed<<arrival_lambda<<setprecision(2)<<"	"
		<<setw(8)<<total_Cpu_time/final_CPU_event_time*100<<"%"<<"	"
		<<setw(8)<<total_Disk1_time/final_disk1_event_time*100<<"%"<<"	"
		<<setw(8)<<total_Disk2_time/final_disk2_event_time*100<<"%"<<"	"<<endl;
	<<setw(9)<<finished_number<<"	"
		//<<setw(9)<<total_finished_time/finished_number<<"	"<<endl;		//<<setw(9)<<average_memory_queue<<"	";
	//<<setw(9)<<setprecision(4)<<average_job<<"	";		//<<setw(9)<<setprecision(2)<<average_job/MAX_JOB_NUMBER*100<<"%"<<endl;
		
	/* for (i=0; i<cnt; i++)
	 {
		 //sum+=pow(abs(ct[i]-mct), 2)/finished_number;
		 sum+=pow(abs(ct[i]-average_job), 2)*ctt[i]/previous_operating_job_time;
	 }
	 sum=sqrt(sum);

	 cout<<cnt<<"	"<<cnt-1<<"	";

	 cout<<sum<<"	"<<1.833*sum/sqrt(double(cnt-1))<<endl;		 */
	 
	Re_initialization();
	
	}
	cin.get();
	cin.get();
	return 0;
}