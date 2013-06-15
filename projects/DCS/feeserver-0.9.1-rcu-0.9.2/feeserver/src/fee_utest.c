/************************************************************************
 * **
 * **
 * ** This file is property of and copyright by the Department of Physics
 * ** Institute for Physic and Technology, University of Bergen,
 * ** Bergen, Norway.
 * ** In cooperation with Center for Technology Transfer and 
 * ** Telecommunication (ZTT), University of Applied Sciences Worms
 * ** Worms, Germany.
 * **
 * ** This file has been written by Sebastian Bablok,
 * ** Sebastian.Bablok@uib.no
 * **
 * ** Important: This file is provided without any warranty, including
 * ** fitness for any particular purpose. Further distribution of this file,
 * ** even with changes in the code, is only allowed, when this copyright
 * ** and warranty paragraph is kept unchanged and included to the sources. 
 * **
 * **
 * *************************************************************************/

#ifdef __UTEST

#include <unistd.h>
#include <sys/time.h>	// for gettimeofday()
#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include "fee_utest.h"
#include "fee_errors.h"
#include "fee_types.h"
#include "fee_defines.h"
#include "fee_functions.h"
#include "ce_command.h"

int count;

void testFrameWork() {
	bool succeeded = true;
	struct timeval start;
	struct timeval end;
	long time = 0;
	count = 0;

	printf(" \n Running tests for the feeserver now !\n \n");
	gettimeofday(&start, 0);

	succeeded = (test((void*) &testAdd_item_node) ? succeeded : false);
	succeeded = (test((void*) &testSignalCEready) ? succeeded : false);
	succeeded = (test((void*) &testAck_service) ? succeeded : false);
	succeeded = (test((void*) &testChecksumCal) ? succeeded : false);
	succeeded = (test((void*) &testCheckLocation) ? succeeded : false);

//	succeeded = (test((void*) &testPublish) ? succeeded : false);
//	succeeded = (test((void*) &testChecksumCal) ? succeeded : false);

//  does NOT work properly, causes rebooting	 !!!!!!!!!
//	succeeded = (test((void*) &testStart) ? succeeded : false);


	gettimeofday(&end, 0);
	time = end.tv_usec - start.tv_usec;
	if (succeeded) {
		printf(" \n -- Executed %d testcase(s) successfully. --\n \n", count);
	} else {
		printf(" \n !! Some errors occured while running %d test(s)!!\n", count);
		printf("    Please see above for details.\n \n");
	}
	printf(" Total Time elapsed: %ld usec\n\n", time);
	return;
}


bool test(TestFuncPtr function) {
	bool bRet = false;
	int runCount = 0;
	int errorCount = 0;
	int failureCount = 0;
	struct timeval start;
	struct timeval end;
	long time = 0;

	gettimeofday(&start, 0);
	setUp();
	bRet = function(&runCount, &failureCount, &errorCount);
	tearDown();
	gettimeofday(&end, 0);
	time = end.tv_usec - start.tv_usec;

	printf("TestRuns [%2d], Failures [%2d], Errors [%2d] - Time %ld usec\n",
			runCount, failureCount, errorCount, time);
	fflush(stdout);
	++count;
	return bRet;
}

void setUp() {
	char* name = 0;
	name = (char*) malloc(6);
	sprintf(name, "uTest");
	setServerName(name);
	//when something goes wrong here, show message
}


void tearDown() {

	if (deleteItemList() != FEE_OK) {
		printf(" Deleting list failed !!!\n");
		fflush(stdout);
	}

	clearServerName();
	//when something goes wrong here, show message

}

