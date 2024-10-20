#ifndef __PROTOCOL__
#define __PROTOCOL__



struct PacketHeader
{
	BYTE	byCode;			// ��Ŷ�ڵ� 0x96 ����.
	BYTE	bySize;			// ��Ŷ ������.
	BYTE	byType;			// ��ŶŸ��.
};


#define dfPACKET_CODE		0x96

#define dfPACKET_MAX_SIZE 40

#define	dfPACKET_SC_CREATE_MY_CHARACTER			0
//---------------------------------------------------------------
// Ŭ���̾�Ʈ �ڽ��� ĳ���� �Ҵ�		Server -> Client
//
// ������ ���ӽ� ���ʷ� �ްԵǴ� ��Ŷ���� �ڽ��� �Ҵ���� ID ��
// �ڽ��� ���� ��ġ, HP �� �ް� �ȴ�. (ó���� �ѹ� �ް� ��)
// 
// �� ��Ŷ�� ������ �ڽ��� ID,X,Y,HP �� �����ϰ� ĳ���͸� �������Ѿ� �Ѵ�.
//
//	4	-	ID
//	1	-	Direction	(LL / RR)
//	2	-	X
//	2	-	Y
//	1	-	HP
//
//---------------------------------------------------------------

struct SC_CreateMyCharacter // 0
{
	PacketHeader header;
	int ID;
	BYTE Direction; // (LL / RR)
	short X;
	short Y;
	BYTE HP;
};


#define	dfPACKET_SC_CREATE_OTHER_CHARACTER		1
//---------------------------------------------------------------
// �ٸ� Ŭ���̾�Ʈ�� ĳ���� ���� ��Ŷ		Server -> Client
//
// ó�� ������ ���ӽ� �̹� ���ӵǾ� �ִ� ĳ���͵��� ����
// �Ǵ� �����߿� ���ӵ� Ŭ���̾�Ʈ���� ������ ����.
//
//
//	4	-	ID
//	1	-	Direction	(LL / RR)
//	2	-	X
//	2	-	Y
//	1	-	HP
//
//---------------------------------------------------------------

struct SC_CreateOtherCharacter // 1
{
	PacketHeader header;
	int ID;
	BYTE Direction; // (LL / RR)
	short X;
	short Y;
	BYTE HP;
};


#define	dfPACKET_SC_DELETE_CHARACTER			2
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ						Server -> Client
//
// ĳ������ �������� �Ǵ� ĳ���Ͱ� �׾����� ���۵�.
//
//	4	-	ID
//
//---------------------------------------------------------------

// ĳ���� ����
struct SC_DeleteCharacter // 2
{
	PacketHeader header;
	int ID;
};

#define dfPACKET_SC_CREATE_MONSTER 3

struct SC_CreateMonster
{
	PacketHeader header;
	int ID;
	BYTE Direction;
	short X;
	short Y;
	BYTE HP;
};

#define dfPACKET_SC_DELETE_MONSTER 4

struct SC_DeleteMonster
{
	PacketHeader header;
	int ID;
};



#define	dfPACKET_CS_MOVE_START					10
//---------------------------------------------------------------
// ĳ���� �̵����� ��Ŷ						Client -> Server
//
// �ڽ��� ĳ���� �̵����۽� �� ��Ŷ�� ������.
// �̵� �߿��� �� ��Ŷ�� ������ ������, Ű �Է��� ����Ǿ��� ��쿡��
// ������� �Ѵ�.
//
// (���� �̵��� ���� �̵� / ���� �̵��� ���� ���� �̵�... ���)
//
//	1	-	Direction	( ���� ������ �� 4���� ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
#define dfPACKET_MOVE_DIR_LL					0
//#define dfPACKET_MOVE_DIR_LU					1
#define dfPACKET_MOVE_DIR_UU					2
//#define dfPACKET_MOVE_DIR_RU					3
#define dfPACKET_MOVE_DIR_RR					4
//#define dfPACKET_MOVE_DIR_RD					5
#define dfPACKET_MOVE_DIR_DD					6
//#define dfPACKET_MOVE_DIR_LD					7

