// ParallelPhilosophists.cpp: ���������� ����� ����� ��� ����������� ����������.
//
#include "stdafx.h"
#include <mpi.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <Windows.h>

//#define SINGLE_PROCESS
#define SILENT

#define HUNGER_CHANCE 30
#define ACT_SPEED 1

/*
������� � ����������. ��� �������� ��� int[2]; [0] - ��� ���������. � ��� �� �������. [1] - ��������
*/

enum LANG { LANGREQ_NONE,//������� ���������
	LANGREQ_HUNGRY,//��������� � ���, ��� ������� �������
	LANGREQ_GIVE,//��������� � ���, ��� ������� ����� ������ �����
	LANGREQ_INSIST,//������� ����������, ��� ����� ������ ��������� ���. ������������ TAKE
	LANGREQ_TAKE,//������� ��������, ��� ����� �����
	LANGREQ_AST,//������� ��������, ��� ����� �����
	LANGREQ_LOSEWAIT,//������� �������� ���� �� �����, �� ����, ����� ��� �����������
	LANGREQ_ABORT,//������� �� ���� �������� ��� �����, � ������ ���� ����� �����������
LANGREQ_FINEAT,//������� �������� ����, ������ ����� � �������� � ��������� ������������
LANGREQ_ABORT_AST,//������� ��������, ��� ������ �� ����� �������
LANGREQ_LOSEWIN,//����������� ������ ��� � �����������, �� �� ����� ������ ������ ��� �����.
//LANGREQ_REPEAT,//����� ��������� ����� ����� � ��� ������, ����� ��� ��������� ������� ��. � ��� �� ��� ������ ����. ��� ��� ������ ���� ��, � �� ������ ����!
//���� ��� ��������� ������ ��������� LANGREQ_TAKE ����� � �����. ��� ������ ��������� ������, ��� � ��� ���� ���� ������
LANGREQ_FINEAT_AST};//������� ��������, ��� ����� �������


enum STATE { STATE_IDLE,//����������� / ����� ��������
	STATE_READY_TO_TAKE,//�������, ��� ����� ����, � �� ��� ��������� / ����� ������
	STATE_EATING,//��� ��� / ����� ������
	STATE_ABORTED,//�� ���� �������� ��� ����� � ���� / ����� ��������
	STATE_HUNGRY,//�������, ��� ����� ���� /����� ��������
	STATE_GIVING,//�������, ��� ������ ����� / ����� ��������.��� ��������� ���������� ������ �����, ����� ��� �� ����� ��������� � ��� ��������!
	STATE_LOSEWAIT,//��������� ����� �����, �� �������� / ����� ��������. ���� ��� �� ����� ���������, �� �� ���������� �������� ����� ���, �� ���������� ���� ��� ���������!
	//STATE_TAKING,//����� �������� ����� �����. ������ ��� ��������� ������������ � ��� ������, ���� ����� ������ �� ��� �� ����� �������� ��� ����� � ��� �������� ����� ���
	STATE_LOSEWIN//�� ������� � ��� �� �����, �� �������, �� �� ���� �������� ��� �����
};

void TranslateLANGREQ(LANG &Value, char* Buffer)
{
	switch (Value)
	{
	case LANGREQ_NONE: {sprintf(Buffer, "LANGREQ_NONE"); return;}
	case LANGREQ_HUNGRY: {sprintf(Buffer, "LANGREQ_HUNGRY"); return;}
	case LANGREQ_GIVE: {sprintf(Buffer, "LANGREQ_GIVE"); return;}
	case LANGREQ_INSIST: {sprintf(Buffer, "LANGREQ_INSIST"); return;}
	case LANGREQ_TAKE: {sprintf(Buffer, "LANGREQ_TAKE"); return;}
	case LANGREQ_AST: {sprintf(Buffer, "LANGREQ_AST"); return;}
	case LANGREQ_LOSEWAIT: {sprintf(Buffer, "LANGREQ_LOSEWAIT"); return;}
	case LANGREQ_ABORT: {sprintf(Buffer, "LANGREQ_ABORT"); return;}
	case LANGREQ_FINEAT: {sprintf(Buffer, "LANGREQ_FINEAT"); return;}
	case LANGREQ_ABORT_AST: {sprintf(Buffer, "LANGREQ_ABORT_AST"); return;}
	case LANGREQ_FINEAT_AST: {sprintf(Buffer, "LANGREQ_FINEAT_AST"); return;}
	case LANGREQ_LOSEWIN: {sprintf(Buffer, "LANGREQ_LOSEWIN"); return;}
	}
}
void TranslateSTATE(STATE &Value, char* Buffer)
{
	switch (Value)
	{
	case STATE_IDLE: {sprintf(Buffer, "STATE_IDLE"); return;}
	case STATE_READY_TO_TAKE: {sprintf(Buffer, "STATE_READY_TO_TAKE"); return;}
	case STATE_EATING: {sprintf(Buffer, "STATE_EATING"); return;}
	case STATE_ABORTED: {sprintf(Buffer, "STATE_ABORTED"); return;}
	case STATE_HUNGRY: {sprintf(Buffer, "STATE_HUNGRY"); return;}
	case STATE_GIVING: {sprintf(Buffer, "STATE_GIVING"); return;}
	case STATE_LOSEWAIT: {sprintf(Buffer, "STATE_LOSEWAIT"); return;}
	case STATE_LOSEWIN: {sprintf(Buffer, "STATE_LOSEWIN"); return;}
	}
}


class philosopher;