bool testPublish(int* runs, int* failures, int* errors) {
	bool bRet = true;
	Item* testItem = 0; //  = (Item*) malloc(sizeof(Item));
	float aFloat;
	char* name = 0;
	//const ItemNode* tmp = 0;

	printf("\tTesting \"publish()\":\t\t");
	fflush(stdout);
	name = (char*) malloc(5);
	if (name == 0) {
		printf("\n No memory available !\n");
		return false;
	}
	strcpy(name, "puby\0");
	aFloat = 31.7;

printf(" pos 1 ");
fflush(stdout);
	//-- Testing for handling of incomming nullpointers --
	if (publish(testItem) != FEE_NULLPOINTER) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;
printf(" pos 2 ");
fflush(stdout);
	//-----
	testItem = 0;
	testItem = (Item*) malloc(sizeof(Item));
	testItem->name = 0;
printf(" pos 2a ");
fflush(stdout);
	if (testItem == 0) {
		printf(" No Memory available !!\n");
		fflush(stdout);
		return false;
	}
/*
	testItem->location = &aFloat;
	if(publish(testItem) != FEE_NULLPOINTER) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;

	//-----
	testItem->location = 0;
	testItem->name = name;
	if (publish(testItem) != FEE_NULLPOINTER) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;

	//-------------------------------

	//-- Testing the behaviour in wrong state --
	setState(RUNNING);
	testItem->location = &aFloat;
	testItem->name = name;
	if (publish(testItem) != FEE_WRONG_STATE) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;

	//-- Publish item and look, if it is in list
	setState(COLLECTING);
	if (publish(testItem) != FEE_OK) {
		bRet = false;
		(*failures)++;
	} else {
		// Check for right size
		if (listSize() != 1) {
			bRet = false;
			(*failures)++;
		} else {
			tmp = getFirstNode();
			if ((tmp == 0) || (tmp->item == 0) || (tmp->item->name == 0)
					|| (tmp->item->location == 0)) {
				return false;
			} else {
				return bRet;
			}

			// check for correct item values
			if (!((strcmp(tmp->item->name, name) == 0) && (*(tmp->item->location) == aFloat))) {
				bRet = false;
				(*failures)++;
			}
			(*runs)++;

		}
		(*runs)++;
	}
	(*runs)++;

*/
	return bRet;
}

bool testAdd_item_node(int* runs, int* failures, int* errors) {
	bool bRet = true;
	Item* testItem = 0;
	float aFloat = 31.7;
	char* name = 0;
	Item* testItem2 = 0;
	float aFloat2 = 31.7;
	char* name2 = 0;
	const ItemNode* tmp = 0;

	printf("\tTesting \"add_item_node()\":\t");
	fflush(stdout);

	testItem = (Item*) malloc(sizeof(Item));
	if (testItem == 0) {
		printf(" No memory available !\n");
		return false;
	}

	name = (char*) malloc(8);
	if (name == 0) {
		printf(" No memory available !\n");
		return false;
	}

	strcpy(name, "NewNode\0");
	testItem->name = name;
	testItem->location = &aFloat;

	testItem2 = (Item*) malloc(sizeof(Item));
	if (testItem2 == 0) {
		printf(" No memory available !\n");
		return false;
	}
	name2 = (char*) malloc(8);
	if (name2 == 0) {
		printf(" No memory available !\n");
		return false;
	}
	strcpy(name2, "anyNode\0");
	testItem2->name = name2;
	testItem2->location = &aFloat2;

	// -- testing the adding
	add_item_node(5, testItem);
	add_item_node(8, testItem2);

	// Check for right size
	if (listSize() != 2) {
		bRet = false;
		(*failures)++;
	} else {
		tmp = getFirstNode();
		if (tmp == 0) {
			(*errors)++;
			return false;
		}
		// check for correct item values
		if (!((strcmp(tmp->item->name, name) == 0) && (*(tmp->item->location) == aFloat) &&
    				(tmp->id == 5))) {
			bRet = false;
			(*failures)++;
		} else {
			tmp = tmp->next;
			if (tmp != 0) {
				// check for correct item values
			 	if (!((strcmp(tmp->item->name, name2) == 0) && (*(tmp->item->location) == aFloat2) &&
						(tmp->id == 8))) {
					bRet = false;
					(*failures)++;
				}
				(*runs)++;
			} else {
				(*errors)++;
			}
		}
		(*runs)++;
	}
	(*runs)++;

	return bRet;
}

