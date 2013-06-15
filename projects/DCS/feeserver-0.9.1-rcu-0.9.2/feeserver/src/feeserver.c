/************************************************************************
 **
 **
 ** This file is property of and copyright by the Department of Physics
 ** Institute for Physic and Technology, University of Bergen,
 ** Bergen, Norway.
 ** In cooperation with Center for Technology Transfer and 
 ** Telecommunication (ZTT), University of Applied Sciences Worms
 ** Worms, Germany.
 **
 ** This file has been written by Sebastian Bablok,
 ** Sebastian.Bablok@uib.no
 **
 ** Important: This file is provided without any warranty, including
 ** fitness for any particular purpose. Further distribution of this file,
 ** even with changes in the code, is only allowed, when this copyright
 ** and warranty paragraph is kept unchanged and included to the sources. 
 **
 **
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>				// for pause() necessary
#include <string.h>
#include <dim/dis.h>				// dimserver library
#include <math.h>				// for fabsf

#include <time.h>				// time for threads
#include <sys/time.h>			// for gettimeofday()
#include <pthread.h>
//#include <stdbool.h>			// included by fee_types.h
#include <errno.h>      		// for the error numbers
#include <signal.h>

#include "fee_types.h"			// declaration of own datatypes
#include "fee_functions.h"		// declaration of feeServer functions
#include "fee_defines.h"		// declaration of all globaly used constants
#include "feepacket_flags.h"	// declaration of flag bits in a feepacket
#include "fee_errors.h"			// defines of error codes
#include "ce_command.h"			//control engine header file

#ifdef __UTEST
#include "fee_utest.h"
#endif

/**
 * @defgroup feesrv_core The FeeServer core
 */

//-- global variables --

/**
 * state of the server, possible states are: COLLECTING, RUNNING and ERROR_STATE.
 * @ingroup feesrv_core
 */
static int state = COLLECTING;

/**
 * indicates, if CEReady has been signaled (used in backup solution of init watch dog).
 * @ingroup feesrv_core
 */
static bool ceReadySignaled = false;

/**
 * Variable provides the init state of the CE.
 * @ingroup feesrv_core
 */
static int ceInitState = CE_OK;

/**
 * pointer to the first ItemNode of the doubly linked list (float)
 * @ingroup feesrv_core
 */
static ItemNode* firstNode = 0;

/**
 * pointer to the last ItemNode of the doubly linked list (float)
 * @ingroup feesrv_core
 */
static ItemNode* lastNode = 0;

/**
 * The message struct providing the data for an event message.
 * @ingroup feesrv_core
 */
static MessageStruct message;

/**
 * Stores the last send message over the message channel to compare it with
 * a newly triggered messages.
 * @ingroup feesrv_core
 */
static MessageStruct lastMessage;

/**
 * Counter for replication of log messages.
 * @ingroup feesrv_core
 */
static unsigned short replicatedMsgCount = 0;

/**
 * Indicates if the watchdog for replicated log messages is running
 * (true = running).
 *
 * @ingroup feesrv_core
 */
static bool logWatchDogRunning = false;

/**
 * Timeout for the watchdog of replicated log messages. After this time the
 * hold back message will be sent for sure, if no other type of message has
 * triggered its sending before (replicated messages are collected until
 * either this timeout occurs or a different message is triggered. In the later
 * case the hold back message is sent first).
 *
 * @ingroup feesrv_core
 */
static unsigned int logWatchDogTimeout = DEFAULT_LOG_WATCHDOG_TIMEOUT;

/**
 * Stores the number of added nodes to the item list (float).
 * @ingroup feesrv_core
 */
static unsigned int nodesAmount = 0;

/**
 * Indicates if the float monitor thread for published items has been started
 * (true = started).
 *
 * @ingroup feesrv_core
 */
static bool monitorThreadStarted = false;

/**
 * DIM-serviceID for the dedicated acknowledge-service
 * @ingroup feesrv_core
 */
static unsigned int serviceACKID;

/**
 * DIM-serviceID for the dedicated message - service
 * @ingroup feesrv_core
 */
static unsigned int messageServiceID;

/**
 * DIM-commandID
 * @ingroup feesrv_core
 */
static unsigned int commandID;

/**
 * Pointer to acknowledge Data (used by the DIM-framework)
 * @ingroup feesrv_core
 */
static char* cmndACK = 0;

/**
 * size of the acknowledge Data
 * @ingroup feesrv_core
 */
static int cmndACKSize = 0;

/**
 * Name of the FeeServer
 * @ingroup feesrv_core
 */
static char* serverName = 0;

/**
 * length of FeeServer name
 * @ingroup feesrv_core
 */
static int serverNameLength = 0;

/**
 * Update rate, in which the whole Item-list should be checked for changes.
 * This value is given in milliseconds.
 * @ingroup feesrv_core
 */
static unsigned short updateRate = DEFAULT_UPDATE_RATE;

/**
 * Timeout for call of issue - the longest time a command can be executed by the CE,
 * before the watch dog kills this thread. This value is given in milliseconds.
 * @ingroup feesrv_core
 */
static unsigned long issueTimeout = DEFAULT_ISSUE_TIMEOUT;

/**
 * Stores the current log level for this FeeServer.
 * In case that an environmental variable (FEE_LOG_LEVEL) tells the desired
 * loglevel, the DEFAULT_LOGLEVEL is overwritten during init process.
 * @ingroup feesrv_core
 */
static unsigned int logLevel = DEFAULT_LOGLEVEL;

/**
 * thread handle for the initialize thread
 * @ingroup feesrv_core
 */
static pthread_t thread_init;

/**
 * thread handle for the monitoring thread (float list)
 * @ingroup feesrv_core
 */
static pthread_t thread_mon;

/**
 * thread handle for the watchdog of replicated log messages.
 * This watchdog checks, if there have been replicated log messages during a
 * time period given by "replicatedLogMessageTimeout", which have been hold
 * back. The content of these messages is then send including the number of
 * how often this has been triggered and hold back.
 * Afterwards the counter is set back to zero again and backup of the last
 * send log message is cleared.
 *
 * @ingroup feesrv_core
 */
static pthread_t thread_logWatchdog;

/**
 * thread condition variable for the "watchdog" timer
 * @ingroup feesrv_core
 */
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/**
 * thread condition variable for the "initialisation complete" - signal
 * @ingroup feesrv_core
 */
static pthread_cond_t init_cond = PTHREAD_COND_INITIALIZER;

/**
 * thread mutex variable for the "watchdog" in command handler
 * @ingroup feesrv_core
 */
static pthread_mutex_t wait_mut = PTHREAD_MUTEX_INITIALIZER;

/**
 * thread mutex variable for the initialize CE thread
 * @ingroup feesrv_core
 */
static pthread_mutex_t wait_init_mut = PTHREAD_MUTEX_INITIALIZER;

/**
 * thread mutex variable for the commandAck data
 * @ingroup feesrv_core
 */
static pthread_mutex_t command_mut = PTHREAD_MUTEX_INITIALIZER;

/**
 * mutex to lock access to the logging function ( createLogMessage() )
 * @ingroup feesrv_core
 */
static pthread_mutex_t log_mut = PTHREAD_MUTEX_INITIALIZER;


////   --------- NEW FEATURE SINCE VERSION 0.8.1 (2007-06-12) ---------- /////

/**
 * pointer to the first IntItemNode of the doubly linked list (int)
 * @ingroup feesrv_core
 */
static IntItemNode* firstIntNode = 0;

/**
 * pointer to the last IntItemNode of the doubly linked list (int)
 * @ingroup feesrv_core
 */
static IntItemNode* lastIntNode = 0;

/**
 * Stores the number of added integer nodes to the IntItem list.
 * @ingroup feesrv_core
 */
static unsigned int intNodesAmount = 0;

/**
 * thread handle for the monitoring thread (int - list)
 * @ingroup feesrv_core
 */
static pthread_t thread_mon_int;

/**
 * Indicates if the int monitor thread for published IntItems has been started
 * (true = started).
 *
 * @ingroup feesrv_core
 */
static bool intMonitorThreadStarted = false;


////    ------------- NEW Memory Management (2007-07-25) ----------------- ////

/**
 * pointer to the first MemoryNode of the doubly linked list (memory management)
 * @ingroup feesrv_core
 */
static MemoryNode* firstMemoryNode = 0;

/**
 * pointer to the last MemoryNode of the doubly linked list (memory management)
 * @ingroup feesrv_core
 */
static MemoryNode* lastMemoryNode = 0;


/// ---- NEW FEATURE SINCE VERSION 0.8.2b [Char Channel] (2007-07-28) ----- ///

/**
 * Stores the number of added Character item nodes to the CharItem list.
 * @ingroup feesrv_core
 */
static unsigned int charNodesAmount = 0;

/**
 * pointer to the first CharItemNode of the doubly linked list (char)
 * @ingroup feesrv_core
 */
static CharItemNode* firstCharNode = 0;

/**
 * pointer to the last CharItemNode of the doubly linked list (char)
 * @ingroup feesrv_core
 */
static CharItemNode* lastCharNode = 0;



//-- Main --

/**
 * Main of FeeServer.
 * This programm represents the DIM-Server running on the DCS-boards.
 * It uses the DIM-Server-Library implemented by C. Gaspar from Cern.
 *
 * @author Christian Kofler, Sebastian Bablok
 *
 * @date 2003-04-24
 *
 * @update 2004-11-22 (and many more dates ...)
 *
 * @version 0.8.1
 * @ingroup feesrv_core
 */
int main(int argc, char** arg) {
	//-- only for unit tests
#	ifdef __UTEST
	// insert here the testfunction-calls
	testFrameWork();
	return 0;
#	endif

	// now here starts the real stuff
	initialize();
	// test server (functional test)
	while (1) {
		// maybe do some checks here, like:
		// - monitoring thread is still in good state
		// - CE is still in good state
		// - everything within the FeeServer is OK (assertions?)
		pause();
	}
	return 0;
}


void initialize() {
	//-- Declaring variables --
	struct timeval now;
	struct timespec timeout;
	pthread_attr_t attr;
	int nRet;
	int status;
	int initState  = FEE_CE_NOTINIT;
	char* name = 0;
	char* dns = 0;
	bool initOk = true;
	unsigned int envVal = 0;
	char msg[250];
	int restartCount = 0;

	//-- register interrupt handler (CTRL-C)
	// not used yet, causes problems
//	if (signal(SIGINT, interrupt_handler) == SIG_ERR) {
//#		ifdef __DEBUG
//		printf("Unable to register interrupt handler.\n");
//		printf("This is not fatal -> continuing.\n");
//#		endif
//	}

	//-- get name of the server --
	name = getenv("FEE_SERVER_NAME");
	if (name == 0) {
#		ifdef __DEBUG
		printf("No FEE_SERVER_NAME \n");
#		endif
		exit(202);
	}

	serverName = (char*) malloc(strlen(name) + 1);
	if (serverName == 0) {
		//no memory available!
#		ifdef __DEBUG
		printf("no memory available while trying to create server name!\n");
#		endif
		exit(201);
	}
	strcpy(serverName, name);
	serverNameLength = strlen(serverName);

	//-- test, if DIM_DNS_NODE is specified
	dns = getenv("DIM_DNS_NODE");
	if (dns == 0) {
#		ifdef __DEBUG
		printf("No DIM_DNS_NODE specified. \n");
#		endif
		exit(203);
	}

	// set the desired log level, if provided
	if (getenv("FEE_LOG_LEVEL")) {
		sscanf(getenv("FEE_LOG_LEVEL"), "%d", &envVal);
		if ((envVal < 0) || (envVal > MSG_MAX_VAL)) {
#		    ifdef __DEBUG
			printf("Environmental variable has invalid Log Level, using default instead.\n");
		   	fflush(stdout);
#			endif
		} else {
			logLevel = envVal | MSG_ALARM;
		}
	}

	// set logWatchDogTimeout, if env variable "FEE_LOGWATCHDOG_TIMEOUT" is set
    if (getenv("FEE_LOGWATCHDOG_TIMEOUT")) {
        sscanf(getenv("FEE_LOGWATCHDOG_TIMEOUT"), "%d", &envVal);
        if ((envVal <= 0) || (envVal > MAX_TIMEOUT)) {
#           ifdef __DEBUG
            printf("Environmental variable has invalid LogWatchDog Timeout, using default instead.\n");
            fflush(stdout);
#           endif
        } else {
            logWatchDogTimeout = envVal;
        }
    }

	// get restart counter
	if (getenv("FEESERVER_RESTART_COUNT")) {
		restartCount = atoi(getenv("FEESERVER_RESTART_COUNT"));
	}

	// Initial printout
# 	ifdef __DEBUG
	printf("\n  **  FeeServer version %s  ** \n\n", FEESERVER_VERSION);
	printf("FeeServer name: %s\n", serverName);
	printf("Using DIM_DNS_NODE: %s\n", dns);
#   ifdef __BENCHMARK
    printf(" -> Benchmark version of FeeServer <- \n");
#	endif
	printf("Current log level is: %d (MSG_ALARM (%d) is always on)\n", logLevel, MSG_ALARM);
#	endif

	//set dummy exit_handler to disable framework exit command, returns void
	dis_add_exit_handler(&dim_dummy_exit_handler);

	//set error handler to catch DIM framework messages
	dis_add_error_handler(&dim_error_msg_handler);

	// to ensure that signal is in correct state before init procedure
	ceReadySignaled = false;

	// lock mutex
	status = pthread_mutex_lock(&wait_init_mut);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Lock init mutex error: %d\n", status);
		fflush(stdout);
#		endif
		initOk = false;
	} else {
		// initiailisation of thread attribute only if mutex has been locked
		status = pthread_attr_init(&attr);
		if (status != 0) {
#			ifdef __DEBUG
			printf("Init attribute error: %d\n", status);
			fflush(stdout);
#			endif
			initOk = false;
		} else {
			status = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			if (status != 0) {
#				ifdef __DEBUG
				printf("Set attribute error: %d\n", status);
				fflush(stdout);
#				endif
				initOk = false;
			}
		}
	}

	if (initOk == true) {
		// call only if initOk == true,
#		ifdef __DEBUG
	        time_t initStartTime=time(NULL);
#		endif //__DEBUG
		status = pthread_create(&thread_init, &attr, (void*) &threadInitializeCE, 0);
		if (status != 0) {
#			ifdef __DEBUG
			printf("Create thread error: %d\n", status);
			fflush(stdout);
#			endif
			initState = FEE_CE_NOTINIT;
		} else {
			// timeout set in ms, should be enough for initialisation; see fee_defines.h for current value
			status = gettimeofday(&now, 0);
			if ((status != 0) || (restartCount <= 0)) {
				// backup solution for detetcting end of init process
#				ifdef __DEBUG
			        //printf("Get time of day error: %d, using backup solution\n", status);
			        //fflush(stdout);
#				endif
				// unlock mutex to enable functionality of signalCEReady
				status = pthread_mutex_unlock(&wait_init_mut);
#				ifdef __DEBUG
				if (status != 0) {
					printf("Unlock mutex error: %d\n", status);
					fflush(stdout);
				}
#				endif
				// sleep init-timeout length
				usleep((TIMEOUT_INIT_CE_MSEC * 1000));
				// check each second if finished
				const int sleepFraction=1;
				int sleepLoops=TIMEOUT_INIT_CE_SEC/sleepFraction;
				int cycles=0;
				do {
				  dtq_sleep(sleepFraction);
				  if (ceReadySignaled) break;
				} while (cycles++<sleepLoops);
				if (ceReadySignaled == false) {
					status = pthread_cancel(thread_init);
#					ifdef __DEBUG
					if (status != 0) {
						printf("No thread to cancel: %d\n", status);
						fflush(stdout);
					}
#					endif
					// start with "the CE is not initialized!"
					initState = FEE_CE_NOTINIT;
#					ifdef __DEBUG
					printf("Timeout in init [sleep]: %d\n", initState);
					fflush(stdout);
#					endif
				} else {
					if (ceInitState != CE_OK) {
						// init failed, but no timeout occured
						// (insufficient memory, etc. ... or something else)
#						ifdef __DEBUG
						printf("Init of CE failed, error: %d\n", ceInitState);
						fflush(stdout);
#						endif
						initState = FEE_CE_NOTINIT;
					} else {
						// start with "everything is fine"
						initState = FEE_OK;
#						ifdef __DEBUG
						printf("Init OK\n");
						fflush(stdout);
#						endif
					}
				}
			} else {
				timeout.tv_sec = now.tv_sec + TIMEOUT_INIT_CE_SEC;
				timeout.tv_nsec = (now.tv_usec * 1000) +
						(TIMEOUT_INIT_CE_MSEC * 1000000);

				// wait for finishing "issue" or timeout after the mutex is unlocked
				// a retcode of 0 means, that pthread_cond_timedwait has returned
				// with the cond_init signaled
				status = pthread_cond_timedwait(&init_cond, &wait_init_mut, &timeout);
				// -- start FeeServer depending on the state of the CE --
				if (status != 0) {
					status = pthread_cancel(thread_init);
#					ifdef __DEBUG
					if (status != 0) {
						printf("No thread to cancel: %d\n", status);
						fflush(stdout);
					}
#					endif
					// start with "the CE is not initialized!"
					initState = FEE_CE_NOTINIT;
#					ifdef __DEBUG
					printf("Timeout in init [timed_wait]: %d\n", initState);
					fflush(stdout);
#					endif
				} else {
					if (ceInitState != CE_OK) {
						// init failed, but no timeout occured
						// (insufficient memory, etc. ... or something else)
#						ifdef __DEBUG
						printf("Init of CE failed, error: %d\n", ceInitState);
						fflush(stdout);
#						endif
						initState = FEE_CE_NOTINIT;
					} else {
						// start with "everything is fine"
						initState = FEE_OK;
#						ifdef __DEBUG
						printf("Init OK\n");
						fflush(stdout);
#						endif
					}
				}
			}
#			ifdef __DEBUG
			if (initState!=FEE_OK) {
			  time_t initStopTime=time(NULL);
			  printf("CE init tread started %s killed %s\n", ctime(&initStartTime), ctime(&initStopTime));
			  fflush(stdout);
			}
#			endif //__DEBUG
		}
		// destroy thread attribute
		status = pthread_attr_destroy(&attr);
#		ifdef __DEBUG
		if (status != 0) {
			printf("Destroy attribute error: %d\n", status);
			fflush(stdout);
		}
#		endif
	}

	// init message struct -> FeeServer name, version and DNS are also provided
	initMessageStruct();

	if (initState != FEE_OK) {
		// remove all services of Items of ItemList
#		ifdef __DEBUG
		printf("Init failed, unpublishing item list\n");
		fflush(stdout);
#		endif
		unpublishItemList();
		// new since version 0.8.1 -> int channels
		unpublishIntItemList();
        // new since version 0.8.2b -> char channels
        unpublishCharItemList();
	}

	// add div. services and the command channel and then start DIM server
	nRet = start(initState);

	// unlock mutex
	status = pthread_mutex_unlock(&wait_init_mut);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Unlock mutex error: %d\n", status);
		fflush(stdout);