class PhiloStates//�����-������, ����������� � ���� ��������� ��������. ����� �����-�����������
{
protected:
public:
	virtual PhiloStates* Act(philosopher* Philo) = 0;
	PhiloStates() {};
};
class PhiloState_idle : public PhiloStates
{
	PhiloStates* Act(philosopher * Philo);
public:
	PhiloState_idle() {}
};
class PhiloState_hungry : public PhiloStates
{
	PhiloStates* Act(philosopher * Philo);
public:
	PhiloState_hungry() {}
};
class PhiloState_losewait : public PhiloStates
{
	PhiloStates* Act(philosopher * Philo);
public:
	PhiloState_losewait() {}
};
class PhiloState_TAKE : public PhiloStates
{
	PhiloStates* Act(philosopher * Philo);
public:
	PhiloState_TAKE() {}
};
/*
class PhiloState_FINAL : public PhiloStates
{
	PhiloStates* Act(philosopher * Philo);
public:
	PhiloState_FINAL() {}
};
*/


class philosopher
{
public://� ��� ��� ��� ��������. ������������ ��� ��������!
	friend PhiloStates;
	static int Amount;//����� ��������� �� ������
	
	int Rank;//������� ������� ��������
	bool Hunger;//������� �� �������
	int MPI_Rank;//����� ��������, �� ������� �������� �������

	long CycleCount;
	
	

	struct MSG
	{
		LANG Type;
		int Value;
		MSG() : Type(LANGREQ_NONE), Value(0) {}
	};

	MPI_Request ReqSend[2], ReqRecv[2];//�� ��� �������� �� ����� � �������� ���������
	MPI_Status StatSend[2], StatRecv[2];//�������� ���� �� ��� �� 2

	int DiceRes;/*����� ������� ������������, �� ������ ����� � �������� ��������� ������ ����� �������.
				���� ��� ����� �� ����� ����, �� �� ������ �������� �����, ����� ��������� ������ ��������
				����������, ���� ���������� �����*/
	int NeghborDice[2];
	STATE SelfState;//����������� ���������
	STATE NeghborState[2];//��������� ������
	MSG LastRecved[2];
	MSG LastSended[2];
	bool NewMSG[2];
	bool ToSendMSG[2];
	bool Answered[2];
	bool AST[2];
	FILE *PhiloLog; //������������� ��������.

	int EatingTimes;
	int TotalWaiting;


	PhiloStates *STATE;

	//��� ������� ��������� �������� ����� ������ ����� � ������
	inline int LeftNeighbor()
	{
		return  (Rank - 1 + Amount) % Amount;
	}
	inline int RightNeighbor()
	{
		return  (Rank + 1 + Amount) % Amount;
	}
public:

	//��������� ���� ���� �� ����� ��������
	void Cycle()
	{
		
#ifndef SILENT
		printf("________________________________________\n");	
		printf("|Philo #%i is performing %i'th cycle\n", Rank, CycleCount);		
#endif
		fprintf(PhiloLog, "________________________________________\n");
		fprintf(PhiloLog, "|Philo #%i is performing %i'th cycle\n", Rank, CycleCount);

		if (!Hunger)
		{
			int Dice = (rand()+100) % 100;
#ifndef SILENT
			printf("He's throwing dice:%i, ",Dice);	
#endif
			fprintf(PhiloLog, "He's throwing dice:%i, ", Dice);
			if (Dice < HUNGER_CHANCE) 
			{ 
				Hunger = true;
#ifndef SILENT
				printf("and became hunger\n");
#endif
				fprintf(PhiloLog, "and became hunger\n");
			}//���� ������� �� �������, �� � ��������� ������ �� ����� �������������
			else
			{
#ifndef SILENT
				printf("and stil full\n");
#endif
				fprintf(PhiloLog, "and stil full\n");
			}
		}
		else
		{
#ifndef SILENT
			printf("He's still hunger!\n");
#endif
		}

		char buffer1[32], buffer2[32], buffer3[32];
		TranslateSTATE(SelfState, buffer1);
		TranslateSTATE(NeghborState[0], buffer2);
		TranslateSTATE(NeghborState[1], buffer3);
#ifndef SILENT
		printf("His Official State is %s; Neighbor[0]:%s; Neighbour[1]:%s\n", buffer1, buffer2, buffer3);
#endif
		fprintf(PhiloLog, "His Official State is %s; Neighbor[0]:%s; Neighbour[1]:%s\n", buffer1, buffer2, buffer3);

		NewMSG[0] = false; NewMSG[1] = false;//����� ����, � ������ ���������� ���������.
		/*����� ��������: �� ���� ���� �������
		������� ������ ��������������(�������� �����, � ������ ������)			|State|
		����� �������� �������� ��������� �� ����� �������,						|Recv |
		� �� ��� �� ������ ��������� ������ ����� �� ���������� ���-��.			|Send |
		� ���� �� ������ ������ � �������� �� ������ �� ����					
		*/

		//���� ��������� ���������

		int Recved = false;

		MPI_Iprobe(LeftNeighbor(), Rank * 100 + LeftNeighbor(), MPI_COMM_WORLD, &Recved, &(StatRecv[0]));//�������, ������ �� ��� �������� �� ������ �����	
		if (Recved)
		{//���� ��������� ����, �� �������� ��� � ����������, ��� ��������� ����������
			MPI_Recv(&(LastRecved[0]), 2, MPI_INT, LeftNeighbor(), Rank * 100 + LeftNeighbor(), MPI_COMM_WORLD, &(StatRecv[0]));
			NewMSG[0] = true;

			char LANGREQIN1[32];
			TranslateLANGREQ(LastRecved[0].Type, LANGREQIN1);
#ifndef SILENT
			printf("He reccived new message from LeftNeghbour; [%s,%i]\n", LANGREQIN1, LastRecved[0].Value);
#endif
			fprintf(PhiloLog, "He reccived new message from LeftNeghbour; [%s,%i]\n", LANGREQIN1, LastRecved[0].Value);
		}
		Recved = false;
		MPI_Iprobe(RightNeighbor(), Rank * 100 + RightNeighbor(), MPI_COMM_WORLD, &Recved, &(StatRecv[1]));
		if (Recved)
		{
			MPI_Recv(&(LastRecved[1]), 2, MPI_INT, RightNeighbor(), Rank * 100 + RightNeighbor(), MPI_COMM_WORLD, &(StatRecv[1]));
			NewMSG[1] = true;

			char LANGREQIN2[32];
			TranslateLANGREQ(LastRecved[1].Type, LANGREQIN2);
#ifndef SILENT
			printf("He reccived new message from RightNeghbour; [%s,%i]\n", LANGREQIN2, LastRecved[1].Value);
#endif
			fprintf(PhiloLog, "He reccived new message from LeftNeghbour; [%s,%i]\n", LANGREQIN2, LastRecved[1].Value);
		}
		
		//���� ��������� �������
#ifndef SILENT
		printf("Now he performing his own state\n");
#endif
		fprintf(PhiloLog, "Now he performing his own state\n");
		PhiloStates *tmp = STATE->Act(this);//��������� �������� � ����������� � ����������
		delete STATE; //������� ������ ���������
		STATE = tmp;//� ������ �� ��� ����� ��, ��� ��� ������� ����

		//���� ������
	
			if (ToSendMSG[0])
			{
				char LANGREQOUT1[32];
				TranslateLANGREQ(LastSended[0].Type, LANGREQOUT1);
#ifndef SILENT
				printf("He sanding Message [%s;%i] to his LeftNeghbour\n", LANGREQOUT1, LastSended[0].Value);
#endif
				fprintf(PhiloLog, "He sanding Message [%s;%i] to his LeftNeghbour\n", LANGREQOUT1, LastSended[0].Value);
				MPI_Isend(&LastSended[0], 2, MPI_INT, LeftNeighbor(), LeftNeighbor()*100 + Rank, MPI_COMM_WORLD, &(ReqSend[0]));
			}
			if (ToSendMSG[1])
			{
				char LANGREQOUT2[32];
				TranslateLANGREQ(LastSended[1].Type, LANGREQOUT2);
#ifndef SILENT
				printf("He sanding Message [%s;%i] to his RightNeghbour\n", LANGREQOUT2, LastSended[1].Value);
#endif
				fprintf(PhiloLog, "He sanding Message [%s;%i] to his RightNeghbour\n", LANGREQOUT2, LastSended[1].Value);
				MPI_Isend(&LastSended[1], 2, MPI_INT, RightNeighbor(), RightNeighbor() * 100 + Rank, MPI_COMM_WORLD, &(ReqSend[1]));
			}

			char STATEOUT2[32];
			TranslateSTATE(SelfState, STATEOUT2);
#ifndef SILENT
			printf("And he's done with a state %s!\n", STATEOUT2);
			printf("________________________________________\n\n\n");
#endif
			fprintf(PhiloLog, "And he's done with a state %s!\n", STATEOUT2);
			fprintf(PhiloLog, "________________________________________\n\n\n");

			Sleep(ACT_SPEED);
		ToSendMSG[0] = false; ToSendMSG[1] = false;//��� ��������� �����������
		CycleCount++;
	}

