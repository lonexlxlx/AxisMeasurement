//Copyright(c) 2012, KEYENCE CORPORATION. All rights reserved.
/** @file
@brief	LS9IF_ErrorCode.h
*/

#define LS9IF_RC_OK							0x0000	// Normal termination
#define LS9IF_RC_ERR_OPEN					0x1000	// Failed in opening a communication path.
#define LS9IF_RC_ERR_NOT_OPEN				0x1001	// A communication path has not been established.
#define LS9IF_RC_ERR_SEND					0x1002	// Failed in sending a command.
#define LS9IF_RC_ERR_RECEIVE				0x1003	// Failed in receiving a response.
#define LS9IF_RC_ERR_TIMEOUT				0x1004	// A time-out occurred in receiving a response.
#define LS9IF_RC_ERR_NOMEMORY				0x1005	// Failed in allocating memory.
#define LS9IF_RC_ERR_PARAMETER				0x1006	// An invalid parameter was passed.
#define LS9IF_RC_ERR_RECV_FMT				0x1007	// The received response data is invalid.
#define LS9IF_RC_ERR_OPEN_YET				0x100A	// The USB is already opened.