bool testStart(int* runs, int* failures, int* errors) {
	bool bRet = true;

	printf("\tTesting \"start()\":\t\t");
	fflush(stdout);

	if (getState() != COLLECTING) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;

	//-- testing for correct state handling and
	//-- testing if CmndACK is set according to input param
	if (start(FEE_OK) != FEE_OK) {
			if (strcmp(getCmndACK(), "  0/") != 0) {
				bRet = false;
				(*failures)++;
			}
			(*runs)++;
			bRet = false;
			(*failures)++;
		}
	(*runs)++;
	stopServer();

	if (start(FEE_CE_NOTINIT) != FEE_OK) {
				if (strcmp(getCmndACK(), " -6/") != 0) {
					bRet = false;
					(*failures)++;
				}
				(*runs)++;
				bRet = false;
				(*failures)++;
			}
	(*runs)++;
	stopServer();

	//-- testing with different states
	if (getState() != RUNNING) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;

	if (start(FEE_OK) != FEE_OK) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;

	if (getState() != RUNNING) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;

	//--provoke a wrong state and start server again
	setState(COLLECTING);
	if (getState() != COLLECTING) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;
	if (start(FEE_OK) != FEE_OK) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;
	if (getState() != RUNNING) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;

	return bRet;
}

bool testChecksumCal(int* runs, int* failures, int* errors) {
	bool bRet = true;

	printf("\tTesting \"checkCommand()\":\t");
	fflush(stdout);

	// test correct checksum
	if (!checkCommand("Hello Alice", 11, 392496115)) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;
	// test incorrect checksum
	if (checkCommand("Hello Alice", 11, 392496125)) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;

	return bRet;
}

bool testAck_service(int* runs, int* failures, int* errors) {
	bool bRet = true;
	int tag;
	int size = -1;
	char* result = 0;

	printf("\tTesting \"Ack_service()\":\t");
	fflush(stdout);

	setCmndACK("  0/TestTest");
	setCmndACKSize(12);

	//-- here the result data should be assigned
	tag = ACK_SERVICE_TAG;
	ack_service(&tag, &result, &size);
	if (size != 12) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;
	if (strcmp(result, "  0/TestTest") != 0) {
		bRet = false;
		(*failures)++;
	}
	(*runs)++;


	return bRet;
}


bool testSignalCEready(int* runs, int* failures, int* errors) {
	bool bRet = true;
	pthread_attr_t attrTest1;
	pthread_t threadTest1;
	//pthread_attr_t attrTest2;
	//pthread_t threadTest2;
	struct timeval now;
	struct timespec timeout;
	int retcode = -1;

	printf("\tTesting \"signalCEready()\":\t");
	fflush(stdout);

	// do initialisation stuff of the initializeCE - thread
	pthread_cond_init(getInitCondPtr(), 0);

	// lock mutex
	pthread_mutex_lock(getWaitInitMutPtr());

	//-- start mockup for collecting items --
	pthread_attr_init(&attrTest1);
	pthread_attr_setdetachstate(&attrTest1, PTHREAD_CREATE_DETACHED);
	pthread_create(&threadTest1, &attrTest1, (void*)&signalThread, 0);
	pthread_attr_destroy(&attrTest1);


	// timeout set to ???? ms, should be enough for initialisation
	gettimeofday(&now, 0);
	timeout.tv_sec = now.tv_sec;
	timeout.tv_nsec = (now.tv_usec * 1000) + 100000;

	// a retcode of 0 means, that pthread_cond_timedwait has returned with the cond_init signaled
//	while ((retcode != ETIMEDOUT) && (retcode != 0)) {
		// wait for finishing "issue" or timeout
	retcode = pthread_cond_timedwait(getInitCondPtr(), getWaitInitMutPtr(), &timeout);
//	}

 // unlock mutex
	pthread_mutex_unlock(getWaitInitMutPtr());


	if (retcode == ETIMEDOUT) {
		pthread_cancel(threadTest1);
		(*failures)++;
		bRet = false;
	} else if (retcode != 0) {
		pthread_cancel(threadTest1);
		(*errors)++;
		bRet = false;
	}
	(*runs)++;



	//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
	// THE FOLLOWING TEST IS NOT A UNIT TEST OF THE FEESERVER ITSELF !!!
	//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
	// it is used to test the functionality of pthread_cond_timedwait
	//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
/*
	// do initialisation stuff of the initializeCE - thread
	pthread_cond_init(getInitCondPtr(), 0);
	// lock mutex
	pthread_mutex_lock(getWaitInitMutPtr());

	//-- start mockup for collecting items --
	pthread_attr_init(&attrTest2);
	pthread_attr_setdetachstate(&attrTest2, PTHREAD_CREATE_DETACHED);
	pthread_create(&threadTest2, &attrTest2, (void*)&signalThreadTimeout, 0);
	pthread_attr_destroy(&attrTest2);


	gettimeofday(&now, 0);
	timeout.tv_sec = now.tv_sec + 2;
	timeout.tv_nsec = (now.tv_usec * 1000) + 1000000000;

	// a retcode of 0 means, that pthread_cond_timedwait has returned with the cond_init signaled
//	while ((retcode != ETIMEDOUT) && (retcode != 0)) {
		// wait for finishing "issue" or timeout
	retcode = pthread_cond_timedwait(getInitCondPtr(), getWaitInitMutPtr(), &timeout);
//	}

 // unlock mutex
	pthread_mutex_unlock(getWaitInitMutPtr());

	if (retcode != ETIMEDOUT) {
		(*failures)++;
		bRet = false;
	}
	(*runs)++;
	pthread_cancel(threadTest2);
*/
	return bRet;
}