	philosopher()
	{
		Rank = Amount;
		Amount++;
		CycleCount = 0;
		DiceRes = -1;

		SelfState = STATE_IDLE;
		NeghborState[0] = STATE_IDLE; NeghborState[1] = STATE_IDLE;
		NewMSG[0] = false; NewMSG[1] = false;
		ToSendMSG[0] = false; ToSendMSG[1] = false;
		Answered[0] = false; Answered[1] = false;
		AST[0] = false; AST[1] = false;
		NeghborDice[0] = -1; NeghborDice[1] = -1;
		STATE = new PhiloState_idle;//��������� �������� ������������

		char ForkName[32];
		sprintf(ForkName, "Fork%i.txt", Rank);
		FILE *ForkFile = fopen(ForkName, "w");//������ ������� �������� � ����� ���� ����� � ������ � ������
		fclose(ForkFile);

		sprintf(ForkName, "Philo%i.txt", Rank);
		PhiloLog = fopen(ForkName, "w");//� �� ���� ��� � �������� ������ ���� �������������.

		EatingTimes = 0;
		TotalWaiting = 0;

	}

	~philosopher()
	{
		fprintf(PhiloLog, "\n----------------------------------------------\nEatingTimes:%i; TotalWaiting:%i; AvgWaitng:%f", EatingTimes, TotalWaiting, (float)TotalWaiting/EatingTimes);
		printf("\n----------------------------------------------\nPhilo#%i: EatingTimes:%i; TotalWaiting:%i; AvgWaitng:%f\n", Rank, EatingTimes, TotalWaiting, (float)TotalWaiting / EatingTimes);
		fclose(PhiloLog);
		Amount--;
	}

};
int philosopher::Amount = 0;