#		endif
		if (nRet == FEE_OK) {
			createLogMessage(MSG_WARNING, "Unable to unlock init mutex.", 0);
		}
	}

	if (nRet != FEE_OK) {
#		ifdef __DEBUG
		printf("unable to start DIM server, exiting.\n");
		fflush(stdout);
#		endif
		fee_exit_handler(205);
	} else {
#		ifdef __DEBUG
		printf("DIM Server successfully started, ready to accept commands.\n");
		fflush(stdout);
#		endif
	}

#	ifdef __DEBUG
	printf("DEBUG - Init-State: %d, RestartCount: %s, Restart-Env: %d.\n",
				initState, getenv("FEESERVER_RESTART_COUNT"), restartCount);
	fflush(stdout);
#	endif

	// test for failed init of CE and init restart counter,
	// counter counts backwards: only if counter > 0 restart is triggerd
	if ((initState != FEE_OK) && (getenv("FEESERVER_RESTART_COUNT")) &&
			(restartCount > 0)) {
		msg[sprintf(msg,
				"Triggering a FeeServer restart to give CE init another try. Restart count (backward counter): %d ",
				restartCount)] = 0;
		createLogMessage(MSG_WARNING, msg, 0);
#		ifdef __DEBUG
		printf("Triggering a FeeServer restart for another CE init try (backward count: %d).\n",
				restartCount);
		fflush(stdout);
#		endif
		// small sleep, that DIM is able to send log messages before restart
		dtq_sleep(1);
		// trigger restart to give it another try for the CE to init
		triggerRestart(FEE_EXITVAL_TRY_INIT_RESTART);
		// NOTE this function won't return ...
	}
	// look through watchdog and backup solution about ceInitState and check it again !!!
	// afterwards the following line won't be necessary !!!
	// needed later in information about properties !!!
//	ceInitState = initState;

	return;
}


void threadInitializeCE() {
	int status = -1;
	status = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
	// if cancelation is not able, it won't hurt ?!
#	ifdef __DEBUG
	if (status != 0) {
		printf("Set cancel state (init) error: %d\n", status);
		fflush(stdout);
	}
#	endif

	status = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	// if cancelation is not able, it won't hurt ?!
#	ifdef __DEBUG
	if (status != 0) {
		printf("Set cancel type (init) error: %d\n", status);
		fflush(stdout);
	}
#	endif

	// Here starts the actual CE
	initializeCE();

	// not necessary, return 0 is better
//	pthread_exit(0);
	return;

}


void signalCEready(int ceState) {
	int status = -1;

	// set cancel type to deferred
	status = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, 0);
#   ifdef __DEBUG
	if (status != 0) {
		printf("Set cancel type error: %d\n", status);
	    fflush(stdout);
	}
#   endif

	//lock the mutex before broadcast
	status = pthread_mutex_lock(&wait_init_mut);
#	ifdef __DEBUG
	if (status != 0) {
		printf("Lock mutex error: %d\n", status);
		fflush(stdout);
	}
#	endif

	// provide init state of CE
	ceInitState = ceState;

	//signal that CE has completed initialisation
	// maybe try the call pthread_cond_signal instead for performance
	pthread_cond_broadcast(&init_cond);

	// set variable for backup solution
	ceReadySignaled = true;

	// unlock mutex
	status = pthread_mutex_unlock(&wait_init_mut);
#	ifdef __DEBUG
	if (status != 0) {
		printf("Unlock mutex error: %d\n", status);
		fflush(stdout);
	}
#	endif

    // set cancel type to asyncroneous
    status = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
#   ifdef __DEBUG
    if (status != 0) {
        printf("Set cancel type error: %d\n", status);
		fflush(stdout);
    }
#   endif
}


// -- Command handler routine --
void command_handler(int* tag, char* address, int* size) {
	struct timeval now;
	struct timespec timeout;
	int retcode  = -1;
	int status = -1;
	pthread_t thread_handle;
	pthread_attr_t attr;
	IssueStruct issueParam;
	CommandHeader header;
	char* pHeaderStream = 0;
	MemoryNode* memNode = 0;
	bool useMM = false;

#ifdef __BENCHMARK
	char benchmsg[200];
	// make benchmark entry
	if ((size != 0 ) && (*size >= 4)) {
		benchmsg[sprintf(benchmsg,
				"FeeServer CommandHandler (Received command) - Packet-ID: %d",
				*address)] = 0;
		createBenchmark(benchmsg);
	} else {
		createBenchmark("FeeServer CommandHandler (Received command)");
	}
#endif

	// init struct
	initIssueStruct(&issueParam);

	issueParam.nRet = FEE_UNKNOWN_RETVAL;

	// check state (ERROR state is allowed for FeeServer commands, not CE)
	if ((state != RUNNING) && (state != ERROR_STATE)) {
		return;
	}

	// lock command mutex to save command &ACK data until it is send
	// and only one CE-Thread exists at one time
	status = pthread_mutex_lock(&command_mut);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Lock command mutex error: %d\n", status);
		fflush(stdout);
#		endif
		createLogMessage(MSG_WARNING, "Unable to lock command mutex.", 0);
	}

	if ((tag == 0) || (address == 0) || (size == 0)) {
		leaveCommandHandler(0, FEE_NULLPOINTER, MSG_WARNING,
 				"Received null pointer of DIM framework in command handler.");
		return;
	}

	if (*size < HEADER_SIZE) {
		leaveCommandHandler(0, FEE_INVALID_PARAM, MSG_WARNING,
 				"FeeServer received corrupted command.");
		return;
	}

#	ifdef __DEBUG
	printf(" Cmnd - Size: %d\n", *size);
	fflush(stdout);
#	endif

	//-- storing the header information in struct --
	memcpy(&header.id, address, HEADER_SIZE_ID);
	memcpy(&header.errorCode, address + HEADER_OFFSET_ID, HEADER_SIZE_ERROR_CODE);
	memcpy(&header.flags, address + HEADER_OFFSET_ERROR_CODE, HEADER_SIZE_FLAGS);
	memcpy(&header.checksum, address + HEADER_OFFSET_FLAGS, HEADER_SIZE_CHECKSUM);

	// --------------------- Check Flags --------------------------
	if ((header.flags & HUFFMAN_FLAG) != 0) {
		//-- do Huffmann decoding if flag is set --
		// not implemented yet !!!
	}

	issueParam.size = *size - HEADER_SIZE;
	issueParam.command = (address + HEADER_SIZE);
	// !!! if Huffman decoding necessary, think about memory management ???

	if ((header.flags & CHECKSUM_FLAG) != 0) {
		//-- do checksum test if flag is set --
		if (!checkCommand(issueParam.command, issueParam.size, header.checksum)) {
			// -- checksum failed - notification
			leaveCommandHandler(header.id, FEE_CHECKSUM_FAILED, MSG_WARNING,
 					"FeeServer received corrupted command data (checksum failed).");
			return;
		}
	}

	// -- here start the Commands for the FeeServer itself --
	if ((header.flags & FEESERVER_UPDATE_FLAG) != 0) {
#ifdef ENABLE_MASTERMODE
		updateFeeServer(&issueParam);
#else
		createLogMessage(MSG_WARNING, "FeeServer is not authorized to execute shell programs, skip ...", 0);
#endif //ENABLE_MASTERMODE
		// this is only reached, if update has not been sucessful
		issueParam.nRet = FEE_FAILED;
		issueParam.size = 0;
	} else if ((header.flags & FEESERVER_RESTART_FLAG) != 0) {
		restartFeeServer();
	} else if ((header.flags & FEESERVER_REBOOT_FLAG) != 0) {
		createLogMessage(MSG_INFO, "Rebooting DCS board.", 0);
		system("reboot");
		exit(0);
	} else if ((header.flags & FEESERVER_SHUTDOWN_FLAG) != 0) {
		createLogMessage(MSG_INFO, "Shuting down DCS board.", 0);
		system("poweroff");
		exit(0);
	} else if ((header.flags & FEESERVER_EXIT_FLAG) != 0) {
		fee_exit_handler(0);
	} else if ((header.flags & FEESERVER_SET_DEADBAND_FLAG) != 0) {
		issueParam.nRet = setDeadband(&issueParam);
	} else if ((header.flags & FEESERVER_GET_DEADBAND_FLAG) != 0) {
		issueParam.nRet = getDeadband(&issueParam);
	} else if ((header.flags & FEESERVER_SET_ISSUE_TIMEOUT_FLAG) != 0) {
		issueParam.nRet = setIssueTimeout(&issueParam);
	} else if ((header.flags & FEESERVER_GET_ISSUE_TIMEOUT_FLAG) != 0) {
		issueParam.nRet = getIssueTimeout(&issueParam);
	} else if ((header.flags & FEESERVER_SET_UPDATERATE_FLAG) != 0) {
		issueParam.nRet = setUpdateRate(&issueParam);
	} else if ((header.flags & FEESERVER_GET_UPDATERATE_FLAG) != 0) {
		issueParam.nRet = getUpdateRate(&issueParam);
	} else if ((header.flags & FEESERVER_SET_LOGLEVEL_FLAG) != 0) {
		issueParam.nRet = setLogLevel(&issueParam);
	} else if ((header.flags & FEESERVER_GET_LOGLEVEL_FLAG) != 0) {
		issueParam.nRet = getLogLevel(&issueParam);
	} else {
		// commands for CE are not allowed in ERROR state
		if (state == ERROR_STATE) {
			leaveCommandHandler(header.id, FEE_WRONG_STATE, MSG_ERROR,
 					"FeeServer is in ERROR_STATE, ignoring command for CE!");
			return;
		}

		// packet with no flags in header and no payload makes no sense
		if (issueParam.size == 0) {
			leaveCommandHandler(header.id, FEE_INVALID_PARAM, MSG_WARNING,
 					"FeeServer received empty command.");
			return;
		}

		// lock mutex
		status = pthread_mutex_lock(&wait_mut);
		if (status != 0) {
			leaveCommandHandler(header.id, FEE_THREAD_ERROR, MSG_ERROR,
 					"Unable to lock condition mutex for watchdog.");
			return;
		}

		status = pthread_attr_init(&attr);
		if (status != 0) {
			unlockIssueMutex();
			leaveCommandHandler(header.id, FEE_THREAD_ERROR, MSG_ERROR,
 					"Unable to initialize issue thread.");
			return;
		}

		status = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		if (status != 0) {
			unlockIssueMutex();
			leaveCommandHandler(header.id, FEE_THREAD_ERROR, MSG_ERROR,
 					"Unable to initialize issue thread.");
			return;
		}

		status = pthread_create(&thread_handle, &attr, &threadIssue, (void*) &issueParam);
		if (status != 0) {
			unlockIssueMutex();
			leaveCommandHandler(header.id, FEE_THREAD_ERROR, MSG_ERROR,
 					"Unable to create issue thread.");
			return;
		}

		status = pthread_attr_destroy(&attr);
		if (status != 0) {
#			ifdef __DEBUG
			printf("Destroy attribute error: %d\n", status);
			fflush(stdout);
#			endif
			createLogMessage(MSG_WARNING,
					"Unable to destroy thread attribute.", 0);
		}

		// timeout set in ms, see fee_defines.h for current value
		status = gettimeofday(&now, 0);
		if (status == 0) {
			// issueTimeout is in milliseconds:
			// get second-part with dividing by 1000
			timeout.tv_sec = now.tv_sec + (int) (issueTimeout / 1000);
			// get rest of division by 1000 (which is milliseconds)
			// and make it nanoseconds
			timeout.tv_nsec = (now.tv_usec * 1000) +
										((issueTimeout % 1000) * 1000000);

			// wait for finishing "issue" or timeout, if signal has been sent
			// retcode is 0 !
			// this is the main logic of the watchdog for the CE of the FeeServer
			retcode = pthread_cond_timedwait(&cond, &wait_mut, &timeout);
#			ifdef __DEBUG
			printf("Retcode of CMND timedwait: %d\n", retcode);
			fflush(stdout);
#			endif

			// check retcode to detect and handle Timeout
			if (retcode == ETIMEDOUT) {
#				ifdef __DEBUG
				printf("ControlEngine watchdog detected TimeOut.\n");
				fflush(stdout);
#				endif
				createLogMessage(MSG_WARNING,
						"ControlEngine watch dog noticed a time out for last command.", 0);

				// kill not finished thread. no problem if this returns an error
				pthread_cancel(thread_handle);
				// setting errorCode to "a timout occured"
				issueParam.nRet = FEE_TIMEOUT;
				issueParam.size = 0;
			} else if (retcode != 0) {
				// "handling" of other error than timeout
#				ifdef __DEBUG
				printf("ControlEngine watchdog detected unknown error.\n");
				fflush(stdout);
#				endif
				createLogMessage(MSG_WARNING,
						"ControlEngine watch dog received an unknown for last command.", 0);

				// kill not finished thread. no problem if this returns an error
				pthread_cancel(thread_handle);
				// setting errorCode to "a thread error occured"
				issueParam.nRet = FEE_THREAD_ERROR;
				issueParam.size = 0;
			}

		} else {
#			ifdef __DEBUG
			printf("Get time of day error: %d\n", status);
			fflush(stdout);
#			endif
			createLogMessage(MSG_WARNING,
				"Watchdog timer could not be initialized. Using non-reliable sleep instead.",
				0);
			// release mutex to avoid hang up in issueThread before signaling condition
			unlockIssueMutex();
			// watchdog with condition signal could not be used, because gettimeofday failed.
			// sleeping instead for usual amount of time and trying to cancel thread aftterwards.
			usleep(issueTimeout * 1000);
			status = pthread_cancel(thread_handle);
			// if thread did still exist something went wrong -> "timeout" (== 0)
			if (status == 0) {
#				ifdef __DEBUG
				printf("TimeOut occured.\n");
#				endif
				createLogMessage(MSG_WARNING,
						"ControlEngine issue did not return in time.", 0);
				issueParam.nRet = FEE_TIMEOUT;
				issueParam.size = 0;
			}
		}

		unlockIssueMutex();
	}
	//--- end of CE call area --------------------

	// ---------- start to compose result -----------------