bool testCheckLocation(int* runs, int* failures, int* errors) {
	bool bRet = true;
	bool bResult = false;
	Item* testItem = 0;
	float aFloat = 742.3792;
	unsigned int checkMus;
	char* name = 0;
	ItemNode* testNode = 0;

	printf("\tTesting \"checkLocation()\":\t");
	fflush(stdout);

	testItem = (Item*) malloc(sizeof(Item));
	if (testItem == 0) {
		printf(" No memory available !\n");
		return false;
	}

	name = (char*) malloc(8);
	if (name == 0) {
		printf(" No memory available !\n");
		return false;
	}

	strcpy(name, "NewNode\0");
	testItem->name = name;
	testItem->location = &aFloat;

	add_item_node(123, testItem);

	(const ItemNode*) testNode = getFirstNode();

	//-- check if everything worked as usual, should never fail! --
	bResult = checkLocation(testNode);
	if(!bResult) {
		(*errors)++;
		bRet = false;
	}
	(*runs)++;

	//-- check for all repair mechanisms --

	//-- change location backup and check --
	testNode->locBackup = (float*) 12345;
	bResult = checkLocation(testNode);
	if(!bResult) {
		(*errors)++;
		bRet = false;
	}
	(*runs)++;

	// compare to be sure that reparation worked
	if(testNode->locBackup != &aFloat) {
		(*failures)++;
		bRet = false;
	}
	(*runs)++;

	//-- change location and check --
	testNode->item->location = (float*) 12345;
	bResult = checkLocation(testNode);
	if(!bResult) {
		(*errors)++;
		bRet = false;
	}
	(*runs)++;

	// compare to be sure that reparation worked
	if(testNode->item->location != &aFloat) {
		(*failures)++;
		bRet = false;
	}
	(*runs)++;

	//-- change location AND checksum then check --
	testNode->item->location = (float*) 12345;
	checkMus =  testNode->checksum;
	testNode->checksum = 12345;
	bResult = checkLocation(testNode);
	if(!bResult) {
		(*errors)++;
		bRet = false;
	}
	(*runs)++;

	// compare to be sure that reparation worked
	if(testNode->checksum != checkMus || testNode->item->location != &aFloat) {
		(*failures)++;
		bRet = false;
	}
	(*runs)++;

	//-- change location backup and checksum then check
	testNode->locBackup = (float*) 12345;
	checkMus =  testNode->checksum;
	testNode->checksum = 12345;
	bResult = checkLocation(testNode);
	if(!bResult) {
		(*errors)++;
		bRet = false;
	}
	(*runs)++;

	// compare to be sure that reparation worked
	if(testNode->checksum != checkMus || testNode->locBackup != &aFloat) {
		(*failures)++;
		bRet = false;
	}
	(*runs)++;

	// -- negative test --
	testNode->item->location = (float*) 12345;
	testNode->locBackup = (float*) 67890;
	bResult = checkLocation(testNode);
	if(bResult) {
		(*errors)++;
		bRet = false;
	}
	(*runs)++;

	free(testItem->name);
	free(testItem);

	return bRet;
}

void signalThread() {
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
//	printf("OK vor Signal\n");
//	fflush(stdout);
	signalCEready(CE_OK);
//	printf("OK nach Signal\n");
//	fflush(stdout);
	pthread_exit(0);
}

void signalThreadTimeout() {
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	sleep(1);
	signalCEready(CE_OK);
	pthread_exit(0);
}

#endif

