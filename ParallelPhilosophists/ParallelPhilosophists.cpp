// ParallelPhilosophists.cpp: определяет точку входа для консольного приложения.
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
Немного о сообщениях. Они выглядят как int[2]; [0] - Тип сообщения. О чем мы говорим. [1] - Параметр
*/

enum LANG { LANGREQ_NONE,//Нулевое сообщение
	LANGREQ_HUNGRY,//Сообщение о том, что философ голоден
	LANGREQ_GIVE,//Сообщение о том, что философ готов отдать вилку
	LANGREQ_INSIST,//Филосов настаивает, что вилка должна достаться ему. Эквивалентен TAKE
	LANGREQ_TAKE,//Филосов сообщает, что берет вилку
	LANGREQ_AST,//Филосов запомнил, что вилка взята
	LANGREQ_LOSEWAIT,//Филосов проиграл спор за вилку, но ждет, когда она освободится
	LANGREQ_ABORT,//Филосов не смог получить обе вилки, и теперь ждет такой возможности
LANGREQ_FINEAT,//Филосов закончил есть, вернул вилку и вернулся в положение простаивания
LANGREQ_ABORT_AST,//Филосов запомнил, что запрос на вилку отменен
LANGREQ_LOSEWIN,//Отправитель виграл бой с получателем, но не может сейчас занять обе вилки.
//LANGREQ_REPEAT,//Сосед попытался взять вилку в тот момент, когда это собрались сделать мы. У нас на нее больше прав. Так что сейчас едим мы, а он пускай ждет!
//Пока что попробуем просто отправить LANGREQ_TAKE прямо в ответ. Это должно намекнуть соседу, что у нас таки прав больше
LANGREQ_FINEAT_AST};//Филосов запомнил, что вилку вернули


enum STATE { STATE_IDLE,//Простаивает / Вилка свободна
	STATE_READY_TO_TAKE,//Сообщил, что хочет есть, а мы ему разрешили / Вилка занята
	STATE_EATING,//Уже ест / Вилка занята
	STATE_ABORTED,//Не смог получить обе вилки и ждет / Вилка свободна
	STATE_HUNGRY,//Сообщил, что хочет есть /Вилка свободна
	STATE_GIVING,//Сообщил, что отдает вилку / Вилка свободна.Это сообщение посылается именно тогда, когда бой за вилку состоялся и был проигран!
	STATE_LOSEWAIT,//Попытался взять вилку, но проиграл / Вилка свободна. Если бой за вилку состоялся, но не получилось получить сразу обе, то победитель шлет это сообщение!
	//STATE_TAKING,//Сосед пытается взять вилку. Обычно это состояние используется в том случае, если прямо сейчас мы уже не можем уступить ему вилку и ему придется ждать еще
	STATE_LOSEWIN//Мы сыграли с ним за вилку, он победил, но не смог получить обе вилки
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

class PhiloStates//Класс-корень, воплощающий в себе состояния философа. Класс чисто-виртуальный
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
public://У нас тут все публично. Инкапсуляция для слабаков!
	friend PhiloStates;
	static int Amount;//Всего философов за столом
	
	int Rank;//Позиция данного философа
	bool Hunger;//Голоден ли философ
	int MPI_Rank;//Номер процесса, на котором работает философ

	long CycleCount;
	
	

	struct MSG
	{
		LANG Type;
		int Value;
		MSG() : Type(LANGREQ_NONE), Value(0) {}
	};

	MPI_Request ReqSend[2], ReqRecv[2];//По два реквеста на прием и отправку сообщения
	MPI_Status StatSend[2], StatRecv[2];//Статусов тоже на все по 2

	int DiceRes;/*Когда философ проголодался, он кидает кости и сообщает результат броска своим соседям.
				Если его сосед не хочет есть, то он просто уступает вилку, иначе результат броска помогает
				определить, кому достанется вилка*/
	int NeghborDice[2];
	STATE SelfState;//Собственное состояние
	STATE NeghborState[2];//Состояние соседа
	MSG LastRecved[2];
	MSG LastSended[2];
	bool NewMSG[2];
	bool ToSendMSG[2];
	bool Answered[2];
	bool AST[2];
	FILE *PhiloLog; //Жизнеописание философа.