// ĳ���� �̵� ���� (Ŭ���̾�Ʈ -> ����)
struct CS_MoveStart  // 10
{
	PacketHeader header;
	BYTE Direction;
	short X;
	short Y;
};



#define	dfPACKET_SC_MOVE_START					11
//---------------------------------------------------------------
// ĳ���� �̵����� ��Ŷ						Server -> Client
//
// �ٸ� ������ ĳ���� �̵��� �� ��Ŷ�� �޴´�.
// ��Ŷ ���Ž� �ش� ĳ���͸� ã�� �̵�ó���� ���ֵ��� �Ѵ�.
// 
// ��Ŷ ���� �� �ش� Ű�� ����ؼ� ���������� �����ϰ�
// �ش� �������� ��� �̵��� �ϰ� �־�߸� �Ѵ�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ �� 4���� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

// ĳ���� �̵� ���� (���� -> Ŭ���̾�Ʈ)
struct SC_MoveStart  // 11
{
	PacketHeader header;
	int ID;
	BYTE Direction;
	short X;
	short Y;
};




#define	dfPACKET_CS_MOVE_STOP					12
//---------------------------------------------------------------
// ĳ���� �̵����� ��Ŷ						Client -> Server
//
// �̵��� Ű���� �Է��� ��� �����Ǿ��� ��, �� ��Ŷ�� ������ �����ش�.
// �̵��� ���� ��ȯ�ÿ��� ��ž�� ������ �ʴ´�.
//
//	1	-	Direction	( ���� ������ �� ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

// ĳ���� �̵� ����
struct CS_MoveStop // 12
{
	PacketHeader header;
	BYTE Direction; // (LL / RR)
	short X;
	short Y;
};


#define	dfPACKET_SC_MOVE_STOP					13
//---------------------------------------------------------------
// ĳ���� �̵����� ��Ŷ						Server -> Client
//
// ID �� �ش��ϴ� ĳ���Ͱ� �̵��� ������̹Ƿ� 
// ĳ���͸� ã�Ƽ� �����, ��ǥ�� �Է����ְ� ���ߵ��� ó���Ѵ�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

// �ٸ� ĳ���� �̵� ����
struct SC_MoveStop // 13
{
	PacketHeader header;
	int ID;
	BYTE Direction; // (LL // RR)
	short X;
	short Y;
};

#define dfPACKET_SC_MONSTER_MOVE 14

struct SC_MonsterMove // 14
{
	PacketHeader header;
	int ID;
	BYTE Direction;
	short X;
	short Y;
};


#define	dfPACKET_CS_ATTACK1						20
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Client -> Server
//
// ���� Ű �Է½� �� ��Ŷ�� �������� ������.
// �浹 �� �������� ���� ����� �������� �˷� �� ���̴�.
//
// ���� ���� ���۽� �ѹ��� �������� ������� �Ѵ�.
//
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y	
//
//---------------------------------------------------------------

struct CS_Attack1 // 20
{
	PacketHeader header;
	BYTE Direction; // (LL // RR)
	short X;
	short Y;
};

#define	dfPACKET_SC_ATTACK1						21
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Server -> Client
//
// ��Ŷ ���Ž� �ش� ĳ���͸� ã�Ƽ� ����1�� �������� �׼��� �����ش�.
// ������ �ٸ� ��쿡�� �ش� �������� �ٲ� �� ���ش�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

struct SC_Attack1 // 21
{
	PacketHeader header;
	int ID;
	BYTE Direction; // (LL // RR)
	short X;
	short Y;
};






#define	dfPACKET_SC_MONSTER_DAMAGE						30
//---------------------------------------------------------------
// ���� ������ ��Ŷ							Server -> Client
//
// ���ݿ� ���� ������ ������ ����.
//
//	4	-	AttackID	( ������ ID )
//	4	-	DamageID	( ������ ID )
//	1	-	DamageHP	( ������ HP )
//
//---------------------------------------------------------------