#	ifdef __DEBUG
	printf("Issue-nRet: %d\n", issueParam.nRet);
	fflush(stdout);
#	endif
	// check return value of issue
	if ((issueParam.nRet < FEE_UNKNOWN_RETVAL) ||
			(issueParam.nRet > FEE_MAX_RETVAL)) {
		issueParam.nRet = FEE_UNKNOWN_RETVAL;
		createLogMessage(MSG_DEBUG,
				"ControlEngine [command] returned unkown RetVal.", 0);
	}

// start here with new memory management check for ACK
	// check if old ACK data is in MemoryNode list and free it
	if ((cmndACKSize > HEADER_SIZE) && (findMemoryNode(cmndACK + HEADER_SIZE) != 0)) {
		memNode = findMemoryNode(cmndACK + HEADER_SIZE);
		freeMemoryNode(memNode);
	} else { // free cmndACK in original way
		if (cmndACK != 0) {
			free(cmndACK);
			cmndACK = 0;
		}
	}

	// check if new result data is in MemoryNode list
	memNode = findMemoryNode(issueParam.result);
	if (memNode != 0) {
		cmndACK = memNode->ptr;
		useMM = true;
	} else {
		// create Acknowledge as return value of command
		// HEADER_SIZE bytes are added before result to insert the command
		// header before the result -> see CommandHeader in Client for details
		cmndACK = (char*) malloc(issueParam.size + HEADER_SIZE);
	}

	if (cmndACK == 0) {
		//no memory available!
#		ifdef __DEBUG
		printf("no memory available!\n");
		fflush(stdout);
#		endif
		createLogMessage(MSG_ERROR, "Insufficient memory for ACK.", 0);

		// no ACK because no memory!
		cmndACKSize = 0;
		status = pthread_mutex_unlock(&command_mut);
		if (status != 0) {
#			ifdef __DEBUG
			printf("Lock command mutex error: %d\n", status);
			fflush(stdout);
#			endif
			createLogMessage(MSG_WARNING,
					"Error while trying to unlock command mutex.", 0);
		}
		return;
	}

#	ifdef __DEBUG
	if (issueParam.size > 0) {
//		printf("in cmnd-Handler -> issue result: ");
//		printData(issueParam.result, 0, issueParam.size);
//		fflush(stdout);
	}
#	endif

	// checks checksumflag and calculates it if necessary
	if ((header.flags & CHECKSUM_FLAG) != 0) {
		header.checksum = calculateChecksum((unsigned char*) issueParam.result,
					issueParam.size);
		// !!! Do (Huffman- ) encoding, if wished afterwards.
	} else {
		header.checksum = CHECKSUM_ZERO;
	}

	// keep the whole flags also for the result packet
	header.errorCode = (short) issueParam.nRet;
#	ifdef __DEBUG
	printf("ErrorCode in Header: %d\n", header.errorCode);
	fflush(stdout);
#	endif

	pHeaderStream = marshallHeader(&header);
	memcpy((void*) cmndACK, (void*) pHeaderStream, HEADER_SIZE);
	if (pHeaderStream != 0) {
		free(pHeaderStream);
	}

	if (useMM) {
	  /*
#		ifdef __DEBUG
		printf("ACK channel used with MemoryManagement in FeeServer.\n");
		fflush(stdout);
		createLogMessage(MSG_DEBUG,
				"ACK channel used with MemoryManagement in FeeServer.", 0);
#		endif
	  */
	} else {
		memcpy(((void*) cmndACK + HEADER_SIZE), (void*) issueParam.result,
				issueParam.size);
	}

	//store the size of the result globally
	cmndACKSize = issueParam.size + HEADER_SIZE;
	// propagate change of ACK(nowledge channel) to upper Layers
	dis_update_service(serviceACKID);

#	ifdef __DEBUG
	// -- see the cmndACK as a char - string
	printf("ACK \n");
//	printData(cmndACK, HEADER_SIZE, cmndACKSize);
	// -- see the cmndACK in a HEX view for the ALTRO
	//print_package(cmndACK + HEADER_SIZE);
#	endif

	if ((!useMM) && (issueParam.result != 0)) {
		free(issueParam.result);
	}
// end of new stuff for memory managment.

/*
	if (cmndACK != 0) {
		free(cmndACK);
		cmndACK = 0;
	}
	// create Acknowledge as return value of command
	// HEADER_SIZE bytes are added before result to insert the command
	// header before the result -> see CommandHeader in Client for details
	cmndACK = (char*) malloc(issueParam.size + HEADER_SIZE);
	if (cmndACK == 0) {
		//no memory available!
#		ifdef __DEBUG
		printf("no memory available!\n");
		fflush(stdout);
#		endif
		createLogMessage(MSG_ERROR, "Insufficient memory for ACK.", 0);

		// no ACK because no memory!
		cmndACKSize = 0;
		status = pthread_mutex_unlock(&command_mut);
		if (status != 0) {
#			ifdef __DEBUG
			printf("Lock command mutex error: %d\n", status);
			fflush(stdout);
#			endif
			createLogMessage(MSG_WARNING,
					"Error while trying to unlock command mutex.", 0);
		}
		return;
	}

#	ifdef __DEBUG
	if (issueParam.size > 0) {
//		printf("in cmnd-Handler -> issue result: ");
//		printData(issueParam.result, 0, issueParam.size);
//		fflush(stdout);
	}
#	endif

	// checks checksumflag and calculates it if necessary
	if ((header.flags & CHECKSUM_FLAG) != 0) {
		header.checksum = calculateChecksum((unsigned char*) issueParam.result,
					issueParam.size);
		// !!! Do (Huffman- ) encoding, if wished afterwards.
	} else {
		header.checksum = CHECKSUM_ZERO;
	}

	// keep the whole flags also for the result packet
	header.errorCode = (short) issueParam.nRet;
#	ifdef __DEBUG
	printf("ErrorCode in Header: %d\n", header.errorCode);
	fflush(stdout);
#	endif

	pHeaderStream = marshallHeader(&header);
	memcpy((void*) cmndACK, (void*) pHeaderStream, HEADER_SIZE);
	if (pHeaderStream != 0) {
		free(pHeaderStream);
	}
	memcpy(((void*) cmndACK + HEADER_SIZE), (void*) issueParam.result,
				issueParam.size);

	//store the size of the result globally
	cmndACKSize = issueParam.size + HEADER_SIZE;
	// propagate change of ACK(nowledge channel) to upper Layers
	dis_update_service(serviceACKID);

#	ifdef __DEBUG
	// -- see the cmndACK as a char - string
	printf("ACK \n");
//	printData(cmndACK, HEADER_SIZE, cmndACKSize);
	// -- see the cmndACK in a HEX view for the ALTRO
	//print_package(cmndACK + HEADER_SIZE);
#	endif

	if (issueParam.result != 0) {
		free(issueParam.result);
	}

*/


	// unlock command mutex, data has been sent
	status = pthread_mutex_unlock(&command_mut);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Lock command mutex error: %d\n", status);
		fflush(stdout);
#		endif
		createLogMessage(MSG_WARNING,
				"Error while trying to unlock command mutex.", 0);
	}
}


//-- user_routine to provide the ACK-data
void ack_service(int* tag, char** address, int* size) {
#ifdef __BENCHMARK
	char benchmsg[200];
#endif

	if ((tag == 0) || (*tag != ACK_SERVICE_TAG)) {
#		ifdef __DEBUG
		printf("invalid ACK Service\n");
		fflush(stdout);
#		endif
		createLogMessage(MSG_WARNING, "DIM Framework called wrong ACK channel.",
				0);
		return;
	}
// use the line below for checking flags of an outgoing feePacket!
//        printf("\nack_service was called flags are:%x%x\n", *(cmndACK+6), *(cmndACK+7));
	if ((cmndACKSize > 0) && (cmndACK != 0)) {
		*address = cmndACK;
		*size = cmndACKSize;
	} else {
		*size = 0;
	}
#ifdef __BENCHMARK
    // make benchmark entry
	benchmsg[sprintf(benchmsg,
			"FeeServer AckHandler (sending ACK) - Packet-ID: %d", *cmndACK)] = 0;
    createBenchmark(benchmsg);
#endif

}


void leaveCommandHandler(unsigned int id, short errorCode,
			unsigned int msgType, char* message) {
	int status = -1;

#	ifdef __DEBUG
	printf("%s\n", message);
	fflush(stdout);
#	endif

	createLogMessage(msgType, message, 0);

	// tell client that command is ignored
	if (cmndACK != 0) {
		free(cmndACK);
		cmndACK = 0;
	}
	// send error code
	cmndACK = createHeader(id, errorCode, false, false, 0);
	cmndACKSize = HEADER_SIZE;
	dis_update_service(serviceACKID);

	// unlock command mutex to "free" commandHandler
	status = pthread_mutex_unlock(&command_mut);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Lock command mutex error: %d\n", status);
		fflush(stdout);
#		endif
		createLogMessage(MSG_WARNING,
				"Error while trying to unlock command mutex.", 0);
	}
}


void unlockIssueMutex() {
	int status = -1;

	status = pthread_mutex_unlock(&wait_mut);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Unlock condition mutex error: %d. Going in ERROR state!\n", status);
		fflush(stdout);
#		endif
		createLogMessage(MSG_ALARM,
			"Unable to unlock watchdog mutex. No more commands will be possible for CE. Going in ERROR state!",
			0);
		state = ERROR_STATE;
	}
}


//-- publish-function called by CE (Control Engine) to declare Float - Items
int publish(Item* item) {
	unsigned int id;
	char* serviceName = 0;

	// check for right state
	if (state != COLLECTING) {
		return FEE_WRONG_STATE;
	}

	// Testing for NULL - Pointer
	// !! Attention: if pointer is not initialized and also NOT set to NULL, this won't help !!
	if (item == 0) {
#		ifdef __DEBUG
		printf("Bad item, not published\n");
		fflush(stdout);
#		endif
		return FEE_NULLPOINTER;
	}
	if (item->name == 0 || item->location == 0) {
#		ifdef __DEBUG
		printf("Bad item, not published\n");
		fflush(stdout);
#		endif
		return FEE_NULLPOINTER;
	}

	// Check name for duplicate here (float)
	if (findItem(item->name) != 0) {
#		ifdef __DEBUG
		printf("Item name already published (float), new float item discarded.\n");
		fflush(stdout);
#		endif
		return FEE_ITEM_NAME_EXISTS;
	}
	// Check in INT list
	if (findIntItem(item->name) != 0) {
#		ifdef __DEBUG
		printf("Item name already published (int), float item discarded.\n");
		fflush(stdout);
#		endif
		return FEE_ITEM_NAME_EXISTS;
	}
    // Check in Char service list
    if (findCharItem(item->name) != 0) {
#       ifdef __DEBUG
        printf("Item name already published in char list, float item discarded.\n");
        fflush(stdout);
#       endif
        return FEE_ITEM_NAME_EXISTS;
    }

	// -- add item as service --
	serviceName = (char*) malloc(serverNameLength + strlen(item->name) + 2);
	if (serviceName == 0) {
		return FEE_INSUFFICIENT_MEMORY;
	}
	// terminate string with '\0'
	serviceName[sprintf(serviceName, "%s_%s", serverName, item->name)] = 0;
	id = dis_add_service(serviceName, "F", (int*) item->location,
			sizeof(float), 0, 0);
	free(serviceName);
	add_item_node(id, item);

	return FEE_OK;
}


//-- function to add service to our servicelist
void add_item_node(unsigned int _id, Item* _item) {
	//create new node with enough memory
	ItemNode* newNode = 0;

	newNode = (ItemNode*) malloc(sizeof(ItemNode));
	if (newNode == 0) {
		//no memory available!
#		ifdef __DEBUG
		printf("no memory available while adding itemNode!\n");
#		endif
		// !!! unable to run FeeServer, write msg in kernel logger !!! (->Tobias)
		cleanUp();
		exit(201);
	}
	//initialize "members" of node
	newNode->prev = 0;
	newNode->next = 0;
	newNode->id = _id;
	newNode->item = _item;
	newNode->lastTransmittedValue = *(_item->location);
	//if default deadband is negative -> set threshold 0, otherwise set half of defaultDeadband
	newNode->threshold = (_item->defaultDeadband < 0) ? 0.0 : (_item->defaultDeadband / 2);
 /*
		if(_item->defaultDeadband < 0) {
		newNode->threshold = 0.0;
        } else {
		newNode->threshold = _item->defaultDeadband / 2;
	}
*/
	newNode->locBackup = _item->location;
	newNode->checksum = calculateChecksum((unsigned char*) &(_item->location),
			sizeof(volatile float*));
	newNode->checksumBackup = newNode->checksum;

#ifdef __DEBUG
	// complete debug display of added Item
/*
	printf("Item: %d\n", _id);
	printf("location: %f, locBackup %f\n", *(_item->location), *(newNode->locBackup));
	printf("location addr: %p, locBackup addr %p\n", _item->location, newNode->locBackup);
	printf("checksum1: %d, checksum2: %d\n\n", newNode->checksum,
			newNode->checksumBackup);
*/
#endif

#	ifdef __DEBUG
	// short debug display of added Item
	printf("init of %s with ID %d: %f\n", newNode->item->name, newNode->id,
				newNode->lastTransmittedValue);
	fflush(stdout);
#	endif

	++nodesAmount;
	//redirect pointers of doubly linked list
	if (firstNode != 0) {
		lastNode->next = newNode;
		newNode->prev = lastNode;
		lastNode = newNode;
	} else {
		firstNode = newNode;
		lastNode = newNode;
	}
}


//-- Logging function -----
void createLogMessage(unsigned int type, char* description, char* origin) {
	int status = -1; // for mutex
	int descLength = 0;
	int originLength = 0;
	time_t timeVal;
	struct tm* now = 0;

/* 	if (state != RUNNING) { */
/* 	  return; */
/* 	} */

	//lock access with mutex due to the fact that FeeServer & CE can use it
	status = pthread_mutex_lock(&log_mut);
	// discard eventual error, this would cause more problems
	// in each case, do NOT call createLogMessage ;) !
#	ifdef __DEBUG
	if (status != 0) {
		printf("Lock log mutex error: %d\n", status);
		fflush(stdout);
	}
#	endif

	if (!checkLogLevel(type)) {   //if not -> unlock mutex -> return
		//unlock mutex
		status = pthread_mutex_unlock(&log_mut);
		// discard eventual error, this would cause more problems
		// in each case, do NOT call createLogMessage ;)
#		ifdef __DEBUG
		if (status != 0) {
			printf("Unlock log mutex error: %d\n", status);
			fflush(stdout);
		}
#		endif
		return;
	}

	// check if message is a replicate of the last log message
	if ((logWatchDogRunning) && (strncmp(description, lastMessage.description,
				(MSG_DESCRIPTION_SIZE - 1)) == 0)) {
		replicatedMsgCount++;

		//unlock mutex
        status = pthread_mutex_unlock(&log_mut);
        // discard eventual error, this would cause more problems
        // in each case, do NOT call createLogMessage ;)
#       ifdef __DEBUG
        if (status != 0) {
            printf("Unlock log mutex error: %d\n", status);
            fflush(stdout);
        }
#       endif
		// message is a replicate of last message, leave Messenger
        return;
	} else {
		// message is not a replicate of last one send,
		// check if replicated log messages are pending
		if (checkReplicatedLogMessage()) {
			// sleep a small amount to let Dim update channel
//			usleep(1000);
		}
	}

	// prepare data (cut off overlength)
	if (description != 0) {
		// limit description to maximum of field in message struct if longer
		descLength = ((strlen(description) >= MSG_DESCRIPTION_SIZE)
				? (MSG_DESCRIPTION_SIZE - 1) : strlen(description));
	}
	if (origin != 0) {
		// limit origin to maximum of field in message struct if longer
		// be aware that "source" also contains server name and a slash
		originLength = ((strlen(origin) >= MSG_SOURCE_SIZE - serverNameLength - 1)
				? (MSG_SOURCE_SIZE - serverNameLength - 2) : strlen(origin));
	}

	//set type
	message.eventType = type;
	//set detector
	memcpy(message.detector, LOCAL_DETECTOR, MSG_DETECTOR_SIZE);
	//set origin
	strcpy(message.source, serverName);
	if (origin != 0) {
		// append slash
		strcpy(message.source + serverNameLength, "/");
		// append origin maximum til end of source field in message struct
		strncpy(message.source + serverNameLength + 1, origin, originLength);
		// terminate with '\0'
		message.source[serverNameLength + 1 + originLength] = 0;
	}
	//set description
	if (description != 0) {
		// fill description field of message struct maximum til end
		strncpy(message.description, description, descLength);
		// terminate with '\0'
		message.description[descLength] = 0;
	} else {
		strcpy(message.description, "No description specified.");
	}
	//set current date and time
	time(&timeVal);
	now = localtime(&timeVal);
	message.date[strftime(message.date, MSG_DATE_SIZE, "%Y-%m-%d %H:%M:%S",
				now)] = 0;

	//updateService
	dis_update_service(messageServiceID);

	// copy send message to storage of last message
	lastMessage = message;

	//unlock mutex
	status = pthread_mutex_unlock(&log_mut);
	// discard eventual error, this would cause more problems
	// in each case, do NOT call createLogMessage ;)
#	ifdef __DEBUG
	if (status != 0) {
		printf("Unlock log mutex error: %d\n", status);
		fflush(stdout);
	}
#	endif
}

