#pragma once

#define LISTEN_PORT		9000
#define MAX_CONNECTION	10000

#define CONNECT_SERVER_ADDR	"127.0.0.1"
#define CONNECT_SERVER_PORT 9001

//#define SQL_SERVER_CONN_STR	L"Driver={SQL Server};Server=BLASTINGZONE-PC\\SQLEXPRESS;Database=NGServer;UID=next;PWD=1234;"
#define SQL_SERVER_CONN_STR	L"Driver={SQL Server};Server=127.0.0.1,1433\\YANGHYUNCHAN-PC;Database=GameDB;UID=justdid;PWD=1234"

//#define GQCS_TIMEOUT	10 //INFINITE
#define GQCS_TIMEOUT	INFINITE //20