PhiloStates * PhiloState_idle::Act(philosopher * Philo)
	{

		/*�� �������������, � ������ �� ��� ���� ��� ���-������ �� �������*/
		Philo->SelfState = STATE_IDLE;
		
		//���� ������

		if (Philo->NewMSG[0])//���� ���� ����� ��������� �� ������
		{
			switch (Philo->LastRecved[0].Type)
			{
				case LANGREQ_HUNGRY://���� ����� ��������� ���, ��� ������� � ����� ����� �����
				{
					Philo->LastSended[0].Type = LANGREQ_GIVE; Philo->LastSended[0].Value = -1;//������ ��� ���
					Philo->NeghborState[0] = STATE_READY_TO_TAKE;//����������, ��� ������ ������ ���������� ����� �����
					Philo->ToSendMSG[0] = true; //��������, ��� ����� ��������� ���������
					Philo->NeghborDice[0] = Philo->LastSended[0].Value;
					break;
				}
				case LANGREQ_TAKE://���� ����� ��������� ���, ��� ��� ��� ������ � �� ����� �����
				{
					Philo->LastSended[0].Type = LANGREQ_AST; Philo->LastSended[0].Value = -1;//��������, ��� �� ���� ���
					Philo->NeghborState[0] = STATE_EATING;//����������, ��� ������ ������ ���������� ����� �����
					Philo->ToSendMSG[0] = true; //��������, ��� ����� ��������� ���������
					break;
				}
				case LANGREQ_ABORT://����� ���������� ������������ ������� ���������� �����
				{
					Philo->LastSended[0].Type = LANGREQ_ABORT_AST; Philo->LastSended[0].Value = -1;//��������, ��� �� ���� ���
					Philo->NeghborState[0] = STATE_ABORTED;//����� ��� ��� �������, �� ������ �� ����� ���������� ��� �����
					Philo->ToSendMSG[0] = true; //��������, ��� ����� ��������� ���������
					break;
				}
				case LANGREQ_FINEAT://����� �������� � ����
				{
					//Philo->LastSended[0].Type = LANGREQ_FINEAT_AST; Philo->LastSended[0].Value = -1;//��������, ��� �� ���� ���
					Philo->NeghborState[0] = STATE_IDLE;//����� ������ ��������
					//Philo->ToSendMSG[0] = true; //��������, ��� ����� ��������� ���������
					break;
				}
				default:
				{
					printf("Rank[%i];STATE_ILDE; Message from LEFT [%i,%i]; is unexpected!\n",Philo->Rank, Philo->LastRecved[0].Type, Philo->LastRecved[0].Value);
				}
			}
		}

		if (Philo->NewMSG[1])//���� ���� ����� ��������� �� ������
		{
			switch (Philo->LastRecved[1].Type)
			{
			case LANGREQ_HUNGRY://���� ����� ��������� ���, ��� ������� � ����� ����� �����
			{
				Philo->LastSended[1].Type = LANGREQ_GIVE; Philo->LastSended[1].Value = -1;//������ ��� ���
				Philo->NeghborState[1] = STATE_READY_TO_TAKE;//����������, ��� ������ ������ ���������� ����� �����
				Philo->ToSendMSG[1] = true; //��������, ��� ����� ��������� ���������
				Philo->NeghborDice[1] = Philo->LastSended[1].Value;
				break;
			}
			case LANGREQ_TAKE://���� ����� ��������� ���, ��� ��� ��� ������ � �� ����� �����
			{
				Philo->LastSended[1].Type = LANGREQ_AST; Philo->LastSended[1].Value = -1;//��������, ��� �� ���� ���
				Philo->NeghborState[1] = STATE_EATING;//����������, ��� ������ ������ ���������� ����� �����
				Philo->ToSendMSG[1] = true; //��������, ��� ����� ��������� ���������
				break;
			}
			case LANGREQ_ABORT://����� ���������� ������������ ������� ���������� �����
			{
				Philo->LastSended[1].Type = LANGREQ_ABORT_AST; Philo->LastSended[1].Value = -1;//��������, ��� �� ���� ���
				Philo->NeghborState[1] = STATE_ABORTED;//����� ��� ��� �������, �� ������ �� ����� ���������� ��� �����
				Philo->ToSendMSG[1] = true; //��������, ��� ����� ��������� ���������
				break;
			}
			case LANGREQ_FINEAT://����� �������� � ����
			{
				//Philo->LastSended[1].Type = LANGREQ_FINEAT_AST; Philo->LastSended[1].Value = -1;//��������, ��� �� ���� ���
				Philo->NeghborState[1] = STATE_IDLE;//����� ������ ��������
				//Philo->ToSendMSG[1] = true; //��������, ��� ����� ��������� ���������
				break;
			}
			default:
			{
				printf("Rank[%i];STATE_ILDE; Message from LEFT [%i,%i]; is unexpected!\n", Philo->Rank, Philo->LastRecved[1].Type, Philo->LastRecved[1].Value);
			}
			}
		}

		if (Philo->Hunger)//���� �������������, �� ��������� ��� �������� � ��������� ������. ����� ����� ������������.
		{
			return new PhiloState_hungry;
		}
		else
		{
			return new PhiloState_idle;
		}
	}