bool checkLogLevel(int event) {
	// Comparision with binary AND, if result has 1 as any digit, event is
	// included in current logLevel
	if ((logLevel & event) != 0) {
		return true;
	}
	return false;
}

void dim_error_msg_handler(int severity, int error_code, char* msg) {
	char type[8];
	int eventType = 0;
	char message[MSG_DESCRIPTION_SIZE];

	// map severity to own log levels
	switch (severity) {
		case 0:
			type[sprintf(type, "INFO")] = 0;
			eventType = MSG_INFO;
			break;
		case 1:
			type[sprintf(type, "WARNING")] = 0;
			eventType = MSG_WARNING;
			break;
		case 2:
			type[sprintf(type, "ERROR")] = 0;
			eventType = MSG_ERROR;
			break;
		case 3:
			type[sprintf(type, "FATAL")] = 0;
			eventType = MSG_ERROR;
			break;
		default :
			type[sprintf(type, "UNKNOWN")] = 0;
			eventType = MSG_WARNING;
			break;
	}

#   ifdef __DEBUG
	// print to command line if wanted
	printf("DIM: [%s] - %s.\n", type, msg);
	fflush(stdout);
#	endif

	// send message only if FeeServer is in serving or error state
	if ((state == RUNNING) || (state == ERROR_STATE)) {
		strncpy(message, msg, (MSG_DESCRIPTION_SIZE - 1));
		message[MSG_DESCRIPTION_SIZE - 1] = 0;
		// deliver message to FeeServer message system
		createLogMessage(eventType, message, "DIM\0");
	}
}


//-- tells the server, that he can start serving;
//-- no services can be added when server is in state RUNNING
int start(int initState) {
	int nRet = FEE_UNKNOWN_RETVAL;
	char* serviceName = 0;
	char* messageName = 0;
	char* commandName = 0;
	char msgStructure[50];

	if (state == COLLECTING) {
		//----- add service for acknowledge -----
		serviceName = (char*) malloc(serverNameLength + 13);
		if (serviceName == 0) {
			//no memory available!
#			ifdef __DEBUG
			printf("no memory available while trying to create ACK channel!\n");
			fflush(stdout);
#			endif
			// !!! unable to run FeeServer, write msg in kernel logger !!! (-> Tobias)
			cleanUp();
			exit(201);
		}
		// compose ACK channel name and terminate with '\0'
		serviceName[sprintf(serviceName, "%s_Acknowledge", serverName)] = 0;
		if (cmndACK != 0) {
			free(cmndACK);
			cmndACK = 0;
		}
		// take created header
		cmndACK = createHeader(0, initState, false, false, 0);
		cmndACKSize = HEADER_SIZE;
		// add ACK channel as service to DIM
		serviceACKID = dis_add_service(serviceName, "C", 0, 0, &ack_service,
				ACK_SERVICE_TAG);
		free(serviceName);

		//----- add message service -----
		messageName = (char*) malloc(serverNameLength + 9);
		if (messageName == 0) {
			//no memory available!
#			ifdef __DEBUG
			printf("no memory available while trying to create message channel!\n");
			fflush(stdout);
#			endif
			// !!! unable to run FeeServer, write msg in kernel logger !!! (->Tobias)
			cleanUp();
			exit(201);
		}
		// compose message channel name and terminate with '\0'
		messageName[sprintf(messageName, "%s_Message", serverName)] = 0;
		// compose message structure
		msgStructure[sprintf(msgStructure, "I:1;C:%d;C:%d;C:%d;C:%d",
				MSG_DETECTOR_SIZE, MSG_SOURCE_SIZE, MSG_DESCRIPTION_SIZE,
				MSG_DATE_SIZE)] = 0;
		// add message channel as service to DIM
		messageServiceID = dis_add_service(messageName, msgStructure, (int*) &message,
				sizeof(unsigned int) + MSG_DETECTOR_SIZE + MSG_SOURCE_SIZE +
				MSG_DESCRIPTION_SIZE + MSG_DATE_SIZE, 0, 0);
		free(messageName);

		//----- before start serving we add the only command handled by the server -----
		commandName = (char*) malloc(serverNameLength + 9);
		if (commandName == 0) {
			//no memory available!
#			ifdef __DEBUG
			printf("no memory available while trying to create CMD channel!\n");
			fflush(stdout);
#			endif
			// !!! unable to run FeeServer, write msg in kernel logger !!! (->Tobias)
			cleanUp();
			exit(201);
		}
		// compose Command channel name and terminate with '\0'
		commandName[sprintf(commandName, "%s_Command", serverName)] = 0;
		// add CMD channel as command to DIM, no tag needed,
		// only one command possible
		commandID = dis_add_cmnd(commandName, "C", &command_handler, 0);
		free(commandName);

		//-- now start serving --
		if (dis_start_serving(serverName) == 1) {
			// if start server was successful
			if (initState == FEE_OK) {
				state = RUNNING;
				// start monitoring thread now
				nRet = startMonitorThread();
				if (nRet != FEE_OK) {
#					ifdef __DEBUG
					printf("Could NOT start monitor thread, error: %d\n", nRet);
					fflush(stdout);
#					endif
					createLogMessage(MSG_ERROR,
							"Unable to start monitor thread on FeeServer.", 0);
					return nRet;
				}
				// inform CE about update rate
				provideUpdateRate();
				createLogMessage(MSG_INFO,
						"FeeServer started correctly, including monitor thread.", 0);
				nRet = FEE_OK;
			} else {
				state = ERROR_STATE;
				createLogMessage(MSG_ERROR,
						"Initialisation of ControlEngine failed. FeeServer is running in ERROR state (without CE).",
						0);
				// starting itself worked, so nRet is OK
				nRet = FEE_OK;
			}
			// start "relicated log messages" watchdog now
			nRet = startLogWatchDogThread();
            if (nRet != FEE_OK) {
#               ifdef __DEBUG
                printf("Could NOT start log watch dog thread, error: %d; FeeServer will run without it.\n",
						nRet);
                fflush(stdout);
#               endif
                createLogMessage(MSG_WARNING,
                        "Can not start LogWatchDog thread (filters replicated MSGs). Uncritical error - running without it.",
						 0);
            }
		} else {
			// starting server was not successful, so remove added core - services
			// so they can be added again by next start() - call
			dis_remove_service(serviceACKID);
			free(cmndACK);
			cmndACK = 0;
			cmndACKSize = 0;
			dis_remove_service(messageServiceID);
			dis_remove_service(commandID);
			nRet = FEE_FAILED;
		}
		return nRet;
	}
	//server is already running
	return FEE_OK;
}

// ****************************************
// ---- starts the monitoring thread ----
// ****************************************
int startMonitorThread() {
	int status = -1;
	pthread_attr_t attr;

	// when item lists are empty, no monitor threads are needed
	if ((nodesAmount == 0) && (intNodesAmount == 0)) {
		createLogMessage(MSG_INFO,
				"No Items (float and int) for monitoring are available.", 0);
		return FEE_OK;
	}

	// init thread attribut and set it
	status = pthread_attr_init(&attr);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Init attribute error [mon]: %d\n", status);
		fflush(stdout);
#		endif
		return FEE_MONITORING_FAILED;
	}
	status = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Set attribute error [mon]: %d\n", status);
		fflush(stdout);
#		endif
		return FEE_MONITORING_FAILED;
	}

	// start the monitor thread for float values
	if (nodesAmount > 0) {
		status = pthread_create(&thread_mon, &attr, (void*)&monitorValues, 0);
		if (status != 0) {
#			ifdef __DEBUG
			printf("Create thread error [mon - float]: %d\n", status);
			fflush(stdout);
#			endif
			return FEE_MONITORING_FAILED;
		}
	}

	//start the monitor thread for int values --> NEW v.0.8.1
	if (intNodesAmount > 0) {
		status = pthread_create(&thread_mon_int, &attr, (void*)&monitorIntValues, 0);
		if (status != 0) {
#			ifdef __DEBUG
			printf("Create thread error [mon - int]: %d\n", status);
			fflush(stdout);
#			endif
			return FEE_MONITORING_FAILED;
		}
	}

	// cleanup attribut
	status = pthread_attr_destroy(&attr);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Destroy attribute error [mon]: %d\n", status);
		fflush(stdout);
#		endif
		// no error return value necessary !
	}
	return FEE_OK;
}

// --- this is the monitoring thread ---
void monitorValues() {
	int status = -1;
	int nRet;
	unsigned long sleepTime = 0;
	ItemNode* current = 0;
	char msg[120];
    unsigned long innerCounter = 0; // used for update check after time interval
    unsigned long outerCounter = 0; // used for update check after time interval

	// set flag, that monitor thread has been started
	monitorThreadStarted = true;

	// set cancelation type
	status = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Set cancel state error [mon - float]: %d\n", status);
		fflush(stdout);
#		endif
		createLogMessage(MSG_WARNING,
			"Unable to configure monitor thread (float) properly. Monitoring is not affected.", 0);
	}
	status = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Set cancel type error [mon - float]: %d\n", status);
		fflush(stdout);
#		endif
		createLogMessage(MSG_WARNING,
			"Unable to configure monitor thread (float) properly. Monitoring is not affected.", 0);
	}

	createLogMessage(MSG_DEBUG, "Started monitor thread for FLOAT values successfully.", 0);

	while (1) {
		current = firstNode;
		sleepTime = (unsigned long) (updateRate / nodesAmount);
		while (current != 0) { // is lastNode->next (end of list)
 			if (!checkLocation(current)) {
				msg[sprintf(msg, "Value of item %s (float) is corrupt, reconstruction failed. Ignoring!",
						current->item->name)] = 0;
				createLogMessage(MSG_ERROR, msg, 0);
				// message and do some stuff (like invalidate value)
				// (test, what happens if pointer is redirected ???)
			} else {
				if ((fabsf((*(current->item->location)) - current->lastTransmittedValue)
						>= current->threshold) || (outerCounter ==
						(innerCounter * TIME_INTERVAL_MULTIPLIER))) {
					nRet = dis_update_service(current->id);
					current->lastTransmittedValue = *(current->item->location);
#					ifdef __DEBUG
					//printf("Updated %d clients for service %s [float]: %f\n", nRet,
					//		current->item->name, *(current->item->location));
					//fflush(stdout);
#					endif
				}
			}

			++innerCounter;
			current = current->next;
			usleep(sleepTime * 1000);
			// sleeps xy microseconds, needed milliseconds-> "* 1000"
		}
		// with the check of both counter, each service is at least updated after
		// every (deadband updateRate * nodesAmount) seconds
		innerCounter = 0;
		++outerCounter;
		// after every service in list is updated set counter back to 0
		// the TIME_INTERVAL_MULTIPLIER is used to enlarge the time interval of
		// the request of services without touching the deadband checker updateRate
		if (outerCounter >= (nodesAmount * TIME_INTERVAL_MULTIPLIER)) {
			outerCounter = 0;
		}
	}
	// should never be reached !
	pthread_exit(0);
}

// checks against bitflips in location
bool checkLocation(ItemNode* node) {
	if (node->item->location == node->locBackup) {
		// locations are identical, so no bitflip
		return true;
	}
	// locations are not identical, check further

	if (node->checksum == calculateChecksum((unsigned char*)
			&(node->item->location), sizeof(volatile float*))) {
		// checksum tells, that first location should be valid, repair backup
		node->locBackup = node->item->location;
		return true;
	}
	// original location or first checksum is wrong, continue checking

	if (node->checksum == calculateChecksum((unsigned char*)
			&(node->locBackup), sizeof(volatile float*))) {
		// checksum tells, that location backup should be valid, repair original
		node->item->location = node->locBackup;
		return true;
	}
	// location backup or first checksum is wrong, continue checking

	if (node->checksum == node->checksumBackup) {
		// it seems that location and location backup are wrong
		// or checksum value runs banana, not repairable
		return false;
	}
	// it seems that first checksum is wrong
	// try to fix with second checksum

	if (node->checksumBackup == calculateChecksum((unsigned char*)
			&(node->item->location), sizeof(volatile float*))) {
		// checksum backup tells, that first location should be valid, repair backup
		node->locBackup = node->item->location;
		// repair first checksum
		node->checksum = node->checksumBackup;
		return true;
	}
	// original location or second checksum is wrong, continue checking

	if (node->checksumBackup == calculateChecksum((unsigned char*)
			&(node->locBackup), sizeof(volatile float*))) {
		// checksum backup tells, that location backup should be valid, repair original
		node->item->location = node->locBackup;
		// repair checksum
		node->checksum = node->checksumBackup;
		return true;
	}
	// value is totally banana, no chance to fix
	return false;
}

// ****************************************
// ---- starts the LogWatchdog Thread  ----
// ****************************************
int startLogWatchDogThread() {
    int status = -1;
    pthread_attr_t attr;

    // init thread attribut and set it
    status = pthread_attr_init(&attr);
    if (status != 0) {
#       ifdef __DEBUG
        printf("Init attribute error [LogWatchDog]: %d\n", status);
        fflush(stdout);
#       endif
        return FEE_THREAD_ERROR;
    }
    status = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (status != 0) {
#       ifdef __DEBUG
        printf("Set attribute error [LogWatchDog]: %d\n", status);
        fflush(stdout);
#       endif
        return FEE_THREAD_ERROR;
    }

    // start the LogWatchDog thread
    status = pthread_create(&thread_logWatchdog, &attr, (void*) &runLogWatchDog,
		0);
    if (status != 0) {
#       ifdef __DEBUG
        printf("Create thread error [LogWatchDog]: %d\n", status);
        fflush(stdout);
#       endif
        return FEE_THREAD_ERROR;
    }

    // cleanup attribut
    status = pthread_attr_destroy(&attr);
    if (status != 0) {
#       ifdef __DEBUG
        printf("Destroy attribute error [LogWatchDog]: %d\n", status);
        fflush(stdout);
#       endif
        // no error return value necessary !
    }

	return FEE_OK;
}

void runLogWatchDog() {
	int status = -1;
	unsigned int sleepSec = 0;
	unsigned int sleepMilliSec = 0;

    // set cancelation type
    status = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
    if (status != 0) {
#       ifdef __DEBUG
        printf("Set cancel state error [LogWatchDog]: %d\n", status);
        fflush(stdout);
#       endif
        createLogMessage(MSG_WARNING,
            "Can not set cancel state for LogWatchDog thread. WatchDog should not not be affected.",
			0);
    }
    status = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
    if (status != 0) {
#       ifdef __DEBUG
        printf("Set cancel type error [LogWatchDog]: %d\n", status);
        fflush(stdout);
#       endif
        createLogMessage(MSG_WARNING,
            "Can not set cancel type for LogWatchDog thread. WatchDog should not not be affected.",
			0);
    }

    // thread started successfully, set flag accordingly
    logWatchDogRunning = true;
    createLogMessage(MSG_DEBUG,
            "LogWatchDog thread for filtering replicated log messages successfully started.",
            0);
#   ifdef __DEBUG
    printf("LogWatchDog thread for filtering replicated log messages successfully started.\n");
    fflush(stdout);
#   endif

	while (1) {
		// before test for replicated messages lock mutex,
		// DON'T call createLogMessage() inside mutex lock
    	status = pthread_mutex_lock(&log_mut);
    	// discard eventual error, this would cause more problems
#   	ifdef __DEBUG
    	if (status != 0) {
        	printf("Lock log mutex error: %d\n", status);
        	fflush(stdout);
    	}
#   	endif

		// perform check here
		checkReplicatedLogMessage();

		// release mutex
        status = pthread_mutex_unlock(&log_mut);
        // discard eventual error, this would cause more problems
#       ifdef __DEBUG
        if (status != 0) {
            printf("Unlock log mutex error: %d\n", status);
            fflush(stdout);
        }
#       endif

		// set cancelation point
		pthread_testcancel();
		// prepare sleep time (timeout)
		sleepSec = logWatchDogTimeout / 1000;
		sleepMilliSec = logWatchDogTimeout % 1000;
		usleep(sleepMilliSec * 1000);
		dtq_sleep(sleepSec);
		// set cancelation point
		pthread_testcancel();
	}

	// should never be reached !
	pthread_exit(0);
}