	int EatingTimes;
	int TotalWaiting;


	PhiloStates *STATE;

	//Эти функции позволяют получить номер соседа слева и справа
	inline int LeftNeighbor()
	{
		return  (Rank - 1 + Amount) % Amount;
	}
	inline int RightNeighbor()
	{
		return  (Rank + 1 + Amount) % Amount;
	}
public:

	//Реализует один такт из жизни философа
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
			}//Если филосов не голоден, то с некоторым шансом он может проголодаться
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

		NewMSG[0] = false; NewMSG[1] = false;//Новый цикл, а значит сбрасываем сообщения.
		/*Новый положняк: за один такт философ
		сначала делает предподготовку(вычислят голод, и прочая ерунда)			|State|
		потом пытается получить сообщения от своих соседей,						|Recv |
		и за тем на основе собранных данных нужно ли отправлять что-то.			|Send |
		И того по одному приему и отправке на соседа за такт					
		*/

		//БЛОК ПОЛУЧЕНИЯ СООБЩЕНИЙ

		int Recved = false;

		MPI_Iprobe(LeftNeighbor(), Rank * 100 + LeftNeighbor(), MPI_COMM_WORLD, &Recved, &(StatRecv[0]));//Пробуем, пришло ли нам соощение от соседа слева	
		if (Recved)
		{//Если сообщение есть, то получаем его и записываем, как последнее полученное
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
		
		//БЛОК ОБРАБОТКИ ДАНННЫХ
#ifndef SILENT
		printf("Now he performing his own state\n");
#endif
		fprintf(PhiloLog, "Now he performing his own state\n");
		PhiloStates *tmp = STATE->Act(this);//Выполняем действия в соответсвии с состоянием
		delete STATE; //Удаляем старое состояние
		STATE = tmp;//И ставим на его место то, что нам вернули выше

		//БЛОК ОТВЕТА
	
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
		ToSendMSG[0] = false; ToSendMSG[1] = false;//Все пересылки закончились
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
		STATE = new PhiloState_idle;//Состояния простого простаивания

		char ForkName[32];
		sprintf(ForkName, "Fork%i.txt", Rank);
		FILE *ForkFile = fopen(ForkName, "w");//Каждый философ приносит с собой одну вилку и кладет её справа
		fclose(ForkFile);

		sprintf(ForkName, "Philo%i.txt", Rank);
		PhiloLog = fopen(ForkName, "w");//А за одно еще и начинает писать свою автобиографию.

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

		/*Мы бездельничаем, и делаем мы это пока нам кто-нибудь не напишет*/
		Philo->SelfState = STATE_IDLE;
		
		//БЛОК ОТВЕТА

		if (Philo->NewMSG[0])//Если есть новые сообщения от соседа
		{
			switch (Philo->LastRecved[0].Type)
			{
				case LANGREQ_HUNGRY://Если сосед оповещает нас, что голоден и хочет взять вилку
				{
					Philo->LastSended[0].Type = LANGREQ_GIVE; Philo->LastSended[0].Value = -1;//Отдаем без боя
					Philo->NeghborState[0] = STATE_READY_TO_TAKE;//Запоминаем, что отдали соседу разрешение взять вилку
					Philo->ToSendMSG[0] = true; //Помечаем, что нужно отправить сообщение
					Philo->NeghborDice[0] = Philo->LastSended[0].Value;
					break;
				}
				case LANGREQ_TAKE://Если сосед оповещает нас, что уже все готово и он берет вилку
				{
					Philo->LastSended[0].Type = LANGREQ_AST; Philo->LastSended[0].Value = -1;//Сообщаем, что мы учли это
					Philo->NeghborState[0] = STATE_EATING;//Запоминаем, что отдали соседу разрешение взять вилку
					Philo->ToSendMSG[0] = true; //Помечаем, что нужно отправить сообщение
					break;
				}
				case LANGREQ_ABORT://Сосед сбрасывает предпринятую попытку заполучить вилку
				{
					Philo->LastSended[0].Type = LANGREQ_ABORT_AST; Philo->LastSended[0].Value = -1;//Сообщаем, что мы учли это
					Philo->NeghborState[0] = STATE_ABORTED;//Сосед все еще голоден, но сейчас не может заполучить обе вилки
					Philo->ToSendMSG[0] = true; //Помечаем, что нужно отправить сообщение
					break;
				}
				case LANGREQ_FINEAT://Сосед закончил с едой
				{
					//Philo->LastSended[0].Type = LANGREQ_FINEAT_AST; Philo->LastSended[0].Value = -1;//Сообщаем, что мы учли это
					Philo->NeghborState[0] = STATE_IDLE;//Вилка теперь свободна
					//Philo->ToSendMSG[0] = true; //Помечаем, что нужно отправить сообщение
					break;
				}
				default:
				{
					printf("Rank[%i];STATE_ILDE; Message from LEFT [%i,%i]; is unexpected!\n",Philo->Rank, Philo->LastRecved[0].Type, Philo->LastRecved[0].Value);
				}
			}
		}

		if (Philo->NewMSG[1])//Если есть новые сообщения от соседа
		{
			switch (Philo->LastRecved[1].Type)
			{
			case LANGREQ_HUNGRY://Если сосед оповещает нас, что голоден и хочет взять вилку
			{
				Philo->LastSended[1].Type = LANGREQ_GIVE; Philo->LastSended[1].Value = -1;//Отдаем без боя
				Philo->NeghborState[1] = STATE_READY_TO_TAKE;//Запоминаем, что отдали соседу разрешение взять вилку
				Philo->ToSendMSG[1] = true; //Помечаем, что нужно отправить сообщение
				Philo->NeghborDice[1] = Philo->LastSended[1].Value;
				break;
			}
			case LANGREQ_TAKE://Если сосед оповещает нас, что уже все готово и он берет вилку
			{
				Philo->LastSended[1].Type = LANGREQ_AST; Philo->LastSended[1].Value = -1;//Сообщаем, что мы учли это
				Philo->NeghborState[1] = STATE_EATING;//Запоминаем, что отдали соседу разрешение взять вилку
				Philo->ToSendMSG[1] = true; //Помечаем, что нужно отправить сообщение
				break;
			}
			case LANGREQ_ABORT://Сосед сбрасывает предпринятую попытку заполучить вилку
			{
				Philo->LastSended[1].Type = LANGREQ_ABORT_AST; Philo->LastSended[1].Value = -1;//Сообщаем, что мы учли это
				Philo->NeghborState[1] = STATE_ABORTED;//Сосед все еще голоден, но сейчас не может заполучить обе вилки
				Philo->ToSendMSG[1] = true; //Помечаем, что нужно отправить сообщение
				break;
			}
			case LANGREQ_FINEAT://Сосед закончил с едой
			{
				//Philo->LastSended[1].Type = LANGREQ_FINEAT_AST; Philo->LastSended[1].Value = -1;//Сообщаем, что мы учли это
				Philo->NeghborState[1] = STATE_IDLE;//Вилка теперь свободна
				//Philo->ToSendMSG[1] = true; //Помечаем, что нужно отправить сообщение
				break;
			}
			default:
			{
				printf("Rank[%i];STATE_ILDE; Message from LEFT [%i,%i]; is unexpected!\n", Philo->Rank, Philo->LastRecved[1].Type, Philo->LastRecved[1].Value);
			}
			}
		}

		if (Philo->Hunger)//Если проголодались, то следующий ход начинаем с состояния голода. Иначе снова простаивание.
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

		//Проверяем, можем ли мы пытаться претендовать на вилку
		//Поправка. Мы все равно сообщаем соседям, что хотим вилку, что бы не получилось так, что они ими пользуются а мы все ждем

		//Если мы только проголодались, то шлем об этом сообщения
		if (Philo->SelfState != STATE_HUNGRY)
		{
			Philo->DiceRes = rand();//Кидаем кости
			for (int i = 0; i < 2; i++)
			{
				if (Philo->NeghborState[i] == STATE_READY_TO_TAKE)//Если мы уже отдали ему вилку
				{
					/*Немного про этот кусок. Если у нас стоит READY_TO_TAKE на соседе, значит он уже получил от нас LANGREQ_GIVE
					Мы, конечно, ему проиграли. И нам все равно нужно его предупредить о том, что мы хотим вилку, так
					Что все равно шлем сообщение*/
					Philo->LastSended[i].Type = LANGREQ_LOSEWAIT; Philo->LastSended[i].Value = -1;//То сразу проигрываем ему
					Philo->SelfState = STATE_LOSEWAIT;
				}
				else
				{
					Philo->LastSended[i].Type = LANGREQ_HUNGRY; Philo->LastSended[i].Value = Philo->DiceRes;
					Philo->ToSendMSG[i] = true;
				}
				//Сообщаем соседям, что голодны, и за одно результат броска кубика
			}
			
			
			//Не важно проиграли мы или нет, сообщение пропадет, если на него не ответить! Потому таки остаемся тут и готовимся отвечать!
			//if (Philo->SelfState == STATE_LOSEWAIT)//Если после всего этого мы уже проиграли, то отсылаем сообщения и на этом заканчиваем.
			//{ return new PhiloState_losewait; }

			
			Philo->SelfState = STATE_HUNGRY;


			
		}
		//А теперь запоминаем, что мы проголодались и уже всех оповестили


		//БЛОК ОБРАБОТКИ СООБЩЕНИЯ

		for (int i = 0; i < 2; i++)
		{
			if (Philo->NewMSG[i])//Если есть новые сообщения от соседа
			{
				switch (Philo->LastRecved[i].Type)
				{
					case LANGREQ_HUNGRY://Если другой философ тоже голоден
					{
						if (Philo->SelfState == STATE_LOSEWAIT)//Если мы уже перед этим проиграли бой за вилку
						{
							Philo->NeghborDice[i] = Philo->LastSended[i].Value;
							if (Philo->LastRecved[i].Value < Philo->DiceRes)//Если он проиграл бы бой
							{
								Philo->LastSended[i].Type = LANGREQ_LOSEWIN; Philo->LastSended[i].Value = -1;//Так бы мы забрали эту вилку, но вот незадача, перед этим мы уже проиграли бой
								Philo->NeghborState[i] = STATE_LOSEWAIT;//Но как только сосед закончит есть, мы возьмем эту вилку!
								Philo->ToSendMSG[i] = true;
							}
							else if (Philo->LastRecved[i].Value < Philo->DiceRes)//Если бы мы проиграли
							{
								Philo->LastSended[i].Type = LANGREQ_LOSEWAIT; Philo->LastSended[i].Value = -1;//Можешь считать, что я просто простаиваю. Я бы все равно не получил эту вилку
								Philo->NeghborState[i] = STATE_READY_TO_TAKE;//Запоминаем, что он пытался взять вилку
								Philo->ToSendMSG[i] = true;	
							}
							
						}
						else if (Philo->LastRecved[i].Value < Philo->DiceRes)//Если его число оказалось меньше, то мы берем вилку
						{
							//Philo->LastSended[i].Type = LANGREQ_TAKE; Philo->LastSended[i].Value = -1;//Пока не шлем сообщение. Ждем ответ от второго
							Philo->NeghborState[i] = STATE_LOSEWAIT;//Запоминаем, что он пытался взять вилку
							//Philo->ToSendMSG[i] = true;
							
						}
						else if (Philo->LastRecved[i].Value > Philo->DiceRes)//Если его число оказалось больше, то сообщаем ему об этом
						{
							Philo->LastSended[i].Type = LANGREQ_LOSEWAIT; Philo->LastSended[i].Value = -1;
							Philo->SelfState = STATE_LOSEWAIT;
							Philo->NeghborState[i] = STATE_READY_TO_TAKE;//Запоминаем, что он должен забрать вилку
							Philo->ToSendMSG[i] = true;
							//Может так случится, что сосед справда получит от нас первым не LANGREQ_HUNGRY, а сразу LANGREQ_LOSEWAIT
							//Однако это происходит только в том случае, если мы уже получили от него сообщение LANGREQ_HUNGRY и все посчитали за него
							//Что же, ему же лучше!
							//Однако такое может случится только если мы перед этим проиграли другому соседу. А вот если мы играем с ним с первым или прошлого выиграли,
						}
						else
						{
							printf("Rank[%i];STATE_HUNGRY; Dices MATCH!!!\n");
						}
						break;
					}

					case LANGREQ_GIVE://Если другой философ отдает нам 
					{
						Philo->NeghborState[i] = STATE_GIVING;//Запоминаем это
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
					case LANGREQ_FINEAT://Если сосед только что поел, то запоминаем, что теперь он свободен. 
					{
						//Philo->NeghborState[i] = STATE_IDLE;Он не просто свободен! Он точно знает, так или иначе, что мы голодны, а значит уже должен отдать нам вилку!
						Philo->NeghborState[i] = STATE_GIVING;
						break;

					}
					case LANGREQ_LOSEWIN:
					{
						Philo->NeghborState[i] = STATE_LOSEWIN;//Сосед бы забрал нашу вилку, будь у него возможность забрать и вторую
						break;
					}
					case LANGREQ_TAKE://Мы только что проголодались, и тут оказалось, что мы еще с прошлого раза должны вилку отдать
					{//Наш-то сосед об этом не забыл. Он получил LANGREQ_FINEAT и теперь яростно желает свою вилку!
					//Ну так не будем разочеровывать голодающего. Запоминаем, что он начал есть!
						//Этой ситуации нельзя просто избежать, подождав цикл, потому что сообщение пропадет, если на него не ответить!


						
					
						Philo->LastSended[i].Type = LANGREQ_AST; Philo->LastSended[i].Value = Philo->DiceRes;//Мы отвечаем, что бы начинал и вместе с этим отправляем ему наш бросок что бы обозначить, что мы таки голодаем!
						Philo->NeghborState[i] = STATE_EATING;//Запоминаем, что отдали соседу разрешение взять вилку
						Philo->ToSendMSG[i] = true; //Помечаем, что нужно отправить сообщение
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
			||//Если оба наши соседа сейчас не могут получить вилку
			((Philo->NeghborState[0] == STATE_LOSEWAIT) || (Philo->NeghborState[0] == STATE_LOSEWIN)) &&
			((Philo->NeghborState[1] == STATE_LOSEWAIT) || (Philo->NeghborState[1] == STATE_LOSEWIN))
			)
		{//Если оба соседа отдали нам вилку, переходим к её взятию!
			for (int i = 0; i < 2; i++)
			{
				Philo->LastSended[i].Type = LANGREQ_TAKE; Philo->LastSended[i].Value = -1;//Сообщаем всем, что берем вилку
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
		if (Philo->NewMSG[i])//Если есть новые сообщения от соседа
		{
			switch (Philo->LastRecved[i].Type)
			{
				case LANGREQ_HUNGRY:
				{	
					Philo->NeghborDice[i] = Philo->LastSended[i].Value;
					if (Philo->NeghborState[i] != STATE_GIVING)//Если перед этим сосед нам не отдавал вилку
					{
						//Делаем все то же самое, что и в STATE_HUNGRY, но уже заведомо помним, что бой проиграли
						if (Philo->LastRecved[i].Value < Philo->DiceRes)//Если он проиграл бы бой
						{
							Philo->LastSended[i].Type = LANGREQ_LOSEWIN; Philo->LastSended[i].Value = -1;//Так бы мы забрали эту вилку, но вот незадача, перед этим мы уже проиграли бой
							Philo->NeghborState[i] = STATE_LOSEWAIT;//Но как только сосед закончит есть, мы возьмем эту вилку!
							Philo->ToSendMSG[i] = true;
						}
						else if (Philo->LastRecved[i].Value < Philo->DiceRes)//Если бы мы проиграли
						{
							Philo->LastSended[i].Type = LANGREQ_LOSEWAIT; Philo->LastSended[i].Value = -1;//Можешь считать, что я просто простаиваю. Я бы все равно не получил эту вилку
							Philo->NeghborState[i] = STATE_READY_TO_TAKE;//Запоминаем, что он пытался взять вилку
							Philo->ToSendMSG[i] = true;
						}
					}
				break;
				}
				case LANGREQ_TAKE://Сосед берет вилку!
				{
					//Вот этот блок актуален только для LANGREQ_LOSEWAIT, потому что иначе сосед бы не получил от нас такого сообщения
					//Вполне возможно что наш сосед получил перед этим от нас LOSEWAIT или LOSEWIN, и то же самое от своего друго соседа
					//И тепрь думает "Ну раз уж вы оба сейчас ждете, поем ка я"
					//И это хорошо. Но может так выйти, что в тот же момент, когда он шлет нам ТАКЕ, мы получаем от своего другого соседа
					//Разрешение таки поесть
					if (Philo->NeghborState[i] == STATE_LOSEWAIT)//Если сосед нам проиграл и пытается взять вилку прямо сейчас
					{
						DelegateTAKE[i] = true;//Пока запомним его намерение. В конце если вдруг есть необходимость
						//Мы отправим ему LANGREQ_AST если он таки может взять вилку и
						//LANGREQ_TAKE если вилку берем мы!
						break;
					}
					Philo->LastSended[i].Type = LANGREQ_AST; Philo->LastSended[i].Value = -1;//Мы отвечаем, что бы начинал
					Philo->NeghborState[i] = STATE_EATING;//Запоминаем, что отдали соседу разрешение взять вилку
					Philo->ToSendMSG[i] = true; //Помечаем, что нужно отправить сообщение
					break;
				}
				case LANGREQ_ABORT://Сосед не может взять вилку сейчас
				{
					//Philo->LastSended[0].Type = LANGREQ_ABORT_AST; Philo->LastSended[0].Value = -1;//Пока попробуем без LANGREQ_ABORT_AST
					Philo->NeghborState[i] = STATE_ABORTED;//Запоминаем, что отдали соседу разрешение взять вилку
					Philo->ToSendMSG[i] = true; //Помечаем, что нужно отправить сообщение
					break;
				}
				case LANGREQ_FINEAT://Сосед закончил есть
				{
					//Пока ничего не шлем, но сосед вроде как должен быть в курсе и ждать, когда мы возьмем его вилку.
					//Philo->LastSended[i].Type = LANGREQ_TAKE; Philo->LastSended[0].Value = -1;//Пытаемся тут же получить вилку
					Philo->NeghborState[i] = STATE_GIVING;//Считаем, что сосед отдает нам вилку.
					//Philo->ToSendMSG[i] = true; //Помечаем, что нужно отправить сообщение
					break;
				}

				case LANGREQ_LOSEWAIT://Сосед тоже проиграл бой за вилку, но мы проиграли и ему, а значит сперва ест он, а потом мы.
				{
					Philo->NeghborState[i] = STATE_LOSEWAIT;
					break;
				}

				case LANGREQ_GIVE://Сосед готов отдать нам вилку. Быть может он проиграл бой с другим соседом, а потом оказалось, что проиграл бы и нам. Мы едим прежде него!
				{
					Philo->NeghborState[i] = STATE_GIVING;//Запоминаем его позорное поражение
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
		||//Если оба наши соседа сейчас не могут получить вилку
		((Philo->NeghborState[0] == STATE_LOSEWAIT) || (Philo->NeghborState[0] == STATE_LOSEWIN)) &&
		((Philo->NeghborState[1] == STATE_LOSEWAIT) || (Philo->NeghborState[1] == STATE_LOSEWIN))
		)//Если оба соседа готовы отдать нам вилку
	{
		for (int i = 0; i < 2; i++)
		{
			Philo->LastSended[i].Type = LANGREQ_TAKE; Philo->LastSended[i].Value = -1;//Сообщаем всем, что берем вилку
			Philo->ToSendMSG[i] = true;
		}
		//Philo->SelfState = STATE_EATING;
		return new PhiloState_TAKE;
	}
	else if (DelegateTAKE[0] || DelegateTAKE[1])//Если сосед пытался заполучить вилку, хотя по правам первыми шли мы
	{//К слову если мы оказались тут, значит мы все еще ждем. Иначе бы мы выше по ифам уже отправили бы ему LANGREQ_TAKE
		for (int i = 0; i < 2; i++)
		{
			if(DelegateTAKE[i])
			{
				Philo->LastSended[i].Type = LANGREQ_AST; Philo->LastSended[i].Value = -1;//Мы отвечаем, что бы начинал
				Philo->NeghborState[i] = STATE_EATING;//Запоминаем, что отдали соседу разрешение взять вилку
				Philo->ToSendMSG[i] = true; //Помечаем, что нужно отправить сообщение
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
		if (Philo->NewMSG[i])//Если есть новые сообщения от соседа
		{
			switch (Philo->LastRecved[i].Type)
			{
			case LANGREQ_AST://Сосед разрешил нам есть
				{
					if (Philo->LastRecved[i].Value != -1)//Значит сосед подтвердил наше сообщение, хотя сам теперь голодает
					{
						Philo->NeghborState[i] = STATE_LOSEWAIT;//Запоминаем его доброту
					}
					Philo->AST[i] = true;
					break;
				}
			case LANGREQ_HUNGRY: //А вот раз и в асинхронном варианте вполне себе сулчается. Ну в самом деле, может наш сосед уже поел
				{//Не может быть такого, что сосед разрешил нам перейти в TAKE, и при этом не знал, что мы уже берем вилку.
					//А значит это просто рядовое сообщение, которое можно проигнорировать
					Philo->NeghborState[i] = STATE_LOSEWAIT;//Считаем, что он не просто проголодался, но еще и проиграл нам, ведь мы уже едим
					Philo->AST[i] = true;//А раз он нам проиграл, то мы начинаем есть!
					break;
				}
			case LANGREQ_LOSEWAIT://Сосед хочет есть, но заранее нам уступает, потому что прежде мы уступили ему
				{
					//Philo->NeghborDice[i] = Philo->LastSended[i].Value;Кажеться оно таки не нужно
					Philo->NeghborState[i] = STATE_LOSEWAIT;//Но таки запоминаем, что он проголодался
					Philo->AST[i] = true;//Так же и тут. Раз он проиграл - мы начинаем есть!
				}
			case LANGREQ_GIVE://Полевые опыты показали, что это сообщение частенько приходит. Будем считать, что это просто из-за разницы во времени.
			{
				//А потому ничего не будем делать. Мы просто проглатываем это сообщение и ждем актуального
			}
			default:
				{
					printf("Rank[%i];STATE_TAKE; Message from [%i] [%i,%i]; is unexpected!\n", Philo->Rank, i, Philo->LastRecved[i].Type, Philo->LastRecved[i].Value);

				}

			}
		}
	}

	if (Philo->AST[0] == true && Philo->AST[1] == true)//если оба соседа приняли наш запрос на покушать
	{
		char buffer[50];//Едим первой вилкой
		sprintf(buffer, "Fork%i.txt", Philo->Rank);
		FILE *FORK = fopen(buffer, "a");
		fprintf(FORK, "Philosopher №%i eating with this fork on his %i's cycle, yay!\n", Philo->Rank, Philo->CycleCount);
		fclose(FORK);

		sprintf(buffer, "Fork%i.txt", Philo->LeftNeighbor());//Едим второй вилкой
		FORK = fopen(buffer, "a");
		fprintf(FORK, "Philosopher №%i eating with this fork on his %i's cycle, yay!\n", Philo->Rank, Philo->CycleCount);
		fclose(FORK);

		Philo->EatingTimes++;
		Philo->TotalWaiting--;//В этот ход мы все таки едим, а не ждем

		for (int i = 0; i < 2; i++)
		{		
		Philo->LastSended[i].Type = LANGREQ_FINEAT; Philo->LastSended[i].Value = -1;
		Philo->ToSendMSG[i] = true;
		Philo->AST[i] = false;
		if ((Philo->NeghborState[i] == STATE_LOSEWAIT))//Если наш сосед голоден
			{
				Philo->NeghborState[i] = STATE_READY_TO_TAKE;//Если сосед до этого нам проиграл, то мы сразу готовимся ему отдать вилку.
			}
		if(Philo->NeghborState[i] == STATE_GIVING)
			{
				Philo->NeghborState[i] = STATE_IDLE;//Если сосед просто отдавал нам вилку, то скорее всего он просто простаивает
			}
		}
		Philo->Hunger = false;
		Philo->SelfState = STATE_IDLE;


		return new PhiloState_idle;//Возвращаемся в состояние ожидания
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
	/*Вот так вот все работает на одном процессе.
	Все философы отрабатывают по очереди, а адреса в пересылке им заменяют тэги*/

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
#else //А вот это вот на многопоточный случай
	philosopher A;
	A.MPI_Rank = rank;
	A.Rank = rank;
	A.Amount = Size;

	srand(rank * 120 + rank * 6 + rank);
	MPI_Request ENDSend[2];//Реквесты для отправки сообщений на соседние процессы
	MPI_Status ENDStat;//Статус для приема финальных сообщений
	int ENDCounter = 0;//Сколько соседей закончило. Как только закончат оба, мы закрываемся

	int END = 7612;//Самая важная переменная, на которой держиться весь код
	int Recved = false;

	while (true)
	{
		
		A.Cycle();



	
		//Все посылают сообщения о завершении на первый процесс. Как только все закончили, сварачиваемся.
		if (A.CycleCount == 11000)//Достигли сотни. Шлем соседям сообщение о том, что мы закончили и ждем их.
		{
			ENDCounter++;
			MPI_Isend(&END, 1, MPI_INT, 0, 99999, MPI_COMM_WORLD, &(ENDSend[0]));//Посылаем на первый процесс, что мол все

		}
		if (A.CycleCount >= 11000)
		{
			if (rank == 0)//А вот что делает нулевой процесс
			{
				Recved = false;
				MPI_Iprobe(MPI_ANY_SOURCE, 99999, MPI_COMM_WORLD, &Recved, &ENDStat);
				while (Recved)//Он начинает подсчитывать количество пришедших завершающих сообщений
				{
					MPI_Recv(&END, 2, MPI_INT, MPI_ANY_SOURCE, 99999, MPI_COMM_WORLD, &ENDStat);
					ENDCounter++;
					MPI_Iprobe(MPI_ANY_SOURCE, 99999, MPI_COMM_WORLD, &Recved, &ENDStat);//Пока они не кончатся
				}
				if (ENDCounter == Size)
				{
					for (int i = 1; i < Size; i++)
					{
						MPI_Send(&END, 1, MPI_INT, i, 99999, MPI_COMM_WORLD);//Мы уже готовы завершать. Тут рассылка не асинхронная, так что мы настроены серьезно!	
					}
					MPI_Barrier(MPI_COMM_WORLD);//Встаем вместе со всеми к барьеру
					MPI_Finalize();//И наконец заканчиваем
					return 0;
				}

			}




			MPI_Iprobe(0, 99999, MPI_COMM_WORLD, &Recved, &ENDStat);//Ждем когда он скажет, что все закончили
			if (Recved)
			{
				MPI_Recv(&END, 2, MPI_INT, 0, 99999, MPI_COMM_WORLD, &ENDStat);
				MPI_Barrier(MPI_COMM_WORLD);//На всякий случай завершаемся синхронно.
				MPI_Finalize();
				return 0;
			}
		}
		
	}

#endif // SINGLE_PROCESS
	
	MPI_Finalize();
    return 0;
}