PhiloStates* PhiloState_hungry::Act(philosopher* Philo)
	{
		Philo->TotalWaiting++;

		//���������, ����� �� �� �������� ������������ �� �����
		//��������. �� ��� ����� �������� �������, ��� ����� �����, ��� �� �� ���������� ���, ��� ��� ��� ���������� � �� ��� ����

		//���� �� ������ �������������, �� ���� �� ���� ���������
		if (Philo->SelfState != STATE_HUNGRY)
		{
			Philo->DiceRes = rand();//������ �����
			for (int i = 0; i < 2; i++)
			{
				if (Philo->NeghborState[i] == STATE_READY_TO_TAKE)//���� �� ��� ������ ��� �����
				{
					/*������� ��� ���� �����. ���� � ��� ����� READY_TO_TAKE �� ������, ������ �� ��� ������� �� ��� LANGREQ_GIVE
					��, �������, ��� ���������. � ��� ��� ����� ����� ��� ������������ � ���, ��� �� ����� �����, ���
					��� ��� ����� ���� ���������*/
					Philo->LastSended[i].Type = LANGREQ_LOSEWAIT; Philo->LastSended[i].Value = -1;//�� ����� ����������� ���
					Philo->SelfState = STATE_LOSEWAIT;
				}
				else
				{
					Philo->LastSended[i].Type = LANGREQ_HUNGRY; Philo->LastSended[i].Value = Philo->DiceRes;
					Philo->ToSendMSG[i] = true;
				}
				//�������� �������, ��� �������, � �� ���� ��������� ������ ������
			}
			
			
			//�� ����� ��������� �� ��� ���, ��������� ��������, ���� �� ���� �� ��������! ������ ���� �������� ��� � ��������� ��������!
			//if (Philo->SelfState == STATE_LOSEWAIT)//���� ����� ����� ����� �� ��� ���������, �� �������� ��������� � �� ���� �����������.
			//{ return new PhiloState_losewait; }

			
			Philo->SelfState = STATE_HUNGRY;


			
		}
		//� ������ ����������, ��� �� ������������� � ��� ���� ����������


		//���� ��������� ���������

		for (int i = 0; i < 2; i++)
		{
			if (Philo->NewMSG[i])//���� ���� ����� ��������� �� ������
			{
				switch (Philo->LastRecved[i].Type)
				{
					case LANGREQ_HUNGRY://���� ������ ������� ���� �������
					{
						if (Philo->SelfState == STATE_LOSEWAIT)//���� �� ��� ����� ���� ��������� ��� �� �����
						{
							Philo->NeghborDice[i] = Philo->LastSended[i].Value;
							if (Philo->LastRecved[i].Value < Philo->DiceRes)//���� �� �������� �� ���
							{
								Philo->LastSended[i].Type = LANGREQ_LOSEWIN; Philo->LastSended[i].Value = -1;//��� �� �� ������� ��� �����, �� ��� ��������, ����� ���� �� ��� ��������� ���
								Philo->NeghborState[i] = STATE_LOSEWAIT;//�� ��� ������ ����� �������� ����, �� ������� ��� �����!
								Philo->ToSendMSG[i] = true;
							}
							else if (Philo->LastRecved[i].Value < Philo->DiceRes)//���� �� �� ���������
							{
								Philo->LastSended[i].Type = LANGREQ_LOSEWAIT; Philo->LastSended[i].Value = -1;//������ �������, ��� � ������ ����������. � �� ��� ����� �� ������� ��� �����
								Philo->NeghborState[i] = STATE_READY_TO_TAKE;//����������, ��� �� ������� ����� �����
								Philo->ToSendMSG[i] = true;	
							}
							
						}
						else if (Philo->LastRecved[i].Value < Philo->DiceRes)//���� ��� ����� ��������� ������, �� �� ����� �����
						{
							//Philo->LastSended[i].Type = LANGREQ_TAKE; Philo->LastSended[i].Value = -1;//���� �� ���� ���������. ���� ����� �� �������
							Philo->NeghborState[i] = STATE_LOSEWAIT;//����������, ��� �� ������� ����� �����
							//Philo->ToSendMSG[i] = true;
							
						}
						else if (Philo->LastRecved[i].Value > Philo->DiceRes)//���� ��� ����� ��������� ������, �� �������� ��� �� ����
						{
							Philo->LastSended[i].Type = LANGREQ_LOSEWAIT; Philo->LastSended[i].Value = -1;
							Philo->SelfState = STATE_LOSEWAIT;
							Philo->NeghborState[i] = STATE_READY_TO_TAKE;//����������, ��� �� ������ ������� �����
							Philo->ToSendMSG[i] = true;
							//����� ��� ��������, ��� ����� ������� ������� �� ��� ������ �� LANGREQ_HUNGRY, � ����� LANGREQ_LOSEWAIT
							//������ ��� ���������� ������ � ��� ������, ���� �� ��� �������� �� ���� ��������� LANGREQ_HUNGRY � ��� ��������� �� ����
							//��� ��, ��� �� �����!
							//������ ����� ����� �������� ������ ���� �� ����� ���� ��������� ������� ������. � ��� ���� �� ������ � ��� � ������ ��� �������� ��������,
						}
						else
						{
							printf("Rank[%i];STATE_HUNGRY; Dices MATCH!!!\n");
						}
						break;
					}

					case LANGREQ_GIVE://���� ������ ������� ������ ��� 
					{
						Philo->NeghborState[i] = STATE_GIVING;//���������� ���
						break;
					}
					case LANGREQ_ABORT:
					{
						Philo->NeghborState[i] = STATE_ABORTED;
						break;
					}
					case LANGREQ_LOSEWAIT:
					{
						Philo->NeghborState[i] = STATE_LOSEWAIT;
						break;
					}
					case LANGREQ_FINEAT://���� ����� ������ ��� ����, �� ����������, ��� ������ �� ��������. 
					{
						//Philo->NeghborState[i] = STATE_IDLE;�� �� ������ ��������! �� ����� �����, ��� ��� �����, ��� �� �������, � ������ ��� ������ ������ ��� �����!
						Philo->NeghborState[i] = STATE_GIVING;
						break;

					}
					case LANGREQ_LOSEWIN:
					{
						Philo->NeghborState[i] = STATE_LOSEWIN;//����� �� ������ ���� �����, ���� � ���� ����������� ������� � ������
						break;
					}
					case LANGREQ_TAKE://�� ������ ��� �������������, � ��� ���������, ��� �� ��� � �������� ���� ������ ����� ������
					{//���-�� ����� �� ���� �� �����. �� ������� LANGREQ_FINEAT � ������ ������� ������ ���� �����!
					//�� ��� �� ����� �������������� �����������. ����������, ��� �� ����� ����!
						//���� �������� ������ ������ ��������, �������� ����, ������ ��� ��������� ��������, ���� �� ���� �� ��������!


						
					
						Philo->LastSended[i].Type = LANGREQ_AST; Philo->LastSended[i].Value = Philo->DiceRes;//�� ��������, ��� �� ������� � ������ � ���� ���������� ��� ��� ������ ��� �� ����������, ��� �� ���� ��������!
						Philo->NeghborState[i] = STATE_EATING;//����������, ��� ������ ������ ���������� ����� �����
						Philo->ToSendMSG[i] = true; //��������, ��� ����� ��������� ���������
						break;
					}
					default:
					{
						printf("Rank[%i];STATE_HUNGRY; Message from [%i] [%i,%i]; is unexpected!\n", Philo->Rank, i, Philo->LastRecved[i].Type, Philo->LastRecved[i].Value);

					}
				}
			}
		}

		if (Philo->SelfState == STATE_LOSEWAIT)
		{
			return new PhiloState_losewait;
		}
		else if (((Philo->NeghborState[0] == STATE_LOSEWAIT) || (Philo->NeghborState[0] == STATE_GIVING)) &&
			((Philo->NeghborState[1] == STATE_LOSEWAIT) || (Philo->NeghborState[1] == STATE_GIVING))
			||//���� ��� ���� ������ ������ �� ����� �������� �����
			((Philo->NeghborState[0] == STATE_LOSEWAIT) || (Philo->NeghborState[0] == STATE_LOSEWIN)) &&
			((Philo->NeghborState[1] == STATE_LOSEWAIT) || (Philo->NeghborState[1] == STATE_LOSEWIN))
			)
		{//���� ��� ������ ������ ��� �����, ��������� � � ������!
			for (int i = 0; i < 2; i++)
			{
				Philo->LastSended[i].Type = LANGREQ_TAKE; Philo->LastSended[i].Value = -1;//�������� ����, ��� ����� �����
				Philo->SelfState = STATE_EATING;
				Philo->ToSendMSG[i] = true;
			}
			return new PhiloState_TAKE;
		}
		else
		{
			return new PhiloState_hungry;
		}
	}