bool checkReplicatedLogMessage() {
	int tempLength = 0;
	time_t timeVal;
	struct tm* now = 0;

	// check if replicated messages are pending
	if (replicatedMsgCount > 0) {
        // replicated messages occured in between, informing upper layer ...
        message.description[sprintf(message.description,
                "Log message repeated %d times: ", replicatedMsgCount)] = 0;
        // append original message as far as possible
        tempLength = strlen(message.description);
		if ((strlen(lastMessage.description) + tempLength) >=
				MSG_DESCRIPTION_SIZE) {
            // copy only a part of the original message that fits in the
            // description field
            strncpy((message.description + tempLength), lastMessage.description,
                    (MSG_DESCRIPTION_SIZE - tempLength));
            message.description[MSG_DESCRIPTION_SIZE - 1] = 0;
        } else {
            // enough space free, copy the whole original message
            strcpy((message.description + tempLength),
                    lastMessage.description);
            message.description[strlen(lastMessage.description) +
                    tempLength - 1] = 0;
        }
		// set correct timestamp
        time(&timeVal);
		now = localtime(&timeVal);
		message.date[strftime(message.date, MSG_DATE_SIZE, "%Y-%m-%d %H:%M:%S",
				now)] = 0;

        //update MsgService with notification of repeated messages
        dis_update_service(messageServiceID);
        // clearing counter
        replicatedMsgCount = 0;
		return true;
    }
	return false;
}

void* threadIssue(void* threadParam) {
	IssueStruct* issueParam = (IssueStruct*) threadParam;
	int status;

	status = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Set cancel state error: %d\n", status);
		fflush(stdout);
#		endif
		createLogMessage(MSG_WARNING,
			"Unable to configure issue thread properly. Execution might eventually be affected.", 0);
	}

	status = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Set cancel type error error: %d\n", status);
		fflush(stdout);
#		endif
		createLogMessage(MSG_WARNING,
			"Unable to configure issue thread properly. Execution might eventually be affected.", 0);
	}

	// executing command inside CE
	issueParam->nRet = issue(issueParam->command, &(issueParam->result), &(issueParam->size));

    //set cancel type to deferred
    status = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, 0);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Set cancel type error: %d\n", status);
		fflush(stdout);
#	   endif
		createLogMessage(MSG_WARNING,
			"Unable to configure issue thread properly. Execution might eventually be affected.", 0);
    }

	//lock the mutex before broadcast
	status = pthread_mutex_lock(&wait_mut);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Lock cond mutex error: %d\n", status);
		fflush(stdout);
#		endif
		createLogMessage(MSG_WARNING,
			"Unable to lock condition mutex for watchdog in issue thread. Execution might eventually be affected.",
			0);
	}

	//signal that issue has returned from ControlEngine
	// maybe try the call pthread_cond_signal instead for performance
	pthread_cond_broadcast(&cond);

	// unlock mutex
	status = pthread_mutex_unlock(&wait_mut);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Unlock cond mutex error: %d\n", status);
		fflush(stdout);
#		endif
		createLogMessage(MSG_WARNING,
			"Unable to unlock condition mutex for watchdog in issue thread. Execution might eventually be affected.",
			0);
	}

//	not needed, return 0 is better solution
//	pthread_exit(0);
	return 0;
}


char* createHeader(unsigned int id, short errorCode, bool huffmanFlag,
					bool checksumFlag, int checksum) {
	char* pHeader = 0;
	FlagBits flags = NO_FLAGS;

	if (huffmanFlag) {
		//set huffman flag via binary OR
		flags |= HUFFMAN_FLAG;
	}
	if (checksumFlag) {
		//set checksum flag via binary OR
		flags |= CHECKSUM_FLAG;
	}

	pHeader = (char*) malloc(HEADER_SIZE);
	if (pHeader == 0) {
		//no memory available!
#		ifdef __DEBUG
		printf("no memory available while trying to create header!\n");
		fflush(stdout);
#		endif
		createLogMessage(MSG_ALARM,
				"No more memory available, unable to continue serving - exiting.",
				0);
		cleanUp();
		exit(201);
	}

	memcpy(pHeader, &id, HEADER_SIZE_ID);
	memcpy(pHeader + HEADER_OFFSET_ID, &errorCode, HEADER_SIZE_ERROR_CODE);
	memcpy(pHeader + HEADER_OFFSET_ERROR_CODE, &flags, HEADER_SIZE_FLAGS);
	memcpy(pHeader + HEADER_OFFSET_FLAGS, &checksum, HEADER_SIZE_CHECKSUM);

	return pHeader;
}


bool checkCommand(char* payload, int size, unsigned int checksum) {
	unsigned int payloadChecksum = 0;

	// payload has to contain data, if size is greater than 0,
	// and size must not be negative
	if (((payload == 0) && (size > 0)) || (size < 0)) {
		return false;
	}

	payloadChecksum = calculateChecksum((unsigned char*) payload, size);
#	ifdef __DEBUG
	printf("\nReceived Checksum: \t%x ,  \nCalculated Checksum: \t%x .\n\n",
		checksum, payloadChecksum);
	fflush(stdout);
#	endif
	return (payloadChecksum == checksum) ? true : false;
}

// ! Problems with signed and unsigned char, make differences in checksum
// -> so USE "unsigned char*" !
unsigned int calculateChecksum(unsigned char* buffer, int size) {
	int n;
	unsigned int checks = 0;
	unsigned long adler = 1L;
	unsigned long part1 = adler & 0xffff;
	unsigned long part2 = (adler >> 16) & 0xffff;

	// calculates the checksum with the Adler32 algorithm
	for (n = 0; n < size; n++) {
		part1 = (part1 + buffer[n]) % ADLER_BASE;
		part2 = (part2 + part1) % ADLER_BASE;
	}
	checks = (unsigned int) ((part2 << 16) + part1);

	return checks;
}

char* marshallHeader(CommandHeader* pHeader) {
	char* tempHeader = 0;

	tempHeader = (char*) malloc(HEADER_SIZE);
	if (tempHeader == 0) {
		//no memory available!
#		ifdef __DEBUG
		printf("no memory available!\n");
		fflush(stdout);
#		endif
		createLogMessage(MSG_ALARM,
				"No more memory available, unable to continue serving - exiting.",
				0);
		cleanUp();
		exit(201);
	}

	memcpy(tempHeader, &(pHeader->id), HEADER_SIZE_ID);
	memcpy(tempHeader + HEADER_OFFSET_ID, &(pHeader->errorCode), HEADER_SIZE_ERROR_CODE);
	memcpy(tempHeader + HEADER_OFFSET_ERROR_CODE, &(pHeader->flags), HEADER_SIZE_FLAGS);
	memcpy(tempHeader + HEADER_OFFSET_FLAGS, &(pHeader->checksum), HEADER_SIZE_CHECKSUM);

	return tempHeader;
}

// *********************************************************************************************
// ---------------- here come all the FeeServer commands ----------------------------------------
// *********************************************************************************************
void updateFeeServer(IssueStruct* issueParam) {
#ifdef ENABLE_MASTERMODE
	int status = 0;
	int i = 0;
	FILE* fp = 0;

	if ((*issueParam).size == 0) {
		createLogMessage(MSG_ERROR, "Received new FeeServer with size 0.", 0);
		return;
	}
#	ifdef __DEBUG
	printf("Received update command, updating FeeServer now!\n");
	fflush(stdout);
#	endif
	// execute instruction self and release mutex before return
	fp = fopen("newFeeserver", "w+b");
	if (fp == 0) {
		createLogMessage(MSG_ERROR, "Unable to save new FeeServer binary.", 0);
		return;
	}

	for ( i = 0; i < (*issueParam).size; ++i) {
		fputc((*issueParam).command[i], fp);
	}
	fclose(fp);
	createLogMessage(MSG_INFO, "FeeServer updated.", 0);

	// should we call cleanUp() before restart
	// better not: another possibility to hang ???
	// -> cleanUp is in restart implicit ! except opened drivers
	// could only be necessary, if some driver conns have to be closed.
	cleanUp();
	status = pthread_mutex_unlock(&command_mut);
#	ifdef __DEBUG
	if (status != 0) {
		printf("Unlock FeeCommand mutex error: %d\n", status);
		fflush(stdout);
	}
#	endif

	// Exit status "3" tells the starting script to finalise update and restart
	// FeeServer after termination.
	exit(3);
#endif //ENABLE_MASTERMODE
}

void restartFeeServer() {
	triggerRestart(FEE_EXITVAL_RESTART);
}

void triggerRestart(int exitVal) {
	int status = 0;
	char msg[80];

    msg[sprintf(msg, "Restarting FeeServer - exit value: %d ", exitVal)] = 0;
    createLogMessage(MSG_INFO, msg, 0);

	// should we call cleanUp() before restart
	// better not: another possibility to hang ???
	// -> cleanUp is in restart implicit ! except opened drivers
	// could only be necessary, if some driver cons has to be closed.
	cleanUp();
	status = pthread_mutex_unlock(&command_mut);
#	ifdef __DEBUG
	if (status != 0) {
		printf("Unlock FeeCommand mutex error: %d\n", status);
		fflush(stdout);
	}
#	endif
	// Exit status tells the startScript which type of restart is performed
	exit(exitVal);
}

int setDeadband(IssueStruct* issueParam) {
	char* itemName = 0;
	float newDeadband = 0;
	int nameLength = 0;
	ItemNode* node = 0;
	IntItemNode* intNode = 0;
	char msg[100];
	unsigned int count = 0;

	if ((*issueParam).size <= sizeof(newDeadband)) {
		(*issueParam).size = 0;
		createLogMessage(MSG_DEBUG,
				"FeeServer command for setting dead band contained invalid parameter.",
				0);
		return FEE_INVALID_PARAM;
	}

	nameLength = (*issueParam).size - sizeof(newDeadband);

	if (nameLength <= 0) {
		(*issueParam).size = 0;
		createLogMessage(MSG_DEBUG,
				"FeeServer command for setting dead band contained no service name.",
				0);
		return FEE_INVALID_PARAM;
	}

	itemName = (char*) malloc(nameLength + 1);
	if (itemName == 0) {
		(*issueParam).size = 0;
		return FEE_INSUFFICIENT_MEMORY;
	}

	memcpy(&newDeadband, (*issueParam).command, sizeof(newDeadband));
	memcpy(itemName, (*issueParam).command + sizeof(newDeadband), nameLength);
	itemName[nameLength] = 0;

	if (itemName[0] != '*') {
		// search wanted itemNode
		node = findItem(itemName);
		if (node == 0) {
			//check in IntItemList
			intNode = findIntItem(itemName);
		}
		if ((node == 0) && (intNode == 0)) {
			// message is NOT sent in findItem() or findIntItem()
			msg[sprintf(msg, "Item %s not found in list.", itemName)] = 0;
			createLogMessage(MSG_WARNING, msg, 0);
#			ifdef __DEBUG
			printf("Item %s not found in list.\n", itemName);
			fflush(stdout);
#			endif

			free(itemName);
			(*issueParam).size = 0;
			createLogMessage(MSG_DEBUG,
					"FeeServer command for setting dead band contained invalid parameter.",
					0);
			return FEE_INVALID_PARAM;
		} else {
			// set new threshold ( = dead  band / 2)
			if (node != 0) {
				node->threshold = newDeadband / 2;
			} else {
				intNode->threshold = newDeadband / 2;
			}
#			ifdef __DEBUG
			printf("Set deadband on item %s to %f.\n", itemName, newDeadband);
			fflush(stdout);
#			endif
			msg[sprintf(msg, "New dead band (%f) is set for item %s.",
					newDeadband, itemName)] = 0;
			createLogMessage(MSG_INFO, msg, 0);
		}
	} else {
		// set now for all wanted value the new deadband
		count = setDeadbandBroadcast(itemName, newDeadband);

#		ifdef __DEBUG
		printf("Set deadband for %d items (%s) to %f.\n", count, itemName, newDeadband);
		fflush(stdout);
#		endif
		msg[sprintf(msg, "New dead band (%f) is set for %d items (%s).",
				newDeadband, count, itemName)] = 0;
		createLogMessage(MSG_INFO, msg, 0);
	}

	free(itemName);
	(*issueParam).size = 0;
	return FEE_OK;
}

int getDeadband(IssueStruct* issueParam) {
	char* itemName = 0;
	int nameLength = 0;
	ItemNode* node = 0;
	IntItemNode* intNode = 0;
	float currentDeadband = 0;
	char msg[120];

	if ((*issueParam).size <= 0) {
		(*issueParam).size = 0;
		createLogMessage(MSG_DEBUG,
				"FeeServer command for getting dead band contained invalid parameter.",
				0);
		return FEE_INVALID_PARAM;
	}

	nameLength = (*issueParam).size;
	itemName = (char*) malloc(nameLength + 1);
	if (itemName == 0) {
		(*issueParam).size = 0;
		return FEE_INSUFFICIENT_MEMORY;
	}

	memcpy(itemName, (*issueParam).command, nameLength);
	itemName[nameLength] = 0;

	// search wanted itemNode
	node = findItem(itemName);
	if (node == 0) {
		//check in IntItemList
		intNode = findIntItem(itemName);
	}
	if ((node == 0) && (intNode == 0)) {
		// message is NOT sent in findItem() or findIntItem()
		msg[sprintf(msg, "Item %s not found in list.", itemName)] = 0;
		createLogMessage(MSG_WARNING, msg, 0);
#		ifdef __DEBUG
		printf("Item %s not found in list.\n", itemName);
		fflush(stdout);
#		endif

		free(itemName);
		(*issueParam).size = 0;
		createLogMessage(MSG_DEBUG,
				"FeeServer command for getting dead band contained invalid parameter.",
				0);
		return FEE_INVALID_PARAM;
	} else {
		(*issueParam).result = (char*) malloc(sizeof(float) + nameLength);
		if ((*issueParam).result == 0) {
			(*issueParam).size = 0;
			return FEE_INSUFFICIENT_MEMORY;
		}

		// copy current deadband value to ACK result
		if (node != 0) {
			currentDeadband = node->threshold * 2.0; // compute deadband
		} else {
			currentDeadband = intNode->threshold * 2.0; // compute deadband
		}

		memcpy((*issueParam).result, &currentDeadband, sizeof(float));
		memcpy((*issueParam).result + sizeof(float), itemName, nameLength);
		(*issueParam).size = sizeof(float) + nameLength;
#		ifdef __DEBUG
		printf("Current deadband on item %s is %f.\n", itemName, currentDeadband);
		fflush(stdout);
#		endif
		msg[sprintf(msg, "Current deadband for item %s is %f.", itemName,
				currentDeadband)] = 0;
		createLogMessage(MSG_DEBUG, msg, 0);
	}
	free(itemName);
	return FEE_OK;
}

int setIssueTimeout(IssueStruct* issueParam) {
	char msg[70];
	unsigned long newTimeout;

	if ((*issueParam).size < sizeof(unsigned long)) {
		(*issueParam).size = 0;
		createLogMessage(MSG_DEBUG,
				"FeeServer command for setting issue timeout contained invalid parameter.",
				0);
		return FEE_INVALID_PARAM;
	}
	memcpy(&newTimeout, (*issueParam).command, sizeof(unsigned long));

	// check new timeout for possible buffer overflow (value will be multiplied
	// with 1000 later -> has to lower than 4294967 )
	if (newTimeout > MAX_ISSUE_TIMEOUT) {
        (*issueParam).size = 0;
        createLogMessage(MSG_WARNING,
                "New timeout for issue watchdog exceeded value limit (4294967).",
                0);
        return FEE_INVALID_PARAM;
    }

	issueTimeout = newTimeout;
#	ifdef __DEBUG
	printf("set new Issue timeout to %lu\n", issueTimeout);
	fflush(stdout);
#	endif
	msg[sprintf(msg, "Watch dog time out is set to %lu.", issueTimeout)] = 0;
	createLogMessage(MSG_INFO, msg, 0);

	(*issueParam).size = 0;
	return FEE_OK;
}