struct SC_MonsterDamage
{
	PacketHeader header;
	int attackID; // ������ ID
	int damageID; // ������ ID
	BYTE damageHP; // ������ HP
};

#define	dfPACKET_SC_CHARACTER_DAMAGE						31
//---------------------------------------------------------------
// ���� ������ ��Ŷ							Server -> Client
//
// ���ݿ� ���� ĳ������ ������ ����.
//
//	4	-	AttackID	( ������ ID )
//	4	-	DamageID	( ������ ID )
//	1	-	DamageHP	( ������ HP )
//
//---------------------------------------------------------------

struct SC_CharacterDamage
{
	PacketHeader header;
	int attackID; // ������ ID
	int damageID; // ������ ID
	BYTE damageHP; // ������ HP
};

#define dfPACKET_SC_CHAT						40

struct SC_CHAT
{
	PacketHeader header;
	int ID;
	WCHAR message[32];
};

#define dfPACKET_CS_CHAT						41

struct CS_CHAT
{
	PacketHeader header;
	int ID;
	WCHAR message[32];
};

#define	dfPACKET_SC_SYNC						251
//---------------------------------------------------------------
// ����ȭ�� ���� ��Ŷ					Server -> Client
//
// �����κ��� ����ȭ ��Ŷ�� ������ �ش� ĳ���͸� ã�Ƽ�
// ĳ���� ��ǥ�� �������ش�.
//
//	4	-	ID
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

struct SC_SYNC
{
	PacketHeader header;
	int ID;
	short X;
	short Y;
};

#define	dfPACKET_CS_HEARTBEAT						252
//---------------------------------------------------------------
// Echo �� ��Ŷ					Client -> Server
//
//	4	-	Time
//
//---------------------------------------------------------------

struct CS_HEARTBEAT
{
	PacketHeader header;
	int Time;
};

#define	dfPACKET_SC_HEARTBEAT						253
//---------------------------------------------------------------
// Echo ���� ��Ŷ				Server -> Client
//
//	4	-	Time
//
//---------------------------------------------------------------

struct SC_HEARTBEAT
{
	PacketHeader header;
	int Time;
};



//#define	dfPACKET_CS_ATTACK2						22
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Client -> Server
//
// ���� Ű �Է½� �� ��Ŷ�� �������� ������.
// �浹 �� �������� ���� ����� �������� �˷� �� ���̴�.
//
// ���� ���� ���۽� �ѹ��� �������� ������� �Ѵ�.
//
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

//struct CS_Attack2 // 22
//{
//	PacketHeader header;
//	BYTE Direction; // (LL // RR)
//	short X;
//	short Y;
//};


//#define	dfPACKET_SC_ATTACK2						23
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Server -> Client
//
// ��Ŷ ���Ž� �ش� ĳ���͸� ã�Ƽ� ����2�� �������� �׼��� �����ش�.
// ������ �ٸ� ��쿡�� �ش� �������� �ٲ� �� ���ش�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

//struct SC_Attack2 // 23
//{
//	PacketHeader header;
//	int ID;
//	BYTE Direction; // (LL // RR)
//	short X;
//	short Y;
//};

//#define	dfPACKET_CS_ATTACK3						24
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Client -> Server
//
// ���� Ű �Է½� �� ��Ŷ�� �������� ������.
// �浹 �� �������� ���� ����� �������� �˷� �� ���̴�.
//
// ���� ���� ���۽� �ѹ��� �������� ������� �Ѵ�.
//
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

//struct CS_Attack3 // 24
//{
//	PacketHeader header;
//	BYTE Direction; // (LL // RR)
//	short X;
//	short Y;
//};

//#define	dfPACKET_SC_ATTACK3						25
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Server -> Client
//
// ��Ŷ ���Ž� �ش� ĳ���͸� ã�Ƽ� ����3�� �������� �׼��� �����ش�.
// ������ �ٸ� ��쿡�� �ش� �������� �ٲ� �� ���ش�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

//struct SC_Attack3 // 25
//{
//	PacketHeader header;
//	int ID;
//	BYTE Direction; // (LL // RR)
//	short X;
//	short Y;
//};


#endif