PhiloStates* PhiloState_losewait::Act(philosopher* Philo)
{
	Philo->TotalWaiting++;
	bool DelegateTAKE[2] = { false,false };

	for (int i = 0; i < 2; i++)
	{
		if (Philo->NewMSG[i])//���� ���� ����� ��������� �� ������
		{
			switch (Philo->LastRecved[i].Type)
			{
				case LANGREQ_HUNGRY:
				{	
					Philo->NeghborDice[i] = Philo->LastSended[i].Value;
					if (Philo->NeghborState[i] != STATE_GIVING)//���� ����� ���� ����� ��� �� ������� �����
					{
						//������ ��� �� �� �����, ��� � � STATE_HUNGRY, �� ��� �������� ������, ��� ��� ���������
						if (Philo->LastRecved[i].Value < Philo->DiceRes)//���� �� �������� �� ���
						{
							Philo->LastSended[i].Type = LANGREQ_LOSEWIN; Philo->LastSended[i].Value = -1;//��� �� �� ������� ��� �����, �� ��� ��������, ����� ���� �� ��� ��������� ���
							Philo->NeghborState[i] = STATE_LOSEWAIT;//�� ��� ������ ����� �������� ����, �� ������� ��� �����!
							Philo->ToSendMSG[i] = true;
						}
						else if (Philo->LastRecved[i].Value < Philo->DiceRes)//���� �� �� ���������
						{
							Philo->LastSended[i].Type = LANGREQ_LOSEWAIT; Philo->LastSended[i].Value = -1;//������ �������, ��� � ������ ����������. � �� ��� ����� �� ������� ��� �����
							Philo->NeghborState[i] = STATE_READY_TO_TAKE;//����������, ��� �� ������� ����� �����
							Philo->ToSendMSG[i] = true;
						}
					}
				break;
				}
				case LANGREQ_TAKE://����� ����� �����!
				{
					//��� ���� ���� �������� ������ ��� LANGREQ_LOSEWAIT, ������ ��� ����� ����� �� �� ������� �� ��� ������ ���������
					//������ �������� ��� ��� ����� ������� ����� ���� �� ��� LOSEWAIT ��� LOSEWIN, � �� �� ����� �� ������ ����� ������
					//� ����� ������ "�� ��� �� �� ��� ������ �����, ���� �� �"
					//� ��� ������. �� ����� ��� �����, ��� � ��� �� ������, ����� �� ���� ��� ����, �� �������� �� ������ ������� ������
					//���������� ���� ������
					if (Philo->NeghborState[i] == STATE_LOSEWAIT)//���� ����� ��� �������� � �������� ����� ����� ����� ������
					{
						DelegateTAKE[i] = true;//���� �������� ��� ���������. � ����� ���� ����� ���� �������������
						//�� �������� ��� LANGREQ_AST ���� �� ���� ����� ����� ����� �
						//LANGREQ_TAKE ���� ����� ����� ��!
						break;
					}
					Philo->LastSended[i].Type = LANGREQ_AST; Philo->LastSended[i].Value = -1;//�� ��������, ��� �� �������
					Philo->NeghborState[i] = STATE_EATING;//����������, ��� ������ ������ ���������� ����� �����
					Philo->ToSendMSG[i] = true; //��������, ��� ����� ��������� ���������
					break;
				}
				case LANGREQ_ABORT://����� �� ����� ����� ����� ������
				{
					//Philo->LastSended[0].Type = LANGREQ_ABORT_AST; Philo->LastSended[0].Value = -1;//���� ��������� ��� LANGREQ_ABORT_AST
					Philo->NeghborState[i] = STATE_ABORTED;//����������, ��� ������ ������ ���������� ����� �����
					Philo->ToSendMSG[i] = true; //��������, ��� ����� ��������� ���������
					break;
				}
				case LANGREQ_FINEAT://����� �������� ����
				{
					//���� ������ �� ����, �� ����� ����� ��� ������ ���� � ����� � �����, ����� �� ������� ��� �����.
					//Philo->LastSended[i].Type = LANGREQ_TAKE; Philo->LastSended[0].Value = -1;//�������� ��� �� �������� �����
					Philo->NeghborState[i] = STATE_GIVING;//�������, ��� ����� ������ ��� �����.
					//Philo->ToSendMSG[i] = true; //��������, ��� ����� ��������� ���������
					break;
				}

				case LANGREQ_LOSEWAIT://����� ���� �������� ��� �� �����, �� �� ��������� � ���, � ������ ������ ��� ��, � ����� ��.
				{
					Philo->NeghborState[i] = STATE_LOSEWAIT;
					break;
				}

				case LANGREQ_GIVE://����� ����� ������ ��� �����. ���� ����� �� �������� ��� � ������ �������, � ����� ���������, ��� �������� �� � ���. �� ���� ������ ����!
				{
					Philo->NeghborState[i] = STATE_GIVING;//���������� ��� �������� ���������
					break;
				}
				default:
				{
					printf("Rank[%i];STATE_LOSEWAIT; Message from [%i] [%i,%i]; is unexpected!\n", Philo->Rank, i, Philo->LastRecved[i].Type, Philo->LastRecved[i].Value);

				}


			}
		}
	}


	
	if(((Philo->NeghborState[0] == STATE_LOSEWAIT) || (Philo->NeghborState[0] == STATE_GIVING)) &&
		((Philo->NeghborState[1] == STATE_LOSEWAIT) || (Philo->NeghborState[1] == STATE_GIVING))
		||//���� ��� ���� ������ ������ �� ����� �������� �����
		((Philo->NeghborState[0] == STATE_LOSEWAIT) || (Philo->NeghborState[0] == STATE_LOSEWIN)) &&
		((Philo->NeghborState[1] == STATE_LOSEWAIT) || (Philo->NeghborState[1] == STATE_LOSEWIN))
		)//���� ��� ������ ������ ������ ��� �����
	{
		for (int i = 0; i < 2; i++)
		{
			Philo->LastSended[i].Type = LANGREQ_TAKE; Philo->LastSended[i].Value = -1;//�������� ����, ��� ����� �����
			Philo->ToSendMSG[i] = true;
		}
		//Philo->SelfState = STATE_EATING;
		return new PhiloState_TAKE;
	}
	else if (DelegateTAKE[0] || DelegateTAKE[1])//���� ����� ������� ���������� �����, ���� �� ������ ������� ��� ��
	{//� ����� ���� �� ��������� ���, ������ �� ��� ��� ����. ����� �� �� ���� �� ���� ��� ��������� �� ��� LANGREQ_TAKE
		for (int i = 0; i < 2; i++)
		{
			if(DelegateTAKE[i])
			{
				Philo->LastSended[i].Type = LANGREQ_AST; Philo->LastSended[i].Value = -1;//�� ��������, ��� �� �������
				Philo->NeghborState[i] = STATE_EATING;//����������, ��� ������ ������ ���������� ����� �����
				Philo->ToSendMSG[i] = true; //��������, ��� ����� ��������� ���������
				return new PhiloState_losewait;
			}
		}
	}
	else if (Philo->SelfState == STATE_LOSEWAIT)
	{
		return new PhiloState_losewait;
	}

		printf("rank:%i; losewait; I don't know where to go from this state!", Philo->Rank);
	

}