int getIssueTimeout(IssueStruct* issueParam) {
	char msg[50];

	(*issueParam).result = (char*) malloc(sizeof(unsigned long));
	if ((*issueParam).result == 0) {
		(*issueParam).size = 0;
		return FEE_INSUFFICIENT_MEMORY;
	}

	// copy current timeout for issue to ACK result
	memcpy((*issueParam).result, &issueTimeout, sizeof(unsigned long));
	(*issueParam).size = sizeof(unsigned long);
#	ifdef __DEBUG
	printf("issue timeout is %lu\n", issueTimeout);
	fflush(stdout);
#	endif
	msg[sprintf(msg, "Issue timeout is %lu.", issueTimeout)] = 0;
	createLogMessage(MSG_DEBUG, msg, 0);

	return FEE_OK;
}

int setUpdateRate(IssueStruct* issueParam) {
	char msg[70];
	unsigned short newRate;

	if ((*issueParam).size < sizeof(unsigned short)) {
		(*issueParam).size = 0;
		createLogMessage(MSG_DEBUG,
				"FeeServer command for setting update rate contained invalid parameter.",
				0);
		return FEE_INVALID_PARAM;
	}
	memcpy(&newRate, (*issueParam).command, sizeof(unsigned short));
	updateRate = newRate;
	// inform CE about update rate change
	provideUpdateRate();
#	ifdef __DEBUG
	printf("set new update rate to %d\n", updateRate);
	fflush(stdout);
#	endif
	msg[sprintf(msg, "New update rate for monitoring items: %d.", updateRate)] = 0;
	createLogMessage(MSG_INFO, msg, 0);

	(*issueParam).size = 0;
	return FEE_OK;
}

int getUpdateRate(IssueStruct* issueParam) {
	char msg[50];

	(*issueParam).result = (char*) malloc(sizeof(unsigned short));
	if ((*issueParam).result == 0) {
		(*issueParam).size = 0;
		return FEE_INSUFFICIENT_MEMORY;
	}

	// copy current update rate to ACK result
	memcpy((*issueParam).result, &updateRate, sizeof(unsigned short));
	(*issueParam).size = sizeof(unsigned short);
#	ifdef __DEBUG
	printf("update rate is %d\n", updateRate);
	fflush(stdout);
#	endif
	msg[sprintf(msg, "Monitoring update rate is %d.", updateRate)] = 0;
	createLogMessage(MSG_DEBUG, msg, 0);

	return FEE_OK;
}

// be aware of different size of unsigned int in heterogen systems !!
int setLogLevel(IssueStruct* issueParam) {
	int status = -1;
	char msg[70];
	unsigned int testLogLevel = 0;

	if ((*issueParam).size < sizeof(unsigned int)) {
		(*issueParam).size = 0;
		createLogMessage(MSG_DEBUG,
				"FeeServer command for setting log level contained invalid parameter.",
				0);
		return FEE_INVALID_PARAM;
	}
	memcpy(&testLogLevel, (*issueParam).command, sizeof(unsigned int));
	// check loglevel for valid data
	if (testLogLevel > MSG_MAX_VAL) {
#		ifdef __DEBUG
		printf("received invalid log level %d\n", testLogLevel);
		fflush(stdout);
#		endif
		createLogMessage(MSG_DEBUG,
				"FeeServer command for setting log level contained invalid parameter.",
				0);
		return FEE_INVALID_PARAM;
	}

	status = pthread_mutex_lock(&log_mut);
	// discard eventual error
	if (status != 0) {
#		ifdef __DEBUG
		printf("Lock log mutex error: %d\n", status);
		fflush(stdout);
#		endif
		(*issueParam).size = 0;
		return FEE_FAILED;
	} else {
		logLevel = testLogLevel | MSG_ALARM;

		status = pthread_mutex_unlock(&log_mut);
		if (status != 0) {
#			ifdef __DEBUG
			printf("Unlock log mutex error: %d\n", status);
			fflush(stdout);
#			endif
			createLogMessage(MSG_WARNING, "Unable to unlock logger mutex.", 0);
		}

#		ifdef __DEBUG
		printf("set new logLevel to %d\n", logLevel);
		fflush(stdout);
#		endif
		msg[sprintf(msg, "New log level on FeeServer: %d.", logLevel)] = 0;
		createLogMessage(MSG_INFO, msg, 0);

		(*issueParam).size = 0;
		return FEE_OK;
	}
}

int getLogLevel(IssueStruct* issueParam) {
	char msg[50];

	(*issueParam).result = (char*) malloc(sizeof(unsigned int));
	if ((*issueParam).result == 0) {
		(*issueParam).size = 0;
		return FEE_INSUFFICIENT_MEMORY;
	}

	// copy current update rate to ACK result
	memcpy((*issueParam).result, &logLevel, sizeof(unsigned int));
	(*issueParam).size = sizeof(unsigned int);
#	ifdef __DEBUG
	printf("Requested loglevel is %d\n", logLevel);
	fflush(stdout);
#	endif
	msg[sprintf(msg, "Requested LogLevel is %d.", logLevel)] = 0;
	createLogMessage(MSG_DEBUG, msg, 0);

	return FEE_OK;
}

void provideUpdateRate() {
	FeeProperty feeProp;
	if ((ceInitState == CE_OK) && ((nodesAmount > 0) || (intNodesAmount > 0))) {
		feeProp.flag = PROPERTY_UPDATE_RATE;
		feeProp.uShortVal = updateRate;
		signalFeePropertyChanged(&feeProp);
	}
}


unsigned int setDeadbandBroadcast(char* name, float newDeadbandBC) {
	unsigned int count = 0;
	ItemNode* current = 0;
	IntItemNode* intCurrent = 0;
	char* namePart = 0;
	char* itemNamePart = 0;

	if (name == 0) {
		return count;
	}

	// pointer at first occurance of "_"
	namePart = strpbrk(name, "_");
	if (namePart == 0) {
		return count;
	}

	// go through list of float values
	current = firstNode;
	while (current != 0) {  // is end of list
		// pointer at first occurance of "_"
		itemNamePart = strpbrk(current->item->name, "_");
		// check if "_" not first character in name and is existing
		if ((itemNamePart == 0) || (itemNamePart == current->item->name)) {
			current = current->next;
			continue;
		}
		if (strcmp(namePart, itemNamePart) == 0) {
			// success, set threshold (= deadband / 2)
			current->threshold = newDeadbandBC / 2;
			++count;
		}
		current = current->next;
	}

	// go through list of integer values
	intCurrent = firstIntNode;
	while (intCurrent != 0) {  // is end of list
		// pointer at first occurance of "_"
		itemNamePart = strpbrk(intCurrent->intItem->name, "_");
		// check if "_" not first character in name and is existing
		if ((itemNamePart == 0) || (itemNamePart == intCurrent->intItem->name)) {
			intCurrent = intCurrent->next;
			continue;
		}
		if (strcmp(namePart, itemNamePart) == 0) {
			// success, set threshold (= deadband / 2)
			intCurrent->threshold = newDeadbandBC / 2;
			++count;
		}
		intCurrent = intCurrent->next;
	}

	return count;
}

int updateFeeService(char* serviceName) {
	char msg[80];
	ItemNode* node = 0;
	IntItemNode* intNode = 0;
	CharItemNode* charNode = 0;
	int nRet = 0;
#	ifdef __DEBUG
	char* addr = 0;
	char* data = 0;
	int size = 0;
#	endif

	if (state != RUNNING) {
		return FEE_WRONG_STATE;
	}
	if (serviceName == 0) {
		return FEE_NULLPOINTER;
	}

	// find desired service
	node = findItem(serviceName);
	if (node == 0) {
		//check in IntItemList
		intNode = findIntItem(serviceName);
	}
	if ((node == 0) || (intNode == 0)) {
		//check in CharItemList
        charNode = findCharItem(serviceName);
	}

	// check node
	if ((node == 0) && (intNode == 0) && (charNode == 0)) {
		// message is NOT sent in findItem(), findIntItem() or findCharItem()
		msg[sprintf(msg, "Item %s not found in list.", serviceName)] = 0;
		createLogMessage(MSG_WARNING, msg, 0);
#		ifdef __DEBUG
		printf("Item %s not found in list.\n", serviceName);
		fflush(stdout);
#		endif
		return FEE_INVALID_PARAM;
	} else {
		if (node != 0) {
			nRet = dis_update_service(node->id);
#			ifdef __DEBUG
/* 			printf("CE triggered an updated on %d clients for service %s [float]: %f\n", */
/* 					nRet, node->item->name, *(node->item->location)); */
/* 			fflush(stdout); */
#			endif
		} else if (intNode != 0) {
			nRet = dis_update_service(intNode->id);
#           ifdef __DEBUG
/*             printf("CE triggered an updated on %d clients for service %s [int]: %d\n", */
/*                     nRet, intNode->intItem->name, *(intNode->intItem->location)); */
/*             fflush(stdout); */
#           endif
		} else {
            nRet = dis_update_service(charNode->id);
#           ifdef __DEBUG
			(charNode->charItem->user_routine)(&(charNode->charItem->tag),
					(int**) &addr, &size);
			data = (char*) malloc(size +1);
			strncpy(data, addr, size);
			data[size] = 0;
/*             printf("CE triggered an updated on %d clients for service %s [char]: %s\n", */
/*                     nRet, charNode->charItem->name, data); */
/*             fflush(stdout); */
			free(data);
#           endif
		}
	}

	// return number of updated clients
	return nRet;
}


// *********************************************************************************************
// ---------- here comes all the initialisation of selfdefined datatypes -----------------------
// *********************************************************************************************
void initIssueStruct(IssueStruct* issueStr) {
	issueStr->nRet = 0;
	issueStr->command = 0;
	issueStr->result = 0;
	issueStr->size = 0;
}

void initMessageStruct() {
	time_t timeVal;
	struct tm* now;

	// fill the original message struct
	message.eventType = MSG_INFO;
	memcpy(message.detector, LOCAL_DETECTOR, MSG_DETECTOR_SIZE);
	memcpy(message.source, "FeeServer\0", 10);
	message.description[sprintf(message.description,
			"FeeServer %s (Version: %s) has been initialized (DNS: %s) ...",
			serverName, FEESERVER_VERSION, getenv("DIM_DNS_NODE"))] = 0;
	//set current date and time
	time(&timeVal);
	now = localtime(&timeVal);
	message.date[strftime(message.date, MSG_DATE_SIZE, "%Y-%m-%d %H:%M:%S",
			now)] = 0;

	// the the storage of the "last Message" struct - must be different in
	// the description compared to the original message struct above in init
	lastMessage.eventType = MSG_DEBUG;
    memcpy(lastMessage.detector, LOCAL_DETECTOR, MSG_DETECTOR_SIZE);
    memcpy(lastMessage.source, "FeeServer\0", 10);
    // now use a different description:
	lastMessage.description[sprintf(lastMessage.description,
			"Init of Backup Message Struct!")] = 0;
    //set current date and time
    time(&timeVal);
    now = localtime(&timeVal);
    lastMessage.date[strftime(lastMessage.date, MSG_DATE_SIZE,
			"%Y-%m-%d %H:%M:%S", now)] = 0;
}


void initItemNode(ItemNode* iNode) {
	iNode->prev = 0;
	iNode->next = 0;
	iNode->id = 0;
	iNode->item = 0;
	iNode->lastTransmittedValue = 0.0;
	iNode->threshold = 0.0;
	iNode->locBackup = 0;
	iNode->checksum = 0;
	iNode->checksumBackup = 0;
}

ItemNode* findItem(char* name) {
//	char msg[70];
	ItemNode* current = 0;

	if (name == 0) {
		return 0;
	}
	current = firstNode;
	while (current != 0) {  // is end of list
		if (strcmp(name, current->item->name) == 0) {
			// success, give back itemNode
			return current;
		}
		current = current->next;
	}
// since two lists, which are searched seperately don't make log output in
// findItem()-function -> move to where called to combine with other 
// findXYItem calls
/*
	if (state == RUNNING) {
		msg[sprintf(msg, "Item %s not found in list.", name)] = 0;
		createLogMessage(MSG_WARNING, msg, 0);
#		ifdef __DEBUG
		printf("Item %s not found in list.\n", name);
		fflush(stdout);
#		endif
	}
*/
	return 0;
}

void unpublishItemList() {
	ItemNode* current = 0;

	current = firstNode;
	while (current != 0) {
		dis_remove_service(current->id);
		current = current->next;
	}
	// pretending ItemList is completely empty to avoid access
	// to not existing elements
	nodesAmount = 0;
	firstNode = 0;
	lastNode = 0;
}

// ****************************************************************************
// ------------------ here come all the closing and cleanup functions ---------
// ****************************************************************************
void interrupt_handler(int sig) {
// *** causes props on DCS board -> not used yet ***
#	ifdef __DEBUG
	printf("Received interrupt: %d, exiting now.\n", sig);
	fflush(stdout);
#	endif

	if ((state == RUNNING) || (state == ERROR_STATE)) {
		fee_exit_handler(0);
	} else {
		cleanUp();
		exit(0);
	}
}

void fee_exit_handler(unsigned int state) {
	char msg[70];

#	ifdef __DEBUG
	printf("Exit state: %d\n\n", state);
	fflush(stdout);
#	endif
	msg[sprintf(msg, "Exiting FeeServer (exit state: %d).", state)] = 0;
	createLogMessage(MSG_INFO, msg, 0);

	cleanUp();
	exit(state);
}

void dim_dummy_exit_handler(int* bufp) {
	char msg[200];
	char clientName[50];
	int dummy = 0;

	// if bufp null pointer, redirect to valid value
	if (bufp == 0) {
		bufp = &dummy;
	}

	// DO almost nothing, just to disable the build-in exit command of the DIM framework
	// just notifying about intrusion, except for framework exit
	clientName[0] = 0;
	dis_get_client(clientName);
	// let's asume exit from ambitious user has clientName (pid@host).
	if (clientName[0] == 0) {
#		ifdef __DEBUG
		printf("Framework tries to exit FeeServer (%d)\n", *bufp);
		printf("Most likely FeeServer name already exists.\n");
		fflush(stdout);
#		endif
		// IMPORTANT don't use the bufp - state of framework, it could interfere
		// with own specified exit states !! (e.g. for restarting in case of "2")
		// the same state is signaled by kill all servers of dns !?
		fee_exit_handler(204);
	} else {
		msg[sprintf(msg, "Ambitious user (%s) tried to kill FeeServer, ignoring command!",
				clientName)] = 0;
		createLogMessage(MSG_WARNING, msg, 0);
#		ifdef __DEBUG
		printf("Ambitious user (%s) tried to kill FeeServer (%d)\n", clientName, *bufp);
		fflush(stdout);
#		endif
	}
}

void cleanUp() {
	// the order of the clean up sequence here is important to evade seg faults
	cleanUpCE();

	if (monitorThreadStarted) {
		pthread_cancel(thread_mon);
	}
	if (intMonitorThreadStarted) {
		pthread_cancel(thread_mon_int);
	}
	if (state == RUNNING) {
		pthread_cancel(thread_init);
	}
	if (logWatchDogRunning) {
		pthread_cancel(thread_logWatchdog);
	}

	dis_stop_serving();

	deleteItemList();
	// new since version 0.8.1 -> int channels
	deleteIntItemList();
    // new since version 0.8.2b -> char channels
    deleteCharItemList();

	if (cmndACK != 0) {
		free(cmndACK);
	}
	if (serverName != 0) {
		free(serverName);
	}

	// new since 0.8.3 -> memory list
	//cleanupMemoryList();

}

int deleteItemList() {
	ItemNode* tmp = 0;

	while (firstNode != 0) {
		if (firstNode->item != 0) {
			if (firstNode->item->name != 0) {
				free(firstNode->item->name);
			}
			free(firstNode->item);
		}

		tmp = firstNode->next;
		free(firstNode);
		firstNode = tmp;
	}
	return FEE_OK;
}

/*
MessageStruct copyMessage(const MessageStruct* const orgMsg) {
    MessageStruct msg;
    msg.eventType = orgMsg->eventType;
    memcpy(msg.detector, orgMsg->detector, MSG_DETECTOR_SIZE);
    memcpy(msg.source, orgMsg->source, MSG_SOURCE_SIZE);
    memcpy(msg.description, orgMsg->description, MSG_DESCRIPTION_SIZE);
    memcpy(msg.date, orgMsg->date, MSG_DATE_SIZE);
    return msg;
}
*/

////   --------- NEW FEATURE SINCE VERSION 0.8.1 (2007-06-12) ---------- /////

int publishInt(IntItem* intItem) {
	unsigned int id;
	char* serviceName = 0;

	// check for right state
	if (state != COLLECTING) {
		return FEE_WRONG_STATE;
	}

	// Testing for NULL - Pointer
	// !! Attention: if pointer is not initialized and also NOT set to NULL, this won't help !!
	if (intItem == 0) {
#		ifdef __DEBUG
		printf("Bad intItem, not published\n");
		fflush(stdout);
#		endif
		return FEE_NULLPOINTER;
	}
	if (intItem->name == 0 || intItem->location == 0) {
#		ifdef __DEBUG
		printf("Bad intItem, not published\n");
		fflush(stdout);
#		endif
		return FEE_NULLPOINTER;
	}

	// Check name for duplicate here
	// Check in Float list
	if (findItem(intItem->name) != 0) {
#		ifdef __DEBUG
		printf("Item name already published in float list, int item discarded.\n");
		fflush(stdout);
#		endif
		return FEE_ITEM_NAME_EXISTS;
	}
	// Check in INT list
	if (findIntItem(intItem->name) != 0) {
#		ifdef __DEBUG
		printf("Item name already published in int list, new int item discarded.\n");
		fflush(stdout);
#		endif
		return FEE_ITEM_NAME_EXISTS;
	}
	// Check in Char service list
    if (findCharItem(intItem->name) != 0) {
#       ifdef __DEBUG
        printf("Item name already published in char list, int item discarded.\n");
        fflush(stdout);
#       endif
        return FEE_ITEM_NAME_EXISTS;
    }


	// -- add intItem as service --
	serviceName = (char*) malloc(serverNameLength + strlen(intItem->name) + 2);
	if (serviceName == 0) {
		return FEE_INSUFFICIENT_MEMORY;
	}
	// terminate string with '\0'
	serviceName[sprintf(serviceName, "%s_%s", serverName, intItem->name)] = 0;
	id = dis_add_service(serviceName, "I", (int*) intItem->location,
			sizeof(int), 0, 0);
	free(serviceName);
	add_int_item_node(id, intItem);

	return FEE_OK;
}

void add_int_item_node(unsigned int _id, IntItem* _int_item) {
	//create new node with enough memory
	IntItemNode* newNode = 0;

	newNode = (IntItemNode*) malloc(sizeof(IntItemNode));
	if (newNode == 0) {
		//no memory available!
#		ifdef __DEBUG
		printf("no memory available while adding IntItemNode!\n");
		fflush(stdout);
#		endif
		cleanUp();
		exit(201);
	}
	//initialize "members" of node
	newNode->prev = 0;
	newNode->next = 0;
	newNode->id = _id;
	newNode->intItem = _int_item;
	newNode->lastTransmittedIntValue = *(_int_item->location);
	//if default deadband is negative -> set threshold 0, otherwise set half of defaultDeadband
	newNode->threshold = (_int_item->defaultDeadband < 0) ? 0.0 : (_int_item->defaultDeadband / 2);

	newNode->locBackup = _int_item->location;

	// Check if these feature have to be ported as well ??? !!!
//	newNode->checksum = calculateChecksum((unsigned char*) &(_item->location),
//			sizeof(volatile float*));
//	newNode->checksumBackup = newNode->checksum;

#ifdef __DEBUG
	// complete debug display of added IntItem
/*
	printf("IntItem: %d\n", _id);
	printf("location: %f, locBackup %f\n", *(_int_item->location), *(newNode->locBackup));
	printf("location addr: %p, locBackup addr %p\n", _int_item->location, newNode->locBackup);
	printf("checksum1: %d, checksum2: %d\n\n", newNode->checksum,
			newNode->checksumBackup);
*/
#endif

#	ifdef __DEBUG
	// short debug display of added Item
	printf("init of %s (int) with ID %d: %d\n", newNode->intItem->name,
				newNode->id, newNode->lastTransmittedIntValue);
	fflush(stdout);
#	endif

	++intNodesAmount;
	//redirect pointers of doubly linked list (int)
	if (firstIntNode != 0) {
		lastIntNode->next = newNode;
		newNode->prev = lastIntNode;
		lastIntNode = newNode;
	} else {
		firstIntNode = newNode;
		lastIntNode = newNode;
	}
}


IntItemNode* findIntItem(char* name) {
//	char msg[70];
	IntItemNode* current = 0;

	if (name == 0) {
		return 0;
	}
	current = firstIntNode;
	while (current != 0) {  // is end of list
		if (strcmp(name, current->intItem->name) == 0) {
			// success, give back IntItemNode
			return current;
		}
		current = current->next;
	}
// since two lists, which are searched seperately don't make log output in
// findIntItem()-function -> move to where called to combine with other
// findXYItem calls
/*
	if (state == RUNNING) {
		msg[sprintf(msg, "Item %s not found in IntItem list.", name)] = 0;
		createLogMessage(MSG_WARNING, msg, 0);
#		ifdef __DEBUG
		printf("Item %s not found in IntItem list.\n", name);
		fflush(stdout);
#		endif
	}
*/
	return 0;
}

int deleteIntItemList() {
	IntItemNode* tmp = 0;

	while (firstIntNode != 0) {
		if (firstIntNode->intItem != 0) {
			if (firstIntNode->intItem->name != 0) {
				free(firstIntNode->intItem->name);
			}
			free(firstIntNode->intItem);
		}

		tmp = firstIntNode->next;
		free(firstIntNode);
		firstIntNode = tmp;
	}
	return FEE_OK;
}

void initIntItemNode(IntItemNode* intItemNode) {
	intItemNode->prev = 0;
	intItemNode->next = 0;
	intItemNode->id = 0;
	intItemNode->intItem = 0;
	intItemNode->lastTransmittedIntValue = 0;
	intItemNode->threshold = 0.0;
	intItemNode->locBackup = 0;
	intItemNode->checksum = 0;
	intItemNode->checksumBackup = 0;
}

void unpublishIntItemList() {
	IntItemNode* current = 0;

	current = firstIntNode;
	while (current != 0) {
		dis_remove_service(current->id);
		current = current->next;
	}
	// pretending ItemList is completely empty to avoid access
	// to not existing elements
	intNodesAmount = 0;
	firstIntNode = 0;
	lastIntNode = 0;
}

Item* createItem() {
    Item* item = 0;

    item = (Item*) malloc(sizeof(Item));
    if (item == 0) {
        //no memory available!
#       ifdef __DEBUG
        printf("no memory available!\n");
#       endif
        return 0;
    }
    item->location = 0;
    item->name = 0;
    item->defaultDeadband = 0;
    return item;
}

IntItem* createIntItem() {
	IntItem* intItem = 0;

	intItem = (IntItem*) malloc(sizeof(IntItem));
	if (intItem == 0) {
		//no memory available!
#		ifdef __DEBUG
		printf("no memory available!\n");
#		endif
		return 0;
	}
	intItem->location = 0;
	intItem->name = 0;
	intItem->defaultDeadband = 0;
	return intItem;
}

Item* fillItem(float* floatLocation, char* itemName, float defDeadband) {
	Item* item = 0;

    item = createItem();
    if (item == 0) {
        //no memory available!
#       ifdef __DEBUG
        printf("no memory available!\n");
#       endif
        return 0;
    }
    item->location = floatLocation;
    item->name = (char*) malloc(strlen(itemName) + 1);
    if (item->name == 0) {
        //no memory available!
#       ifdef __DEBUG
        printf("no memory available!\n");
#       endif
		free(item);
        return 0;
    }
	strcpy(item->name, itemName);
    item->defaultDeadband = defDeadband;
    return item;
}

IntItem* fillIntItem(int* intLocation, char* itemName, int defDeadband) {
    IntItem* intItem = 0;

    intItem = createIntItem();
    if (intItem == 0) {
        //no memory available!
#       ifdef __DEBUG
        printf("no memory available!\n");
#       endif
        return 0;
    }
    intItem->location = intLocation;
    intItem->name = (char*) malloc(strlen(itemName) + 1);
    if (intItem->name == 0) {
        //no memory available!
#       ifdef __DEBUG
        printf("no memory available!\n");
#       endif
        free(intItem);
        return 0;
    }
    strcpy(intItem->name, itemName);
    intItem->defaultDeadband = defDeadband;
    return intItem;
}

void monitorIntValues() {
	int status = -1;
	int nRet;
	unsigned long sleepTime = 0;
	IntItemNode* current = 0;
	char msg[120];
    unsigned long innerCounter = 0; // used for update check after time interval
    unsigned long outerCounter = 0; // used for update check after time interval

	// set flag, that monitor thread has been started
	intMonitorThreadStarted = true;

	// set cancelation type
	status = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Set cancel state error [mon - int]: %d\n", status);
		fflush(stdout);
#		endif
		createLogMessage(MSG_WARNING,
			"Unable to configure monitor thread (int) properly. Monitoring is not affected.", 0);
	}
	status = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	if (status != 0) {
#		ifdef __DEBUG
		printf("Set cancel type error [mon - int]: %d\n", status);
		fflush(stdout);
#		endif
		createLogMessage(MSG_WARNING,
			"Unable to configure monitor thread (int) properly. Monitoring is not affected.", 0);
	}

	createLogMessage(MSG_DEBUG, "Started monitor thread for INT values successfully.", 0);

	while (1) {
		current = firstIntNode;
		sleepTime = (unsigned long) (updateRate / intNodesAmount);
		while (current != 0) { // is lastIntNode->next (end of list)
 			if (!checkIntLocation(current)) {
				msg[sprintf(msg, "Value of item %s (int) is corrupt, reconstruction failed. Ignoring!",
						current->intItem->name)] = 0;
				createLogMessage(MSG_ERROR, msg, 0);
				// message and do some stuff (like invalidate value)
				// (test, what happens if pointer is redirected ???)
			} else {
				if ((abs((*(current->intItem->location)) - current->lastTransmittedIntValue)
						>= current->threshold) || (outerCounter ==
						(innerCounter * TIME_INTERVAL_MULTIPLIER))) {
					nRet = dis_update_service(current->id);
					current->lastTransmittedIntValue = *(current->intItem->location);
#					ifdef __DEBUG
					printf("CE triggered an updated on %d clients for service %s [int]: %d\n",
							nRet, current->intItem->name, *(current->intItem->location));
					fflush(stdout);
#					endif
				}
			}

			++innerCounter;
			current = current->next;
			usleep(sleepTime * 1000);
			// sleeps xy microseconds, needed milliseconds-> "* 1000"
		}
		// with the check of both counter, each service is at least updated after
		// every (deadband updateRate * nodesAmount) seconds
		innerCounter = 0;
		++outerCounter;
		// after every service in list is updated set counter back to 0
		// the TIME_INTERVAL_MULTIPLIER is used to enlarge the time interval of
		// the request of services without touching the deadband checker updateRate
		if (outerCounter >= (intNodesAmount * TIME_INTERVAL_MULTIPLIER)) {
			outerCounter = 0;
		}
	}
	// should never be reached !
	pthread_exit(0);
}

// checks against bitflips in location (integer Item)
bool checkIntLocation(IntItemNode* node) {
	if (node->intItem->location == node->locBackup) {
		// locations are identical, so no bitflip
		return true;
	}
	// locations are not identical, check further

	if (node->checksum == calculateChecksum((unsigned char*)
			&(node->intItem->location), sizeof(volatile int*))) {
		// checksum tells, that first location should be valid, repair backup
		node->locBackup = node->intItem->location;
		return true;
	}
	// original location or first checksum is wrong, continue checking

	if (node->checksum == calculateChecksum((unsigned char*)
			&(node->locBackup), sizeof(volatile int*))) {
		// checksum tells, that location backup should be valid, repair original
		node->intItem->location = node->locBackup;
		return true;
	}
	// location backup or first checksum is wrong, continue checking

	if (node->checksum == node->checksumBackup) {
		// it seems that location and location backup are wrong
		// or checksum value runs banana, not repairable
		return false;
	}
	// it seems that first checksum is wrong
	// try to fix with second checksum

	if (node->checksumBackup == calculateChecksum((unsigned char*)
			&(node->intItem->location), sizeof(volatile int*))) {
		// checksum backup tells, that first location should be valid, repair backup
		node->locBackup = node->intItem->location;
		// repair first checksum
		node->checksum = node->checksumBackup;
		return true;
	}
	// original location or second checksum is wrong, continue checking

	if (node->checksumBackup == calculateChecksum((unsigned char*)
			&(node->locBackup), sizeof(volatile int*))) {
		// checksum backup tells, that location backup should be valid, repair original
		node->intItem->location = node->locBackup;
		// repair checksum
		node->checksum = node->checksumBackup;
		return true;
	}
	// value is totally banana, no chance to fix
	return false;
}

// --- Wrapper for the Float Items --- //

int publishFloat(FloatItem* floatItem) {
	return publish(floatItem);
}


FloatItem* createFloatItem() {
	return createItem();
}


FloatItem* fillFloatItem(float* floatLocation, char* itemName, float defDeadband) {
	return fillItem(floatLocation, itemName, defDeadband);
}


////    ------------- NEW Memory Management (2007-07-25) ----------------- ////

MemoryNode* findMemoryNode(void* addr) {
	MemoryNode* current = 0;

	if (addr == 0) {
	  /*
		createLogMessage(MSG_WARNING,
				"Reqesting MemoryNode with ID \"NULL\", discarding call!", 0);
#		ifdef __DEBUG
		printf("Reqesting MemoryNode with ID \"NULL\", discarding call!\n");
		fflush(stdout);
#		endif
	  */
		return 0;
	}

	current = firstMemoryNode;
	while (current != 0) {  // is end of list
		if (current->identityAddr == addr) {
			// success, give back MemoryNode
			return current;
		}
		current = current->next;
	}

	if (current == 0) {
		char msg[80];
		msg[sprintf(msg, "Unable to find MemoryNode with ID \"%p\".", 
				addr)] = 0;
		createLogMessage(MSG_ERROR, msg, 0);
#		ifdef __DEBUG
		printf("Unable to find MemoryNode with ID \"%p\".\n", addr);
		fflush(stdout);
#		endif
	}
	return 0;
}


MemoryNode* createMemoryNode(unsigned int size, char type, char* module,
		unsigned int preSize) {

	// check, if size is greater then preSize
	if (size <= preSize) {
		createLogMessage(MSG_ERROR,
				"Error: asking for memory smaller or equal than its prefix size.",
				0);
#		ifdef __DEBUG
		printf("Error: asking for memory smaller or equal than its prefix size.\n");
		fflush(stdout);
#		endif
		return 0;
	}

	MemoryNode* memNode = (MemoryNode*) malloc(sizeof(MemoryNode));
	if (memNode == 0) {
		// insufficient memory
		createLogMessage(MSG_ERROR,
				"Insufficient memory! Unable to allocate memory for MemoryNode.",
				0);
#		ifdef __DEBUG
		printf("Insufficient memory! Unable to allocate memory for MemoryNode.\n");
		fflush(stdout);
#		endif
	}

	// fill MemoryNode
	void* ptr = (void*) malloc(size);
	if (ptr == 0) {
		// insufficient memory
		char msg[100];
		msg[sprintf(msg,
				"Insufficient memory! Unable to allocate memory block of %d .",
				size)] = 0;
		createLogMessage(MSG_ERROR, msg, 0);
#		ifdef __DEBUG
		printf("Insufficient memory! Unable to allocate memory block of %d .\n",
				size);
		fflush(stdout);
#		endif

		free(memNode);
		return 0;
	}

	memNode->ptr = ptr;
	memNode->identityAddr = ptr + preSize;
	memNode->mmData.memSize = size;
	memNode->mmData.memType = type;
	if (module != 0) {
		strncpy(memNode->mmData.memDest, module, 30);
		memNode->mmData.memDest[29] = 0;
	} else {
		memNode->mmData.memDest[0] = 0;
	}
	if (preSize > 0) {
		memNode->mmData.prefixed = true;
	} else {
		memNode->mmData.prefixed = false;
	}
	memNode->mmData.prefixSize = preSize;

	// add Node to list (add at end)
	memNode->prev = lastMemoryNode;
	memNode->next = 0;

	if (lastMemoryNode != 0) {
		lastMemoryNode->next = memNode;
	}
	lastMemoryNode = memNode;

	if (firstMemoryNode == 0) {
		firstMemoryNode = memNode;
	}

	return memNode;
}