PhiloStates* PhiloState_TAKE::Act(philosopher* Philo)
{
	Philo->TotalWaiting++;
	for (int i = 0; i < 2; i++)
	{
		if (Philo->NewMSG[i])//���� ���� ����� ��������� �� ������
		{
			switch (Philo->LastRecved[i].Type)
			{
			case LANGREQ_AST://����� �������� ��� ����
				{
					if (Philo->LastRecved[i].Value != -1)//������ ����� ���������� ���� ���������, ���� ��� ������ ��������
					{
						Philo->NeghborState[i] = STATE_LOSEWAIT;//���������� ��� �������
					}
					Philo->AST[i] = true;
					break;
				}
			case LANGREQ_HUNGRY: //� ��� ��� � � ����������� �������� ������ ���� ���������. �� � ����� ����, ����� ��� ����� ��� ����
				{//�� ����� ���� ������, ��� ����� �������� ��� ������� � TAKE, � ��� ���� �� ����, ��� �� ��� ����� �����.
					//� ������ ��� ������ ������� ���������, ������� ����� ���������������
					Philo->NeghborState[i] = STATE_LOSEWAIT;//�������, ��� �� �� ������ ������������, �� ��� � �������� ���, ���� �� ��� ����
					Philo->AST[i] = true;//� ��� �� ��� ��������, �� �� �������� ����!
					break;
				}
			case LANGREQ_LOSEWAIT://����� ����� ����, �� ������� ��� ��������, ������ ��� ������ �� �������� ���
				{
					//Philo->NeghborDice[i] = Philo->LastSended[i].Value;�������� ��� ���� �� �����
					Philo->NeghborState[i] = STATE_LOSEWAIT;//�� ���� ����������, ��� �� ������������
					Philo->AST[i] = true;//��� �� � ���. ��� �� �������� - �� �������� ����!
				}
			case LANGREQ_GIVE://������� ����� ��������, ��� ��� ��������� ��������� ��������. ����� �������, ��� ��� ������ ��-�� ������� �� �������.
			{
				//� ������ ������ �� ����� ������. �� ������ ������������ ��� ��������� � ���� �����������
			}
			default:
				{
					printf("Rank[%i];STATE_TAKE; Message from [%i] [%i,%i]; is unexpected!\n", Philo->Rank, i, Philo->LastRecved[i].Type, Philo->LastRecved[i].Value);

				}

			}
		}
	}

	if (Philo->AST[0] == true && Philo->AST[1] == true)//���� ��� ������ ������� ��� ������ �� ��������
	{
		char buffer[50];//���� ������ ������
		sprintf(buffer, "Fork%i.txt", Philo->Rank);
		FILE *FORK = fopen(buffer, "a");
		fprintf(FORK, "Philosopher �%i eating with this fork on his %i's cycle, yay!\n", Philo->Rank, Philo->CycleCount);
		fclose(FORK);

		sprintf(buffer, "Fork%i.txt", Philo->LeftNeighbor());//���� ������ ������
		FORK = fopen(buffer, "a");
		fprintf(FORK, "Philosopher �%i eating with this fork on his %i's cycle, yay!\n", Philo->Rank, Philo->CycleCount);
		fclose(FORK);

		Philo->EatingTimes++;
		Philo->TotalWaiting--;//� ���� ��� �� ��� ���� ����, � �� ����

		for (int i = 0; i < 2; i++)
		{		
		Philo->LastSended[i].Type = LANGREQ_FINEAT; Philo->LastSended[i].Value = -1;
		Philo->ToSendMSG[i] = true;
		Philo->AST[i] = false;
		if ((Philo->NeghborState[i] == STATE_LOSEWAIT))//���� ��� ����� �������
			{
				Philo->NeghborState[i] = STATE_READY_TO_TAKE;//���� ����� �� ����� ��� ��������, �� �� ����� ��������� ��� ������ �����.
			}
		if(Philo->NeghborState[i] == STATE_GIVING)
			{
				Philo->NeghborState[i] = STATE_IDLE;//���� ����� ������ ������� ��� �����, �� ������ ����� �� ������ �����������
			}
		}
		Philo->Hunger = false;
		Philo->SelfState = STATE_IDLE;


		return new PhiloState_idle;//������������ � ��������� ��������
	}
	else
	{
		return new PhiloState_TAKE;
	}
	
}