void freeMemoryNode(MemoryNode* node) {
	if (node == 0) {
		return;
	}

	// free memory corresponding to this node
	if (node->ptr != 0) {
		free(node->ptr);
	}

	// redirect links in doubly linked list
	if (node->next != 0) {
		node->next->prev = node->prev;
	} else {
		lastMemoryNode = node->prev;
	}

	if (node->prev != 0) {
		node->prev->next = node->next;
	} else {
		firstMemoryNode = node->next;
	}

	//free node itself
	free(node);
}


void cleanupMemoryList() {
	MemoryNode* current = firstMemoryNode;
	while (current != 0) {
		MemoryNode* nextMemNode = current->next;
		freeMemoryNode(current);
		current = nextMemNode;
	}
}


// ------ NEW interface functions for memory management ------ //

int allocateMemory(unsigned int size, char type, char* module,
		char prefixPurpose, void** ptr) {
	MemoryNode*	memNode = 0;
	unsigned int realSize = 0;
	unsigned int addSize = 0;
	char msg[200];
	*ptr = 0;

	if (size == 0) {
		createLogMessage(MSG_WARNING,
				"FeeServer shall allocate memory of size 0; discarding call!", 0);
#		ifdef __DEBUG
		printf("FeeServer shall allocate memory of size 0; discarding call!\n");
		fflush(stdout);
#		endif
		return FEE_INVALID_PARAM;
	}

	switch (prefixPurpose) {
		case ('0'):
			realSize = size;
			break;

		case ('A'):
			realSize = size + HEADER_SIZE;
			addSize = HEADER_SIZE;
			break;

		default:
			msg[sprintf(msg,
					"Received allocateMemory call with unknown prefix purpose ('%c'), discarding call.",
					prefixPurpose)] = 0;
			createLogMessage(MSG_ERROR, msg, 0);
#			ifdef __DEBUG
			printf("%s\n", msg);
			fflush(stdout);
#			endif
			return FEE_INVALID_PARAM;
	}

	memNode = createMemoryNode(realSize, type, module, addSize);
	if (memNode == 0) {
		return FEE_FAILED;
	}

	*ptr = memNode->identityAddr;
/*	// only for debug output to test functyionality
	msg[sprintf(msg,
			"Allocated memory (type %c) for %s of size %d + prefix memory size %d. Purpose is set to: %c.",
			type, memNode->mmData.memDest, size, addSize, prefixPurpose)] = 0;
	createLogMessage(MSG_DEBUG, msg, 0);
#	ifdef __DEBUG
	printf("%s\n", msg);
	fflush(stdout);
#	endif
*/
	return FEE_OK;
}


int freeMemory(void* addr) {
	MemoryNode* memNode = 0;
	if (addr == 0) {
		// received NULL pointer
		createLogMessage(MSG_WARNING,
				"Received an NULL pointer for freeing memory, discarding call!", 0);
#		ifdef __DEBUG
		printf("Received an NULL pointer for freeing memory, discarding call! \n");
		fflush(stdout);
#		endif
		return FEE_NULLPOINTER;
	}

	memNode = findMemoryNode(addr);
	if (memNode == 0) {
		// memory pointer id not found in list
		return FEE_INVALID_PARAM;
	}

	freeMemoryNode(memNode);
	return FEE_OK;
}


///   --- NEW FEATURE SINCE VERSION 0.8.2b [Char channel] (2007-07-28) --- ///

int publishChar(CharItem* charItem) {
    unsigned int id;
    char* serviceName = 0;

    // check for right state
    if (state != COLLECTING) {
        return FEE_WRONG_STATE;
    }

    // Testing for NULL - Pointer
    // !! Attention: if pointer is not initialized and also NOT set to NULL, this won't help !!
    if (charItem == 0) {
#       ifdef __DEBUG
        printf("Bad charItem, not published\n");
        fflush(stdout);
#       endif
        return FEE_NULLPOINTER;
    }
    if (charItem->name == 0 || charItem->user_routine == 0) {
#       ifdef __DEBUG
        printf("Bad charItem, not published\n");
        fflush(stdout);
#       endif
        return FEE_NULLPOINTER;
    }

    // Check name for duplicate here
    // Check in Float list
    if (findItem(charItem->name) != 0) {
#       ifdef __DEBUG
        printf("Item name already published in float list, char item discarded.\n");
        fflush(stdout);
#       endif
        return FEE_ITEM_NAME_EXISTS;
    }
    // Check in INT list
    if (findIntItem(charItem->name) != 0) {
#       ifdef __DEBUG
        printf("Item name already published in int list, char item discarded.\n");
        fflush(stdout);
#       endif
        return FEE_ITEM_NAME_EXISTS;
    }
    // Check in CHAR list
    if (findCharItem(charItem->name) != 0) {
#       ifdef __DEBUG
        printf("Item name already published in char list, new char item discarded.\n");
        fflush(stdout);
#       endif
        return FEE_ITEM_NAME_EXISTS;
    }

    // -- add charItem as service --
    serviceName = (char*) malloc(serverNameLength + strlen(charItem->name) + 2);
    if (serviceName == 0) {
        return FEE_INSUFFICIENT_MEMORY;
    }
    // terminate string with '\0'
    serviceName[sprintf(serviceName, "%s_%s", serverName, charItem->name)] = 0;
    // add service in DIM
    id = dis_add_service(serviceName, "C", 0, 0, charItem->user_routine,
            charItem->tag);
    free(serviceName);

    // !!! implement add_char_item_node() func !!!
    add_char_item_node(id, charItem);

    return FEE_OK;
}


CharItem* createCharItem() {
    CharItem* charItem = 0;

    charItem = (CharItem*) malloc(sizeof(CharItem));
    if (charItem == 0) {
        //no memory available!
#       ifdef __DEBUG
        printf("no memory available for CharItem!\n");
#       endif
        return 0;
    }
    charItem->user_routine = 0;
    charItem->name = 0;
    charItem->tag = 0;
    return charItem;
}


//CharItem* fillCharItem(void* funcPointer, char* itemName, long tag) {
CharItem* fillCharItem(void (*funcPointer)(long*, int**, int*), char* itemName,
        long tag) {
    CharItem* charItem = 0;

    charItem = createCharItem();
    if (charItem == 0) {
        //no memory available!
#       ifdef __DEBUG
        printf("no memory available for CharItem!\n");
#       endif
        return 0;
    }
    charItem->user_routine = funcPointer;
    charItem->name = (char*) malloc(strlen(itemName) + 1);
    if (charItem->name == 0) {
        //no memory available!
#       ifdef __DEBUG
        printf("no memory available!\n");
#       endif
        free(charItem);
        return 0;
    }
    strcpy(charItem->name, itemName);
    charItem->tag = tag;
    return charItem;
}


void add_char_item_node(unsigned int _id, CharItem* _char_item) {
    //create new node with enough memory
    CharItemNode* newNode = 0;

    newNode = (CharItemNode*) malloc(sizeof(CharItemNode));
    if (newNode == 0) {
        //no memory available!
#       ifdef __DEBUG
        printf("no memory available while adding CharItemNode!\n");
        fflush(stdout);
#       endif
        cleanUp();
        exit(201);
    }
    //initialize "members" of node
    newNode->prev = 0;
    newNode->next = 0;
    newNode->id = _id;
    newNode->charItem = _char_item;

#	ifdef __DEBUG
    // complete debug display of added charItem
/*
    printf("CharItem ID: %d\n", _id);
	printf("CharItem name: %s\n", _char_item->name);
    printf("CharItem user_routine: %p\n", _char_item->user_routine);
	printf("CharItem tag: %d\n", _char_item->tag);
*/
#	endif

#   ifdef __DEBUG
    // short debug display of added Item
    printf("init of %s (char) with ID %d; user_routine at: %p\n", 
			newNode->charItem->name, newNode->id, 
			newNode->charItem->user_routine);
    fflush(stdout);
#   endif

    ++charNodesAmount;
    //redirect pointers of doubly linked list (char)
    if (firstCharNode != 0) {
        lastCharNode->next = newNode;
        newNode->prev = lastCharNode;
        lastCharNode = newNode;
    } else {
        firstCharNode = newNode;
        lastCharNode = newNode;
    }
}


CharItemNode* findCharItem(char* name) {
//  char msg[70];
    CharItemNode* current = 0;

    if (name == 0) {
        return 0;
    }
    current = firstCharNode;
    while (current != 0) {  // is end of list
        if (strcmp(name, current->charItem->name) == 0) {
            // success, give back CharItemNode
            return current;
        }
        current = current->next;
    }
// since three lists, which are searched seperately don't make log output in
// findCharItem()-function -> move to where called to combine with other 
// findXYItem calls
/*
    if (state == RUNNING) {
        msg[sprintf(msg, "Item %s not found in CharItem list.", name)] = 0;
        createLogMessage(MSG_WARNING, msg, 0);
#       ifdef __DEBUG
        printf("Item %s not found in CharItem list.\n", name);
        fflush(stdout);
#       endif
    }
*/
    return 0;
}


int deleteCharItemList() {
    CharItemNode* tmp = 0;

    while (firstCharNode != 0) {
        if (firstCharNode->charItem != 0) {
            if (firstCharNode->charItem->name != 0) {
                free(firstCharNode->charItem->name);
            }
            free(firstCharNode->charItem);
        }

        tmp = firstCharNode->next;
        free(firstCharNode);
        firstCharNode = tmp;
    }
    return FEE_OK;
}


void initCharItemNode(CharItemNode* charItemNode) {
    charItemNode->prev = 0;
    charItemNode->next = 0;
    charItemNode->id = 0;
    charItemNode->charItem = 0;
}


void unpublishCharItemList() {
    CharItemNode* current = 0;

    current = firstCharNode;
    while (current != 0) {
        dis_remove_service(current->id);
        current = current->next;
    }
    // pretending CharItemList is completely empty to avoid access
    // to not existing DIM services. Don't delete charItems, CE might still 
	// try to access their content (same for float and int lists)
    charNodesAmount = 0;
    firstCharNode = 0;
    lastCharNode = 0;
}


/// -- NEW FEATURE V. 0.9.1 [Set FeeServer Properies in CE] (2007-08-17) -- ///

bool setFeeProperty(FeeProperty* prop) {
	bool retVal = false;
	if (state != INITIALIZING) {
		createLogMessage(MSG_WARNING, 
				"Trying to change a FeeProperty during wrong FeeServer state, ignoring ...",
				0);
#		ifdef __DEBUG
		printf("Trying to change a FeeProperty during wrong FeeServer state, ignoring ...\n");
		fflush(stdout);
#		endif
		return retVal;
	}

	if (prop == 0) {
#       ifdef __DEBUG
        printf("Received NULL pointer in setting FeeProperty, ignoring...\n");
		fflush(stdout);
#		endif
		// No log message: in "INITIALIZING" state are no DIM channels available
		return retVal;
	}

	switch (prop->flag) {
		case (PROPERTY_UPDATE_RATE):
            if (prop->uShortVal > 0) {
                updateRate = prop->uShortVal;
                retVal = true;
#               ifdef __DEBUG
                printf("FeeProperty changed: new update rate (%d).\n", 
						updateRate);
                fflush(stdout);
#               endif
            } else {
#               ifdef __DEBUG
                printf("Received invalid value for setting update rate (%d), ignoring...\n",
                        prop->uShortVal);
                fflush(stdout);
#               endif
            }
			break;

		case (PROPERTY_LOGWATCHDOG_TIMEOUT):
            if (prop->uIntVal > 0) {
                logWatchDogTimeout = prop->uIntVal;
                retVal = true;
#               ifdef __DEBUG
                printf("FeeProperty changed: new log watchdog timeout (%d).\n",
						logWatchDogTimeout);
                fflush(stdout);
#               endif
            } else {
#               ifdef __DEBUG
                printf("Received invalid value for setting log watchdog timeout (%d), ignoring...\n",
                        prop->uIntVal);
                fflush(stdout);
#               endif
            }
			break;

		case (PROPERTY_ISSUE_TIMEOUT):
			if (prop->uLongVal > 0) {
                issueTimeout = prop->uLongVal;
                retVal = true;
#               ifdef __DEBUG
                printf("FeeProperty changed: new issue timeout (%ld).\n", 
						issueTimeout);
                fflush(stdout);
#               endif
            } else {
#               ifdef __DEBUG
                printf("Received invalid value for setting issue timeout (%ld), ignoring...\n",
                        prop->uLongVal);
                fflush(stdout);
#               endif
            }
			break;
		
		case (PROPERTY_LOGLEVEL):
			if ((prop->uIntVal > 0) && (prop->uIntVal <= MSG_MAX_VAL)) {
				logLevel = prop->uIntVal | MSG_ALARM;		
				retVal = true;
#				ifdef __DEBUG
				printf("FeeProperty changed: new log level (%d).\n", logLevel);
				fflush(stdout);
#				endif
			} else {
#				ifdef __DEBUG
				printf("Received invalid value for setting loglevel (%d), ignoring...\n",
			    		prop->uIntVal);
				fflush(stdout);
#				endif
			}
			break;

		default:
			// unknown property flag, but no logging
#			ifdef __DEBUG
			printf("Received unknown flag in setting FeeProperty (%d), ignoring...\n",
					prop->flag);
			fflush(stdout);
#			endif
	}

	return retVal;
}


/// ***************************************************************************
/// -- only for the benchmarking cases necessary
/// ***************************************************************************
#ifdef __BENCHMARK
void createBenchmark(char* msg) {
	// this part is only used for benchmarking
	// timestamp to benchmark reception of a command
    struct tm *today;
	struct timeval tStamp;
	char benchmark_msg[200];
	int status = 0;
	FILE* pFile = 0;

    status = gettimeofday(&tStamp, 0);
    if (status != 0) {
	benchmark_msg[sprintf(benchmark_msg,
                "Unable to get timestamp for benchmark!")] = 0;
	} else {
        today = localtime(&(tStamp.tv_sec));
    	benchmark_msg[sprintf(benchmark_msg,
				"%s: \t%.8s - %ld us", msg, asctime(today) + 11,
				tStamp.tv_usec)] = 0;
	}

	if (getenv("FEE_BENCHMARK_FILENAME")) {
		pFile = fopen(getenv("FEE_BENCHMARK_FILENAME"), "a+");
		if (pFile) {
			fprintf(pFile, benchmark_msg);
			fprintf(pFile, "\n");
			fclose(pFile);
		} else {
#ifdef __DEBUG
			printf("Unable to open benchmark file.\n");
			printf("%s\n", benchmark_msg);
#endif
			createLogMessage(MSG_WARNING, "Unable to write to benchmarkfile.", 0);
		}
	} else {
		createLogMessage(MSG_SUCCESS_AUDIT, benchmark_msg, 0);
	}
}
#else
// empty function
void createBenchmark(char* msg) {
}
#endif



/// ***************************************************************************
/// -- only for the debugging cases necessary
/// ***************************************************************************
#ifdef __DEBUG
void printData(char* data, int start, int size) {
	int i;
	int iBackUp;

	if ((data == 0) || (size == 0)) {
		return;
	}
	iBackUp = start;
	for (i = start; (i < size) || (iBackUp < size); ++i) {
		++iBackUp;
		printf("%c", data[i]);
	}
	printf("\nData - Size: %d", size);
	printf("\n\n");
	fflush(stdout);
}
#endif


//-- only for the testcases necessary
#ifdef __UTEST
const int getState() {
	return state;
}

const ItemNode* getFirstNode() {
	return firstNode;
}

const ItemNode* getLastNode() {
	return lastNode;
}

int listSize() {
	ItemNode* tmp = firstNode;
	int count = 0;

	while (tmp != 0) {
		tmp = tmp->next;
		++count;
	}
	return count;
}

const char* getCmndACK() {
	return cmndACK;
}

void setState(int newState) {
	state = newState;
}

void setCmndACK(char* newData) {
	cmndACK = newData;
}

void setCmndACKSize(int size) {
	cmndACKSize = size;
}

void setServerName(char* name) {
	if (serverName != 0) {
		free(serverName);
	}
	serverName = (char*) malloc(strlen(name) + 1);
	if (serverName == 0) {
		printf(" No memory available !\n");
	}
	strcpy(serverName, name);
}

void clearServerName() {
	if (serverName != 0) {
		free(serverName);
	}
}

void stopServer() {
	dis_remove_service(serviceACKID);
	dis_remove_service(commandID);
	dis_stop_serving();
}

pthread_cond_t* getInitCondPtr() {
	return &init_cond;
}


pthread_mutex_t* getWaitInitMutPtr() {
	return &wait_init_mut;
}

#endif