int main(int argc, char *argv[])
{
	int rank, Size;
	MPI_Status stat;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &Size);


#ifdef SINGLE_PROCESS 
	/*��� ��� ��� ��� �������� �� ����� ��������.
	��� �������� ������������ �� �������, � ������ � ��������� �� �������� ����*/

	philosopher A[5];
	A[0].MPI_Rank = 0;
	A[1].MPI_Rank = 0;
	A[2].MPI_Rank = 0;
	A[3].MPI_Rank = 0;
	A[4].MPI_Rank = 0;
	A[2].MPI_Rank = 0;

	while (true)
	{
		A[0].Cycle();
		A[1].Cycle();
		A[2].Cycle();
		A[3].Cycle();
		A[4].Cycle();
		A[2].Cycle();
		printf("------------------AND OF CYCLE!---------------------\n\n\n");

		if ((A[0].CycleCount > 100) && (A[1].CycleCount > 100) && (A[2].CycleCount > 100) && (A[3].CycleCount > 100) && (A[4].CycleCount > 100))
		{
			MPI_Finalize();
			return 0;
		}

	}
#else //� ��� ��� ��� �� ������������� ������
	philosopher A;
	A.MPI_Rank = rank;
	A.Rank = rank;
	A.Amount = Size;

	srand(rank * 120 + rank * 6 + rank);
	MPI_Request ENDSend[2];//�������� ��� �������� ��������� �� �������� ��������
	MPI_Status ENDStat;//������ ��� ������ ��������� ���������
	int ENDCounter = 0;//������� ������� ���������. ��� ������ �������� ���, �� �����������

	int END = 7612;//����� ������ ����������, �� ������� ��������� ���� ���
	int Recved = false;

	while (true)
	{
		
		A.Cycle();



	
		//��� �������� ��������� � ���������� �� ������ �������. ��� ������ ��� ���������, �������������.
		if (A.CycleCount == 11000)//�������� �����. ���� ������� ��������� � ���, ��� �� ��������� � ���� ��.
		{
			ENDCounter++;
			MPI_Isend(&END, 1, MPI_INT, 0, 99999, MPI_COMM_WORLD, &(ENDSend[0]));//�������� �� ������ �������, ��� ��� ���

		}
		if (A.CycleCount >= 11000)
		{
			if (rank == 0)//� ��� ��� ������ ������� �������
			{
				Recved = false;
				MPI_Iprobe(MPI_ANY_SOURCE, 99999, MPI_COMM_WORLD, &Recved, &ENDStat);
				while (Recved)//�� �������� ������������ ���������� ��������� ����������� ���������
				{
					MPI_Recv(&END, 2, MPI_INT, MPI_ANY_SOURCE, 99999, MPI_COMM_WORLD, &ENDStat);
					ENDCounter++;
					MPI_Iprobe(MPI_ANY_SOURCE, 99999, MPI_COMM_WORLD, &Recved, &ENDStat);//���� ��� �� ��������
				}
				if (ENDCounter == Size)
				{
					for (int i = 1; i < Size; i++)
					{
						MPI_Send(&END, 1, MPI_INT, i, 99999, MPI_COMM_WORLD);//�� ��� ������ ���������. ��� �������� �� �����������, ��� ��� �� ��������� ��������!	
					}
					MPI_Barrier(MPI_COMM_WORLD);//������ ������ �� ����� � �������
					MPI_Finalize();//� ������� �����������
					return 0;
				}

			}




			MPI_Iprobe(0, 99999, MPI_COMM_WORLD, &Recved, &ENDStat);//���� ����� �� ������, ��� ��� ���������
			if (Recved)
			{
				MPI_Recv(&END, 2, MPI_INT, 0, 99999, MPI_COMM_WORLD, &ENDStat);
				MPI_Barrier(MPI_COMM_WORLD);//�� ������ ������ ����������� ���������.
				MPI_Finalize();
				return 0;
			}
		}
		
	}

#endif // SINGLE_PROCESS
	
	MPI_Finalize();
    return 0;
}

